#include <stdint.h>
#include <stdbool.h>
#include "driverlib/gpio.h"


/* Board dependent */
#define ALL         0xFF
#define RS          GPIO_PIN_0
#define RW          GPIO_PIN_1
#define EN          GPIO_PIN_2
/*
 * LCD Command Set
 */
#define CMD_CLR     0X01
#define CMD_HOME    0X02
#define CMD_MODE    0X04
#define CMD_ONOFF   0X08
#define CMD_SHIFT   0X10
#define CMD_FUNC    0X20
#define CGRAM_ADDR  0X40
#define DDRAM_ADDR  0X80

/*
 * Entry Mode Set
 */
#define DISP_SHIFT  0X01
#define CURSOR_INC  0X02

/*
 * Display On/Off Control
 */
#define BLINK_ON    0X01
#define CURSOR_ON   0X02
#define DISPLAY_ON  0X04

/*
 *
 * Cursor or Display Shift
 */
#define CURSOR_R    0X04
#define CUR_SHIFT   0X08


/*
 * Function set
 */
#define FONT_5x10   0X04
#define TWO_LINE    0X08
#define DATA_8BIT   0X10



#define BUSY_FLAG   0X80

#define LCD_CH_LEN      16
#define LCD_START_LINE1 0x00
#define LCD_START_LINE2 0x40


void toggleEn(void);
void cmdWrite(char cmd);
void dataWrite(char data);
void LCDPrint(const uint8_t *p, uint32_t count);
void LCDInit();



