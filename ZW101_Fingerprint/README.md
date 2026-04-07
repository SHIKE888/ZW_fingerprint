# ZW101 Fingerprint Library

ZW101 指纹模块 Arduino 库，支持 UNO 和 ESP32，协议实现依据 ZW101 用户手册。

## 项目信息

- 项目名称: ZW101 Fingerprint Library
- 作者: STESON
- 文档日期: 2026-04-07
- 开源协议: MIT

## 目录

- [ZW101 Fingerprint Library](#zw101-fingerprint-library)
  - [项目信息](#项目信息)
  - [目录](#目录)
  - [功能特性](#功能特性)
  - [支持平台](#支持平台)
  - [目录结构](#目录结构)
  - [安装方式](#安装方式)
  - [硬件接线](#硬件接线)
  - [快速开始](#快速开始)
  - [示例教程](#示例教程)
    - [教程 1: 一次按压识别](#教程-1-一次按压识别)
    - [教程 2: 自动识别流程](#教程-2-自动识别流程)
    - [教程 3: 录入一个新指纹](#教程-3-录入一个新指纹)
    - [教程 4: 串口控制台调试](#教程-4-串口控制台调试)
  - [串口控制台命令](#串口控制台命令)
  - [指令详细说明](#指令详细说明)
  - [返回码参考](#返回码参考)
  - [常见问题](#常见问题)
  - [许可证](#许可证)

## 功能特性

- 支持硬串口和软串口
- 支持按压触发识别和自动识别
- 支持录入、删除、清库、LED 控制
- 支持感应引脚门控，避免重复触发

## 支持平台

- Arduino UNO
- ESP32

## 目录结构

- [ZW101_Fingerprint.h](ZW101_Fingerprint.h)
- [ZW101_Fingerprint.cpp](ZW101_Fingerprint.cpp)
- [library.properties](library.properties)
- [LICENSE](LICENSE)
- [examples/fingerprint/fingerprint.ino](examples/fingerprint/fingerprint.ino)
- [examples/enroll/enroll.ino](examples/enroll/enroll.ino)
- [examples/delete/delete.ino](examples/delete/delete.ino)
- [examples/auto_identify/auto_identify.ino](examples/auto_identify/auto_identify.ino)
- [examples/serial_console_demo/serial_console_demo.ino](examples/serial_console_demo/serial_console_demo.ino)

## 安装方式

1. 将仓库中的库目录复制到 Arduino libraries 目录。
2. 重启 Arduino IDE。
3. 打开任意示例并编译。

可选目录参考:

- Windows: Documents/Arduino/libraries

## 硬件接线

- Sensor TX -> MCU RX
- Sensor RX -> MCU TX
- GND -> GND
- VCC -> 模块额定供电

默认示例串口:

- UNO: SoftwareSerial(2, 3)
- ESP32: Serial2, RX=16, TX=17

感应引脚默认配置:

- TOUCH_PIN = 4
- TOUCH_ACTIVE_LEVEL = HIGH

触发规则:

- 仅按下时执行动作
- 单次按压仅执行一次
- 松手后才允许下一次触发

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
  } else {
    Serial.println("ZW101 not found");
  }
}

void loop() {}
```

## 示例教程

### 教程 1: 一次按压识别

使用示例: [examples/fingerprint/fingerprint.ino](examples/fingerprint/fingerprint.ino)

步骤:

1. 烧录示例后打开串口监视器。
2. 将行结束符设置为 Newline，波特率设置为 115200。
3. 按下手指一次，串口输出匹配结果。
4. 松手后，再按下一次才会触发下一次识别。

### 教程 2: 自动识别流程

使用示例: [examples/auto_identify/auto_identify.ino](examples/auto_identify/auto_identify.ino)

步骤:

1. 烧录后观察启动日志。
2. 按压手指触发 autoIdentify。
3. 识别成功时输出 ID 与分数。
4. 未识别到时输出 No match 或阶段码信息。

### 教程 3: 录入一个新指纹

使用示例: [examples/enroll/enroll.ino](examples/enroll/enroll.ino)

步骤:

1. 串口输入目标 ID。
2. 按压一次完成第一次采集。
3. 松手后再次按压完成第二次采集。
4. 模板合并并存储，输出录入结果。

### 教程 4: 串口控制台调试

使用示例: [examples/serial_console_demo/serial_console_demo.ino](examples/serial_console_demo/serial_console_demo.ino)

步骤:

1. 烧录后打开串口监视器。
2. 输入 help 查看全部命令。
3. 先执行 info 和 count 检查模块状态。
4. 再执行 search 或 autoid 进行识别调试。

## 串口控制台命令

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

串口参数:

- 波特率: 115200
- 行结束符: Newline

## 指令详细说明

help

- 作用: 显示全部命令。
- 参数: 无。
- 示例: help

info

- 作用: 读取模块系统参数。
- 参数: 无。
- 典型输出: getParameters=0x0, capacity, security, packetLength, baudRate。
- 示例: info

count

- 作用: 读取当前模板数量。
- 参数: 无。
- 典型输出: getTemplateCount=0x0 count=数字。
- 示例: count

search

- 作用: 按压触发一次 1:N 检索流程。
- 参数: 无。
- 典型输出:
  - MATCH id=数字 score=数字
  - NO MATCH
  - search err=0x错误码
- 示例: search

enroll [id] [times]

- 作用: 自动录入模板。
- 参数:
  - id: 目标 ID，范围 1 到 65535，默认 1。
  - times: 录入次数，范围 1 到 10，默认 3。
- 典型输出: autoEnroll rc=0x.. step=0x.. val=0x..
- 示例: enroll 1 3

autoid [id|-1]

- 作用: 自动识别。
- 参数:
  - -1 或省略: 1:N 检索。
  - id: 指定 ID，做 1:1 比对。
- 典型输出:
  - MATCH id=数字 score=数字
  - NO VALID MATCH
  - autoIdentify rc=0x.. stage=0x..
- 示例:
  - autoid -1
  - autoid 1

delete [id] [count]

- 作用: 从指定 ID 开始删除模板。
- 参数:
  - id: 起始 ID，范围 0 到 65535。
  - count: 删除数量，范围 1 到 65535，默认 1。
- 典型输出: delete rc=0x..
- 示例: delete 1 1

clear

- 作用: 清空整库模板。
- 参数: 无。
- 典型输出: empty rc=0x..
- 示例: clear

ledmode [0|255]

- 作用: 切换 LED 自动/手动模式。
- 参数:
  - 0: 手动模式。
  - 255: 自动模式。
- 典型输出: ledmode rc=0x..
- 示例:
  - ledmode 0
  - ledmode 255

led [func] [start] [end] [cycles] [duty] [period]

- 作用: 控制 LED 灯效。
- 参数:
  - func: 功能码，常用 1 呼吸, 2 闪烁, 3 常亮, 4 常灭, 5 渐开, 6 渐灭。
  - start: 起始颜色码。
  - end: 结束颜色码。
  - cycles: 循环次数，0 表示持续。
  - duty: 占空比，可选。
  - period: 周期参数，可选。
- 常见颜色码: 0x01 蓝, 0x02 绿, 0x04 红, 0x07 白。
- 典型输出: led rc=0x..
- 示例:
  - led 3 1 1 0
  - led 2 4 4 10 0x38 20

cancel

- 作用: 取消自动录入或自动识别流程。
- 参数: 无。
- 典型输出: cancel rc=0x..
- 示例: cancel

## 返回码参考

- 0x00: 成功
- 0x01: 通信或收包错误
- 0x02: 无手指
- 0x09: 未匹配
- 0x10: 删除失败
- 0x11: 清库失败
- 0x26: 超时

## 常见问题

1. 出现通信错误 0x01
原因: 串口引脚接反、波特率不一致、供电不稳。

2. 按下后没有触发
原因: 感应引脚电平方向不一致，可尝试将 TOUCH_ACTIVE_LEVEL 从 HIGH 改为 LOW。

3. 能采图但经常未匹配
原因: 模板质量低或录入次数不足，建议重新录入并提高按压稳定度。

## 许可证

本项目采用 MIT License，详见 [LICENSE](LICENSE)。
