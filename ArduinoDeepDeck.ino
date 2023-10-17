#include <Wire.h>
#include <Adafruit_GFX.h>
/*DSD prefix library are part of shared i2c workaround*/
#include <DSD_Adafruit_SSD1306.h>
#include <DSD_Adafruit_APDS9960.h>
#include <BleKeyboard.h>
#include <FastLED.h>
#include <ESP32Encoder.h>
#include <Keypad.h>
#include <ctype.h>

/*
User Interface

GUI using OLED and knob , navigation is done by using right knob.
At start up it will show Splash , to acces to the menus press left knob button.
Every press switch to another menu , rotating will change the option.

There are 3 menus:
  Layout
  Effects
  Settings (Only dummy)

At Layou menu you can select one of this
  Layout 1 is
      {'a', 's', 'd', 'f'},
      {'q', 'w', 'e', 'r'},
      {'z', 'x', 'c', 'v'},
      {'t', 'y', 'u', '#'},

  Layout 2 is
      {'7', '8', '9', '+'},
      {'4', '5', '6', '-'},
      {'1', '2', '3', '='},
      {'0', '%', '$', '#'},
  Layout 3 is
      {'A', 'S', 'D', 'F'},
      {'Q', 'W', 'E', 'R'},
      {'Z', 'X', 'C', 'V'},
      {'T', 'Y', 'U', '#'},

At Effects menu you can select one of this
  Fadeing
  Rainbow
  Breathing

At Settings menu you can select one of this
  Speed
  Color
  Brightness

Gestures

To activate the gesture please hold your hand infront of the sensor for a while.
Only required once.

Bluetooth keyboard

The default name of Bluetooth device is "AhuyamaKeyboard".
the LED in the top left corner will indicate if is device is connected to the PC.
RedLight is not connected , BlueLight means connected

Features:
  Keypad : every button will send a character accodint to layout
  Gesture : left and right gesture will skip fordward and backward a song
  Knob : Left knob controls the volume

*/

/*Don't remove the lines bellow .It's a workaround to solve shared i2c issue */
#if defined(ESP32)
SemaphoreHandle_t shared_i2c_mutex;
#define DSD_I2C_MUTEX_CREATE shared_i2c_mutex = xSemaphoreCreateMutex()
#endif

/*Deepdeck pin mapping*/
#include "Definitions.h"

/*Deepdeck pin mapping*/
#include "BoardPins.h"

/*Layout definition for keyboard (button matrix)*/
#include "KeyBoardLayout.h"

/*User Interface*/
#include "UI.h"

/* Global var for battery level  */
byte gBatteryLevel = 100;

/*Ble keyboard Classes*/
BleKeyboard bleKeyboard(BLE_KEYBOARD_NAME, BLE_KEYBOARD_MANUFACTURER, BLE_KEYBOARD_DEFAULT_BAT_LEVEL);

#define APDS9960_INT GESTURE_INT_PIN

DSD_Adafruit_APDS9960 apds;

/*Encoders Classes*/

ESP32Encoder ENC1;
int ENC1_min = 0;
int ENC1_max = 2;

ESP32Encoder ENC2;
int ENC2_min = 0;
int ENC2_max = 100;

/* flag to disable gesture if needed*/
byte disable_gesture = 0;

byte current_layout = LAYOUT_0; // Start with the numeric keypad.

/*Fix the position between the phisical distribution of the keys and
the virtual*/
void FixLayerPosition(const char *src, char *dst)
{

  int i, j;
  char *p = (char *)src;
  for (i = 0; i < ROWS; i++)
    for (j = 0; j < COLS; j++)
    {
      byte index = keys_postions[i][j];
      dst[i * COLS + j] = p[index];
    }
}
/* change keyboard layer using index*/
void SetKeyboardLayer(byte index)
{
  current_layout = index;
  FixLayerPosition(KPLayouts[current_layout], KeypadLayer);
}
/* get the current layer index */
byte GetKeyboardLayer(void)
{
  return current_layout;
}

/*Keypad Class*/
Keypad keypad(makeKeymap(KeypadLayer), rowPins, colPins, sizeof(rowPins), sizeof(colPins));

