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


void clear_TXBuffer(void) {
    for (uint8_t i = 0; i<BUF_SIZE; i++)
        TX_BUF[i] = '\0';
}

inline static uint8_t CRC8(volatile uint8_t word[9]) {
    uint8_t crc = 0;
	uint8_t flag = 0;
	uint8_t data = 0;
	for(uint8_t i = 0; i<8; i++){
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
		RX_BUF[RXi] = USART_ReceiveData(USART1); //���������� �������� ������� �������� ���������� �����

		if ((RXi == BUF_SIZE-1)&&(RX_BUF[BUF_SIZE-1]==CRC8(RX_BUF))){//�������� �� ����������� ������ ������ (�������� � ����������� �����)
			RX_FLAG_END_LINE = 1; //���������� ��������� ������ � �������� �����
			RXi = 0;//��������� �������� �������
		}
		else {
			RXi++;
		}
    }
}

void USARTSend(volatile uint8_t pucBuffer[BUF_SIZE])
{
    for (uint8_t i=0;i<BUF_SIZE;i++)
    {
        USART_SendData(USART1, pucBuffer[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
        {
        }
    }
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



/*******************************************************************/
uint8_t I2C_single_read(uint8_t HW_address, uint8_t addr)
{
	uint8_t data;
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
	I2C_GenerateSTART(I2C1, ENABLE);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
	I2C_Send7bitAddress(I2C1, HW_address, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
	I2C_SendData(I2C1, addr);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	I2C_GenerateSTART(I2C1, ENABLE);
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
	I2C_Send7bitAddress(I2C1, HW_address, I2C_Direction_Receiver);
	while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_RECEIVED));
	data = I2C_ReceiveData(I2C1);
	I2C_AcknowledgeConfig(I2C1, DISABLE);
	I2C_GenerateSTOP(I2C1, ENABLE);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
	return data;
}


inline static uint8_t read_temp(void){
	uint8_t temp = 1;

	// �������� ������ BSY
	while(temp)
	{
		temp = I2C_single_read(DS_ADDRESS, DS3231_STATUS);
		temp &= (1 << DS3231_BSY);
	}

	// ������ �������� CONTROL � ��������� ���� CONV
	temp = I2C_single_read(DS_ADDRESS, DS3231_CONTROL);
	temp |= (1 << DS3231_CONV);
	I2C_single_write(DS_ADDRESS, DS3231_CONTROL, temp);

	// ������ �����������
	return I2C_single_read(DS_ADDRESS, DS3231_T_MSB);
}

int main(void)
{

	SetSysClockTo72();
	usart_init();
	servo_init();
	ports_init();
	I2C1_init();
	//TX_BUF[1] = I2C_single_read(DS_ADDRESS, 0x00);

    while(1)
    {
    	if (RX_FLAG_END_LINE == 1) {
			RX_FLAG_END_LINE = 0;
			//USARTSend(RX_BUF);
			switch (RX_BUF[0]) {            //������ ������ �������� ����-�������

				case GET_TIME:
					TX_BUF[0] = GET_TIME;
					for(uint8_t i = 3; i; i--)
						TX_BUF[4-i] = I2C_single_read(DS_ADDRESS, (i-1));
					USARTSend(TX_BUF);
					break;

				case SET_TIME:
					for(uint8_t i = 3; i; i--)
						I2C_single_write(DS_ADDRESS, (i-1), RX_BUF[4-i]);
					break;

				case GET_TEMP:
					TX_BUF[0] = GET_TEMP;
					TX_BUF[1] = read_temp();
					USARTSend(TX_BUF);
					break;

			}
			clear_TXBuffer();
		}
    }
}
