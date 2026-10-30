#include "stm32l1xx_hal.h"
#include "sht1x.h"

void SHT1x_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    //SCK
    GPIO_InitStruct.Pin = SHT1x_SCK_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SHT1x_SCK_GPIO, &GPIO_InitStruct);

    //DATA
    GPIO_InitStruct.Pin = SHT1x_DATA_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SHT1x_DATA_GPIO, &GPIO_InitStruct);

    SHT1x_SCK_RESET();
    //SHT1x_SCK_SET();
    SHT1x_DATA_SET();
}


//Transmission Start
void SHT1x_TS(void)
{
    SHT1x_SCK_SET();
    SHT1x_DATA_SET();
    SHT1x_DATA_RESET();
    SHT1x_SCK_RESET();
    SHT1x_SCK_SET();
    SHT1x_DATA_SET();
}


//write byte, if sucess return 0, else return -1;
int SHT1x_write_byte(char byte)
{
    int i;
    char data;

    SHT1x_SCK_RESET();
    //MSB first
    for(i = 7; i >= 0; i--) {
        data = byte & (0x01 << i);
        if(data) {
            SHT1x_DATA_SET();
        } else {
            SHT1x_DATA_RESET();
        }
        SHT1x_SCK_SET();
        SHT1x_SCK_RESET();
    }

    //check ACK
    SHT1x_SCK_SET();
    if(SHT1x_DATA_READ() != GPIO_PIN_RESET) {
        SHT1x_SCK_RESET();
        printf("ACK NG when write %x\r\n", byte);
        return -1;
    }
    SHT1x_SCK_RESET();
    return 0;

}

char SHT1x_read_byte( char is_ack)
{
    int i;
    char data;
    char byte = 0;

    SHT1x_SCK_RESET();
    //MSB first
    for(i = 0; i <= 7; i++) {
        SHT1x_SCK_SET();
        data = SHT1x_DATA_READ();
        byte <<= 1;
        byte |= data;
        SHT1x_SCK_RESET();
    }
    SHT1x_DATA_SET();

    //send ACK
    if(is_ack) {
        SHT1x_DATA_RESET();
    } else {
        SHT1x_DATA_SET();
    }
    SHT1x_SCK_SET();
    SHT1x_SCK_RESET();
    SHT1x_DATA_SET();

    return byte;
}

char const CRC_table[] = {0, 49,
                          98, 83, 196, 245, 166, 151, 185, 136, 219,
                          234, 125, 76, 31, 46, 67, 114, 33, 16, 135,
                          182, 229, 212, 250, 203, 152, 169, 62, 15,
                          92, 109, 134, 183, 228, 213, 66, 115, 32,
                          17, 63, 14, 93,108, 251, 202, 153, 168,
                          197, 244, 167, 150, 1, 48, 99, 82, 124, 77,
                          30, 47, 184, 137, 218, 235, 61, 12, 95,
                          110, 249, 200, 155, 170, 132, 181, 230,
                          215, 64, 113, 34, 19, 126, 79, 28, 45, 186,
                          139, 216, 233, 199, 246, 165, 148, 3, 50,
                          97, 80, 187, 138, 217, 232, 127, 78, 29,
                          44, 2, 51, 96, 81, 198, 247, 164, 149, 248,
                          201, 154, 171, 60, 13, 94, 111, 65, 112,
                          35, 18, 133, 180, 231, 214, 122, 75, 24,
                          41, 190, 143, 220, 237, 195, 242, 161, 144,
                          7, 54, 101, 84, 57, 8, 91, 106, 253, 204,
                          159, 174, 128, 177, 226, 211, 68, 117, 38,
                          23, 252, 205, 158, 175, 56, 9, 90, 107, 69,
                          116, 39, 22, 129, 176, 227, 210, 191, 142,
                          221, 236, 123, 74, 25, 40, 6, 55, 100, 85,
                          194, 243, 160, 145, 71, 118, 37, 20, 131,
                          178, 225, 208, 254, 207, 156, 173, 58, 11,
                          88, 105, 4, 53, 102, 87, 192, 241, 162,
                          147, 189, 140, 223, 238, 121, 72, 27, 42,
                          193, 240, 163, 146, 5, 52, 103, 86, 120,
                          73, 26, 43, 188, 141, 222, 239, 130, 179,
                          224, 209, 70, 119, 36, 21, 59, 10, 89, 104,
                          255, 206, 157, 172
                         };

//following: x8 + x5 + x4 + 1.
char calc_CRC8(char* data, int len)
{
    char crc_val = 0x00, final_crc = 0;;
    int i;

    for(i = 0; i < len; i++) {
        crc_val = CRC_table[crc_val ^ data[i]];
    }

    final_crc = ((crc_val&0x01)<<7)
                |((crc_val&0x02)<<5)
                |((crc_val&0x04)<<3)
                |((crc_val&0x08)<<1)
                |((crc_val&0x10)>>1)
                |((crc_val&0x20)>>3)
                |((crc_val&0x40)>>5)
                |((crc_val&0x80)>>7);


    return final_crc;
}

