#pragma once

#include "../guition_p4_board.h"

// Hardware routing and display timings from Guition's jc4880p443 board definition.
inline constexpr GuitionP4Config kBoardConfig = {
    .audio_din = GPIO_NUM_48,
    .audio_pa = GPIO_NUM_11,
    .lcd_reset = GPIO_NUM_5,
    .lane_bit_rate_mbps = 550,
    .dpi_clock_freq_mhz = 34,
    .video_timing =
        {
            .h_size = 480,
            .v_size = 800,
            .hsync_pulse_width = 12,
            .hsync_back_porch = 42,
            .hsync_front_porch = 42,
            .vsync_pulse_width = 2,
            .vsync_back_porch = 8,
            .vsync_front_porch = 166,
        },
};
