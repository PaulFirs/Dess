#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "init.h"


//********************************************************************************************
//function	 посылка команды в SD                                		            //
//Arguments	 команда и ее аргумент                                                      //
//return	 0xff - нет ответа   			                                    //
//********************************************************************************************
uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg)
{
  uint8_t response, wait=0, tmp;

  //для карт памяти SD выполнить коррекцию адреса, т.к. для них адресация побайтная
  if(SDHC == 0)
  if(cmd == READ_SINGLE_BLOCK || cmd == WRITE_SINGLE_BLOCK )  {arg = arg << 9;}
  //для SDHC коррекцию адреса блока выполнять не нужно(постраничная адресация)

  SS_SD_ENABLE;

  //передать код команды и ее аргумент
  spi_send(cmd | 0x40);
  spi_send(arg>>24);
  spi_send(arg>>16);
  spi_send(arg>>8);
  spi_send(arg);

  //передать CRC (учитываем только для двух команд)
  if(cmd == SEND_IF_COND) spi_send(0x87);
  else                    spi_send(0x95);

  //ожидаем ответ
  while((response = spi_read()) == 0xff)
   if(wait++ > 0xfe) break;                //таймаут, не получили ответ на команду

  //проверка ответа если посылалась команда READ_OCR
  if(response == 0x00 && cmd == 58)
  {
    tmp = spi_read();                      //прочитать один байт регистра OCR
    if(tmp & 0x40) SDHC = 1;               //обнаружена карта SDHC
    else           SDHC = 0;               //обнаружена карта SD
    //прочитать три оставшихся байта регистра OCR
    spi_read();
    spi_read();
    spi_read();
  }

  spi_read();

  SS_SD_DISABLE;

  return response;
}

//********************************************************************************************
//function	 инициализация карты памяти                         			    //
//return	 0 - карта инициализирована  					            //
//********************************************************************************************
uint8_t SD_init(void)
{
  uint8_t   i;
  uint8_t   response;
  uint8_t   SD_version = 2;	          //по умолчанию версия SD = 2
  uint16_t  retry = 0 ;

  for(i=0;i<10;i++) spi_send(0xff);      //послать свыше 74 единиц

  //выполним программный сброс карты
  SS_SD_ENABLE;

  while(SD_sendCommand(GO_IDLE_STATE, 0)!=0x01)
    if(retry++>0xF0)  return 1;

  SS_SD_DISABLE;

  spi_send (0xff);
  spi_send (0xff);

  retry = 0;
  while(SD_sendCommand(SEND_IF_COND,0x000001AA)!=0x01)
  {
    if(retry++>0xfe)
    {
      SD_version = 1;
      break;
    }
  }

 retry = 0;
 do
 {
   response = SD_sendCommand(APP_CMD,0);
   response = SD_sendCommand(SD_SEND_OP_COND,0x40000000);
   retry++;
   if(retry>0xffe) return 1;
 }while(response != 0x00);


 //читаем регистр OCR, чтобы определить тип карты
 retry = 0;
 SDHC = 0;
 if (SD_version == 2)
 {
   while(SD_sendCommand(READ_OCR,0)!=0x00)
	 if(retry++>0xfe)  break;
 }

 return 0;
}

//********************************************************************************************
//function	 чтение выбранного сектора SD                         			    //
//аrguments	 номер сектора,указатель на буфер размером 512 байт                         //
//return	 0 - сектор прочитан успешно   					            //
//********************************************************************************************
uint8_t SD_ReadSector(uint32_t BlockNumb,uint8_t *buff)
{
  uint16_t i=0;

  //послать команду "чтение одного блока" с указанием его номера
  if(SD_sendCommand(READ_SINGLE_BLOCK, BlockNumb)) return 1;
  SS_SD_ENABLE;
  //ожидание  маркера данных
  while(spi_read() != 0xfe)
  if(i++ > 0xfffe) {SS_SD_DISABLE; return 1;}

  //чтение 512 байт	выбранного сектора
  for(i=0; i<512; i++) *buff++ = spi_read();

  spi_read();
  spi_read();
  spi_read();

  SS_SD_DISABLE;

  return 0;
}

//********************************************************************************************
//function	 запись выбранного сектора SD                         			    //
//аrguments	 номер сектора, указатель на данные для записи                              //
//return	 0 - сектор записан успешно   					            //
//********************************************************************************************
uint8_t SD_WriteSector(uint32_t BlockNumb,uint8_t *buff)
{
  uint8_t     response;
  uint16_t    i,wait=0;

  //послать команду "запись одного блока" с указанием его номера
  if( SD_sendCommand(WRITE_SINGLE_BLOCK, BlockNumb)) return 1;

  SS_SD_ENABLE;
  spi_send(0xfe);

  //записать буфер сектора в карту
  for(i=0; i<512; i++) spi_send(*buff++);

  spi_send(0xff);                //читаем 2 байта CRC без его проверки
  spi_send(0xff);

  response = spi_read();

  if( (response & 0x1f) != 0x05) //если ошибка при приеме данных картой
  { SS_SD_DISABLE; return 1; }

  //ожидаем окончание записи блока данных
  while(!spi_read())             //пока карта занята,она выдает ноль
  if(wait++ > 0xfffe){SS_SD_DISABLE; return 1;}

  SS_SD_DISABLE;
  spi_send(0xff);
  SS_SD_ENABLE;

  while(!spi_read())               //пока карта занята,она выдает ноль
  if(wait++ > 0xfffe){SS_SD_DISABLE; return 1;}
  SS_SD_DISABLE;

  return 0;
}
