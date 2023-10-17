#pragma once

const byte ROWS = 4;
const byte COLS = 4;

#define SCREEN_WIDTH 128        /* OLED display width, in pixels*/
#define SCREEN_HEIGHT 64        /* OLED display height, in pixels*/
#define OLED_RESET -1           /* Reset pin -1 not used*/
#define SCREEN_I2C_ADDRESS 0x3C /* 0x3C for 128x32*/

#define BLE_KEYBOARD_NAME "AhuyamaKeyboard"
#define BLE_KEYBOARD_MANUFACTURER "DSD"
#define BLE_KEYBOARD_DEFAULT_BAT_LEVEL 100
