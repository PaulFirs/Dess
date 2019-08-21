#ifndef __INIT_H
#define __INIT_H

#ifdef __cplusplus
 extern "C" {
#endif



#define BUF_SIZE 9
 volatile uint8_t RX_BUF[BUF_SIZE];
 volatile uint8_t TX_BUF[BUF_SIZE];


 uint8_t error_i2c;




 //макроопределения для управления выводом SS
 #define CS_ENABLE        GPIOB->BSRR = GPIO_BSRR_BR12;
 #define CS_DISABLE    	  GPIOB->BSRR = GPIO_BSRR_BS12;

 //макроопределения для управления выводом DC
 #define DC_ENABLE        GPIOB->BSRR = GPIO_BSRR_BR8;
 #define DC_DISABLE    	  GPIOB->BSRR = GPIO_BSRR_BS8;

 //макроопределения для управления выводом A3 люстры
 #define A3_ENABLE        GPIOA->BSRR = GPIO_BSRR_BS3;
 #define A3_DISABLE    	  GPIOA->BSRR = GPIO_BSRR_BR3;

 //макроопределения для управления выводом SS_SD
 #define SS_SD_ENABLE        GPIOA->BSRR = GPIO_BSRR_BR4;
 #define SS_SD_DISABLE    	  GPIOA->BSRR = GPIO_BSRR_BS4;

#define SPI2_DR_8bit         *(__IO uint8_t*)&(SPI2->DR)

 /*
  * Comands for I2C
  */
#define GET_TIME 			0x00
#define SET_TIME 			0x01
#define GET_SET_ALARM 		0x02
#define GET_SENSORS			0x03
 /*
  * Comands for Sensors
  */

#define DELAY_SENSORS		40
#define GET_TEMP			0x01
#define GET_CARB			0x02

 /*
  * Comands for ERRORS
  */
#define ERROR		 		 0x7F

 //-------------------------------------------//
 //		Настройки для управления люстрой	 //
 //-------------------------------------------//


 #define DELAY_POUSE    242					//Длительность четверти бита, мкс
 #define DELAY_BIT      3*DELAY_POUSE		//Длительность трех четвертей бита, мкс

 #define DELAY_MESSAGE  28*DELAY_POUSE		//Задержка между сообщениями, мс

 #define MES   0b0000000111110110011100000	//Управляющее слово для люстры



/*
 * Errors
 */
#define NOT_FULL_DATA 		 0x01
#define NOT_EQUAL_CRC 		 0x02
#define I2C_ERR 			 0x03



 /*
  * DS3231
  */

#define DS_ADDRESS            		0xD0
#define DS_ADDRESS_WRITE            (DS_ADDRESS|0)
#define DS_ADDRESS_READ             (DS_ADDRESS|1)

#define DS3231_CONTROL				0x0E	// Адрес регистра управления
#define DS3231_A1IE					0x00
#define DS3231_A2IE					0x01
#define DS3231_INTCN				0x02
#define DS3231_CONV					0x05
#define DS3231_BBSQW				0x06

#define DS3231_STATUS				0x0F	// Адрес регистра состояния
#define DS3231_A1F					0x00
#define DS3231_A2F					0x01
#define DS3231_BSY					0x02


#define DS3231_DY					0x0A	// Адрес регистра критерия срабатывания
#define DS3231_DY_PERSEC			0b1111
#define DS3231_DY_SEC				0b1110
#define DS3231_DY_MINSEC			0b1100
#define DS3231_DY_HMINSEC			0b1000


#define DS3231_T_MSB				0x11	// Адрес регистра последней измеренной температуры старшиий байт
#define DS3231_T_LSB				0x12	// Адрес регистра последней измеренной температуры младшиий байт

 /*
  * Alarm States
  */

#define ALARM_CHAN			0b0001
#define ALARM_LIGHT			0b0010
#define ALARM_SING			0b0100

 /*
  * Getting sensors
  */

#define TEMPERATURE			0b001
#define CARBONEUM			0b010








/*команды управления SD картой*/

#define GO_IDLE_STATE 0 											//Программная перезагрузка
#define SEND_IF_COND 8 												//Для SDC V2 - проверка диапазона напряжений
#define READ_SINGLE_BLOCK 17 									//Чтение указанного блока данных
#define WRITE_SINGLE_BLOCK 24 								//Запись указанного блока данных
#define SD_SEND_OP_COND 41 										//Начало процесса инициализации
#define APP_CMD 55 														//Главная команда из ACMD команд
#define READ_OCR 58 													//Чтение регистра OCR

//глобальная переменная для определения типа карты
uint8_t  SDHC;







void SetSysClockTo72(void);
void usart1_init(void);
void usart2_init(void);
void servo_init(void);
void ports_init(void);
void timer_init(void);
void SPI2_init(void);
void I2C1_init(void);

#ifdef __cplusplus
}
#endif

#endif /*__STM32F10x_TIM_H */
