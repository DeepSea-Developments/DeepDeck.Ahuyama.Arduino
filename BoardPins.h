#pragma once

#include "Definitions.h"

/* key martix  */

#define R1 0
#define R2 4
#define R3 5
#define R4 12
#define C1 13
#define C2 14
#define C3 15
#define C4 16

byte rowPins[ROWS] = {C1, C2, C3, C4};
byte colPins[COLS] = {R1, R2, R3, R4};


#define GESTURE_INT_PIN 19 

/* ENC 1*/
#define ENC1_CLK_PIN 32 // A1
#define ENC1_DT_PIN 33  // B1
#define ENC1_SW_PIN 27  // S1

/* ENC 2*/
#define ENC2_CLK_PIN 25 // A2
#define ENC2_DT_PIN 26  // B2
#define ENC2_SW_PIN 34  // S2

// neopixel backlight
#define MATRIX_NUM_LEDS 16
#define MATRIX_NUM_DATA_PIN 17

#define STATUS_NUM_LEDS 2
#define STATUS_NUM_DATA_PIN 23

// battery voltage
#define batPin 35

// status indicator LED [connected/disconnected]
#define statusLed 2

float vbat = 3.7;
byte count = 0;
uint8_t hue = 0;
// int brightness = 240;
int fadeAmount = 3;
unsigned long timer, timer_oled;
