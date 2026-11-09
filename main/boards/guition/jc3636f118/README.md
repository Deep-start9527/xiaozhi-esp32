# Guition JC3636F118

ESP32-S31，16 MB Flash / 八线 PSRAM，360×360 ST77916 QSPI 圆屏。需要 ESP-IDF 6.1。

```fish
source ~/.espressif/tools/activate_idf_v6.1.fish
python3 scripts/build.py guition/jc3636f118 --name guition-jc3636f118
```

| 功能 | GPIO / 配置 |
| --- | --- |
| LCD QSPI CLK / CS / D0–D3 | 12 / 13 / 14, 15, 16, 17；80 MHz |
| LCD RESET / 背光 | 18 / 5；背光高有效、5 kHz PWM |
| PDM 麦克风 CLK / DATA | 39 / 37；I2S0，16 kHz PCM |
| I2S 功放 BCLK / WS / DATA | 53 / 51 / 52；I2S1，24 kHz |
| 网络 | ESP32-S31 原生 Wi-Fi |

参考 `Guition-fork/components/board/guition_boards/jc3636f118`。屏幕初始化表保持原厂序列与颜色反转。启动先将背光 PWM 置零，显示初始化结束后恢复亮度。输入采集 PDM 两个时隙，当前以左时隙作为单声道麦克风；右时隙不当作 AEC 参考。实机需要确认麦克风有效时隙，若板卡焊接版本改变，应依据接线调整采样映射。输出将小智单声道 PCM 复制到左右两个 16 位 I2S 时隙。

本次接入显示、亮度、网络和录放音；未初始化触摸、编码器、SD 卡、陀螺仪、电池 ADC 或猜测接线的按键。设备端 AEC 默认关闭。首次启动使用标准 Wi-Fi 配网流程。

实机验收：屏幕方向、颜色与亮度；配网和重连；PDM 有效时隙与录音幅值；播放、唤醒及打断。编译通过不表示这些硬件项目已验证。

板级引脚参考保留 MIT 许可；LCD 命令表保留 Espressif Apache-2.0 声明，许可证见 `LICENSE`、`LICENSES/Apache-2.0.txt` 和 `NOTICE`。
