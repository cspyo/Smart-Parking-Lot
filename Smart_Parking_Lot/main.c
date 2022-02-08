/*
 * 2019-06-03   Programmer : 최승표
 * MCU : TM4C1294NCPDT
 *
 * Vcc  : 5V
 * Trig : PM4
 * Echo : PM5
 * GND  : GND
 *
 * SW1을 누르면 초음파 센서를 통해 센서와 장애물 사이의 거리를 UART를 통해서 출력해본다.
 *
 * PE0~2 : lcd rw rs e
 * PK0~7 : lcd data 0~7
 * PE3~5 : 3,4,5번 조도센서의 값을 받아주는 포트
 * PD 5 4, 6 3, 0 2 : RGB 제어
 * PF3 : 부저에 PWM 생성 후 제공
 * PB0 : 인체감지 센서 입력
 * PB1 : 인체감지 센서 입력에 따른 전원 공급
 */
#include "lcd1602_drv.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include "driverlib/pwm.h"

#include "uartstdio.h"


uint32_t SysClock,timer_start,timer0_cnt,timer1_cnt;
double cm;
int de_cm;
int int_cm;
int delay;
unsigned long ulPeriod;
unsigned int cds0, cds1, cds2,pir;
unsigned int ADC[8];
char lcd_top[] = {"Parking Lot"};
char lcd_bottom[9] = {"Empty : 3"};


