// SPDX-License-Identifier: MIT
#include "../guition_simplex_audio.h"
#include "config.h"
#include "display/lcd_display.h"
#include "wifi_board.h"

#include <driver/spi_common.h>
#include <esp_lcd_st77916.h>

#include "lcd_init_commands.h"

class GuitionJc3636f118 : public WifiBoard {
public:
    GuitionJc3636f118() {
        // Construct PWM at zero duty before the controller's lengthy reset/init sequence.
        GetBacklight();
        spi_bus_config_t bus = {};
        bus.sclk_io_num = DISPLAY_CLK_PIN;
        bus.data0_io_num = DISPLAY_DATA0_PIN;
        bus.data1_io_num = DISPLAY_DATA1_PIN;
        bus.data2_io_num = DISPLAY_DATA2_PIN;
        bus.data3_io_num = DISPLAY_DATA3_PIN;
        bus.max_transfer_sz = DISPLAY_WIDTH * 40 * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = GPIO_NUM_NC;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 80000000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 32;
        io_config.lcd_param_bits = 8;
        io_config.flags.quad_mode = true;
        esp_lcd_panel_io_handle_t io = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io));

        st77916_vendor_config_t vendor = {};
        vendor.init_cmds = guition_lcd_init_cmds;
        vendor.init_cmds_size = sizeof(guition_lcd_init_cmds) / sizeof(guition_lcd_init_cmds[0]);
        vendor.flags.use_qspi_interface = true;
        esp_lcd_panel_dev_config_t config = {};
        config.reset_gpio_num = DISPLAY_RST_PIN;
        config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        config.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
        config.bits_per_pixel = 16;
        config.vendor_config = &vendor;
        esp_lcd_panel_handle_t panel = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st77916(io, &config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
        display_ =
            new SpiLcdDisplay(io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, 0, false, false, false);
        GetBacklight()->RestoreBrightness();
    }

    AudioCodec* GetAudioCodec() override {
        static GuitionSimplexAudio codec(
            {AUDIO_TX_BCLK_PIN, AUDIO_TX_WS_PIN, AUDIO_TX_DATA_PIN, GPIO_NUM_NC, AUDIO_PDM_CLK_PIN,
             GPIO_NUM_NC, AUDIO_PDM_DATA_PIN, GPIO_NUM_NC},
            true);
        return &codec;
    }

    Display* GetDisplay() override { return display_; }

    Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, false, 5000);
        return &backlight;
    }

private:
    Display* display_ = nullptr;
};

DECLARE_BOARD(GuitionJc3636f118);