/* OLED Class*/
DSD_Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* LEDs variables*/
CRGB matrix_leds[MATRIX_NUM_LEDS];
CRGB status_leds[STATUS_NUM_DATA_PIN];

/*Global GUI variables*/
int brightness = 100;
int menu_count = -1;
int menu_option = -1;
Menu menu;

int menu_state;
int menu_ops;
Layout layout;
Effect effect;
Settings param;

void setup()
{
  /*Start Serial to print out messages*/
  Serial.begin(115200);

  /*wait till arduino terminal is ready*/
  delay(2000);

  /*Don't remove the linee bellow .It's a workaround to solve shared i2c issue */
  DSD_I2C_MUTEX_CREATE;

  Serial.println(F("Welcome to DeepDeck Ahuyama!!!"));
  /*Keypad matrix of matrix_leds*/
  FastLED.addLeds<WS2812, MATRIX_NUM_DATA_PIN, GRB>(matrix_leds, MATRIX_NUM_LEDS);
  /*Top Two status matrix_leds*/
  FastLED.addLeds<WS2812, STATUS_NUM_DATA_PIN, GRB>(status_leds, STATUS_NUM_LEDS);
  /*common matrix_leds Brightness*/
  FastLED.setBrightness(brightness);

  /*Initialize Display*/
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  SplashScreen();

  pinMode(batPin, INPUT);

  ESP32Encoder::useInternalWeakPullResistors = UP;
  ENC1.attachHalfQuad(ENC1_CLK_PIN, ENC1_DT_PIN);
  ENC1.clearCount();
  ENC1.setFilter(100);
  ENC2.attachHalfQuad(ENC2_CLK_PIN, ENC2_DT_PIN);
  ENC2.clearCount();
  ENC2.setFilter(100);

  pinMode(ENC1_SW_PIN, INPUT_PULLUP);
  pinMode(ENC2_SW_PIN, INPUT_PULLUP);

  keypad.addEventListener(keypadEvent); // Add an event listener.
  keypad.setHoldTime(500);              // Default is 1000mS
  SetKeyboardLayer(LAYOUT_0);

  bleKeyboard.begin();

  while (!apds.begin(10, APDS9960_AGAIN_4X,
                     APDS9960_ADDRESS, &Wire))
  {
    Serial.println("failed to initialize device! Please check your wiring.");
  }

  Serial.println("Device initialized!");

  /* gesture mode will be entered once proximity mode senses something close.
  This mean you need to hold your hand close to it for 2 seconds   */

  apds.enableProximity(true);
  apds.enableGesture(true);

  /* task for gesture sensing  core 0*/
  xTaskCreatePinnedToCore(
      taskGesture,   /* Task function. */
      "taskGesture", /* name of task. */
      10000,         /* Stack size of task */
      NULL,          /* parameter of the task */
      1,             /* priority of the task */
      NULL,          /* Task handle to keep track of created task */
      0);            /* pin task to core 0 */

  /* task for  keypad leds effects core 1*/
  xTaskCreatePinnedToCore(
      taskLeds,   /* Task function. */
      "taskLeds", /* name of task. */
      10000,      /* Stack size of task */
      NULL,       /* parameter of the task */
      1,          /* priority of the task */
      NULL,       /* Task handle to keep track of created task */
      1);         /* pin task to core 1 */
}

void taskGesture(void *parameter)
{

  while (1)
  {
    CheckGesture();
    delay(50);
  }
}