//pm4 가 high이면 초음파 발사, low이면 멈춤
//10us의 길이로 발사해야함
//1ms에 한번씩 초음파 발사
void Int_Timer0(){
    TimerIntClear(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
    timer0_cnt++;
    if(timer0_cnt==1000){
        GPIOPinWrite(GPIO_PORTM_BASE,GPIO_PIN_4,GPIO_PIN_4);
        timer1_cnt=0;
    }
    if(timer0_cnt==1001){
        GPIOPinWrite(GPIO_PORTM_BASE,GPIO_PIN_4,0);
        timer0_cnt=0;
    }
}

void Int_Timer1(){
    TimerIntClear(TIMER1_BASE,TIMER_TIMA_TIMEOUT);
    timer1_cnt++;
}


//초음파는 초당 340m 이동
//1us 당 0.034cm 이동
//발사된 초음파가 돌아오면 pm5 low
void Int_GPIOM(){
    if(GPIOPinRead(GPIO_PORTM_BASE,GPIO_PIN_5) == GPIO_PIN_5){
        timer_start=timer1_cnt;
    }
    if(GPIOPinRead(GPIO_PORTM_BASE,GPIO_PIN_5) == 0){
        cm=(timer1_cnt-timer_start)*34/1000/2;
    }
    int_cm = (int)cm;
    de_cm = (cm-int_cm)*100;
    GPIOIntClear(GPIO_PORTM_BASE,GPIO_PIN_5);
}

//UART
void Int_GPIOJ(){
    UARTprintf("\033[2J\033[H");
    UARTprintf("Distance = %d.%d\n",int_cm,de_cm);
    GPIOIntClear(GPIO_PORTJ_BASE,GPIO_PIN_0);
}

//ADC
void Int_GPIOD(){
    int asd =3;
    if(GPIOPinRead(GPIO_PORTD_BASE,GPIO_PIN_4) == GPIO_PIN_4){
        asd--;
    }
    if(GPIOPinRead(GPIO_PORTD_BASE,GPIO_PIN_3) == GPIO_PIN_3){
        asd--;
    }
    if(GPIOPinRead(GPIO_PORTD_BASE,GPIO_PIN_2) == GPIO_PIN_2){
        asd--;
    }

    if(asd == 3)
        lcd_bottom[8] = '3';
    else if(asd == 2)
        lcd_bottom[8] = '2';
    else if(asd == 1)
        lcd_bottom[8] = '1';
    else
        lcd_bottom[8] = '0';


    LCDPrintS_pos(lcd_bottom, 1, 3);

    GPIOIntClear(GPIO_PORTD_BASE,GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4);

}

void ADCSeq0Handler(void)
{
    ADCSequenceDataGet(ADC0_BASE, 0, ADC);
    cds0 = ADC[0];
    ADCIntClear(ADC0_BASE, 0);
}
void ADCSeq1Handler(void)
{
    ADCSequenceDataGet(ADC0_BASE, 1, ADC);
    cds1 = ADC[1];
    ADCIntClear(ADC0_BASE, 1);
}
void ADCSeq2Handler(void)
{
    ADCSequenceDataGet(ADC0_BASE, 2, ADC);
    cds2 = ADC[2];
    ADCIntClear(ADC0_BASE, 2);
}


void Delay(int x){
    int i;
    for(i=0; i<x; i++);
}

int main(void) {
    timer_start = 0;
    timer0_cnt = 0;
    timer1_cnt = 0;

    SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ|SYSCTL_OSC_MAIN|SYSCTL_USE_PLL|SYSCTL_CFG_VCO_480,120000000);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // uart PA0,1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // uart PJ0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM); // 초음파 PM4,5
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);  // GPIO K 사용 (LCD PK0~7)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  // GPIO E 사용 (ADC, PE3,4,5) (LCD PE0,1,2)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);   //GPIO Port F 사용(PWM, PF3)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);   //GPIO Port D 사용 RGB 제어 5 4 6 3 0 2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // ADC 사용
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);     //PWM 기능 사용
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1));

    ///////////////PWMInit///////////////////////////
    ulPeriod = SysClock/3000;

    GPIOPinConfigure(GPIO_PF3_M0PWM3); //GPIO_PF3_M0PWM3  :  PF3를 PWM3번을 사용하겠다.
    //PF3를 PWM으로 사용하겠습니다.
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_3);
    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_1);  //PWM 분주를 0으로 설정
    //PWM Generator 1번설정
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, ulPeriod);
    // PWM4를 PG0를 통해 출력 신호로 enable
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);
    //PWM ratio(비율)을 50%로 설정
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, ulPeriod*0.5);
    //PWM setting
    PWMGenDisable(PWM0_BASE, PWM_GEN_1);


    ////////////////ADCInit///////////////////////
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5); // PE3,4,5 핀을 ADC로 설정
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH9 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 2, ADC_CTL_CH8 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 0);
    ADCSequenceEnable(ADC0_BASE, 1);
    ADCSequenceEnable(ADC0_BASE, 2);
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_FULL, 0);


    //////////////GPIOInit///////////////////////
    //RGB 제어
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE,GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_3 | GPIO_PIN_0 | GPIO_PIN_2,0);

    //LCD
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1|GPIO_PIN_2);
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_0 | GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1|GPIO_PIN_2,GPIO_PIN_0 | GPIO_PIN_1|GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_0 | GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7,GPIO_PIN_0 | GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

    ////////////////////UARTInit/////////////////////////////
    UARTStdioConfig(0, 115200, SysClock);


    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_DIR_MODE_IN);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE );


    //Ultra
    GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE,GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTM_BASE,GPIO_PIN_5);
    GPIOIntTypeSet(GPIO_PORTM_BASE, GPIO_PIN_5, GPIO_BOTH_EDGES );
    //RGB
    GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4, GPIO_BOTH_EDGES);


    ////////////TIMERInit////////////////
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE,TIMER_A,SysClock/100000);  //10us
    TimerLoadSet(TIMER1_BASE,TIMER_A,SysClock/1000000); // 1us


    //Interrupt
    IntMasterEnable();

    ADCIntEnable(ADC0_BASE, 0);
    ADCIntEnable(ADC0_BASE, 1);
    ADCIntEnable(ADC0_BASE, 2);
    IntEnable(INT_ADC0SS0);
    IntEnable(INT_ADC0SS1);
    IntEnable(INT_ADC0SS2);
    IntEnable(INT_TIMER0A);
    IntEnable(INT_TIMER1A);
    IntEnable(INT_GPIOJ);
    IntEnable(INT_GPIOM);
    IntEnable(INT_GPIOD);

    TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER1_BASE,TIMER_TIMA_TIMEOUT);

    TimerEnable(TIMER0_BASE,TIMER_A);
    TimerEnable(TIMER1_BASE,TIMER_A);

    GPIOIntEnable(GPIO_PORTD_BASE,GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4);
    GPIOIntClear(GPIO_PORTD_BASE,GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4);

    GPIOIntEnable(GPIO_PORTM_BASE,GPIO_PIN_5);
    GPIOIntClear(GPIO_PORTM_BASE,GPIO_PIN_5);

    GPIOIntEnable(GPIO_PORTJ_BASE,GPIO_PIN_0);
    GPIOIntClear(GPIO_PORTJ_BASE,GPIO_PIN_0);

    LCDInit();
    LCDPrintS_pos(lcd_top, 0, 2);
    LCDPrintS_pos(lcd_bottom, 1, 3);


    while(1){
        //ADC
        ADCProcessorTrigger(ADC0_BASE, 0);  //ADC0 실행
        ADCProcessorTrigger(ADC0_BASE, 1);
        ADCProcessorTrigger(ADC0_BASE, 2);

        //RGB
        if(cds0 > 1000){
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_5,0);
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_4,GPIO_PIN_4);
        }
        else{
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_5,GPIO_PIN_5);
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_4,0);
        }

        if(cds1 > 1000){
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_6,0);
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_3,GPIO_PIN_3);
        }
        else{
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_6,GPIO_PIN_6);
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_3,0);
        }

        if(cds2 > 1000){
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_0,0);
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,GPIO_PIN_2);
        }
        else{
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_0,GPIO_PIN_0);
            GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,0);
        }



        //PWM
        if(int_cm < 10){
            delay = 600000;
            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1 , SysClock/5000);
        }
        else if(int_cm < 15){
            delay = 1000000;
            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1 , SysClock/4000);
        }
        else{
            delay = 1300000;
            PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1 , SysClock/3000);
        }

        if(int_cm < 20){
            Delay(delay);
            PWMGenEnable(PWM0_BASE, PWM_GEN_1);
            Delay(delay);
            PWMGenDisable(PWM0_BASE, PWM_GEN_1);
        }
        else{
            PWMGenDisable(PWM0_BASE, PWM_GEN_1);
        }
    }
}
