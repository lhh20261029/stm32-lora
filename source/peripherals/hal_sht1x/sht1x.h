#ifndef __SHT1X_H
#define __SHT1X_H

#ifdef __cplusplus
 extern "C" {
#endif 


#define	SHT1x_SCK_GPIO		GPIOB
#define	SHT1x_SCK_GPIO_PIN	GPIO_PIN_6
#define	SHT1x_DATA_GPIO		GPIOB
#define	SHT1x_DATA_GPIO_PIN	GPIO_PIN_7

#define	SHT1x_SCK_SET()		{HAL_GPIO_WritePin(SHT1x_SCK_GPIO, SHT1x_SCK_GPIO_PIN, GPIO_PIN_SET);char i;for(i = 0; i < 20; i++);}
#define	SHT1x_SCK_RESET()	{HAL_GPIO_WritePin(SHT1x_SCK_GPIO, SHT1x_SCK_GPIO_PIN, GPIO_PIN_RESET);char i;for(i = 0; i < 20; i++);}

#define	SHT1x_DATA_SET()	{HAL_GPIO_WritePin(SHT1x_DATA_GPIO, SHT1x_DATA_GPIO_PIN, GPIO_PIN_SET);char i;for(i = 0; i < 20; i++);}
#define	SHT1x_DATA_RESET()	{HAL_GPIO_WritePin(SHT1x_DATA_GPIO, SHT1x_DATA_GPIO_PIN, GPIO_PIN_RESET);char i;for(i = 0; i < 20; i++);}
#define	SHT1x_DATA_READ()	HAL_GPIO_ReadPin(SHT1x_DATA_GPIO, SHT1x_DATA_GPIO_PIN)


#define	SH1x_MEASURE_T_CMD				0X03
#define	SH1x_MEASURE_RH_CMD				0X05
#define	SH1x_READ_STATUS_CMD			0X07
#define	SH1x_WRITE_STATUS_CMD			0X06
#define	SH1x_RESET_CMD					0X1E

void SHT1x_init(void);
void SHT1x_TS(void);
int SHT1x_write_byte(char byte);
char calc_CRC8(char* data, int len);
char SHT1x_read_byte( char is_ack);
int SHT1x_get_temperature(float* temp);
int SHT1x_get_relative_humidity(float *humidity);
int SHT1x_get_status(char* status);
void SHT1x_write_status(char status);
void SHT1x_connection_reset(void);
void SHT1x_soft_reset(void);


///////////////////////////////////////////
typedef enum{
	SHT1X_IDLE,
	SHT1X_START_T,
	SHT1X_WAIT_T,
	SHT1X_START_RH,
	SHT1X_WAIT_RH,
}SHT1X_STATUS;

void SHT1x_start_measure_temperature(void);
int is_SHT1x_measurement_end(void);
int SHT1x_get_temperature_result(float* temp);
void SHT1x_start_measure_relative_humidity(void);
int SHT1x_get_relative_humidity_result(float *humidity);



#ifdef __cplusplus
}
#endif

#endif /* __SHT1X_H */

