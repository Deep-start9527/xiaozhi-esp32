# Guition 板卡

新增 P4 / S31 板卡的接线和屏幕初始化依据为
`Guition-fork/components/board/guition_boards`。每款保留独立的板级类型与固件名称，
不能用同芯片的其他型号固件替代。既有 JC4827W543 (ESP32-S3) 配置保持独立。

每款板卡的引脚、初始化和专用音频实现均位于自己的型号目录中，
不依赖 Guition 厂商目录或其他型号的公共配置文件。项目级 Board、显示及 codec 接口仍沿用工程现有实现。

| 目录 | 芯片 | 显示 | 音频 | 设备端 AEC |
| --- | --- | --- | --- | --- |
| `jc1060p470` | ESP32-P4 3.x | 1024×600 JD9165 MIPI | ES8311 | 不启用 |
| `jc1060p470-i-w-y` | ESP32-P4 3.x | 1024×600 JD9165 MIPI，LCD V2 | ES8311 | 不启用 |
| `jc4880p443` | ESP32-P4 3.x | 480×800 ST7701 MIPI | ES8311 | 不启用 |
| `jc8012p4a1` | ESP32-P4 3.x | 800×1280 JD9365 MIPI，LCD V2 | ES8311 | 不启用 |
| `jc1060f570c` | ESP32-S31 | 1024×600 RGB | ES8389 | 默认启用，TD0 麦克风 + TD1 回采 |
| `jc8048f543` | ESP32-S31 | 800×480 RGB | ES8389 | 默认启用，TD0 麦克风 + TD1 回采 |
| `jc8048f550` | ESP32-S31 | 800×480 RGB | ES8389 | 默认启用，TD0 麦克风 + TD1 回采 |
| `jc3636f118` | ESP32-S31 | 360×360 ST77916 QSPI | PDM 麦克风 + I2S 输出 | 无回采通道 |
| `jc5454f328` | ESP32-S31 | 540×540 AXS15260 RGB | ES7210 + I2S 输出 | 无回采通道 |

## 构建

在交互式 fish 中加载指定环境，再运行规范构建入口：

```fish
source ~/.espressif/tools/activate_idf_v6.1.fish
idf.py --version
python scripts/build.py guition/jc1060p470 --name guition-jc1060p470
python scripts/build.py guition/jc1060f570c --name guition-jc1060f570c
```

其他型号以表中的目录替换型号部分，固件名称均为 `guition-<目录>`。
构建配置要求 ESP-IDF ≥6.1，S31 自动启用 SDK preview。
语言与唤醒词沿用构建工具选项，例如 `--language zh-CN`。
每次构建都会更新本地 `sdkconfig` 和 `build/`，应在切换型号前保存需要的固件产物。

P4 固件仅支持 3.x 芯片（最低修订 3.0），使用 ESP-Hosted/C6 SDIO 联网；
从机须运行兼容的 ESP-Hosted 固件。S31 使用原生 Wi-Fi。
所有新增型号使用 16 MB Flash 和 `partitions/v2/16m.csv`。

## 功能与硬件验收

接入显示、背光、Wi-Fi、板载录音与播放；本次不接入触摸、SD 卡、摄像头、编码器或传感器。
资料未明确的按键不初始化。JC1060F570C 无 ADC 按键，GPIO42 仅控制功放。

ES8389 使用四个 16 位时隙：TD0/TD2 为麦克风、TD1/TD3 为回采；默认取 TD0/TD1，
向 AFE 提供 `MR`。回采通道不是第二路麦克风。详见对应板卡 README。
编译成功不代表已经完成实机验收，至少应逐款检查：

- 冷启动、屏幕颜色/方向/稳定性、背光恢复与调节。
- 配网、断线重连、连续录音和播放、唤醒及播放中打断。
- ES8389 的四时隙录音：TD1/TD3 随播放变化，AEC 开关及播放中语音识别。
- JC3636F118 的 PDM 麦克风有效时隙；JC5454F328 的 ES7210 时钟和功放开关。

各板卡保留移植来源的 LICENSE、NOTICE 和第三方许可文件。
