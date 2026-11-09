#pragma once

#include "../guition_p4_board.h"

// Hardware routing and display timings from Guition's jc8012p4a1 board definition.
inline constexpr GuitionP4Config kBoardConfig = {
    .audio_din = GPIO_NUM_11,
    .audio_pa = GPIO_NUM_20,
    .lcd_reset = GPIO_NUM_27,
    .lane_bit_rate_mbps = 1500,
    .dpi_clock_freq_mhz = 70,
    .video_timing =
        {
            .h_size = 800,
            .v_size = 1280,
            .hsync_pulse_width = 20,
            .hsync_back_porch = 20,
            .hsync_front_porch = 40,
            .vsync_pulse_width = 10,
            .vsync_back_porch = 4,
            .vsync_front_porch = 30,
        },
};
