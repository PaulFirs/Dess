#include "stm32f10x.h"
#include "init.h"

uint8_t DS3231_read_temp(void){
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



void ds3231_del_alarm(void){
	uint8_t temp = 1;
	temp = I2C_single_read(DS_ADDRESS, DS3231_STATUS);
	temp &= ~(1 << DS3231_A1F);
	I2C_single_write(DS_ADDRESS, DS3231_STATUS, temp);
}

void ds3231_on_alarm(uint8_t stat){

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
