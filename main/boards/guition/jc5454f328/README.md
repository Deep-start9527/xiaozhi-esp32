# Guition JC5454F328

ESP32-S31，16 MB Flash / 八线 PSRAM，540×540 AXS15260 RGB 屏幕与 ES7210 麦克风 ADC。需要 ESP-IDF 6.1。

```fish
source ~/.espressif/tools/activate_idf_v6.1.fish
python3 scripts/build.py guition/jc5454f328 --name guition-jc5454f328
```

| 功能 | GPIO / 配置 |
| --- | --- |
| LCD 初始化 CS / CLK / DATA | 36 / 42 / 40；160 kHz 三线 SPI |
| LCD RGB D0–D15 | 13, 14, 15, 16, 17, 7, 8, 9, 10, 11, 12, 2, 3, 4, 5, 6 |
| LCD HSYNC / VSYNC / DE / PCLK | 35 / 37 / 38 / 18；34 MHz |
| LCD RESET / 背光 | 19 / 48；背光高有效、20 kHz PWM |
| ES7210 I2C SDA / SCL | 53 / 54；I2C1，7 位地址 0x40 |
| ES7210 MCLK / BCLK / WS / DATA | 55 / 56 / 57 / 59；I2S1，16 kHz，MCLK=256×Fs |
| I2S 功放 BCLK / WS / DATA | 49 / 51 / 61；I2S0，24 kHz，无 MCLK |
| 功放使能 | 52，高有效 |
| 网络 | ESP32-S31 原生 Wi-Fi |

参考 `Guition-fork/components/board/guition_boards/jc5454f328`。RGB 同步时序采用参考值；为匹配现有 `RgbLcdDisplay`，使用两个 PSRAM 帧缓冲和 10 行 bounce buffer。三线 SPI 在初始化序列发送完后由 AXS 驱动释放；随后的 panel reset 仅复位 SoC RGB 外设，不再次复位屏幕控制器。PCLK 使用参考配置的正沿与低空闲电平。启动先拉低功放使能并将背光 PWM 置零，显示初始化结束后恢复亮度。

ES7210 启用 MIC1、MIC2 的标准双声道 16 位 I2S，应用读取 MIC1 为单声道，不把 MIC2 当成 AEC 参考；ADC 增益默认 30 dB。输入与输出有独立时钟与 I2S 控制器。输出将单声道 PCM 复制到左右时隙，软件调音量，播放关闭时拉低功放使能。设备端 AEC 默认关闭。

本次接入显示、亮度、网络和录放音；未初始化触摸、SD 卡、SPI NAND 或猜测接线的按键。首次启动使用标准 Wi-Fi 配网流程。

实机验收：屏幕方向、颜色、连续刷新和亮度；配网和重连；MIC1 声道与音量；播放、唤醒、打断及功放开关噪声。编译通过不表示这些硬件项目已验证。

板级引脚参考保留 MIT 许可；LCD 命令表保留 Espressif Apache-2.0 声明，许可证见 `LICENSE`、`LICENSES/Apache-2.0.txt` 和 `NOTICE`。
