# Guition JC8048F550

ESP32-S31，16 MB Flash、Octal PSRAM，800 × 480 RGB565 屏幕，原生 Wi-Fi。
配置依据 Guition-fork 的 `components/board/guition_boards/jc8048f550/` 中有效的 YAML 配置值；部分原始注释与接线值不一致，因此未采用旧注释中的 GPIO。

板级实现、引脚配置和 ES8389/AEC 适配均保存在本型号目录中，可独立修改，不依赖其他 Guition 板型的配置或辅助文件。

## 构建

要求 ESP-IDF release/v6.1。在交互式 fish 中执行：

```fish
source ~/.espressif/tools/activate_idf_v6.1.fish
idf.py --version
python3 scripts/build.py guition/jc8048f550 --name guition-jc8048f550
```

## 已接入硬件

| 功能 | GPIO / 参数 |
| --- | --- |
| RGB D0–D15 | 36, 37, 38, 39, 46, 7, 35, 51, 50, 49, 48, 4, 5, 0, 1, 6 |
| HSYNC / VSYNC / DE / PCLK | 44 / 45 / 43 / 40 |
| 像素时钟 | 24 MHz，下降沿有效 |
| 水平 pulse / back / front porch | 4 / 8 / 8 |
| 垂直 pulse / back / front porch | 4 / 8 / 8 |
| 背光 | GPIO17，高有效，20 kHz PWM |
| ES8389 I²C SDA / SCL | 2 / 3，8 位地址 0x20（7 位 0x10） |
| 音频 BCLK / WS / DOUT / DIN | 54 / 47 / 56 / 16，无 MCLK |
| NS4150 功放使能 | GPIO42，高有效 |

音频使用项目当前 esp_codec_dev 1.6 的四时隙 TDM，16 kHz、每时隙 16 bit，
共享 BCLK 为 64fs（1.024 MHz）、WS 为 16 kHz/32 bit 宽，无外接 MCLK。
`no_dac_ref=false` 启用 ES8389 内部播放参考，并匹配四时隙的 BCLK 比率。

按确认的板载接线，RX 帧为 `TD0=麦克风0、TD1=播放参考0、TD2=麦克风1、TD3=播放参考1`。
驱动完整读取四时隙，默认将 TD0/TD1 转换为小智 AFE 所需的交错 `MR` 双通道输入；
`input_reference=true`，默认启用设备端 AEC。TD2/TD3 保留在总线帧中，但不送入默认单麦 AEC。
TX 将小智单声道 PCM 放入 TD0/TD2，TD1/TD3 填零，分别对应 DAC 左右半帧的有效 16 bit。
两端始终采用相同四时隙格式；暂停播放仅关闭外部功放，不停止采集所用的共享时钟。
esp_codec_dev 1.6.2 的 ES8389 输出 mute 实际操作 ADC 寄存器，因此此配置不调用该接口，以免关闭麦克风。
增益仅作用于 MIC1 的模拟 PGA，不对播放参考样本施加麦克风增益。
LCD 使用两个 PSRAM 帧缓冲和 8 行 bounce buffer，匹配现有 `RgbLcdDisplay`。

不初始化触摸、摄像头、SD 卡或按键。GPIO42 仅用于功放，GPIO0/1 用于 RGB 数据，不能作为启动按键。
未配置网络时沿用 `WifiBoard` 的自动配网流程；对话采用语音唤醒。

## 实机验收

编译通过不代表硬件验证。需检查冷启动、屏幕颜色/方向/稳定性、亮度、配网/重连、
MIC1 录音、左右播放、唤醒、播放打断及重复静音/恢复。逻辑分析仪确认 64fs BCLK、
16 kHz WS 和 TD0/TD1 的 MR 顺序；播放测试音时参考应出现对应波形，MIC1 增益调整不应改变参考增益。
需在播放时说话验证 AEC 消除扬声器回声、保留近端语音，并验证静音/恢复和断网重连后的 AEC。
触摸及第二组麦克风/参考的算法接入不在此次范围。

## 许可证

硬件参数沿用 Guition 的 MIT 许可；原始 NOTICE 及其引用的第三方许可证保留在本目录。
