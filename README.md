# ZW101 Fingerprint Library

面向 ZW101 指纹模块的 Arduino 库，支持 UNO 与 ESP32，支持硬串口与软串口，协议实现基于 ZW101 用户手册。

## 项目信息

- 项目名: ZW101 Fingerprint Library
- 作者: STESON
- 文档日期: 2026-04-07
- 开源协议: MIT

## 功能特性

- 支持平台: Arduino UNO, ESP32
- 支持串口: HardwareSerial, SoftwareSerial/Stream
- 常用识别流程接口: getImage, image2Tz, createModel, storeModel, fingerSearch
- 模组扩展接口: autoEnroll, autoIdentify, cancel, setLedMode, controlLed
- 示例支持感应引脚门控: 按下触发, 松手复位

## 目录结构

- [ZW101_Fingerprint.h](ZW101_Fingerprint.h)
- [ZW101_Fingerprint.cpp](ZW101_Fingerprint.cpp)
- [library.properties](library.properties)
- [examples/fingerprint/fingerprint.ino](examples/fingerprint/fingerprint.ino)
- [examples/enroll/enroll.ino](examples/enroll/enroll.ino)
- [examples/delete/delete.ino](examples/delete/delete.ino)
- [examples/auto_identify/auto_identify.ino](examples/auto_identify/auto_identify.ino)
- [examples/serial_console_demo/serial_console_demo.ino](examples/serial_console_demo/serial_console_demo.ino)

## 安装

1. 将 [ZW101_Fingerprint](.) 目录复制到 Arduino libraries 目录。
2. 重启 Arduino IDE。
3. 在示例中打开任意 .ino 文件进行编译与烧录。

## 硬件接线

- Sensor TX -> MCU RX
- Sensor RX -> MCU TX
- GND -> GND
- VCC -> 模块额定供电

推荐默认串口引脚:

- UNO: SoftwareSerial(2, 3)
- ESP32: Serial2, RX=16, TX=17

## 感应引脚触发规则

- 默认 TOUCH_PIN=4
- 默认 TOUCH_ACTIVE_LEVEL=LOW
- 所有触发式示例均为: 按下才执行一次, 松手后允许下一次执行

## 快速开始

```cpp
#include <ZW101_Fingerprint.h>

ZW101_Fingerprint finger(&Serial2);

void setup() {
  Serial.begin(115200);
  Serial2.begin(57600);
  finger.begin(57600);

  if (finger.handshake() == ZW101_OK) {
    Serial.println("ZW101 online");
  }
}
```

## 示例说明

- [examples/fingerprint/fingerprint.ino](examples/fingerprint/fingerprint.ino): 按压触发的一次识别示例
- [examples/enroll/enroll.ino](examples/enroll/enroll.ino): 按压触发的两次采集录入示例
- [examples/delete/delete.ino](examples/delete/delete.ino): 删除模板示例
- [examples/auto_identify/auto_identify.ino](examples/auto_identify/auto_identify.ino): 自动识别示例
- [examples/serial_console_demo/serial_console_demo.ino](examples/serial_console_demo/serial_console_demo.ino): 串口命令控制台

## Serial Console 命令

- help
- info
- count
- search
- enroll [id] [times]
- autoid [id|-1]
- delete [id] [count]
- clear
- ledmode [0|255]
- led [func] [start] [end] [cycles] [duty] [period]
- cancel

串口监视器参数:

- 波特率: 115200
- 行结束符: Newline

## 返回码参考

- 0x00: 成功
- 0x01: 通信或收包错误
- 0x02: 无手指
- 0x09: 未匹配
- 0x10: 删除失败
- 0x11: 清库失败
- 0x26: 超时

## 许可证

本项目采用 MIT License，详见 [LICENSE](LICENSE)。

## 发布到 GitHub

当前环境无法直接替你推送远端仓库，但你可以在项目目录执行以下命令:

```bash
git init
git add .
git commit -m "feat: standardize README and adopt MIT license"
git branch -M main
git remote add origin <你的仓库地址>
git push -u origin main
```
