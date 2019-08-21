#ifndef __CD_H
#define __CD_H

#ifdef __cplusplus
 extern "C" {
#endif




uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg);
uint8_t SD_init(void);
uint8_t SD_ReadSector(uint32_t BlockNumb,uint8_t *buff);
uint8_t SD_WriteSector(uint32_t BlockNumb,uint8_t *buff);


#ifdef __cplusplus
}
#endif

#endif /*__STM32F10x_TIM_H */
