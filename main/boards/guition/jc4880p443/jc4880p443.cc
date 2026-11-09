#include "config.h"
#include "lcd_init_cmds.h"

class GuitionJC4880P443 : public GuitionP4Board {
public:
    GuitionJC4880P443() : GuitionP4Board(kBoardConfig) {
        st7701_vendor_config_t vendor = {};
        vendor.init_cmds = lcd_cmd;
        vendor.init_cmds_size = sizeof(lcd_cmd) / sizeof(lcd_cmd[0]);
        vendor.flags.use_mipi_interface = true;
        InitializeDisplay(vendor, esp_lcd_new_panel_st7701);
        GetBacklight()->RestoreBrightness();
    }
};

DECLARE_BOARD(GuitionJC4880P443);
