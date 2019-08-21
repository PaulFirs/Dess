/*
 * Программа для взаимодействия с Adroid устройством.
 * Основная задача устройства отправлять данные на Android, т.к. сложные операции выполняет телефон.
 *
 * Взаимодействие:
 * Устройство является подчиненным у Android. Android отправляет пакет данных фиксированного размера.
 * Последним байтом приходит контрольная сумма (далее - КС). Если она совпадает с рассчитанной, то устройство действует в соответствии
 * с пришедшей командой (см. в init.h).
 * Если же байт пришо меньше, чем установлено или не совпали КС, то устройство присылает на Android соответствующий код ошибки (см. в init.h).
 *
 * Работа программы:
 * Инициализация модулей происходит в init.c
 * Работа с модулями осуществлена в соответствующих .с файлах
 * Т.к. команды от Android имеют наивысший приоритет, команда от него заведены на прерывание.
 */


#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "init.h"
#include "stm32f10x_usart.h"

#include "stm32f10x_i2c.h"

#include "misc.h"
#include "stm32f10x_tim.h"

#include "i2c.h"
#include "ds3231.h"

#include "SD.h"
#include "dwt_delay.h"


volatile uint8_t RX_FLAG_END_LINE = 0;//флаг полностью собранного без ошибок сообщения от телефона

uint8_t Buff[512];

volatile uint8_t RXi = 0;
volatile uint8_t timer_uart1 = 0;
volatile uint8_t timer_uart3 = 0;

