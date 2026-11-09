#include "config.h"
#include "lcd_init_cmds.h"

class GuitionJC1060P470IWY : public GuitionP4Board {
public:
    GuitionJC1060P470IWY() : GuitionP4Board(kBoardConfig) {
        jd9165_vendor_config_t vendor = {};
        vendor.init_cmds = lcd_cmd;
        vendor.init_cmds_size = sizeof(lcd_cmd) / sizeof(lcd_cmd[0]);
        InitializeDisplay(vendor, esp_lcd_new_panel_jd9165);
        GetBacklight()->RestoreBrightness();
    }
};

DECLARE_BOARD(GuitionJC1060P470IWY);
