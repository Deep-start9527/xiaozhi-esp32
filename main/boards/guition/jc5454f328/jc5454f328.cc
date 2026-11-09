// SPDX-License-Identifier: MIT
#include "config.h"
#include "display/lcd_display.h"
#include "simplex_audio_codec.h"
#include "wifi_board.h"

#include <esp_lcd_axs15260.h>
#include <esp_lcd_panel_io_additions.h>

#include "lcd_init_commands.h"

class GuitionJc5454f328 : public WifiBoard {
public:
    GuitionJc5454f328() {
        // Hold the amplifier and backlight off throughout display/audio initialization.
        ESP_ERROR_CHECK(gpio_set_level(AUDIO_PA_PIN, 0));
        gpio_config_t pa = {};
        pa.pin_bit_mask = 1ULL << AUDIO_PA_PIN;
        pa.mode = GPIO_MODE_OUTPUT;
        ESP_ERROR_CHECK(gpio_config(&pa));
        GetBacklight();
        i2c_master_bus_config_t i2c = {};
        i2c.i2c_port = I2C_NUM_1;
        i2c.sda_io_num = AUDIO_I2C_SDA_PIN;
        i2c.scl_io_num = AUDIO_I2C_SCL_PIN;
        i2c.clk_source = I2C_CLK_SRC_DEFAULT;
        i2c.glitch_ignore_cnt = 7;
        i2c.flags.enable_internal_pullup = true;
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c, &audio_bus_));

        esp_lcd_panel_io_3wire_spi_config_t spi = {};
        spi.line_config.cs_io_type = IO_TYPE_GPIO;
        spi.line_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        spi.line_config.scl_io_type = IO_TYPE_GPIO;
        spi.line_config.scl_gpio_num = DISPLAY_SPI_CLK_PIN;
        spi.line_config.sda_io_type = IO_TYPE_GPIO;
        spi.line_config.sda_gpio_num = DISPLAY_SPI_DATA_PIN;
        spi.expect_clk_speed = 160000;
        spi.lcd_cmd_bytes = 1;
        spi.lcd_param_bytes = 1;
        spi.flags.use_dc_bit = true;
        spi.flags.del_keep_cs_inactive = true;
        esp_lcd_panel_io_handle_t io = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&spi, &io));

        esp_lcd_rgb_panel_config_t rgb = {};
        rgb.clk_src = LCD_CLK_SRC_DEFAULT;
        rgb.timings.pclk_hz = 34000000;
        rgb.timings.h_res = DISPLAY_WIDTH;
        rgb.timings.v_res = DISPLAY_HEIGHT;
        rgb.timings.hsync_pulse_width = 20;
        rgb.timings.hsync_back_porch = 160;
        rgb.timings.hsync_front_porch = 160;
        rgb.timings.vsync_pulse_width = 4;
        rgb.timings.vsync_back_porch = 20;
        rgb.timings.vsync_front_porch = 80;
        rgb.timings.flags.pclk_active_neg = false;
        rgb.data_width = 16;
        rgb.in_color_format = LCD_COLOR_FMT_RGB565;
        rgb.out_color_format = LCD_COLOR_FMT_RGB565;
        rgb.num_fbs = 2;
        rgb.bounce_buffer_size_px = DISPLAY_WIDTH * 10;
        rgb.dma_burst_size = 64;
        rgb.hsync_gpio_num = DISPLAY_HSYNC_PIN;
        rgb.vsync_gpio_num = DISPLAY_VSYNC_PIN;
        rgb.de_gpio_num = DISPLAY_DE_PIN;
        rgb.pclk_gpio_num = DISPLAY_PCLK_PIN;
        rgb.disp_gpio_num = GPIO_NUM_NC;
        const int pins[] = DISPLAY_RGB_PINS;
        for (int i = 0; i < 16; ++i) {
            rgb.data_gpio_nums[i] = static_cast<gpio_num_t>(pins[i]);
        }
        rgb.flags.fb_in_psram = true;

        axs15260_vendor_config_t vendor = {};
        vendor.rgb_config = &rgb;
        vendor.init_cmds = guition_lcd_init_cmds;
        vendor.init_cmds_size = sizeof(guition_lcd_init_cmds) / sizeof(guition_lcd_init_cmds[0]);
        esp_lcd_panel_dev_config_t config = {};
        config.reset_gpio_num = DISPLAY_RST_PIN;
        config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
        config.bits_per_pixel = 16;
        config.vendor_config = &vendor;
        esp_lcd_panel_handle_t panel = nullptr;
        // The AXS factory resets the controller, sends commands and consumes the IO handle.
        ESP_ERROR_CHECK(esp_lcd_new_panel_axs15260(io, &config, &panel));
        // AXS reset now delegates only to the SoC RGB peripheral reset; GPIO19 stays high.
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        display_ = new RgbLcdDisplay(nullptr, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, false,
                                     false, false);
        GetBacklight()->RestoreBrightness();
    }

    AudioCodec* GetAudioCodec() override {
        static GuitionSimplexAudio codec(
            {AUDIO_TX_BCLK_PIN, AUDIO_TX_WS_PIN, AUDIO_TX_DATA_PIN, AUDIO_RX_MCLK_PIN,
             AUDIO_RX_BCLK_PIN, AUDIO_RX_WS_PIN, AUDIO_RX_DATA_PIN, AUDIO_PA_PIN},
            false, audio_bus_);
        return &codec;
    }

    Display* GetDisplay() override { return display_; }

    Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, false, 20000);
        return &backlight;
    }

private:
    i2c_master_bus_handle_t audio_bus_ = nullptr;
    Display* display_ = nullptr;
};

DECLARE_BOARD(GuitionJc5454f328);
