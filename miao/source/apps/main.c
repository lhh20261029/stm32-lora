/* ============================================================
 * 月薪喵 · LoRa 桌面宠物
 * 硬件：新大陆 M-LoRa 板（STM32L151C8T6 + SX1278 + 128x64 OLED）
 * 玩法：
 *   KEY2  摸头   -> 喵变成开心脸，并通过 LoRa 给对面的喵发一个"喵"
 *   KEY3  睡觉/叫醒 -> 切换睡眠脸
 *   收到对面的"喵" -> 自己也开心一下
 * 左半屏是猫，右半屏是状态信息。
 * ============================================================ */
#include <string.h>
#include <stdio.h>
#include "board.h"
#include "tim-board.h"
#include "timer_handles.h"
#include "oledfont.h"
#include "hal_oled.h"
#include "hal_key.h"
#include "led_light.h"
#include "NS_Radio.h"
#include "miao_bmp.h"
#include "miao_spin.h"

#define HDR        0xA5
#define CMD_MEOW   0x01
#define CMD_SLEEP  0x02
#define CMD_WAKE   0x03

#define MIAO_IDLE   0
#define MIAO_HAPPY  1
#define MIAO_SLEEP  2

uint8_t Data[16];

static uint8_t  mood = MIAO_IDLE;
static uint32_t happy_until = 0;
static uint32_t last_blink = 0;
static uint8_t  blink_on = 0;
static uint32_t last_tx = 0;
static uint32_t tx_cnt = 0;
static uint32_t rx_cnt = 0;
static uint8_t  spin_idx = 0;
static uint32_t last_spin = 0;

static const unsigned char *FrameOf(uint8_t m)
{
    if (m == MIAO_SLEEP)
        return miao_sleep;
    if (m == MIAO_HAPPY)
        return miao_happy;
    if (blink_on)
        return miao_blink;
    return miao_idle;
}

static void Draw(void)
{
    if (mood == MIAO_IDLE)
        OLED_DrawBMP(0, 0, 64, 64, (unsigned char *)miao_spin[spin_idx]);
    else
        OLED_DrawBMP(0, 0, 64, 64, (unsigned char *)FrameOf(mood));

    OLED_ShowString(72, 0, (uint8_t *)"MIAO");
    OLED_ShowString(72, 2, (uint8_t *)(mood == MIAO_SLEEP ? "ZZZ " :
                                       (mood == MIAO_HAPPY ? "HAPPY" : "IDLE ")));
    OLED_ShowString(72, 4, (uint8_t *)"T:");
    OLED_ShowNum(88, 4, tx_cnt, 3, 16);
    OLED_ShowString(72, 6, (uint8_t *)"R:");
    OLED_ShowNum(88, 6, rx_cnt, 3, 16);
}

static void SendCmd(uint8_t cmd)
{
    Data[0] = HDR;
    Data[1] = cmd;
    SX1276Send(Data, 2);
    tx_cnt++;
}

static void Happy(void)
{
    mood = MIAO_HAPPY;
    happy_until = HAL_GetTick() + 1500;
}

void Init(void)
{
    BoardInitMcu();
    BoardInitPeriph();
    setTimer2Callback(Time2Handler);
    Tim2McuInit(1);

    NS_RadioInit(433300000, 20, 1000, 1000);

    OLED_Init();
    OLED_Clear();
    keys_init();

    GpioWrite(&Led1, 1);
    GpioWrite(&Led2, 1);
}

int main(void)
{
    Init();
    Draw();

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* ---- 收到对面的消息 ---- */
        uint16_t n = ReadRadioRxBuffer(Data);
        if (n >= 2 && Data[0] == HDR)
        {
            rx_cnt++;
            if (Data[1] == CMD_MEOW)
                Happy();
            else if (Data[1] == CMD_SLEEP)
                mood = MIAO_SLEEP;
            else if (Data[1] == CMD_WAKE)
                mood = MIAO_IDLE;
            Draw();
        }

        /* ---- 按键 ---- */
        if (isKey2Pressed())
        {
            resetKey2();
            Happy();
            SendCmd(CMD_MEOW);
            GpioWrite(&Led1, 0);
            Draw();
        }
        if (isKey3Pressed())
        {
            resetKey3();
            mood = (mood == MIAO_SLEEP) ? MIAO_IDLE : MIAO_SLEEP;
            SendCmd(mood == MIAO_SLEEP ? CMD_SLEEP : CMD_WAKE);
            GpioWrite(&Led1, 1);
            Draw();
        }

        /* ---- 开心 1.5 秒后回到 idle ---- */
        if (happy_until != 0 && now > happy_until)
        {
            happy_until = 0;
            if (mood == MIAO_HAPPY)
            {
                mood = MIAO_IDLE;
                Draw();
            }
        }

        /* ---- 偶尔眨眼 ---- */
        if (mood == MIAO_IDLE)
        {
            if (blink_on == 0 && (now - last_blink) > 3000)
            {
                blink_on = 1;
                last_blink = now;
                Draw();
            }
            else if (blink_on == 1 && (now - last_blink) > 150)
            {
                blink_on = 0;
                last_blink = now;
                Draw();
            }
        }

        /* ---- idle 时让猫转起来 ---- */
        if (mood == MIAO_IDLE && (now - last_spin) > 90)
        {
            last_spin = now;
            spin_idx = (uint8_t)((spin_idx + 1) % MIAO_SPIN_FRAMES);
            Draw();
        }

        /* ---- 每秒给对面报个平安 ---- */
        if (now - last_tx > 1000)
        {
            last_tx = now;
            SendCmd(mood == MIAO_SLEEP ? CMD_SLEEP : CMD_WAKE);
        }
    }
}
