#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "init.h"


//********************************************************************************************
//function	 ������� ������� � SD                                		            //
//Arguments	 ������� � �� ��������                                                      //
//return	 0xff - ��� ������   			                                    //
//********************************************************************************************
uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg)
{
  uint8_t response, wait=0, tmp;

  //��� ���� ������ SD ��������� ��������� ������, �.�. ��� ��� ��������� ���������
  if(SDHC == 0)
  if(cmd == READ_SINGLE_BLOCK || cmd == WRITE_SINGLE_BLOCK )  {arg = arg << 9;}
  //��� SDHC ��������� ������ ����� ��������� �� �����(������������ ���������)

  SS_SD_ENABLE;

  //�������� ��� ������� � �� ��������
  spi_send(cmd | 0x40);
  spi_send(arg>>24);
  spi_send(arg>>16);
  spi_send(arg>>8);
  spi_send(arg);

  //�������� CRC (��������� ������ ��� ���� ������)
  if(cmd == SEND_IF_COND) spi_send(0x87);
  else                    spi_send(0x95);

  //������� �����
  while((response = spi_read()) == 0xff)
   if(wait++ > 0xfe) break;                //�������, �� �������� ����� �� �������

  //�������� ������ ���� ���������� ������� READ_OCR
  if(response == 0x00 && cmd == 58)
  {
    tmp = spi_read();                      //��������� ���� ���� �������� OCR
    if(tmp & 0x40) SDHC = 1;               //���������� ����� SDHC
    else           SDHC = 0;               //���������� ����� SD
    //��������� ��� ���������� ����� �������� OCR
    spi_read();
    spi_read();
    spi_read();
  }

  spi_read();

  SS_SD_DISABLE;

  return response;
}

//********************************************************************************************
//function	 ������������� ����� ������                         			    //
//return	 0 - ����� ����������������  					            //
//********************************************************************************************
uint8_t SD_init(void)
{
  uint8_t   i;
  uint8_t   response;
  uint8_t   SD_version = 2;	          //�� ��������� ������ SD = 2
  uint16_t  retry = 0 ;

  for(i=0;i<10;i++) spi_send(0xff);      //������� ����� 74 ������

  //�������� ����������� ����� �����
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


 //������ ������� OCR, ����� ���������� ��� �����
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
//function	 ������ ���������� ������� SD                         			    //
//�rguments	 ����� �������,��������� �� ����� �������� 512 ����                         //
//return	 0 - ������ �������� �������   					            //
//********************************************************************************************
uint8_t SD_ReadSector(uint32_t BlockNumb,uint8_t *buff)
{
  uint16_t i=0;

  //������� ������� "������ ������ �����" � ��������� ��� ������
  if(SD_sendCommand(READ_SINGLE_BLOCK, BlockNumb)) return 1;
  SS_SD_ENABLE;
  //��������  ������� ������
  while(spi_read() != 0xfe)
  if(i++ > 0xfffe) {SS_SD_DISABLE; return 1;}

  //������ 512 ����	���������� �������
  for(i=0; i<512; i++) *buff++ = spi_read();

  spi_read();
  spi_read();
  spi_read();

  SS_SD_DISABLE;

  return 0;
}

//********************************************************************************************
//function	 ������ ���������� ������� SD                         			    //
//�rguments	 ����� �������, ��������� �� ������ ��� ������                              //
//return	 0 - ������ ������� �������   					            //
//********************************************************************************************
uint8_t SD_WriteSector(uint32_t BlockNumb,uint8_t *buff)
{
  uint8_t     response;
  uint16_t    i,wait=0;

  //������� ������� "������ ������ �����" � ��������� ��� ������
  if( SD_sendCommand(WRITE_SINGLE_BLOCK, BlockNumb)) return 1;

  SS_SD_ENABLE;
  spi_send(0xfe);

  //�������� ����� ������� � �����
  for(i=0; i<512; i++) spi_send(*buff++);

  spi_send(0xff);                //������ 2 ����� CRC ��� ��� ��������
  spi_send(0xff);

  response = spi_read();

  if( (response & 0x1f) != 0x05) //���� ������ ��� ������ ������ ������
  { SS_SD_DISABLE; return 1; }

  //������� ��������� ������ ����� ������
  while(!spi_read())             //���� ����� ������,��� ������ ����
  if(wait++ > 0xfffe){SS_SD_DISABLE; return 1;}

  SS_SD_DISABLE;
  spi_send(0xff);
  SS_SD_ENABLE;

  while(!spi_read())               //���� ����� ������,��� ������ ����
  if(wait++ > 0xfffe){SS_SD_DISABLE; return 1;}
  SS_SD_DISABLE;

  return 0;
}
