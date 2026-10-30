# STM32-Lora

> 用 **STM32L151C8T6 + SX1278** 做的 LoRa 点对点通信小项目：两块板子互发环境数据、按键控制对面 LED，再加一只会在 OLED 上打滚的「月薪喵」。
>
> 学习用 & 职业院校技能大赛备赛。

![月薪喵](miao/doodle_preview.png)

---

## ✨ 做了什么

- **双板点对点收发**：A 端采集光照 / 温度 / 湿度打包发送，B 端接收解析并显示，按键可反向控制对方 LED
- **定长帧协议**：14 字节帧头结构，校验帧头与长度，错帧直接丢弃
- **低功耗切换**：发送完成等 `TxDone` 再进 `Sleep`，需要时切 `Rx` 常驻接收
- **OLED 显示**：1.3 寸 SPI 屏，页式显存，手绘点阵图直接 `OLED_DrawBMP`
- **桌宠「月薪喵」**：64×64 手绘线条猫，4 帧表情 + 12 帧旋转，按键摸头 / 睡觉，收到对面消息会开心

## 🔧 硬件

| 项目 | 型号 / 说明 |
|---|---|
| 主控 | STM32L151C8T6（64 KB Flash / 10 KB RAM） |
| 无线 | SX1278（LSD4RF-2F717N30），433 MHz |
| 显示 | 1.3 寸 SPI OLED |
| 开发板 | 新大陆 LoRa 开发板 |
| USB | MCU 原生 USB，**板载无串口芯片**，只能走串口 ISP 烧录 |

> ⚠️ 开发板有两种丝印：`M-LoRa V2.2`（NLE7E20A）与 `NLE-LoRa V1.0`（NLE7E308），**引脚可能不同，动手前先看板子丝印**。

## 📁 目录结构

```
lora(a)/
├─ p2p/            双板点对点工程（A / B 端同源，靠宏区分）
│  ├─ project/     LoraNode.uvprojx
│  ├─ source/      工程本地库
│  └─ *.hex        已编译的烧录文件
├─ miao/           月薪喵桌宠工程（含 miao.uvprojx）
├─ source/         整理好的通用库
│  ├─ system/  boards/  peripherals/  radio/  mac/  apps/
├─ v2/  vframes/   点阵帧素材
└─ README.md
```

## 📡 数据帧协议

14 字节定长，跨板通信的全部约定：

| 偏移 | 长度 | 内容 |
|---|---|---|
| 0 | 1 | 帧头 `0xA5` |
| 1–4 | 4 | 光照 |
| 5–8 | 4 | 温度 |
| 9–12 | 4 | 湿度 |
| 13 | 1 | 命令 |

- **B 端校验**：`n == 14 && Data[0] == 0xA5`，不满足直接丢
- **A 端时序**：发送后轮询 `TxDone`，确认发完再 `Sleep`，随后转 `Rx`

## 🛠 编译

MDK 5.41 已不再自带 AC5，工程统一用 **AC6**：

```
Options for Target → Target → ARM Compiler → v6.22
```

老代码在 AC6 下要打两处补丁：

```c
/* uart.c —— 关闭半主机 */
// AC5: #pragma import(__use_no_semihosting)
__asm(".global __use_no_semihosting");

/* hal_temHum.c —— 内建函数大小写 */
// __nop();  →  __NOP();
```

编译产物（`Objects/`、`Listings/`、`*.uvguix.*`、`*.uvoptx` 等）已在 `.gitignore` 中排除，克隆后重新编译即可。

## 🔌 烧录

1. `JP1` 拨到「下载」，重新上电
2. 打开 **Flash Loader Demonstrator**：`COM3` / `115200` / `8 Even`
3. 目标芯片选 `STM32L1_Cat.64K`，选 `.hex` → Download
4. `JP1` 拨回「启动」，重新上电运行

> 硬件上 `KEY1`、MCU `NRST`、LoRa `RST` 共用一根线；`JP2` 用来选串口通道。

## 🐱 月薪喵

![表情帧](miao/miao_preview.png)

- 64×64 点阵，纯手绘线条风格，4 帧表情 + 12 帧旋转动画
- 待机每 90 ms 换帧；`KEY2` 摸头、`KEY3` 睡觉；收到对端数据也会蹦一下
- 右半屏显示当前状态与收发计数
- `miao/tools/img2oled.py` 能把任意图片转成页式 C 数组，直接喂给 `OLED_DrawBMP`

## 📄 说明

- 代码与资料仅用于学习交流
- 通用库整理自课程讲义，`source/` 下保留了可独立编译的最小结构

---

如果这个工程帮到你，点个 ⭐ 呗。
