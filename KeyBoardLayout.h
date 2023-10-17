#pragma once

#include "Definitions.h"

const char alphaLayout[ROWS][COLS] = {
    {'a', 's', 'd', 'f'},
    {'q', 'w', 'e', 'r'},
    {'z', 'x', 'c', 'v'},
    {'t', 'y', 'u', '#'},
};

const char numberLayout[ROWS][COLS] = {
    {'7', '8', '9', '+'},
    {'4', '5', '6', '-'},
    {'1', '2', '3', '='},
    {'0', '%', '$', '#'},
};
const char alphaUpperLayout[ROWS][COLS] = {
    {'A', 'S', 'D', 'F'},
    {'Q', 'W', 'E', 'R'},
    {'Z', 'X', 'C', 'V'},
    {'T', 'Y', 'U', '#'},
};
char KeypadLayer[ROWS * COLS] = {
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
    '\0',
};

char keys_postions[ROWS][COLS] = {
    {3, 7, 11, 15},
    {2, 6, 10, 14},
    {1, 5, 9, 13},
    {0, 4, 8, 12},
};
enum Layouts
{
  LAYOUT_0,
  LAYOUT_1,
  LAYOUT_2,
  N_LAYOUTS
};
const char *KPLayouts[N_LAYOUTS] =
    {
        (const char *)alphaLayout,
        (const char *)alphaUpperLayout,
        (const char *)numberLayout,

};

