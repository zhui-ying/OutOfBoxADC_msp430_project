/*******************************************************************************
 *
 * UserMode.c
 *
 * Simple thermometer application that uses the external temperature sensor to
 * measure and compare with external temperature sensor and display temperature 
 * on the segmented LCD screen and send value to PC through bluetooth
 *
 * October 2015
 * Alex.Liu
 ******************************************************************************/

#ifndef USERMODE_H_
#define USERMODE_H_

#define USER_MODE      3

// See device datasheet for TLV table memory mapping
#define CALADC_12V_30C  *((unsigned int *)0x1A1A)       // Temperature Sensor Calibration-30 C
#define CALADC_12V_85C  *((unsigned int *)0x1A1C)       // Temperature Sensor Calibration-85 C

extern volatile unsigned char mode;
extern volatile unsigned char S1buttonDebounce;
extern volatile unsigned char S2buttonDebounce;
extern Timer_A_initUpModeParam initUpParam_A0;
extern volatile unsigned char userModeRunning; 
extern volatile unsigned char tempUnit;

void userModeInit(void);   
void userMode(void);
void displayValue(void);

#endif

