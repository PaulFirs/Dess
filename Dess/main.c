#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "init.h"
#include "stm32f10x_usart.h"

#include "stm32f10x_i2c.h"

#include "misc.h"
#include "stm32f10x_tim.h"

#include "ds3231.h"

volatile uint8_t RX_FLAG_END_LINE = 0;


volatile uint8_t RXi;
volatile uint8_t timer_uart1;
volatile uint8_t timer_uart3 = 0;

const uint8_t getppm[9]			= {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
const uint8_t ppm2k[9]			= {0xff, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xd0, 0x8f};
const uint8_t ppm5k[9]			= {0xff, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xcb};
const uint8_t autoclbdoff[9]	= {0xff, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
const uint8_t clbd[9]			= {0xff, 0x01, 0x88, 0x00, 0x00, 0x00, 0x07, 0xD0, 0xa0};

uint8_t was_I2C_ERR = 0;

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
 * ������ ������� ����� ���������!!!!!!!!!!!!!!!!!!!
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
    clear_Buffer(TX_BUF);//������� ������ TX
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
	clear_Buffer(RX_BUF);
	RXi = 0;
	TX_BUF[0] = ERROR;
	TX_BUF[1] = err;
	USARTSend(TX_BUF);
}

void EXTI0_IRQHandler(void)//���������
{
	GPIOC->ODR^=GPIO_Pin_13; //����������� ��������� ����������
	ds3231_del_alarm();
	EXTI->PR|=0x01; //������� ����
}
void USART1_IRQHandler(void)
{
    if ((USART1->SR & USART_FLAG_RXNE) != (u16)RESET)
    {
    	timer_uart1 = 2;// ������� ����� ����������� �������� (����� ����� �� ���������)
		RX_BUF[RXi] = USART_ReceiveData(USART1); //���������� �������� ������� �������� ���������� �����

		if (RXi == BUF_SIZE-1){
			RXi = 0;//��������� �������� �������. ������ ����� ��� �� ������� ������

			if(RX_BUF[BUF_SIZE-1]==CRC8(RX_BUF)){//�������� �� ����������� ������ ������ (�������� � ����������� �����)
				RX_FLAG_END_LINE = 1; //���������� ��������� ������ � �������� �����
			}
			else{
				USART_Error(NOT_EQUAL_CRC);// �� ���������� CRC. ������ �������� ������. + ������ �� ������������ ������
			}

		}
		else {
			RXi++;//������� � ���������� �������� �������.
		}
    }
}

void USART3_IRQHandler(void)
{
    if ((USART3->SR & USART_FLAG_RXNE) != (u16)RESET)
    {
    	timer_uart3 = 2;// ������� ����� ����������� �������� (����� ����� �� ���������)
		TX_BUF[RXi] = USART_ReceiveData(USART3); //���������� �������� ������� �������� ���������� �����

		if (RXi == BUF_SIZE-1){
			RXi = 0;//��������� �������� �������. ������ ����� ��� �� ������� ������
			timer_uart3 = 0;
			TX_BUF[0] = GET_CO2;
			USARTSend(TX_BUF);
		}
		else {
			RXi++;//������� � ���������� �������� �������.
		}

    }
}




void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, ((uint16_t)0x0001)) != RESET)
	{

		USART3Send(getppm);
		// ����������� ���������� ����
		TIM_ClearITPendingBit(TIM3, ((uint16_t)0x0001));

	}
}



int main(void)
{
	RXi = 0;
	timer_uart1 = 0;


	SetSysClockTo72();


	I2C1_init();
	usart1_init();
	usart2_init();
	ports_init();
	timer_init();

	GPIOC->ODR ^= GPIO_Pin_13;

    while(1)
    {
    	if (RX_FLAG_END_LINE == 1) {
    		timer_uart1 = 0;
			RX_FLAG_END_LINE = 0;

			if(was_I2C_ERR && RX_BUF[0]<10){//���� ���� ������ � ���� I2C � ������ ���� �� ������ ��������� � ���� �����.

				was_I2C_ERR = 0;

				I2C_DeInit(I2C1);
				delay(250000);
				I2C1_init();
				delay(250000);
			}

			TX_BUF[0] = RX_BUF[0];//����������� �������� �������
			switch (RX_BUF[0]) {            //������ ������ �������� ����-�������
				case SET_TIME://������� ������� � GET_TIME. ��� ������� ������ ��� ������������� ������� � ���������
					for(uint8_t i = 3; i; i--)
						I2C_single_write(DS_ADDRESS, (i-1), RX_BUF[4-i]);//������ ������� �� �������� � ������


				case GET_TIME:
					for(uint8_t i = 3; i; i--)
						TX_BUF[4-i] = I2C_single_read(DS_ADDRESS, (i-1));//������ �������
					break;



				case GET_TEMP:
					TX_BUF[1] = DS3231_read_temp();//������ ���������� �� ������
					break;

				case GET_SET_ALARM:

					if(RX_BUF[5]){//���� �����������, ���� ������������ ������������ ���������
						if(RX_BUF[4]!=2)//��������� ������ ���� ���� ��������� ��������� ����������
							ds3231_on_alarm(RX_BUF[4]);

						if(RX_BUF[4]==2){//��������� ���� ���������� ����� ����������
							for(uint8_t i = 3; i; i--)
								I2C_single_write(DS_ADDRESS, (i+6), RX_BUF[4-i]);
						}
						I2C_single_write(DS_ADDRESS, 0x0A, 0b10000000);
					}

					for(uint8_t i = 3; i; i--)
						TX_BUF[4-i] = I2C_single_read(DS_ADDRESS, (i+6));//������ ������� ����������

					TX_BUF[4] = I2C_single_read(DS_ADDRESS, DS3231_CONTROL) & (1 << DS3231_A1IE);//������ ��������� ����������
					break;
				case GET_CO2:
					switch(RX_BUF[1]) {

					case 0:
						//if(TIM3->CR1 & TIM_CR1_CEN)
							//TIM_Cmd(TIM3, DISABLE);
						//else
						TIM_Cmd(TIM3, ENABLE);
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
			if(TX_BUF[0]!=ERROR){
				if(TX_BUF[0]!=GET_CO2)
					USARTSend(TX_BUF);//�������� ���������� ������ ������
			}
			else {
				USART_Error(I2C_ERR);
				was_I2C_ERR = 1;
			}
		}

    	if(timer_uart1) {// ������������ ����� ������� � ������ ������
    		delay(50000);
			timer_uart1--;
			if(!timer_uart1){
				USART_Error(NOT_FULL_DATA);// ���� �� ��������� �����, �� ��� ������ �� ������� ������.
			}
		}
    	if(timer_uart3) {// ������������ ����� ������� � ������ ������
    		delay(50000);
			timer_uart3--;
			if(!timer_uart3){
				RXi = 0;
			}
		}
    }
}