void taskLeds(void *parameter)
{

  while (1)
  {
    LedRefresh();
    delay(20);
  }
}
/*
Auxiliar function to  handle especial keypad events , like hold press , pressed, released
*/
void keypadEvent(KeypadEvent key)
{

  byte kpadState = keypad.getState();

  switch (kpadState)
  {
  case PRESSED:
    break;

  case HOLD:
    if (key == '#')
    {
    }

    break;

  case RELEASED:

    break;
  }
}
/* Function to  handle gestures and perfomr actions*/
void handleGesture()
{

  static unsigned long GestureTicks = 0;

  /*don't check the gestures too often*/
  if ((millis() - GestureTicks) <= 50)
    return;

  GestureTicks = millis();

  unsigned long tick = millis();
  uint8_t gesture = apds.readGesture();

  if (!gesture)
    return;

  /*print out gesture lapse */
  Serial.print("gesture lapse : ");
  Serial.println((millis() - tick));

  /*ignore  gesture if disable falg is activated */
  if (disable_gesture)
  {
    disable_gesture = 0;
    Serial.println("NONE3");
    return;
  }
  /*ignore  gesture if the gesture took so long */
  if ((millis() - tick) > 3000)
  {
    Serial.println("NONE2");
    return;
  }

  /* print and do an action depending on the gesture*/
  switch (gesture)
  {
  case APDS9960_UP:
    Serial.println("UP");
    break;
  case APDS9960_DOWN:
    Serial.println("DOWN");
    break;
  case APDS9960_LEFT:
    Serial.println("LEFT");
    if (bleKeyboard.isConnected())
      bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
    break;
  case APDS9960_RIGHT:
    Serial.println("RIGHT");
    if (bleKeyboard.isConnected())
      bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
    break;
  default:
    Serial.println("NONE");
  }
}

void CheckGesture(void)
{
  handleGesture();
}
void CheckBLE(void)
{
  static unsigned long BLETicks = 0;

  /* every 500ms check if BLE is connected*/
  if ((millis() - BLETicks) <= 500)
    return;

  BLETicks = millis();

  /*Depending on BLE connection status change the color of the led*/
  if (bleKeyboard.isConnected())
  {
    status_leds[0] = CRGB::Blue;
  }
  else
  {
    status_leds[0] = CRGB::Red;
  }

  FastLED.show();
}
void CheckKeypad(void)
{

  static unsigned long KeypadTicks = 0;

  if ((millis() - KeypadTicks) <= 10)
    return;

  KeypadTicks = millis();

  char key = keypad.getKey();
  if (key)
  {
    Serial.print("KEY pressed: ");
    Serial.println(key);
    if (bleKeyboard.isConnected())
    {
      bleKeyboard.write((char)key);
    }
    else
    {
    }
  }
}
void CheckEncoders(void)
{

  static unsigned long encoderTicks = 0;

  if ((millis() - encoderTicks) <= 100)
    return;

  encoderTicks = millis();

  static long pval1 = 0;
  long val = ENC1.getCount();

  // Serial.print("ENC1 val: ");
  // Serial.println(val);

  long delta = pval1 - val;
  pval1 = val;

  if (delta)
  {
    disable_gesture = 1;
  }

  // Serial.print("ENC1 delta: ");
  // Serial.println(delta);

  static long count1 = 0;

  count1 = count1 + delta;

  if (count1 < ENC1_min)
  {
    count1 = ENC1_max;
  }
  else if (count1 > ENC1_max)
  {
    count1 = ENC1_min;
  }

  // Serial.print("ENC1 count: ");
  // Serial.println(count1);
  menu_option = count1;

  static long pval2 = 0;
  val = ENC2.getCount();

  // Serial.print("ENC2 val: ");
  // Serial.println(val);

  delta = pval2 - val;
  pval2 = val;

  if (bleKeyboard.isConnected())
  {
    if (delta > 0)
      bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
    if (delta < 0)
      bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
  }

  // Serial.print("ENC2 delta: ");
  // Serial.println(delta);

  static long count2 = 0;

  count2 = count2 + delta;

  if (count2 < ENC2_min)
  {
    count2 = ENC2_max;
  }
  else if (count2 > ENC2_max)
  {
    count2 = ENC2_min;
  }

  // Serial.print("ENC2 count: ");
  // Serial.println(count2);

  static unsigned long ENC1SWTick = 0;
  int ENC1SWState = digitalRead(ENC1_SW_PIN);
  static unsigned long ENC1SWLapse = 50;
  if (ENC1SWState == HIGH)
  {
    if (millis() - ENC1SWTick > ENC1SWLapse)
    {
      ENC1SWLapse = 250;
      ENC1SWTick = millis();

      Serial.println("ENC1 button pressed");
      if (menu_count == -1)
      {
        menu_count = 0;
        menu_option = 0;
        ENC1.setCount(menu_option);
        ENC1_min = 0;
        ENC1_max = 2;
      }
      else
      {

        if (menu_count < 2)
          menu_count++;
        else
          menu_count = 0;
      }
    }
  }
  else
  {
    ENC1SWLapse = 50;
  }

  static unsigned long ENC2SWTick = 0;
  int ENC2SWState = digitalRead(ENC2_SW_PIN);
  if (ENC2SWState == HIGH)
  {
    if (millis() - ENC2SWTick > 50)
    {
      Serial.println("ENC2 button pressed");
      ENC2SWTick = millis();
    }
  }
}
/*   GUI */