const uint8_t getppm[9]			= {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
const uint8_t ppm2k[9]			= {0xff, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xd0, 0x8f};
const uint8_t ppm5k[9]			= {0xff, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xcb};
const uint8_t autoclbdoff[9]	= {0xff, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
const uint8_t clbd[9]			= {0xff, 0x01, 0x88, 0x00, 0x00, 0x00, 0x07, 0xD0, 0xa0};

uint8_t was_I2C_ERR = 0;
uint8_t stat_alarm = 0;//настройки будильника

volatile uint8_t get_sensors = 0;

void delay(uint32_t delay)
{
for(volatile uint32_t del = 0; del<delay; del++);
}
void clear_Buffer(volatile uint8_t *buf) {
    for (uint8_t i = 0; i<BUF_SIZE; i++)
    	buf[i] = '\0';
}

inline static uint8_t CRC8(volatile uint8_t word[BUF_SIZE]) {
    uint8_t crc = 0;
	uint8_t flag = 0;
	uint8_t data = 0;
	for(uint8_t i = 0; i<(BUF_SIZE-1); i++){
		data = word[i];
		for (int j = 0; j < 8; j++) {
			flag = crc^data;
			flag = flag&0x01;
			crc = crc>>1;
			data = data >> 1;
			if (flag)
				crc = crc ^ 0x8C;
		}
	}
	return crc;
}

/*
 * срочно зделать через указатели!!!!!!!!!!!!!!!!!!!
 */

void USARTSend(volatile uint8_t pucBuffer[BUF_SIZE])
{
	pucBuffer[BUF_SIZE-1] = CRC8(TX_BUF);
    for (uint8_t i=0;i<BUF_SIZE;i++)
    {
        USART_SendData(USART1, pucBuffer[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
        {
        }
    }
    clear_Buffer(TX_BUF);//очистка буфера TX
}

void USARTSendSD(uint8_t buff)
{
	USART_SendData(USART1, buff);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{
	}
}



void USART3Send(const uint8_t pucBuffer[BUF_SIZE])
{
    for (uint8_t i=0;i<BUF_SIZE;i++)
    {
        USART_SendData(USART3, pucBuffer[i]);
        while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
        {
        }
    }
}

void USART_Error(volatile uint8_t err)
{
	RXi = 0;
	TX_BUF[0] = ERROR;
	TX_BUF[1] = err;
	USARTSend(TX_BUF);
}






void USART1_IRQHandler(void)
{
    if ((USART1->SR & USART_FLAG_RXNE) != (u16)RESET)
    {
    	timer_uart1 = 2;							//опытным путем вычисленное значение (имеет право на изменение)
		RX_BUF[RXi] = USART_ReceiveData(USART1); 	//Присвоение элементу массива значения очередного байта


		if (RXi == BUF_SIZE-1){
			RXi = 0;								//обнуление счетчика массива. Только здесь это не вызовет ошибки

			if(RX_BUF[BUF_SIZE-1]==CRC8(RX_BUF)){	//Проверка на целостность пакета данных (Величина и контрольная сумма)
				RX_FLAG_END_LINE = 1; 				//разрешение обработки данных в основном цикле
			}
			else{
				USART_Error(NOT_EQUAL_CRC);			//не совпадение CRC. Значит потеряны данные. + защита от переполнения буфера
			}

		}
		else {
			RXi++;									//переход к следующему элементу массива.
		}
    }
}

void USART3_IRQHandler(void)
{
    if ((USART3->SR & USART_FLAG_RXNE) != (u16)RESET)
    {
    	timer_uart3 = 2;// опытным путем вычисленное значение (имеет право на изменение)
		TX_BUF[RXi] = USART_ReceiveData(USART3); //Присвоение элементу массива значения очередного байта

		if (RXi == BUF_SIZE-1){
			RXi = 0;//обнуление счетчика массива. Только здесь это не вызовет ошибки
			timer_uart3 = 0;
			TX_BUF[0] = GET_SENSORS;
			TX_BUF[1] = GET_CARB;
			USARTSend(TX_BUF);
		}
		else {
			RXi++;//переход к следующему элементу массива.
		}

    }
}


void chan(void){
	for(uint8_t i = 3; i; i--){	//3 раз отправляет 0b0000000111110110011100000 для верности.
		uint32_t mes = MES;
		for(uint8_t i = 25; i; i--){
			if(mes & 1){              // Передача с нулевого бита
				A3_ENABLE;
				DWT_Delay_us(DELAY_BIT);
				A3_DISABLE;
				DWT_Delay_us(DELAY_POUSE);
			}
			else{
				A3_ENABLE;
				DWT_Delay_us(DELAY_POUSE);
				A3_DISABLE;
				DWT_Delay_us(DELAY_BIT);
			}
			mes >>= 1;
		}
		DWT_Delay_us(DELAY_MESSAGE);
	}
}

void TIM3_IRQHandler(void)
{
	static uint8_t timer_sensors = 0;
	if (TIM_GetITStatus(TIM3, ((uint16_t)0x0001)) != RESET)
	{
		//показания сенсоров
		if(timer_sensors == DELAY_SENSORS){
			if(get_sensors & TEMPERATURE){//отправить температуру

				TX_BUF[0] = GET_SENSORS;
				TX_BUF[1] = GET_TEMP;
				TX_BUF[2] = DS3231_read_temp();//Чтение темпеатуры из модуля
				if(error_i2c){
					USART_Error(error_i2c);
					get_sensors &= ~TEMPERATURE;
				}
				USARTSend(TX_BUF);
			}
			if(get_sensors & CARBONEUM){//отправить углекислый газ
				USART3Send(getppm);
			}
			timer_sensors = 0;
		}

		if (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)) {//нажата кнопка ручного включения люстры
			chan();
		}
		if(timer_sensors < DELAY_SENSORS)
			timer_sensors++;


		// Обязательно сбрасываем флаг
		TIM_ClearITPendingBit(TIM3, ((uint16_t)0x0001));
	}
}


void get_alarm(void)//будильник
{
	if(stat_alarm & ALARM_CHAN){
		chan();
	}
	if(stat_alarm & ALARM_LIGHT){
	}
	if(stat_alarm & ALARM_SING){
	}
}

void EXTI1_IRQHandler(void)//будильник
{
	EXTI->PR|=EXTI_PR_PR1; //Очищаем флаг
	ds3231_del_alarm();
	get_alarm();
}


//*********************************************************************************************
//function  обмен данными по SPI1                                                            //
//argument  передаваемый байт                                                                //
//return    принятый байт                                                                    //
//*********************************************************************************************
uint8_t spi_send(uint8_t data)
{
  while (!(SPI2->SR & SPI_SR_TXE));      //убедиться, что предыдущая передача завершена
  (SPI2->DR) = data;                       //загружаем данные для передачи
  while (!(SPI2->SR & SPI_SR_RXNE));     //ждем окончания обмена
  return (SPI2->DR);		         //читаем принятые данные
}

//*********************************************************************************************
//function  прием байт по SPI1                                                               //
//argument  none                                                                             //
//return    принятый байт                                                                    //
//*********************************************************************************************
uint8_t spi_read(void)
{
  return spi_send(0xff);		  //читаем принятые данные
}

void buff_clear()															//	очищаем буфер
{
	int i;
	for(i=0;i<512;i++)
	{
		Buff[i]=0;
	}
}

int main(void)
{
	//GPIOA->ODR^=GPIO_Pin_0;

	SetSysClockTo72();
	ports_init();
	I2C1_init();
	usart1_init();
	usart2_init();
	timer_init();
	DWT_Init();
	DS3231_init();

	SPI2_init();
	if(SD_init()==0)
		GPIOA->ODR|=GPIO_Pin_0;

	if(SD_ReadSector(1, Buff)==0)		//	чтения SD карты в буфер
		{
			GPIOA->ODR|=GPIO_Pin_2;
		}

	DWT_Delay_ms(1000);
	for(uint8_t i=0;i<64;i++)
		USARTSendSD(Buff[i]);
    while(1)
    {


    	if (RX_FLAG_END_LINE == 1) {
    		timer_uart1 = 0;
			RX_FLAG_END_LINE = 0;

			for(uint8_t i=0;i<BUF_SIZE;i++)
				Buff[i] = RX_BUF[i];

			if(SD_WriteSector(1, Buff)==0)	//	запись буфера на SD карту
				{
					GPIOA->ODR|=GPIO_Pin_1;
				}

			if(error_i2c && RX_BUF[0]<10){//Если была ошибка в шине I2C и пришла одна из команд связанных с этой шиной.

				error_i2c = 0;

				I2C_DeInit(I2C1);
				delay(250000);
				I2C1_init();
				delay(250000);
			}

			TX_BUF[0] = RX_BUF[0];//копирование ответной команды

			switch (RX_BUF[0]) {            //читаем первый принятый байт-команду

				case SET_TIME://команда связана с GET_TIME. Обе команды служат для синхронизации времени с телефоном
					for(uint8_t i = 3; i; i--)
						I2C_single_write(DS_ADDRESS, (i-1), RX_BUF[4-i]);//Запись времени от телефона в модуль


				case GET_TIME:
					for(uint8_t i = 3; i; i--)
						TX_BUF[4-i] = I2C_single_read(DS_ADDRESS, (i-1));//Чтение времени
					break;




				case GET_SET_ALARM:
					if(RX_BUF[5]){//Блок выполняется, если пользователь перенастроил будильник
						stat_alarm = RX_BUF[6];
						TX_BUF[6] = RX_BUF[6];
						if(RX_BUF[4]!=2)//Выполнять только если было изменение состояния будильника
							ds3231_on_alarm(RX_BUF[4]);

						if(RX_BUF[4]==2){//Выполнять если изменилось время будильника
							for(uint8_t i = 3; i; i--)
								I2C_single_write(DS_ADDRESS, (i+6), RX_BUF[4-i]);
						}
						//I2C_single_write(DS_ADDRESS, 0x0A, 0b10000000);
					}

					for(uint8_t i = 3; i; i--)
						TX_BUF[4-i] = I2C_single_read(DS_ADDRESS, (i+6));//Чтение времени будильника

					TX_BUF[4] = I2C_single_read(DS_ADDRESS, DS3231_CONTROL) & (1 << DS3231_A1IE);//Чтение состояния будильника

					break;
				case GET_SENSORS:
					switch(RX_BUF[1]) {

					case 0:
						get_sensors |= TEMPERATURE|CARBONEUM;
						break;
					case 1:
						USART3Send(ppm2k);
						break;
					case 2:
						USART3Send(ppm5k);
						break;
					case 3:
						USART3Send(autoclbdoff);
						break;
					case 4:
						USART3Send(clbd);
						break;
					}
					break;
			}
			if(!error_i2c){
				if(TX_BUF[0]!=GET_SENSORS)
					USARTSend(TX_BUF);//отправка собранного пакета данных
			}
			else {
				USART_Error(error_i2c);
			}
		}

    	if(timer_uart1) {// длительность между байтами в пакете данных
    		delay(50000);
			timer_uart1--;
			if(!timer_uart1){

				USART_Error(NOT_FULL_DATA);// если он обнулился здесь, то это ошибка не полного пакета.
			}
		}
    	if(timer_uart3) {// длительность между байтами в пакете данных
    		delay(50000);
			timer_uart3--;
			if(!timer_uart3){
				RXi = 0;
			}
		}
    }
}
