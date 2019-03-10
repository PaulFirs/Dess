#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "init.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_i2c.h"


volatile uint8_t RX_FLAG_END_LINE = 0;
volatile uint8_t RXi = 0;
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

void EXTI0_IRQHandler(void)
{
	GPIOC->ODR^=GPIO_Pin_13; //����������� ��������� ����������
	ds3231_del_alarm();
	EXTI->PR|=0x01; //������� ����
}
void USART1_IRQHandler(void)
{
    if ((USART1->SR & USART_FLAG_RXNE) != (u16)RESET)
    {
    	timer_uart = 2;// ������� ����� ����������� �������� (����� ����� �� ���������)
		RX_BUF[RXi] = USART_ReceiveData(USART1); //���������� �������� ������� �������� ���������� �����

		if (RXi == BUF_SIZE-1){
			if(RX_BUF[BUF_SIZE-1]==CRC8(RX_BUF)){//�������� �� ����������� ������ ������ (�������� � ����������� �����)
				RX_FLAG_END_LINE = 1; //���������� ��������� ������ � �������� �����
				RXi = 0;//��������� �������� �������. ������ ����� ��� �� ������� ������

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

void USART_Error(volatile uint8_t err)
{
	clear_Buffer(RX_BUF);
	timer_uart = 0;
	RXi = 0;
	TX_BUF[0] = ERROR;
	TX_BUF[1] = err;
	USARTSend(TX_BUF);
}



int main(void)
{

	SetSysClockTo72();
	usart_init();
	//servo_init();
	ports_init();
	I2C1_init();

	GPIOC->ODR ^= GPIO_Pin_13;

    while(1)
    {
    	if (RX_FLAG_END_LINE == 1) {
    		timer_uart = 0;
			RX_FLAG_END_LINE = 0;

			if(was_I2C_ERR && RX_BUF[0]<10){//���� ���� ������ � ���� I2C � ������ ���� �� ������ ��������� � ���� �����.

				was_I2C_ERR = 0;

				I2C_DeInit(I2C1);
				delay();
				I2C1_init();
				delay();
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

			}
			if(TX_BUF[0]!=ERROR)
				USARTSend(TX_BUF);//�������� ���������� ������ ������
			else {
				USART_Error(I2C_ERR);
				was_I2C_ERR = 1;
			}
		}
    	if(timer_uart) {// ������������ ����� ������� � ������ ������
			for(uint16_t i = 50000; i; i--);
			timer_uart--;
			if(!timer_uart){
				USART_Error(NOT_FULL_DATA);// ���� �� ��������� �����, �� ��� ������ �� ������� ������.
			}
		}
    }
}