void loop()
{
  CheckEncoders();
  CheckKeypad();
  CheckBLE();
  GUILoop();
  delay(10);
}

void displayPrintHCenter(uint16_t YPos, String text)
{
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

  display.setCursor((SCREEN_WIDTH - width) / 2, YPos);
  display.print(text); // text to display
  display.display();
}
void SplashScreen(void)
{
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE, BLACK);
  display.cp437(true);

  display.clearDisplay();
  display.setTextSize(1);
  displayPrintHCenter(1, "DeepDeck Ahuyama");
  display.setTextSize(2);
  displayPrintHCenter(20, "Welcome");
  display.display();
}

void LedRefresh()
{
  switch (effect)
  {
  // rainbow
  case effect1:
    fill_rainbow(matrix_leds, MATRIX_NUM_LEDS, hue, 30);
    FastLED.show();
    hue += 3;
    break;
  // fading
  case effect2:
    fill_solid(matrix_leds, MATRIX_NUM_LEDS, CHSV(hue, 255, brightness));
    FastLED.show();
    brightness = brightness + fadeAmount;
    if (brightness <= 10 || brightness >= 250)
      fadeAmount = -fadeAmount;
    break;
  // breathing
  case effect3:
    fill_solid(matrix_leds, MATRIX_NUM_LEDS, CHSV(hue, 255, 192));
    FastLED.show();
    hue += 3;
    break;
  }
}

void GUILoop(void)
{

  static int last_menu_count = -1;
  static int last_menu_option = -1;

  if ((menu_count == -1) || (menu_option == -1))
    return;

  if ((last_menu_count == menu_count) && (last_menu_option == menu_option))
    return;

  Serial.printf("menu %u option %u\n", menu_count, menu_option);

  last_menu_count = menu_count;
  last_menu_option = menu_option;

  display.clearDisplay();
  display.setTextSize(1);

  switch (menu_count)
  {
  case layer:
    displayPrintHCenter(2, "Layout");
    break;
  case light:
    displayPrintHCenter(2, "Effects");
    break;
  case settings:
    displayPrintHCenter(2, "Settings");

    break;
  }

  display.setTextSize(2);
  display.setCursor(0, 20);
  if (menu_count == 0)
  {
    switch (menu_option)
    {
    default:
    case layout1:
      SetKeyboardLayer(LAYOUT_0);
      displayPrintHCenter(20, "Layout1");
      break;
    case layout2:
      SetKeyboardLayer(LAYOUT_1);
      displayPrintHCenter(20, "Layout2");
      break;
    case layout3:
      SetKeyboardLayer(LAYOUT_2);
      displayPrintHCenter(20, "Layout3");
      break;
    }
  }
  else if (menu_count == light)
  {
    switch (menu_option)
    {
    default:
    case effect1:
      effect = effect1;
      displayPrintHCenter(20, "Breathing");
      break;
    case effect2:
      effect = effect2;
      displayPrintHCenter(20, "Fadeing");
      break;
    case effect3:
      effect = effect3;
      displayPrintHCenter(20, "Rainbow");
      break;
    }
  }
  else if (menu_count == settings)
  {
    switch (menu_option)
    {
    default:
    case color:
      param = opacity;
      displayPrintHCenter(20, "Brightness");
      break;
    case opacity:
      param = speed;
      displayPrintHCenter(20, "Speed");
      break;
    case speed:
      param = color;
      displayPrintHCenter(20, "Color");
      break;
    }
  }
  display.display();
}
