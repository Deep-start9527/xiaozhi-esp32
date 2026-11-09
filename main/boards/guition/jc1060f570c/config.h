// Hardware mapping adapted from Guition contributors' board YAML (MIT).
// See LICENSE and NOTICE in this directory.
#pragma once

#include <driver/gpio.h>

#define AUDIO_CODEC_I2C_SDA_PIN GPIO_NUM_2
#define AUDIO_CODEC_I2C_SCL_PIN GPIO_NUM_3
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_54
#define AUDIO_I2S_GPIO_WS GPIO_NUM_47
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_56
#define AUDIO_I2S_GPIO_DIN GPIO_NUM_16
// GPIO42 is exclusively the speaker amplifier enable, never an ADC button.
#define AUDIO_CODEC_PA_PIN GPIO_NUM_42

#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 600
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_52
#define DISPLAY_PCLK_HZ 34000000
#define DISPLAY_HSYNC_PULSE 24
#define DISPLAY_HSYNC_BACK_PORCH 160
#define DISPLAY_HSYNC_FRONT_PORCH 136
#define DISPLAY_VSYNC_PULSE 1
#define DISPLAY_VSYNC_BACK_PORCH 23
#define DISPLAY_VSYNC_FRONT_PORCH 11
#define DISPLAY_HSYNC_PIN GPIO_NUM_44
#define DISPLAY_VSYNC_PIN GPIO_NUM_45
#define DISPLAY_DE_PIN GPIO_NUM_43
#define DISPLAY_PCLK_PIN GPIO_NUM_40

// Data lane order is model specific, especially green lanes 7 through 10.
inline constexpr int DISPLAY_DATA_PINS[16] = {36, 37, 38, 39, 46, 7, 35, 48,
                                              49, 50, 51, 4,  5,  0, 1,  6};
