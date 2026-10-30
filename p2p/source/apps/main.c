/* ============================================================
 * Point-to-point demo: L(A) sensor + remote, L(B) LED executor
 * Build with preprocessor symbol BOARD_B=0 for board A,
 * BOARD_B=1 for board B (or edit the default below).
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
#include "adc_reader.h"
#include "sht3x.h"

#ifndef BOARD_B
#define BOARD_B 0
#endif

#define HDR              0xA5
#define CMD_NONE         0
#define CMD_LED1_ON      1
#define CMD_LED1_OFF     2
#define CMD_LED2_BREATH  3
#define CMD_LED2_OFF     4

uint8_t Data[16];
static uint8_t radio_ver = 0;
unsigned char numbuf[20];

#if BOARD_B
/* ---------------- board B: receiver + LED ---------------- */
static uint8_t led1_on = 0;
static uint8_t led2_breath = 0;
static uint16_t rx_count = 0;
static uint8_t last_cmd = 0xEE;

static void Led1Write(uint8_t on)
{
    led1_on = on;
    GpioWrite(&Led1, on ? 0 : 1);      /* LED is active low */
}

static void Led2Breath(uint8_t on)
{
    if (on == led2_breath)
        return;
    switchLed2Breath();                /* flip breathing enable */
    led2_breath = on;
    if (!on)
        GpioWrite(&Led2, 1);           /* off */
}

static void DrawStatus(void)
{
    /* 66=kai 67=qi 68=guan 69=bi 70=hu 71=xi */
    OLED_ShowString(0, 0, (uint8_t *)"LED1:");
    if (led1_on)
    {
        OLED_ShowCHinese(48, 0, 66);
        OLED_ShowCHinese(64, 0, 67);
    }
    else
    {
        OLED_ShowCHinese(48, 0, 68);
        OLED_ShowCHinese(64, 0, 69);
    }

    OLED_ShowString(0, 2, (uint8_t *)"LED2:");
    if (led2_breath)
    {
        OLED_ShowCHinese(48, 2, 70);
        OLED_ShowCHinese(64, 2, 71);
    }
    else
    {
        OLED_ShowCHinese(48, 2, 68);
        OLED_ShowCHinese(64, 2, 69);
    }

    /* debug: received frames / last command */
    OLED_ShowString(0, 4, (uint8_t *)"RX :");
    OLED_ShowNum(32, 4, rx_count, 4, 16);
    OLED_ShowString(0, 6, (uint8_t *)"V:");
    OLED_ShowNum(16, 6, radio_ver, 3, 16);
    OLED_ShowString(48, 6, (uint8_t *)"C:");
    OLED_ShowNum(64, 6, last_cmd, 3, 16);
}
#else
/* ---------------- board A: sensor + remote ---------------- */
float Vicsd, Iiimd, Tun, Hum;
static uint8_t led1_cmd = 0;
static uint8_t led2_cmd = 0;
static uint16_t key_hit = 0;

static void DrawSensor(void)
{
    /* light: 29=guang 30=zhao 23=du */
    sprintf((char *)numbuf, "%.2f", Iiimd);
    OLED_ShowCHinese(0,  0, 29);
    OLED_ShowCHinese(16, 0, 30);
    OLED_ShowCHinese(32, 0, 23);
    OLED_ShowString(48, 0, (uint8_t *)":");
    OLED_ShowString(56, 0, numbuf);

    /* temp: 22=wen 23=du */
    sprintf((char *)numbuf, "%.2f", Tun);
    OLED_ShowCHinese(0,  2, 22);
    OLED_ShowCHinese(16, 2, 23);
    OLED_ShowString(32, 2, (uint8_t *)":");
    OLED_ShowString(40, 2, numbuf);

    /* humi: 24=shi 23=du */
    sprintf((char *)numbuf, "%.2f", Hum);
    OLED_ShowCHinese(0,  4, 24);
    OLED_ShowCHinese(16, 4, 23);
    OLED_ShowString(32, 4, (uint8_t *)":");
    OLED_ShowString(40, 4, numbuf);
}
#endif

void Init(void)
{
    BoardInitMcu();
    BoardInitPeriph();
    setTimer2Callback(Time2Handler);
    Tim2McuInit(1);

    NS_RadioInit(433300000, 20, 1000, 1000);
    radio_ver = Radio.Read(0x42);   /* SX127x RegVersion should be 0x12 */

    OLED_Init();
    OLED_Clear();   /* clear power-on RAM garbage */

#if BOARD_B
    setBreathLedArg(8, 40);            /* slower breathing */
    GpioWrite(&Led1, 1);
    GpioWrite(&Led2, 1);
    DrawStatus();
#else
    ADCS_Init();
    SHTXX_Init();
    keys_init();
#endif
}

int main(void)
{
    Init();

#if BOARD_B
    while (1)
    {
        uint16_t n = ReadRadioRxBuffer(Data);
        if (n >= 8 && Data[0] == HDR)
        {
            rx_count++;
            last_cmd = Data[7];
            switch (Data[7])
            {
            case CMD_LED1_ON:     Led1Write(1);   break;
            case CMD_LED1_OFF:    Led1Write(0);   break;
            case CMD_LED2_BREATH: Led2Breath(1);  break;
            case CMD_LED2_OFF:    Led2Breath(0);  break;
            default: break;
            }
            DrawStatus();
        }
    }
#else
    while (1)
    {
        AdcScanChannel();
        Vicsd = AdcReadCh0();
        Iiimd = (5.0f / 2.06f) * (Vicsd * 100.0f);
        call_sht11(&Tun, &Hum);
        DrawSensor();

        Data[0] = HDR;
        Data[1] = (uint8_t)Iiimd;
        Data[2] = (uint8_t)((Iiimd - (int)Iiimd) * 10);
        Data[3] = (uint8_t)((int)Tun + 40);
        Data[4] = (uint8_t)((Tun - (int)Tun) * 10);
        Data[5] = (uint8_t)Hum;
        Data[6] = (uint8_t)((Hum - (int)Hum) * 10);
        Data[7] = CMD_NONE;

        if (isKey2Pressed())
        {
            resetKey2();
            led1_cmd = !led1_cmd;
            Data[7] = led1_cmd ? CMD_LED1_ON : CMD_LED1_OFF;
        }
        if (isKey3Pressed())
        {
            resetKey3();
            led2_cmd = !led2_cmd;
            Data[7] = led2_cmd ? CMD_LED2_BREATH : CMD_LED2_OFF;
        }

        if (Data[7] != CMD_NONE)
            key_hit++;

        OLED_ShowString(0, 6, (uint8_t *)"K:");
        OLED_ShowNum(16, 6, key_hit, 3, 16);
        OLED_ShowString(48, 6, (uint8_t *)"V:");
        OLED_ShowNum(64, 6, radio_ver, 3, 16);

        SX1276Send(Data, 8);
        if (Data[7] != CMD_NONE)       /* repeat command for reliability */
        {
            HAL_Delay(50);
            SX1276Send(Data, 8);
            HAL_Delay(50);
            SX1276Send(Data, 8);
        }
        HAL_Delay(300);
    }
#endif
}
