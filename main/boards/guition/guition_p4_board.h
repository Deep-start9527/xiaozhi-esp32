#pragma once

#include <driver/i2c_master.h>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_ldo_regulator.h>

#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "wifi_board.h"

struct GuitionP4Config {
    gpio_num_t audio_din;
    gpio_num_t audio_pa;
    gpio_num_t lcd_reset;
    float lane_bit_rate_mbps;
    float dpi_clock_freq_mhz;
    esp_lcd_video_timing_t video_timing;
};

// Shared only by the Guition P4 boards; pin differences stay in each board's config.h.
class GuitionP4Board : public WifiBoard {
private:
    GuitionP4Config config_;
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    esp_ldo_channel_handle_t phy_power_ = nullptr;
    esp_lcd_dsi_bus_handle_t dsi_bus_ = nullptr;
    LcdDisplay* display_ = nullptr;

protected:
    explicit GuitionP4Board(const GuitionP4Config& config) : config_(config) {
        // Keep the amplifier and display dark until their peripherals are ready.
        gpio_config_t gpio = {};
        gpio.pin_bit_mask = (1ULL << config_.audio_pa) | (1ULL << GPIO_NUM_23);
        gpio.mode = GPIO_MODE_OUTPUT;
        ESP_ERROR_CHECK(gpio_config(&gpio));
        ESP_ERROR_CHECK(gpio_set_level(config_.audio_pa, 0));
        ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_23, 0));

        i2c_master_bus_config_t bus = {};
        bus.i2c_port = I2C_NUM_0;
        bus.sda_io_num = GPIO_NUM_7;
        bus.scl_io_num = GPIO_NUM_8;
        bus.clk_source = I2C_CLK_SRC_DEFAULT;
        bus.glitch_ignore_cnt = 7;
        bus.flags.enable_internal_pullup = true;
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus, &i2c_bus_));
    }

    template <typename VendorConfig, typename PanelFactory>
    void InitializeDisplay(VendorConfig vendor, PanelFactory create_panel) {
        esp_ldo_channel_config_t ldo = {};
        ldo.chan_id = 3;
        ldo.voltage_mv = 2500;
        ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo, &phy_power_));

        esp_lcd_dsi_bus_config_t bus = {};
        bus.bus_id = 0;
        bus.num_data_lanes = 2;
        bus.lane_bit_rate_mbps = config_.lane_bit_rate_mbps;
        ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus, &dsi_bus_));

        esp_lcd_dbi_io_config_t dbi = {};
        dbi.virtual_channel = 0;
        dbi.lcd_cmd_bits = 8;
        dbi.lcd_param_bits = 8;
        esp_lcd_panel_io_handle_t io = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus_, &dbi, &io));

        esp_lcd_dpi_panel_config_t dpi = {};
        dpi.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
        dpi.dpi_clock_freq_mhz = config_.dpi_clock_freq_mhz;
        dpi.in_color_format = LCD_COLOR_FMT_RGB565;
        dpi.out_color_format = LCD_COLOR_FMT_RGB565;
        // MipiLcdDisplay uses partial rendering without frame-buffer swapping.
        dpi.num_fbs = 1;
        dpi.video_timing = config_.video_timing;
        vendor.mipi_config.dsi_bus = dsi_bus_;
        vendor.mipi_config.dpi_config = &dpi;

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = config_.lcd_reset;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
        // Match the RGB565 DPI transport and LVGL buffers.
        panel_config.bits_per_pixel = 16;
        panel_config.vendor_config = &vendor;
        esp_lcd_panel_handle_t panel = nullptr;
        ESP_ERROR_CHECK(create_panel(io, &panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        display_ = new MipiLcdDisplay(io, panel, config_.video_timing.h_size,
                                      config_.video_timing.v_size, 0, 0, false, false, false);
    }

public:
    AudioCodec* GetAudioCodec() override {
        // Guition configures ES8311 to derive its clock from BCLK (no_mclk=true).
        static Es8311AudioCodec codec(i2c_bus_, I2C_NUM_0, 24000, 24000, GPIO_NUM_13, GPIO_NUM_12,
                                      GPIO_NUM_10, GPIO_NUM_9, config_.audio_din, config_.audio_pa,
                                      ES8311_CODEC_DEFAULT_ADDR, false);
        return &codec;
    }

    Display* GetDisplay() override { return display_; }

    Backlight* GetBacklight() override {
        static PwmBacklight backlight(GPIO_NUM_23, false);
        return &backlight;
    }
};
