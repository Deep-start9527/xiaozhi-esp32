# Guition JC8012P4A1

本配置仅支持 **ESP32-P4 3.x**，16 MB Flash、板载 PSRAM。屏幕采用 LCD V2（参考配置默认版本），
JD9365 MIPI DSI，800 × 1280，DPI 70 MHz，双通道 1500 Mbps。
不同屏幕版本的初始化表与时序不可混用。

显示格式统一为 RGB565：小智 LVGL 缓冲、DPI 输出与面板设备配置均为 16 位；
参考 YAML 的 `bits_per_pixel: 24` 在这里调整为 16，以保持传输格式一致。

## 接线

| 功能 | GPIO |
| --- | --- |
| ES8311 I2C SDA / SCL | 7 / 8 |
| I2S MCLK / BCLK / WS | 13 / 12 / 10 |
| I2S DOUT / DIN | 9 / 11 |
| 功放使能（高有效） | 20 |
| LCD 复位 / 背光（高有效） | 27 / 23 |
| 无线 SDIO CLK / CMD | 18 / 19 |
| 无线 SDIO D0 / D1 / D2 / D3 | 14 / 15 / 16 / 17 |
| 无线从机复位 | 54 |

MIPI PHY 使用 LDO3，2.5 V。ES8311 使用参考设计的 BCLK 时钟模式
（不依赖 MCLK），I2S0 全双工 24 kHz、单声道麦克风，不启用参考通道/AEC。
需预先烧录与 ESP-Hosted 主机兼容的无线从机固件。

## 构建

在 fish 中启用指定 SDK（建议 ESP-IDF release/v6.1）：

```fish
source ~/.espressif/tools/activate_idf_v6.1.fish
idf.py --version
python3 scripts/build.py guition/jc8012p4a1 --name guition-jc8012p4a1
```

已接入显示、PWM 背光、Wi-Fi 与录放音。触摸、摄像头、SD 卡、编码器、传感器未接入；
没有可靠接线依据的按键不初始化。首次启动使用小智现有配网流程。

## 实机验收

编译通过不代表硬件验证。需检查启动、屏幕颜色/方向/稳定性、亮度、配网/重连、
麦克风录音、扬声器播放、唤醒及打断。特别检查所用实物的屏幕版本匹配本配置。

引脚与屏幕命令表来自 `Guition-fork/components/board/guition_boards/jc8012p4a1`。
移植命令表保留上游版权和许可证，详见 `NOTICE` 与 `LICENSES/`。
