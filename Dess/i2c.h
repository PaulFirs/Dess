#ifndef __I2C_H
#define __I2C_H

#ifdef __cplusplus
 extern "C" {
#endif


 uint8_t cicle(I2C_TypeDef* I2Cx, uint32_t I2C_EVENT);
 void I2C_single_write(uint8_t HW_address, uint8_t addr, uint8_t data);
 uint8_t I2C_single_read(uint8_t HW_address, uint8_t addr);


#ifdef __cplusplus
}
#endif

#endif /*__I2C_H */
