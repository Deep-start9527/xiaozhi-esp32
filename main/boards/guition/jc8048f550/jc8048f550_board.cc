#include "config.h"

#include "display/lcd_display.h"
#include "es8389_codec.h"
#include "wifi_board.h"

#include <driver/i2c_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>

class JC8048F550Board : public WifiBoard {
public:
    JC8048F550Board() {
        // Hold the speakers and backlight off throughout peripheral setup.
        ESP_ERROR_CHECK(gpio_set_direction(AUDIO_CODEC_PA_PIN, GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_level(AUDIO_CODEC_PA_PIN, 0));
        GetBacklight();
        i2c_master_bus_config_t bus_config = {};
        bus_config.i2c_port = I2C_NUM_0;
        bus_config.sda_io_num = AUDIO_CODEC_I2C_SDA_PIN;
        bus_config.scl_io_num = AUDIO_CODEC_I2C_SCL_PIN;
        bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_config.glitch_ignore_cnt = 7;
        bus_config.flags.enable_internal_pullup = true;
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_));

        esp_lcd_rgb_panel_config_t panel_config = {};
        panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
        panel_config.timings.pclk_hz = DISPLAY_PCLK_HZ;
        panel_config.timings.h_res = DISPLAY_WIDTH;
        panel_config.timings.v_res = DISPLAY_HEIGHT;
        panel_config.timings.hsync_pulse_width = DISPLAY_HSYNC_PULSE;
        panel_config.timings.hsync_back_porch = DISPLAY_HSYNC_BACK_PORCH;
        panel_config.timings.hsync_front_porch = DISPLAY_HSYNC_FRONT_PORCH;
        panel_config.timings.vsync_pulse_width = DISPLAY_VSYNC_PULSE;
        panel_config.timings.vsync_back_porch = DISPLAY_VSYNC_BACK_PORCH;
        panel_config.timings.vsync_front_porch = DISPLAY_VSYNC_FRONT_PORCH;
        panel_config.timings.flags.pclk_active_neg = true;
        panel_config.data_width = 16;
        panel_config.in_color_format = LCD_COLOR_FMT_RGB565;
        panel_config.out_color_format = LCD_COLOR_FMT_RGB565;
        // RgbLcdDisplay uses two framebuffers and bounce-buffer callbacks.
        panel_config.num_fbs = 2;
        panel_config.bounce_buffer_size_px = DISPLAY_WIDTH * 8;
        panel_config.dma_burst_size = 64;
        panel_config.hsync_gpio_num = DISPLAY_HSYNC_PIN;
        panel_config.vsync_gpio_num = DISPLAY_VSYNC_PIN;
        panel_config.de_gpio_num = DISPLAY_DE_PIN;
        panel_config.pclk_gpio_num = DISPLAY_PCLK_PIN;
        panel_config.disp_gpio_num = GPIO_NUM_NC;
        for (int i = 0; i < 16; ++i) {
            panel_config.data_gpio_nums[i] = static_cast<gpio_num_t>(DISPLAY_DATA_PINS[i]);
        }
        panel_config.flags.fb_in_psram = true;
        esp_lcd_panel_handle_t panel = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        display_ = new RgbLcdDisplay(nullptr, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, false,
                                     false, false);
        GetBacklight()->RestoreBrightness();
    }

    AudioCodec* GetAudioCodec() override {
        static GuitionEs8389Codec codec(i2c_bus_, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
                                        AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN, AUDIO_CODEC_PA_PIN,
                                        AUDIO_OUTPUT_SOFTWARE_GAIN_DB);
        return &codec;
    }

    Display* GetDisplay() override { return display_; }

    Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, false, 20000);
        return &backlight;
    }

private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    LcdDisplay* display_ = nullptr;
};

DECLARE_BOARD(JC8048F550Board);
