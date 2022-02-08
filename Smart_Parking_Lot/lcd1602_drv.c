#include <stdio.h>
#include "lcd1602_drv.h"
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"

#define CMD_BASE    GPIO_PORTE_BASE
#define DATA_BASE   GPIO_PORTK_BASE

int i;
uint8_t pos=0;
/* Board dependent code */

void toggleEn(){
    GPIOPinWrite(CMD_BASE, EN, EN);
    for(i=0; i<2000;i++);
    GPIOPinWrite(CMD_BASE, EN, 0x0);
    for(i=0; i<2000;i++);
}

void cmdWrite(char cmd){
    GPIOPinWrite(DATA_BASE, ALL, cmd);
    GPIOPinWrite(CMD_BASE, RS, 0x0);
    GPIOPinWrite(CMD_BASE, RW, 0x0);
    toggleEn();
}

void dataWrite(char data){
    GPIOPinWrite(DATA_BASE, ALL, data);
    GPIOPinWrite(CMD_BASE, RS, RS);
    GPIOPinWrite(CMD_BASE, RW, 0x0);
    toggleEn();
}

void LCDInit(){
    cmdWrite(CMD_FUNC|DATA_8BIT|TWO_LINE);
    cmdWrite(CMD_ONOFF);
    cmdWrite(CMD_CLR);
    cmdWrite(CMD_HOME);
    cmdWrite(CMD_MODE|CURSOR_INC);
    cmdWrite(CMD_ONOFF|DISPLAY_ON);
}

void LCDPrintC(char ch){
    if(pos<16){
        cmdWrite(DDRAM_ADDR|(LCD_START_LINE1+pos));

    }
    else if(pos== 16)
        cmdWrite(DDRAM_ADDR|LCD_START_LINE2);
    else if(pos == 32){
        cmdWrite(DDRAM_ADDR|LCD_START_LINE1);
        pos = 0;
    }
    else{
        cmdWrite(DDRAM_ADDR|(LCD_START_LINE2+(pos-16)));

    }

    if(ch == '\n'){
        if(pos<16){
            cmdWrite(DDRAM_ADDR|LCD_START_LINE2);
            pos =0;
        }
        else{
            cmdWrite(DDRAM_ADDR|LCD_START_LINE1);
            pos = 16;
        }
    }


    dataWrite(ch);
    pos++;
    for(i=0; i<2000;i++);
}
void LCDPrintS(char *str){
    while(*str)
        LCDPrintC(*str++);
}
void LCDPrintC_pos(char ch, uint8_t low,uint8_t col){
    setPos(low,col);
    LCDPrintC(ch);
}

void LCDPrintS_pos(char *str, uint8_t low,uint8_t col){
    setPos(low,col);
    LCDPrintS(str);
}

void setPos(uint8_t low, uint8_t col){
    if(low == 0){
        pos=col;
    }
    else
    {
        pos = 16+col;
    }
}



