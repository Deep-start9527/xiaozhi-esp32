#pragma once

#include "../guition_p4_board.h"

// Hardware routing and display timings from Guition's jc1060p470_i_w_y board definition.
inline constexpr GuitionP4Config kBoardConfig = {
    .audio_din = GPIO_NUM_48,
    .audio_pa = GPIO_NUM_11,
    .lcd_reset = GPIO_NUM_0,
    .lane_bit_rate_mbps = 750,
    .dpi_clock_freq_mhz = 52,
    .video_timing =
        {
            .h_size = 1024,
            .v_size = 600,
            .hsync_pulse_width = 24,
            .hsync_back_porch = 136,
            .hsync_front_porch = 160,
            .vsync_pulse_width = 2,
            .vsync_back_porch = 21,
            .vsync_front_porch = 12,
        },
};
