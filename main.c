#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "init.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_i2c.h"


#define BUF_SIZE 9
volatile uint8_t RX_FLAG_END_LINE = 0;
volatile uint8_t RXi = 0;
volatile uint8_t RX_BUF[BUF_SIZE];
volatile uint8_t TX_BUF[BUF_SIZE];
volatile uint8_t timer_uart = 0;

uint8_t was_I2C_ERR = 0;

void delay()
{
for(volatile uint32_t del = 0; del<250000; del++);
}
void clear_Buffer(uint8_t *buf) {
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


void USART1_IRQHandler(void)
{
    if ((USART1->SR & USART_FLAG_RXNE) != (u16)RESET)
    {
    	timer_uart = 2;// опытным путем вычисленное значение (имеет право на изменение)
		RX_BUF[RXi] = USART_ReceiveData(USART1); //Присвоение элементу массива значения очередного байта

		if (RXi == BUF_SIZE-1){
			if(RX_BUF[BUF_SIZE-1]==CRC8(RX_BUF)){//Проверка на целостность пакета данных (Величина и контрольная сумма)
				RX_FLAG_END_LINE = 1; //разрешение обработки данных в основном цикле
				RXi = 0;//обнуление счетчика массива. Только здесь это не вызовет ошибки

			}
			else{
				USART_Error(NOT_EQUAL_CRC);// не совпадение CRC. Значит потеряны данные. + защита от переполнения буфера
			}
		}
		else {
			RXi++;//переход к следующему элементу массива.
		}
    }
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

void USART_Error(volatile uint8_t err)
{
	clear_Buffer(RX_BUF);
	timer_uart = 0;
	RXi = 0;
	TX_BUF[0] = ERROR;
	TX_BUF[1] = err;
	USARTSend(TX_BUF);
}


void I2C_single_write(uint8_t HW_address, uint8_t addr, uint8_t data)
{
	I2C_GenerateSTART(I2C1, ENABLE);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
	I2C_Send7bitAddress(I2C1, HW_address, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
	I2C_SendData(I2C1, addr);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	I2C_SendData(I2C1, data);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	I2C_GenerateSTOP(I2C1, ENABLE);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
}

uint8_t cicle(I2C_TypeDef* I2Cx, uint32_t I2C_EVENT){//обработка событий на шине I2C
	/*
	 * Счетчик проверяет событие и по истечению прописывает команду ошибки
	 * Если событие произошло, то программа выходит из этой функции
	 */

	for(uint8_t i = 255; i; i--){
		if(I2C_CheckEvent(I2Cx, I2C_EVENT))
			return 0;
	}
	TX_BUF[0] = ERROR;
	I2C_GenerateSTOP(I2C1, ENABLE);
	return 1;
}
/*******************************************************************/
uint8_t I2C_single_read(uint8_t HW_address, uint8_t addr)
{
	uint8_t data;



	//GPIOA->ODR ^= 0b1;
	I2C_GenerateSTART(I2C1, ENABLE);
	if(cicle(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
		return 1;//просто для вылета из функции
	//GPIOA->ODR ^= 0b10;
	I2C_Send7bitAddress(I2C1, HW_address, I2C_Direction_Transmitter);
	if(cicle(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		return 1;//просто для вылета из функции
	I2C_SendData(I2C1, addr);
	if(cicle(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		return 1;//просто для вылета из функции
	I2C_GenerateSTART(I2C1, ENABLE);
	if(cicle(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
		return 1;//просто для вылета из функции
	I2C_Send7bitAddress(I2C1, HW_address, I2C_Direction_Receiver);
	if(cicle(I2C1,I2C_EVENT_MASTER_BYTE_RECEIVED))
		return 1;//просто для вылета из функции
	data = I2C_ReceiveData(I2C1);
	I2C_AcknowledgeConfig(I2C1, DISABLE);
	I2C_GenerateSTOP(I2C1, ENABLE);
	return data;
}


inline static uint8_t DS3231_read_temp(void){
	uint8_t temp = 1;

	// Ожидание сброса BSY
	while(temp)
	{
		temp = I2C_single_read(DS_ADDRESS, DS3231_STATUS);
		temp &= (1 << DS3231_BSY);
	}

	// Чтение регистра CONTROL и установка бита CONV
	temp = I2C_single_read(DS_ADDRESS, DS3231_CONTROL);
	temp |= (1 << DS3231_CONV);
	I2C_single_write(DS_ADDRESS, DS3231_CONTROL, temp);

	// Чтение температуры
	return I2C_single_read(DS_ADDRESS, DS3231_T_MSB);
}



inline static void ds3231_del_alarm(void){
	uint8_t temp = 1;
	temp = I2C_single_read(DS_ADDRESS, DS3231_STATUS);
	temp &= ~(1 << DS3231_A1F);
	I2C_single_write(DS_ADDRESS, DS3231_STATUS, temp);
}

inline static void ds3231_on_alarm(uint8_t stat){

	ds3231_del_alarm();
	uint8_t temp = 1;
	temp = I2C_single_read(DS_ADDRESS, DS3231_CONTROL);
	if(stat){
		temp |= (1 << DS3231_A1IE);// включить будильник
	}
	else{
		temp &= ~(1 << DS3231_A1IE);
	}

	I2C_single_write(DS_ADDRESS, DS3231_CONTROL, temp);


}


int main(void)
{

	SetSysClockTo72();
	usart_init();
	servo_init();
	ports_init();
	I2C1_init();

	//GPIOC->ODR ^= GPIO_Pin_13;

    while(1)
    {
    	if (RX_FLAG_END_LINE == 1) {
    		timer_uart = 0;
			RX_FLAG_END_LINE = 0;

			if(was_I2C_ERR && RX_BUF[0]<10){//Если была ошибка в шине I2C и пришла одна из команд связанных с этой шиной.

				was_I2C_ERR = 0;

				I2C_DeInit(I2C1);
				delay();
				I2C1_init();
				delay();
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



				case GET_TEMP:
					TX_BUF[1] = DS3231_read_temp();//Чтение темпеатуры из модуля
					break;

				case GET_SET_ALARM:

					if(RX_BUF[5]){//Блок выполняется, если пользователь перенастроил будильник
						if(RX_BUF[4]!=2)//Выполнять только если было изменение состояния будильника
							ds3231_on_alarm(RX_BUF[4]);

						if(RX_BUF[4]==2){//Выполнять если изменилось время будильника
							for(uint8_t i = 3; i; i--)
								I2C_single_write(DS_ADDRESS, (i+6), RX_BUF[4-i]);
						}
					}

					for(uint8_t i = 3; i; i--)
						TX_BUF[4-i] = I2C_single_read(DS_ADDRESS, (i+6));//Чтение времени будильника

					TX_BUF[4] = I2C_single_read(DS_ADDRESS, DS3231_CONTROL) & (1 << DS3231_A1IE);//Чтение состояния будильника
					break;

			}
			if(TX_BUF[0]!=ERROR)
				USARTSend(TX_BUF);//отправка собранного пакета данных
			else {
				USART_Error(I2C_ERR);
				was_I2C_ERR = 1;
			}
		}
    	if(timer_uart) {// длительность между байтами в пакете данных
			for(uint16_t i = 50000; i; i--);
			timer_uart--;
			if(!timer_uart){
				USART_Error(NOT_FULL_DATA);// если он обнулился здесь, то это ошибка не полного пакета.
			}
		}
    }
}
