#include <driver/i2c_master.h>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_ldo_regulator.h>

#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "wifi_board.h"

#include "config.h"
#include "lcd_init_cmds.h"

class GuitionJC1060P470 : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    esp_ldo_channel_handle_t phy_power_ = nullptr;
    esp_lcd_dsi_bus_handle_t dsi_bus_ = nullptr;
    LcdDisplay* display_ = nullptr;

    void InitializeHardware() {
        // Keep the amplifier and display dark until their peripherals are ready.
        gpio_config_t gpio = {};
        gpio.pin_bit_mask = (1ULL << AUDIO_CODEC_PA_PIN) | (1ULL << DISPLAY_BACKLIGHT_PIN);
        gpio.mode = GPIO_MODE_OUTPUT;
        ESP_ERROR_CHECK(gpio_config(&gpio));
        ESP_ERROR_CHECK(gpio_set_level(AUDIO_CODEC_PA_PIN, 0));
        ESP_ERROR_CHECK(gpio_set_level(DISPLAY_BACKLIGHT_PIN, 0));

        i2c_master_bus_config_t bus = {};
        bus.i2c_port = AUDIO_CODEC_I2C_PORT;
        bus.sda_io_num = AUDIO_CODEC_I2C_SDA_PIN;
        bus.scl_io_num = AUDIO_CODEC_I2C_SCL_PIN;
        bus.clk_source = I2C_CLK_SRC_DEFAULT;
        bus.glitch_ignore_cnt = 7;
        bus.flags.enable_internal_pullup = true;
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus, &i2c_bus_));
    }

    void InitializeDisplay() {
        esp_ldo_channel_config_t ldo = {};
        ldo.chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN;
        ldo.voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV;
        ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo, &phy_power_));

        esp_lcd_dsi_bus_config_t bus = {};
        bus.bus_id = 0;
        bus.num_data_lanes = LCD_MIPI_DSI_LANE_NUM;
        bus.lane_bit_rate_mbps = LCD_MIPI_DSI_LANE_BITRATE_MBPS;
        ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus, &dsi_bus_));

        esp_lcd_dbi_io_config_t dbi = {};
        dbi.virtual_channel = 0;
        dbi.lcd_cmd_bits = 8;
        dbi.lcd_param_bits = 8;
        esp_lcd_panel_io_handle_t io = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus_, &dbi, &io));

        esp_lcd_dpi_panel_config_t dpi = {};
        dpi.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
        dpi.dpi_clock_freq_mhz = LCD_DPI_CLOCK_FREQ_MHZ;
        dpi.in_color_format = LCD_COLOR_FMT_RGB565;
        dpi.out_color_format = LCD_COLOR_FMT_RGB565;
        // MipiLcdDisplay uses partial rendering without frame-buffer swapping.
        dpi.num_fbs = 1;
        dpi.video_timing = {
            .h_size = DISPLAY_WIDTH,
            .v_size = DISPLAY_HEIGHT,
            .hsync_pulse_width = LCD_HSYNC_PULSE_WIDTH,
            .hsync_back_porch = LCD_HSYNC_BACK_PORCH,
            .hsync_front_porch = LCD_HSYNC_FRONT_PORCH,
            .vsync_pulse_width = LCD_VSYNC_PULSE_WIDTH,
            .vsync_back_porch = LCD_VSYNC_BACK_PORCH,
            .vsync_front_porch = LCD_VSYNC_FRONT_PORCH,
        };

        jd9165_vendor_config_t vendor = {};
        vendor.init_cmds = lcd_cmd;
        vendor.init_cmds_size = sizeof(lcd_cmd) / sizeof(lcd_cmd[0]);
        vendor.mipi_config.dsi_bus = dsi_bus_;
        vendor.mipi_config.dpi_config = &dpi;

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RESET_PIN;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
        // Match the RGB565 DPI transport and LVGL buffers.
        panel_config.bits_per_pixel = 16;
        panel_config.vendor_config = &vendor;
        esp_lcd_panel_handle_t panel = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_panel_jd9165(io, &panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        display_ =
            new MipiLcdDisplay(io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, false, false, false);
    }

public:
    GuitionJC1060P470() {
        InitializeHardware();
        InitializeDisplay();
        GetBacklight()->RestoreBrightness();
    }

    AudioCodec* GetAudioCodec() override {
        // Guition configures ES8311 to derive its clock from BCLK (no_mclk=true).
        static Es8311AudioCodec codec(
            i2c_bus_, AUDIO_CODEC_I2C_PORT, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN, AUDIO_CODEC_PA_PIN, ES8311_CODEC_DEFAULT_ADDR, false);
        return &codec;
    }

    Display* GetDisplay() override { return display_; }

    Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }
};

DECLARE_BOARD(GuitionJC1060P470);
