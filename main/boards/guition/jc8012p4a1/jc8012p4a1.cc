#include "config.h"
#include "lcd_init_cmds.h"

class GuitionJC8012P4A1 : public GuitionP4Board {
public:
    GuitionJC8012P4A1() : GuitionP4Board(kBoardConfig) {
        jd9365_vendor_config_t vendor = {};
        vendor.init_cmds = lcd_cmd;
        vendor.init_cmds_size = sizeof(lcd_cmd) / sizeof(lcd_cmd[0]);
        vendor.mipi_config.lane_num = 2;
        InitializeDisplay(vendor, esp_lcd_new_panel_jd9365);
        GetBacklight()->RestoreBrightness();
    }
};

DECLARE_BOARD(GuitionJC8012P4A1);
