#ifndef __DS3231_H
#define __DS3231_H

#ifdef __cplusplus
 extern "C" {
#endif


 void DS3231_init(void);
uint8_t DS3231_read_temp(void);
void ds3231_del_alarm(void);
void ds3231_on_alarm(uint8_t stat);



#ifdef __cplusplus
}
#endif

#endif /*__DS3231_H */
