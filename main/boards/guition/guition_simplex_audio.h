// SPDX-License-Identifier: MIT
#pragma once

#include "audio_codec.h"

#include <driver/i2c_master.h>
#include <driver/i2s_pdm.h>
#include <esp_codec_dev_defaults.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <mutex>

// Independent RX/TX controllers, with stereo wire frames and mono application audio.
// Buffers are bounded and allocated once, including for long playback packets.
class GuitionSimplexAudio : public AudioCodec {
public:
    struct Pins {
        gpio_num_t tx_bclk, tx_ws, tx_data;
        gpio_num_t rx_mclk, rx_bclk, rx_ws, rx_data;
        gpio_num_t pa;
    };

    GuitionSimplexAudio(const Pins& pins, bool pdm, i2c_master_bus_handle_t bus = nullptr)
        : pa_(pins.pa) {
        if (pa_ != GPIO_NUM_NC) {
            ESP_ERROR_CHECK(gpio_set_level(pa_, 0));
            gpio_config_t gpio = {};
            gpio.pin_bit_mask = 1ULL << pa_;
            gpio.mode = GPIO_MODE_OUTPUT;
            ESP_ERROR_CHECK(gpio_config(&gpio));
        }
        input_sample_rate_ = 16000;
        output_sample_rate_ = 24000;
        input_channels_ = output_channels_ = 1;
        input_reference_ = false;
        duplex_ = false;

        i2s_chan_config_t tx = I2S_CHANNEL_DEFAULT_CONFIG(pdm ? 1 : 0, I2S_ROLE_MASTER);
        tx.dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM;
        tx.dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM;
        tx.auto_clear_after_cb = true;
        ESP_ERROR_CHECK(i2s_new_channel(&tx, &tx_handle_, nullptr));
        i2s_std_config_t out = {};
        out.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(static_cast<uint32_t>(output_sample_rate_));
        out.slot_cfg =
            I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
        out.gpio_cfg.mclk = I2S_GPIO_UNUSED;
        out.gpio_cfg.bclk = pins.tx_bclk;
        out.gpio_cfg.ws = pins.tx_ws;
        out.gpio_cfg.dout = pins.tx_data;
        out.gpio_cfg.din = I2S_GPIO_UNUSED;
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &out));

        i2s_chan_config_t rx = I2S_CHANNEL_DEFAULT_CONFIG(pdm ? 0 : 1, I2S_ROLE_MASTER);
        rx.dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM;
        rx.dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM;
        ESP_ERROR_CHECK(i2s_new_channel(&rx, nullptr, &rx_handle_));
        if (pdm) {
            i2s_pdm_rx_config_t in = {};
            in.clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(static_cast<uint32_t>(input_sample_rate_));
            in.slot_cfg =
                I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
            in.gpio_cfg.clk = pins.rx_bclk;
            in.gpio_cfg.din = pins.rx_data;
            ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle_, &in));
        } else {
            auto in = out;
            in.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(static_cast<uint32_t>(input_sample_rate_));
            in.gpio_cfg.mclk = pins.rx_mclk;
            in.gpio_cfg.bclk = pins.rx_bclk;
            in.gpio_cfg.ws = pins.rx_ws;
            in.gpio_cfg.dout = I2S_GPIO_UNUSED;
            in.gpio_cfg.din = pins.rx_data;
            ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &in));
            // Keep MCLK available while configuring the ES7210, on its own I2S port.
            ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
            audio_codec_i2c_cfg_t ctrl = {};
            ctrl.port = I2C_NUM_1;
            ctrl.addr = ES7210_CODEC_DEFAULT_ADDR;
            ctrl.bus_handle = bus;
            ctrl_ = audio_codec_new_i2c_ctrl(&ctrl);
            assert(ctrl_);
            es7210_codec_cfg_t adc = {};
            adc.ctrl_if = ctrl_;
            adc.mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2;
            adc.mclk_div = 256;
            adc_ = es7210_codec_new(&adc);
            assert(adc_);
            esp_codec_dev_sample_info_t fs = {};
            fs.bits_per_sample = 16;
            fs.channel = 2;
            fs.sample_rate = input_sample_rate_;
            fs.mclk_multiple = 256;
            ESP_ERROR_CHECK(adc_->set_fs(adc_, &fs));
            ESP_ERROR_CHECK(adc_->set_mic_gain(adc_, 30.0f));
            input_gain_ = 30.0f;
            ESP_ERROR_CHECK(adc_->enable(adc_, false));
            ESP_ERROR_CHECK(i2s_channel_disable(rx_handle_));
        }
    }

    ~GuitionSimplexAudio() override {
        EnableOutput(false);
        EnableInput(false);
        if (adc_)
            audio_codec_delete_codec_if(adc_);
        if (ctrl_)
            audio_codec_delete_ctrl_if(ctrl_);
        i2s_del_channel(rx_handle_);
        i2s_del_channel(tx_handle_);
    }

    void EnableInput(bool enable) override {
        std::lock_guard<std::mutex> lock(rx_mutex_);
        if (enable == input_enabled_)
            return;
        if (enable) {
            ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
            if (adc_)
                ESP_ERROR_CHECK(adc_->enable(adc_, true));
        } else {
            if (adc_)
                ESP_ERROR_CHECK(adc_->enable(adc_, false));
            ESP_ERROR_CHECK(i2s_channel_disable(rx_handle_));
        }
        AudioCodec::EnableInput(enable);
    }

    void EnableOutput(bool enable) override {
        std::lock_guard<std::mutex> lock(tx_mutex_);
        if (enable == output_enabled_)
            return;
        if (enable)
            ESP_ERROR_CHECK(i2s_channel_enable(tx_handle_));
        if (pa_ != GPIO_NUM_NC)
            ESP_ERROR_CHECK(gpio_set_level(pa_, enable));
        if (!enable)
            ESP_ERROR_CHECK(i2s_channel_disable(tx_handle_));
        AudioCodec::EnableOutput(enable);
    }

    void SetInputGain(float gain) override {
        std::lock_guard<std::mutex> lock(rx_mutex_);
        if (adc_)
            ESP_ERROR_CHECK(adc_->set_mic_gain(adc_, gain));
        AudioCodec::SetInputGain(gain);
    }

    void SetOutputVolume(int volume) override {
        std::lock_guard<std::mutex> lock(tx_mutex_);
        AudioCodec::SetOutputVolume(std::clamp(volume, 0, 100));
    }

