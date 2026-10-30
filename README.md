# STM32-Lora

基于 **STM32L151C8T6 + SX1278** 的 LoRa 点对点物联网小开发，用于学习与备赛（职业院校技能大赛）。

## 硬件

| 项目 | 型号 |
|---|---|
| 主控 | STM32L151C8T6（64 KB Flash / 10 KB RAM） |
| 无线 | SX1278（LSD4RF-2F717N30） |
| 显示 | 1.3 寸 SPI OLED（页式显存，支持取模点阵） |
| 开发板 | 新大陆 LoRa 开发板，有 M-LoRa V2.2（NLE7E20A）与 NLE-LoRa V1.0（NLE7E308）两种丝印，引脚可能不同 |
| USB | MCU 原生 USB，无板载串口芯片，只能走串口 ISP 烧录 |

## 目录结构

```
lora(a)/
├─ p2p/        双板点对点收发工程（A 端 / B 端同源，靠宏区分）
│  ├─ project/LoraNode.uvprojx   Keil 工程
│  ├─ source/                    工程本地库
│  └─ *.hex                      已编译的烧录文件
├─ miao/       月薪喵桌宠：64x64 手绘线条猫，4 帧表情 + 12 帧旋转
├─ source/     整理好的纯通用库
│  ├─ system/  boards/  peripherals/  radio/  mac/  apps/
├─ v2/         帧素材（jpg）
└─ vframes/    帧素材（jpg）
```

## 数据帧协议（p2p）

14 字节定长：

| 偏移 | 内容 |
|---|---|
| 0 | 帧头 `0xA5` |
| 1–4 | 光照 |
| 5–8 | 温度 |
| 9–12 | 湿度 |
| 13 | 命令 |

B 端校验：`n == 14 && Data[0] == 0xA5`。
A 端发送后轮询 `TxDone`，再进入 `Sleep` 并转 `Rx`。

## 编译

MDK 5.41 已不含 AC5，工程统一用 **AC6 v6.22**：`Options → Target → ARM Compiler` 选 `v6.22`。

旧代码需要两处补丁：

```c
// uart.c
#pragma import(__use_no_semihosting)   // AC5 写法
__asm(".global __use_no_semihosting"); // AC6 写法

// hal_temHum.c
__nop();  // 改成
__NOP();
```

## 烧录

1. `JP1` 拨到「下载」，重新上电
2. Flash Loader Demonstrator：`COM3` / `115200` / `8 Even`
3. 目标选 `STM32L1_Cat.64K`，选 `.hex` → 下载
4. `JP1` 拨回「启动」，重新上电运行

> 硬件上 KEY1、MCU NRST、LoRa RST 是同一根线；`JP2` 用来选串口通道。

## 说明

`Objects/`、`Listings/`、`*.uvguix.*`、`*.uvoptx` 等编译产物与本机配置已在 `.gitignore` 中排除，克隆后重新编译即可生成。
