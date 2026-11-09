#pragma once

#include "audio_codec.h"
#include "es8389_tdm.h"

#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/i2s_tdm.h>
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>
#include <esp_log.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <mutex>

// Guition S31 RGB boards: TD0/TD2 are microphone data and TD1/TD3 are
// playback reference. The application receives TD0/TD1 as interleaved MR.
// No MCLK is wired: four 16-bit slots supply the codec's required 64fs BCLK.
class GuitionEs8389Codec : public AudioCodec {
public:
    GuitionEs8389Codec(i2c_master_bus_handle_t bus, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout,
                       gpio_num_t din, gpio_num_t pa_pin)
        : pa_pin_(pa_pin) {
        duplex_ = true;
        input_sample_rate_ = output_sample_rate_ = 16000;
        input_channels_ = guition_es8389::kAecChannels;
        output_channels_ = 1;
        input_reference_ = true;
        input_gain_ = 30;

        ESP_ERROR_CHECK(gpio_set_direction(pa_pin_, GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_level(pa_pin_, 0));
        i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        channel.dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM;
        channel.dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM;
        channel.auto_clear_after_cb = true;
        ESP_ERROR_CHECK(i2s_new_channel(&channel, &tx_handle_, &rx_handle_));
        i2s_tdm_config_t i2s = {};
        i2s.clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(16000);
        i2s.slot_cfg = I2S_TDM_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, static_cast<i2s_tdm_slot_mask_t>(0x0f));
        i2s.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT;
        i2s.slot_cfg.total_slot = guition_es8389::kTdmSlots;
        i2s.slot_cfg.ws_width = 32;
        i2s.slot_cfg.left_align = true;
        i2s.gpio_cfg.mclk = GPIO_NUM_NC;
        i2s.gpio_cfg.bclk = bclk;
        i2s.gpio_cfg.ws = ws;
        i2s.gpio_cfg.dout = dout;
        i2s.gpio_cfg.din = din;
        ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(tx_handle_, &i2s));
        ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(rx_handle_, &i2s));
        ESP_ERROR_CHECK(i2s_channel_enable(tx_handle_));
        ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));

        audio_codec_i2s_cfg_t data_config = {};
        data_config.port = I2S_NUM_0;
        data_config.rx_handle = rx_handle_;
        data_config.tx_handle = tx_handle_;
        data_if_ = audio_codec_new_i2s_data(&data_config);
        assert(data_if_);
        audio_codec_i2c_cfg_t control_config = {};
        control_config.port = I2C_NUM_0;
        control_config.addr = ES8389_CODEC_DEFAULT_ADDR;
        control_config.bus_handle = bus;
        ctrl_if_ = audio_codec_new_i2c_ctrl(&control_config);
        assert(ctrl_if_);
        gpio_if_ = audio_codec_new_gpio();
        assert(gpio_if_);
        es8389_codec_cfg_t codec_config = {};
        codec_config.ctrl_if = ctrl_if_;
        codec_config.gpio_if = gpio_if_;
        codec_config.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
        // Own PA separately: enabling the microphone must never enable speakers.
        codec_config.pa_pin = GPIO_NUM_NC;
        codec_config.use_mclk = false;
        codec_config.no_dac_ref = false;
        codec_config.hw_gain.pa_voltage = 5.0;
        codec_config.hw_gain.codec_dac_voltage = 3.3;
        codec_config.hw_gain.pa_gain = 6.0;
        codec_if_ = es8389_codec_new(&codec_config);
        assert(codec_if_);
        esp_codec_dev_cfg_t device_config = {};
        device_config.codec_if = codec_if_;
        device_config.data_if = data_if_;
        device_config.dev_type = ESP_CODEC_DEV_TYPE_OUT;
        output_dev_ = esp_codec_dev_new(&device_config);
        assert(output_dev_);
        device_config.dev_type = ESP_CODEC_DEV_TYPE_IN;
        input_dev_ = esp_codec_dev_new(&device_config);
        assert(input_dev_);
        esp_codec_set_disable_when_closed(output_dev_, false);
        esp_codec_set_disable_when_closed(input_dev_, false);
    }

    ~GuitionEs8389Codec() override {
        gpio_set_level(pa_pin_, 0);
        esp_codec_dev_close(input_dev_);
        esp_codec_dev_close(output_dev_);
        esp_codec_dev_delete(input_dev_);
        esp_codec_dev_delete(output_dev_);
        audio_codec_delete_codec_if(codec_if_);
        audio_codec_delete_ctrl_if(ctrl_if_);
        audio_codec_delete_gpio_if(gpio_if_);
        audio_codec_delete_data_if(data_if_);
        i2s_channel_disable(rx_handle_);
        i2s_channel_disable(tx_handle_);
        i2s_del_channel(rx_handle_);
        i2s_del_channel(tx_handle_);
    }

    void SetOutputVolume(int volume) override {
        std::lock_guard<std::mutex> lock(control_mutex_);
        if (devices_open_) {
            ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev_, volume));
        }
        AudioCodec::SetOutputVolume(volume);
    }

    void SetInputGain(float gain) override {
        std::lock_guard<std::mutex> lock(control_mutex_);
        if (devices_open_) {
            ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(
                input_dev_, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0), gain));
        }
        AudioCodec::SetInputGain(gain);
    }

    void EnableInput(bool enable) override {
        std::lock_guard<std::mutex> lock(control_mutex_);
        if (enable) {
            OpenDevices();
        }
        AudioCodec::EnableInput(enable);
    }

    void EnableOutput(bool enable) override {
        std::lock_guard<std::mutex> lock(control_mutex_);
        if (enable) {
            OpenDevices();
            ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev_, output_volume_));
        }
        // esp_codec_dev 1.6.2's ES8389 output mute writes ADC register 0x20,
        // so it would also silence the microphone. Gate only the external PA.
        ESP_ERROR_CHECK(gpio_set_level(pa_pin_, enable));
        AudioCodec::EnableOutput(enable);
    }