/*  polling the measurement end flag  */
int SHT1x_get_temperature(float* temp)
{
    char temp_data[3], CRC_calc[3];
    int i;
    short signal_out;

    SHT1x_TS();
    SHT1x_write_byte(SH1x_MEASURE_T_CMD);
    while(SHT1x_DATA_READ() != GPIO_PIN_RESET);

    for(i = 0; i < sizeof(temp_data); i++)
        temp_data[i] = SHT1x_read_byte(1);

#if 0
    for(i = 0; i < sizeof(temp_data); i++)
        printf("temp data%d:%d\r\n", i, temp_data[i]);
#endif

    CRC_calc[0] = SH1x_MEASURE_T_CMD;
    for(i = 0; i < sizeof(temp_data)-1; i++)
        CRC_calc[i+1] = temp_data[i];

    if(calc_CRC8(CRC_calc, sizeof(CRC_calc)) != temp_data[sizeof(temp_data) - 1]) {
        SHT1x_soft_reset();
        printf("temp CRC error:%d %d\r\n", calc_CRC8(CRC_calc, sizeof(CRC_calc)), temp_data[sizeof(temp_data) - 1]);
        return -1;
    }

    signal_out = (temp_data[0] << 8) + temp_data[1];
    *temp = signal_out * 0.01 - 39.7;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
void SHT1x_start_measure_temperature(void)
{
    SHT1x_TS();
    SHT1x_write_byte(SH1x_MEASURE_T_CMD);
}

int is_SHT1x_measurement_end(void)
{
    if(SHT1x_DATA_READ() == GPIO_PIN_RESET) {
        return 1;
    } else {
        return 0;
    }
}

int SHT1x_get_temperature_result(float* temp)
{
    char temp_data[3], CRC_calc[3];
    int i;
    short signal_out;

    for(i = 0; i < sizeof(temp_data); i++)
        temp_data[i] = SHT1x_read_byte(1);

#if 0
    for(i = 0; i < sizeof(temp_data); i++)
        printf("temp data%d:%d\r\n", i, temp_data[i]);
#endif

    CRC_calc[0] = SH1x_MEASURE_T_CMD;
    for(i = 0; i < sizeof(temp_data)-1; i++)
        CRC_calc[i+1] = temp_data[i];

    if(calc_CRC8(CRC_calc, sizeof(CRC_calc)) != temp_data[sizeof(temp_data) - 1]) {
        SHT1x_soft_reset();
        printf("temp CRC error:%d %d\r\n", calc_CRC8(CRC_calc, sizeof(CRC_calc)), temp_data[sizeof(temp_data) - 1]);
        return -1;
    }

    signal_out = (temp_data[0] << 8) + temp_data[1];
    *temp = signal_out * 0.01 - 39.7;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////

int SHT1x_get_relative_humidity(float *humidity)
{
    char humidity_data[3], CRC_calc[3];
    int i;
    short signal_out;

    SHT1x_TS();
    SHT1x_write_byte(SH1x_MEASURE_RH_CMD);
    while(SHT1x_DATA_READ() != GPIO_PIN_RESET);

    for(i = 0; i < sizeof(humidity_data); i++)
        humidity_data[i] = SHT1x_read_byte(1);

#if 0
    for(i = 0; i < sizeof(humidity_data); i++)
        printf("humidity data%d:%d\r\n", i, humidity_data[i]);
#endif

    CRC_calc[0] = SH1x_MEASURE_RH_CMD;
    for(i = 0; i < sizeof(humidity_data)-1; i++)
        CRC_calc[i+1] = humidity_data[i];
    if(calc_CRC8(CRC_calc, sizeof(CRC_calc)) != humidity_data[sizeof(humidity_data) - 1]) {
        SHT1x_soft_reset();
        printf("humidity CRC error:%d %d\r\n", calc_CRC8(CRC_calc, sizeof(CRC_calc)), humidity_data[sizeof(humidity_data) - 1]);
        return -1;
    }

    signal_out = (humidity_data[0] << 8) + humidity_data[1];
    *humidity = -2.0468 + (0.0367 * signal_out) + (-1.5955E-6 * signal_out * signal_out);
    //printf("humidity:%f\r\n", *humidity);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////

void SHT1x_start_measure_relative_humidity(void)
{
    SHT1x_TS();
    SHT1x_write_byte(SH1x_MEASURE_RH_CMD);
}

int SHT1x_get_relative_humidity_result(float *humidity)
{
    char humidity_data[3], CRC_calc[3];
    int i;
    short signal_out;

    for(i = 0; i < sizeof(humidity_data); i++)
        humidity_data[i] = SHT1x_read_byte(1);

#if 0
    for(i = 0; i < sizeof(humidity_data); i++)
        printf("humidity data%d:%d\r\n", i, humidity_data[i]);
#endif

    CRC_calc[0] = SH1x_MEASURE_RH_CMD;
    for(i = 0; i < sizeof(humidity_data)-1; i++)
        CRC_calc[i+1] = humidity_data[i];
    if(calc_CRC8(CRC_calc, sizeof(CRC_calc)) != humidity_data[sizeof(humidity_data) - 1]) {
        SHT1x_soft_reset();
        printf("humidity CRC error:%d %d\r\n", calc_CRC8(CRC_calc, sizeof(CRC_calc)), humidity_data[sizeof(humidity_data) - 1]);
        return -1;
    }

    signal_out = (humidity_data[0] << 8) + humidity_data[1];
    *humidity = -2.0468 + (0.0367 * signal_out) + (-1.5955E-6 * signal_out * signal_out);
    //printf("humidity:%f\r\n", *humidity);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////

int SHT1x_get_status(char* status)
{
    char status_data[2], CRC_calc[2];
    int i;
    //short signal_out;

    SHT1x_TS();
    SHT1x_write_byte(SH1x_READ_STATUS_CMD);

    while(SHT1x_DATA_READ() != GPIO_PIN_RESET);

    for(i = 0; i < sizeof(status_data); i++)
        status_data[i] = SHT1x_read_byte(1);

#if 0
    for(i = 0; i < sizeof(status_data); i++)
        printf("data%d:%d\r\n", i, status_data[i]);
#endif

    CRC_calc[0] = SH1x_READ_STATUS_CMD;
    for(i = 0; i < sizeof(status_data)-1; i++)
        CRC_calc[i+1] = status_data[i];
    if(calc_CRC8(CRC_calc, sizeof(CRC_calc)) != status_data[sizeof(status_data) - 1]) {
        SHT1x_soft_reset();
        printf("status CRC error:%d %d\r\n", calc_CRC8(CRC_calc, sizeof(CRC_calc)), status_data[sizeof(status_data) - 1]);
        return -1;
    }

    *status = status_data[0];
    //printf("status:%x\r\n", *status);
    return 0;
}

void SHT1x_write_status(char status)
{
    SHT1x_TS();
    SHT1x_write_byte(SH1x_WRITE_STATUS_CMD);
    SHT1x_write_byte(status);
}

void SHT1x_connection_reset(void)
{
    int i;

    //While leaving DATA high, toggle SCK nine or more times
    SHT1x_SCK_RESET();
    SHT1x_DATA_SET();
    for(i = 0; i <= 9; i++) {
        SHT1x_SCK_SET();
        SHT1x_SCK_RESET();
    }

    //followed by a Transmission Start
    SHT1x_TS();
}

void SHT1x_soft_reset(void)
{
    SHT1x_connection_reset();
    SHT1x_write_byte(SH1x_RESET_CMD);
}

void SHT1x_test()
{
    float temp, humidity;
    char status;
//	char crc_cal[] = {0x05, 0x04, 0x31};

    //printf("crc:%x\r\n", calc_CRC8(crc_cal, sizeof(crc_cal)));

    SHT1x_get_temperature(&temp);
    SHT1x_get_relative_humidity(&humidity);
    SHT1x_get_status(&status);

    printf("status:%d temp:%f humidity:%f\r\n", status, temp, humidity);

    //SHT1x_write_status(0x07);
    //SHT1x_get_status(&status);
    //SHT1x_write_status(0x01);
    //SHT1x_get_status(&status);
#if 0

    SHT1x_TS();
    SHT1x_write_byte(SH1x_MEASURE_RH_CMD);
    while(SHT1x_DATA_READ() != GPIO_PIN_RESET);

    //HAL_Delay(1000);
    SHT1x_read_byte(&data1);
    SHT1x_read_byte(&data2);
    SHT1x_read_byte(&data3);
    printf("data1:%x\r\n",data1);
    printf("data2:%x\r\n",data2);
    printf("data3:%x\r\n",data3);
#endif

#if 0
    SHT1x_TS();
    SHT1x_write_byte(SH1x_READ_STATUS_CMD);
    SHT1x_read_byte(&data1);
    SHT1x_read_byte(&data2);

    printf("data1:%x\r\n",data1);
    printf("data2:%x\r\n",data2);
#endif
}


void SHT1x_test2()
{
    float temp, humidity;
    char status;

    SHT1x_start_measure_temperature();
    while(!is_SHT1x_measurement_end());
    SHT1x_get_temperature_result(&temp);

    SHT1x_start_measure_relative_humidity();
    while(!is_SHT1x_measurement_end());
    SHT1x_get_relative_humidity_result(&humidity);
    SHT1x_get_status(&status);

    printf("status:%d temp:%f humidity:%f\r\n", status, temp, humidity);

}