protected:
    int Read(int16_t* dest, int samples) override {
        int done = 0;
        while (done < samples) {
            std::lock_guard<std::mutex> lock(rx_mutex_);
            if (!input_enabled_)
                break;
            const int count = std::min(samples - done, kFrames);
            size_t bytes = 0;
            auto err = i2s_channel_read(rx_handle_, rx_buffer_.data(), count * 2 * sizeof(int16_t),
                                        &bytes, 200);
            const int frames = bytes / (2 * sizeof(int16_t));
            // MIC1 (ES7210) / left PDM edge; the second channel is not an AEC reference.
            for (int i = 0; i < frames; ++i)
                dest[done + i] = rx_buffer_[2 * i];
            done += frames;
            if (err != ESP_OK || frames == 0)
                break;
        }
        // AudioCodec::InputData accepts any positive length; don't leave stale tail samples.
        std::fill(dest + done, dest + samples, 0);
        return done;
    }

    int Write(const int16_t* data, int samples) override {
        int done = 0;
        while (done < samples) {
            std::lock_guard<std::mutex> lock(tx_mutex_);
            if (!output_enabled_)
                break;
            const int count = std::min(samples - done, kFrames);
            for (int i = 0; i < count; ++i) {
                const int16_t sample = static_cast<int32_t>(data[done + i]) * output_volume_ / 100;
                tx_buffer_[2 * i] = tx_buffer_[2 * i + 1] = sample;
            }
            size_t bytes = 0;
            auto err = i2s_channel_write(tx_handle_, tx_buffer_.data(), count * 2 * sizeof(int16_t),
                                         &bytes, 200);
            const int frames = bytes / (2 * sizeof(int16_t));
            done += frames;
            if (err != ESP_OK || frames == 0)
                break;
        }
        return done;
    }

private:
    static constexpr int kFrames = AUDIO_CODEC_DMA_FRAME_NUM;
    gpio_num_t pa_;
    const audio_codec_ctrl_if_t* ctrl_ = nullptr;
    const audio_codec_if_t* adc_ = nullptr;
    std::array<int16_t, kFrames * 2> rx_buffer_{};
    std::array<int16_t, kFrames * 2> tx_buffer_{};
    std::mutex rx_mutex_;
    std::mutex tx_mutex_;
};