private:
    const audio_codec_data_if_t* data_if_ = nullptr;
    const audio_codec_ctrl_if_t* ctrl_if_ = nullptr;
    const audio_codec_gpio_if_t* gpio_if_ = nullptr;
    const audio_codec_if_t* codec_if_ = nullptr;
    esp_codec_dev_handle_t output_dev_ = nullptr;
    esp_codec_dev_handle_t input_dev_ = nullptr;
    gpio_num_t pa_pin_;
    bool devices_open_ = false;
    std::mutex control_mutex_;
    std::array<int16_t, AUDIO_CODEC_DMA_FRAME_NUM * guition_es8389::kTdmSlots> input_buffer_{};
    std::array<int16_t, AUDIO_CODEC_DMA_FRAME_NUM * guition_es8389::kTdmSlots> output_buffer_{};

    void OpenDevices() {
        if (devices_open_) {
            return;
        }
        esp_codec_dev_sample_info_t format = {};
        format.bits_per_sample = 16;
        // Keep both adapters at the same four-slot format. Opening playback as
        // two channels would let codec_dev resize slots in the paired channel.
        format.channel = guition_es8389::kTdmSlots;
        format.channel_mask = 0x0f;
        format.sample_rate = 16000;
        format.mclk_multiple = 256;
        // codec_dev 1.6 open() does not propagate set_fmt() errors. Validate the
        // four-slot transport explicitly before opening either device.
        ESP_ERROR_CHECK(data_if_->set_fmt(data_if_, ESP_CODEC_DEV_TYPE_OUT, &format));
        ESP_ERROR_CHECK(esp_codec_dev_open(output_dev_, &format));
        ESP_ERROR_CHECK(data_if_->set_fmt(data_if_, ESP_CODEC_DEV_TYPE_IN, &format));
        ESP_ERROR_CHECK(esp_codec_dev_open(input_dev_, &format));
        // This mask addresses the analog MIC1 PGA, not the interleaved TD1
        // playback reference. Never apply software gain to MR input samples.
        ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(
            input_dev_, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0), input_gain_));
        ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev_, output_volume_));
        // Keep both interfaces open thereafter so disabling the PA cannot stop
        // the shared BCLK used by the microphone or repeatedly reset the codec.
        devices_open_ = true;
    }

    int Read(int16_t* dest, int samples) override {
        if (!input_enabled_ || samples % guition_es8389::kAecChannels != 0) {
            return 0;
        }
        int done = 0;
        while (done < samples) {
            int frames = std::min((samples - done) / guition_es8389::kAecChannels,
                                  AUDIO_CODEC_DMA_FRAME_NUM);
            if (esp_codec_dev_read(input_dev_, input_buffer_.data(),
                                   frames * guition_es8389::kTdmSlots * sizeof(int16_t)) !=
                ESP_CODEC_DEV_OK) {
                return 0;
            }
            guition_es8389::ExtractMicReference(input_buffer_.data(), dest + done, frames);
            done += frames * guition_es8389::kAecChannels;
        }
        return done;
    }

    int Write(const int16_t* data, int samples) override {
        if (!output_enabled_) {
            return 0;
        }
        int done = 0;
        while (done < samples) {
            int frames = std::min(samples - done, AUDIO_CODEC_DMA_FRAME_NUM);
            guition_es8389::PackPlayback(data + done, output_buffer_.data(), frames);
            if (esp_codec_dev_write(output_dev_, output_buffer_.data(),
                                    frames * guition_es8389::kTdmSlots * sizeof(int16_t)) !=
                ESP_CODEC_DEV_OK) {
                return done;
            }
            done += frames;
        }
        return done;
    }
};
