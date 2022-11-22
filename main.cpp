/**
 * @file main.cpp
 * @author Mason Lopez (masonlopez@me.com)
 * @brief in this project is the code for controllign a TFT ili9341 display with a ESP32. The purpose
 * of this display is to remove the need for hardware buttons to control the state of the bluetooth
 * amplifier. With the ESP32 and touchscreen we can emulate button presses by digitalWriting low pulses
 * to respective pins depending on what we wish to control.
 *
 * @note! This project is not complete, i am wating on the amplifier and GPIO expansion pack to come in,
 * there still need to be digital writes and reads impelmented to complete the functionaliity. This will not
 * be difficult to do as all you will need to do is put theses read writed in the loop area when it detects a
 * button has been pushed.
 *
 *
 * @version 0.1
 * @date 2022-11-21
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "FS.h"
#include "GUIfile.h"
#include <WiFi.h>

// for disabling watchdog timer on dual core processes...
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

// Calibration for the touchscreen
#define CALIBRATION_FILE "/TouchCalData123"
#define REPEAT_CAL false

// TFT screen power is connected to pin 17 so we can turn it on or off via touchscreen
#define LCD_POWER_PIN 17
#define ONE_SECOND 1000

// Replace with your network credentials
const char *ssid = "bt speaker";
const char *password = "speaker123";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current background color of the device
uint16_t SCREEN_BG = TFT_BLACK;

// array that holds string version of sleep enum
String sleepModeString[NUM_SLEEP_MODE] = {" 30s sleep", "  5m sleep", "Never sleep"};

// Create TFT object, and buttons for it
TFT_eSPI tft = TFT_eSPI();            // Invoke custom library
TFT_eSPI_Button buttons[NUM_BUTTONS]; // Instantiate the buttons

/**
 * @brief Sprite definitions for making smoother graphics
 *
 */
TFT_eSprite ampGuage = TFT_eSprite(&tft); // Amp gauge sprite
TFT_eSprite dBGauge = TFT_eSprite(&tft);  // Sound decibel gauge
TFT_eSprite vACGauge = TFT_eSprite(&tft); // Voltage of AC signal gauge

/**
 * @brief Enum declarations/instantiations.
 *
 */
DISPLAY_STATE tftState;
GUI_VIEW GUIView;
BT_MODE BTmode;
SLEEP_MODES sleepMode;

/**
 * @brief System functions to calibrate and create the butttons
 *
 */
void touch_calibrate(); // calibrates touchscreen
void init_buttons();    // inits the buttons of the screen

// function similiar to map() but for doubles
double mapf(double val, double in_min, double in_max, double out_min, double out_max);

// For using both cores of the esp32, this is mitigate lag in the USER interface
TaskHandle_t TFThelper;  // TFT helper assigns one core to only deal with graphics
TaskHandle_t WifiHelper; // WiFiHelper assigns another core to only deal with WiFi commands
QueueHandle_t queue;     // Queue used between the cores, when a button is pressed send the notification to the queue so the other core can handle

void GUIloop(void *param);
void WiFiloop(void *param);

void setup()
{
  WiFi.softAP(ssid, password); // setup wifi hidden
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();

  tft.init();
  tft.setRotation(1); // set the orientation upside down for the container
  tft.setSwapBytes(true);

  // TFT power is connected to pin, set to high for default
  pinMode(LCD_POWER_PIN, OUTPUT);
  digitalWrite(LCD_POWER_PIN, HIGH);

  // fill the screen with background color
  tft.fillScreen(SCREEN_BG);

  // init enum states with default
  tftState = DISPLAY_ON;
  GUIView = HOME_VIEW;
  BTmode = TRANSMITTER_M;
  sleepMode = SLEEP_30S;

  Serial.begin(9600); // For debug purposes
  touch_calibrate();

  // create the buttons on the screen
  init_buttons();
  disp_init_menu();
  disp_menu_BTMode();
  disp_home_gauges();

  // create sprite for ampGauge
  ampGuage.createSprite(146, 21);
  dBGauge.createSprite(146, 21);
  vACGauge.createSprite(146, 21);

  queue = xQueueCreate(8, sizeof(int)); // create queue for commands to be filled to
  xTaskCreatePinnedToCore(GUIloop, "GUIHelper", 16000, NULL, 1, &TFThelper, 1);
  delay(500);
  xTaskCreatePinnedToCore(WiFiloop, "WiFiHelper", 16000, NULL, 0, &WifiHelper, 0);
}

/**
 * @brief Nothing happening in loop since assigned tasks to each core..
 *
 */
void loop()
{
  vTaskDelete(NULL);
  delay(1);
}

/**
 * @brief Calibrates the touchscreen sensor of the TFT display.
 * Taken from the example sketch of tft_espi. Runs if there is not a saved
 * file already on the device
 *
 */
void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin())
  {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE))
  {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      fs::File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f)
      {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL)
  {
    // calibration data valid
    tft.setTouch(calData);
  }
  else
  {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL)
    {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    fs::File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

/**
 * @brief Inits and draws the buttons when device turned on. Default view is the home view. In addition also
 * draws the line under the tabs
 *
 */
void init_buttons()
{
  // create the volume and mute button
  buttons[VOL_UP_B].initButtonUL(&tft, CNTRL_COL, VOL_UP_ROW, VOL_W, VOL_H, SCREEN_BG, TFT_WHITE, SCREEN_BG, "+", 3);
  buttons[MUTE_B].initButtonUL(&tft, CNTRL_COL, MUTE_ROW, VOL_W, MUTE_H, TFT_WHITE, SCREEN_BG, TFT_WHITE, "M", 2);
  buttons[VOL_DOWN_B].initButtonUL(&tft, CNTRL_COL, VOL_DOWN_ROW, VOL_W, VOL_H, SCREEN_BG, TFT_WHITE, SCREEN_BG, "-", 3);
  buttons[VOL_UP_B].drawButton();
  buttons[MUTE_B].drawButton();
  buttons[VOL_DOWN_B].drawButton();

  // home and settings tab
  buttons[HOME_B].initButtonUL(&tft, TAB_COL_HOME - 30, TAB_ROW, 64 + 30, 33, SCREEN_BG, SCREEN_BG, TFT_WHITE, "Home", 1);
  buttons[SETTINGS_B].initButtonUL(&tft, TAB_COL_SETTINGS - 10, TAB_ROW, 64 + 30, 33, TFT_BLACK, SCREEN_BG, TFT_WHITE, "Settings", 1);

  buttons[HOME_B].drawButton();
  buttons[SETTINGS_B].drawButton();

  tft.drawLine(80, 50, 300, 50, TFT_WHITE); // draw line under tabs
  tft.drawLine(80, 51, 300, 51, TFT_WHITE); // draw line under tabs
}

/**
 * @brief Changes the TFT to display the home tab
 *
 */
void disp_home()
{
  // draw the home and settings button again to reflect current state
  buttons[HOME_B].initButtonUL(&tft, TAB_COL_HOME - 30, TAB_ROW, 64 + 30, 33, SCREEN_BG, SCREEN_BG, TFT_WHITE, "Home", 1);
  buttons[SETTINGS_B].initButtonUL(&tft, TAB_COL_SETTINGS - 10, TAB_ROW, 64 + 30, 33, TFT_BLACK, SCREEN_BG, TFT_WHITE, "Settings", 1);
  buttons[HOME_B].drawButton();
  buttons[HOME_B].drawButton();
  buttons[SETTINGS_B].drawButton();

  tft.fillRect(69, S_ROW_1, 248, 157, SCREEN_BG); // cover settings stuff
}

/**
 * @brief Changes the TFT to display settings tab
 *
 */
void disp_settings()
{

  tft.fillRect(69, H_GUAGE_ROW_1 - 12, 248, 157, SCREEN_BG); // Cover home tab stuff

  // Redraw the home and setting tabs
  buttons[HOME_B].initButtonUL(&tft, TAB_COL_HOME - 30, TAB_ROW, 64 + 30, 33, TFT_BLACK, SCREEN_BG, TFT_WHITE, "Home", 1);
  buttons[SETTINGS_B].initButtonUL(&tft, TAB_COL_SETTINGS - 10, TAB_ROW, 64 + 30, 33, SCREEN_BG, SCREEN_BG, TFT_WHITE, "Settings", 1);
  buttons[SETTINGS_B].drawButton();
  buttons[HOME_B].drawButton();

  // draw pair, tx, and rx buttons
  buttons[PAIR_B].initButtonUL(&tft, S_COL_1, S_ROW_1, S_BTN_W, S_BTN_H, TFT_GREEN, SCREEN_BG, TFT_WHITE, "Pair", 1);
  buttons[TX_B].initButtonUL(&tft, S_COL_2, S_ROW_1, S_BTN_W, S_BTN_H, TFT_RED, SCREEN_BG, TFT_WHITE, "Tx", 1);
  buttons[RX_B].initButtonUL(&tft, S_COL_3, S_ROW_1, S_BTN_W, S_BTN_H, TFT_SKYBLUE, SCREEN_BG, TFT_WHITE, "Rx", 1);

  // draw display options
  buttons[DISP_OPT_30S_B].initButtonUL(&tft, S_COL_1, S_ROW_2, S_BTN_W, S_BTN_H, TFT_YELLOW, SCREEN_BG, TFT_WHITE, "30s", 1);
  buttons[DISP_OPT_5M_B].initButtonUL(&tft, S_COL_2, S_ROW_2, S_BTN_W, S_BTN_H, TFT_YELLOW, SCREEN_BG, TFT_WHITE, "5m", 1);
  buttons[DISP_OPT_OFF_B].initButtonUL(&tft, S_COL_3, S_ROW_2, S_BTN_W, S_BTN_H, TFT_YELLOW, SCREEN_BG, TFT_WHITE, "Off", 1);

  buttons[PAIR_B].drawButton();
  buttons[TX_B].drawButton();
  buttons[RX_B].drawButton();
  buttons[DISP_OPT_30S_B].drawButton();
  buttons[DISP_OPT_5M_B].drawButton();
  buttons[DISP_OPT_OFF_B].drawButton();
}

/**
 * @brief When first turned on this draws the menu onto the LCD screen... more for looks.
 *
 */
void disp_init_menu()
{
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  // draw line for the menu
  tft.drawLine(0, 14, 319, 14, TFT_WHITE);
  tft.drawLine(0, 15, 319, 15, TFT_WHITE);
  // battery percent
  tft.setCursor(0, 5);
  tft.print("100%");
}

/**
 * @brief Displays the BT mode, reciever or transmitter mode to the menu bar
 *
 */
void disp_menu_BTMode()
{
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.fillRect(0, 0, 70, 13, SCREEN_BG);
  if (BTmode == TRANSMITTER_M)
  {
    tft.setCursor(0, 5);
    tft.print("Transmitter");
  }
  else
  {
    tft.setCursor(0, 5);
    tft.print("Reciever");
  }
}

/**
 * @brief Prints message to the menu bar
 *
 * @param mess The message to be written
 */
void disp_menu_message(String mess)
{
  tft.setTextSize(1);
  tft.fillRect(130, 0, 94, 13, SCREEN_BG);
  tft.setTextColor(TFT_GOLD);
  tft.setCursor(130, 5);
  tft.print(mess);
}

/**
 * @brief Displays the sleeping mode onto settings page by accessing the curent sleep mode.
 *
 */
void disp_sleep_mode()
{
  tft.fillRect(S_COL_2 - 3, S_ROW_2 + 40, 106, 13, SCREEN_BG);
  tft.setTextSize(1);
  tft.setCursor(S_COL_2 - 3, S_ROW_2 + 40);
  tft.setTextColor(TFT_WHITE);
  tft.print(sleepModeString[sleepMode]);
}

/**
 * @brief Builds the skeleton of the gauges when the home screen view is the home tab.
 *
 */
void disp_home_gauges()
{
  // current draw
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);

  // current draw
  tft.drawString("Current Draw: ", H_GUAGE_COL, H_GUAGE_ROW_1 - 12);
  tft.drawRoundRect(H_GUAGE_COL, H_GUAGE_ROW_1, H_GUAGE_WIDTH, H_GUAGE_HEIGHT, 2, TFT_WHITE);

  // sound decibel meter
  tft.drawString("Sound level: ", H_GUAGE_COL, H_GUAGE_ROW_2 - 12);
  tft.drawRoundRect(H_GUAGE_COL, H_GUAGE_ROW_2, H_GUAGE_WIDTH, H_GUAGE_HEIGHT, 2, TFT_WHITE);

  // voltmeter
  tft.drawString("Battery: ", H_GUAGE_COL, H_GUAGE_ROW_3 - 12);
  tft.drawRoundRect(H_GUAGE_COL, H_GUAGE_ROW_3, H_GUAGE_WIDTH, H_GUAGE_HEIGHT, 2, TFT_WHITE);
}

/**
 * @brief Better version of the map function so we can get decimal places for the battery percent
 * and battery voltage. Similiar to standard map functions...
 *
 * @param val
 * @param in_min
 * @param in_max
 * @param out_min
 * @param out_max
 * @return double
 */
double mapf(double val, double in_min, double in_max, double out_min, double out_max)
{
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Task dedicated to a core for controlling touchscreen display.
 * 
 * @param param 
 */
void GUIloop(void *param)
{
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;       // prevent watchdog from crashing program!
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;


  unsigned long lastInteraction = 0; // last time touchscreen was pressed
  unsigned long lastRefresh = 0;     // last time screen was updated
  int counter = 1;                   // dummy variable that incremented for guage purposes
  unsigned long messageTime = 0;     // time a message has been displayed for
  bool dispMessage = false;          // bool to see if there is a message to be displayed
  bool enableSleep = true;           // bool to see if we should sleep the screen (turn off)
  int sleepAmount = 30 * ONE_SECOND; // set the default sleep time to 30 sec
  double bVoltage = 0;               // dummy variable used for guage and testing purposes

  // loop forever
  while (1)
  {
    // touch coordinates of the screen
    uint16_t touchX = 0, touchY = 0;
    bool pressed = tft.getTouch(&touchX, &touchY); // return if screen is pressed
    if (pressed && tftState == DISPLAY_OFF)
    {
      digitalWrite(LCD_POWER_PIN, HIGH); // turn the screen on and toggle enum state
      tftState = DISPLAY_ON;
      lastInteraction = millis();
      delay(ONE_SECOND);
    }
    else
    { // else there is a button press
      // check if any buttons were touched
      for (uint8_t bttn = 0; bttn < NUM_BUTTONS; bttn++)
      {
        if (pressed && buttons[bttn].contains(touchX, touchY))
        {
          buttons[bttn].press(true);
          lastInteraction = millis();
        }
        else
        {
          buttons[bttn].press(false);
        }
      }

      for (uint8_t bttn_index = 0; bttn_index < NUM_BUTTONS; bttn_index++)
      {
        if (buttons[bttn_index].justReleased())
        {
          if (bttn_index >= PAIR_B && bttn_index < NUM_BUTTONS)
          { // make sure only
            if (GUIView == SETTINGS_VIEW)
            {
              buttons[bttn_index].drawButton();
            }
          }
          else
          {
            buttons[bttn_index].drawButton();
            tft.drawLine(80, 50, 300, 50, TFT_WHITE); // draw line under tabs
            tft.drawLine(80, 51, 300, 51, TFT_WHITE); // draw line under tabs
          }
        }

        if (buttons[bttn_index].justPressed()) // if button is just pressed
        {
          if (bttn_index == HOME_B) // if in home tab
          {
            if (GUIView == SETTINGS_VIEW) // change to setting tab
            {
              GUIView = HOME_VIEW;
              disp_home();
              disp_home_gauges();
              tft.drawLine(80, 50, 300, 50, TFT_WHITE); // draw line under tabs
              tft.drawLine(80, 51, 300, 51, TFT_WHITE); // draw line under tabs
              disp_menu_message("Home Tab");
              messageTime = millis();
              dispMessage = true;
            }
          }
          else if (bttn_index == SETTINGS_B) // if in setting tab
          {
            if (GUIView == HOME_VIEW) // change to home tab
            {
              GUIView = SETTINGS_VIEW;
              disp_settings();
              disp_sleep_mode();
              tft.drawLine(80, 50, 300, 50, TFT_WHITE); // draw line under tabs
              tft.drawLine(80, 51, 300, 51, TFT_WHITE); // draw line under tabs
              disp_menu_message("Settings Tab");
              messageTime = millis();
              dispMessage = true;
            }
          }
          else if (bttn_index == VOL_UP_B)
          {
            buttons[bttn_index].drawButton(true);
            disp_menu_message("Volume up");
            messageTime = millis();
            dispMessage = true;
          }
          else if (bttn_index == MUTE_B)
          {
            buttons[bttn_index].drawButton(true);
            disp_menu_message("Mute");
            messageTime = millis();
            dispMessage = true;
          }
          else if (bttn_index == VOL_DOWN_B)
          {
            buttons[bttn_index].drawButton(true);
            disp_menu_message("Volume down");
            messageTime = millis();
            dispMessage = true;
          }
          else if (bttn_index >= PAIR_B && bttn_index < NUM_BUTTONS)
          {
            if (GUIView == SETTINGS_VIEW)
            {
              buttons[bttn_index].drawButton(true);

              if (bttn_index == TX_B)
              {
                if (BTmode == RECIEVER_M)
                {
                  BTmode = TRANSMITTER_M;
                  disp_menu_BTMode();
                }
                else
                {
                  disp_menu_message("Searching Rx...");
                  messageTime = millis();
                  dispMessage = true;
                }
              }
              else if (bttn_index == RX_B)
              {
                if (BTmode == TRANSMITTER_M)
                {
                  BTmode = RECIEVER_M;
                  disp_menu_BTMode();
                }
                else
                {
                  disp_menu_message("Searching Tx...");
                  messageTime = millis();
                  dispMessage = true;
                }
              }
              else if (bttn_index == PAIR_B)
              {
                disp_menu_message("Pairing...");
                messageTime = millis();
                dispMessage = true;
              }
              else if (bttn_index == DISP_OPT_30S_B)
              {
                disp_menu_message("30s sleep...");
                messageTime = millis();
                dispMessage = true;
                enableSleep = true;
                sleepAmount = 30 * ONE_SECOND;
                sleepMode = SLEEP_30S;
                disp_sleep_mode();
              }
              else if (bttn_index == DISP_OPT_5M_B)
              {
                disp_menu_message("5m sleep...");
                messageTime = millis();
                dispMessage = true;
                enableSleep = true;
                sleepAmount = 60 * ONE_SECOND * 5; // 5 min
                sleepMode = SLEEP_5M;
                disp_sleep_mode();
              }
              else if (bttn_index == DISP_OPT_OFF_B)
              {
                disp_menu_message("Never sleep...");
                messageTime = millis();
                dispMessage = true;
                enableSleep = false;
                sleepMode = SLEEP_OFF;
                disp_sleep_mode();
              }
            }
          }
          else
          {
            buttons[bttn_index].drawButton(true);
          }
        }
      }
    }

    // Check if sleep threshold has past and if so change state and turn the screen off!
    if ((millis() - lastInteraction > sleepAmount) && tftState == DISPLAY_ON && enableSleep)
    {
      digitalWrite(LCD_POWER_PIN, LOW);
      tftState = DISPLAY_OFF;
    }

    // If there is a message to display and it has been displayed for one second reset the flag and cover it.
    if ((millis() - messageTime > ONE_SECOND) && dispMessage)
    {
      dispMessage = false;
      disp_menu_message(" ");
    }

    /**
     * @brief NOTE BELOW IS ALL SUPERFICIAL AND A PLACE HOLDER. I am waiting on parts but here
     * is where you would do fun stuff like reading inputs such that you can actually read the
     * battery voltages and such. For now i am just using a variable that increments, replicating
     * values of an ADC such that i can design stuff now
     *
     */
    if ((millis() - lastRefresh > 100) && GUIView == HOME_VIEW) // refresh rate is 100ms, change this as you please
    {
      // cover the old data with a black box, or of your liking
      ampGuage.fillRect(0, 0, counter, 21, TFT_BLACK);
      dBGauge.fillRect(0, 0, counter, 21, TFT_BLACK);
      vACGauge.fillRect(0, 0, counter, 21, TFT_BLACK);

      // again, a fake value for testing purposes
      counter += 4;
      if (counter >= 146)
      {
        counter = 0;
      }

      // battery voltage
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(1);
      double voltage = mapf(bVoltage, 0.0, 1023.0, 15.0, 21.0);

      // Push the battery voltage to upper right of screen
      tft.fillRect(281, 0, 30, 14, SCREEN_BG);
      tft.setCursor(281, 5);
      tft.printf("%2.1fV", voltage);

      // calculate percentage of volage and then map that percentage to a width we can use to show battery bar
      float percentage = mapf(voltage, 15.0, 21.0, 0, 100);
      int mapVolta = mapf(percentage, 0, 100, 1, 146);

      ampGuage.fillRectHGradient(0, 0, counter, H_GUAGE_HEIGHT - 4, TFT_DARKGREY, TFT_SKYBLUE);
      ampGuage.pushSprite(H_GUAGE_COL + 2, H_GUAGE_ROW_1 + 2);

      dBGauge.fillRectHGradient(0, 0, counter, H_GUAGE_HEIGHT - 4, TFT_DARKGREY, TFT_RED);
      dBGauge.pushSprite(H_GUAGE_COL + 2, H_GUAGE_ROW_2 + 2);

      vACGauge.fillRectHGradient(0, 0, mapVolta, H_GUAGE_HEIGHT - 4, TFT_DARKGREY, TFT_MAGENTA);
      vACGauge.pushSprite(H_GUAGE_COL + 2, H_GUAGE_ROW_3 + 2);

      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(1);

      tft.fillRect(H_GUAGE_DATA_COL, H_GUAGE_ROW_1 - 14, 50, 10, SCREEN_BG); // cover the old values
      tft.setCursor(H_GUAGE_DATA_COL, H_GUAGE_ROW_1 - 12);
      tft.setTextColor(TFT_WHITE);
      tft.printf("%2.1f mA", counter * 1.3); // print new value

      tft.fillRect(H_GUAGE_DATA_COL, H_GUAGE_ROW_2 - 14, 50, 10, SCREEN_BG); // cover old
      tft.setCursor(H_GUAGE_DATA_COL, H_GUAGE_ROW_2 - 12);
      tft.printf("%2.1f dB", counter * 3.2); // print new value

      tft.fillRect(H_GUAGE_DATA_COL, H_GUAGE_ROW_3 - 14, 50, 10, SCREEN_BG); // cover old
      tft.setCursor(H_GUAGE_DATA_COL, H_GUAGE_ROW_3 - 12);
      tft.printf("%2.1f%%", percentage); // print new value

      if (bVoltage >= 1023) // upper limit for 10 bit adc
      {
        bVoltage = 0;
      }
      else // reset if its over
      {
        bVoltage += 2;
      }
      lastRefresh = millis(); // update the last refresh time
    }

    // Check the number of items in the queue
    if (uxQueueMessagesWaiting(queue) > 0)
    {
      int buttonRec;
      if (tftState == DISPLAY_OFF)        // if display is off wake it for this..
      {
        digitalWrite(LCD_POWER_PIN, HIGH); // turn the screen on and toggle enum state
        tftState = DISPLAY_ON;
        lastInteraction = millis();
      }
      xQueueReceive(queue, &buttonRec, 0);

      switch (buttonRec)
      {
      case VOL_UP_B:
        disp_menu_message("Volume up");
        messageTime = millis();
        dispMessage = true;
        break;
      case MUTE_B:
        disp_menu_message("Mute");
        messageTime = millis();
        dispMessage = true;
        break;
      case VOL_DOWN_B:
        disp_menu_message("Volume down");
        messageTime = millis();
        dispMessage = true;
        break;
      case PAIR_B:
        disp_menu_message("Pairing...");
        messageTime = millis();
        dispMessage = true;
        break;
      case TX_B:
        if (BTmode == TRANSMITTER_M)
        {
          disp_menu_message("Searching Rx...");
          messageTime = millis();
          dispMessage = true;
        }
        else
        {
          BTmode = TRANSMITTER_M;
          disp_menu_BTMode();
        }

        break;
      case RX_B:
        if (BTmode == RECIEVER_M)
        {
          disp_menu_message("Searching Tx...");
          messageTime = millis();
          dispMessage = true;
        }
        else
        {
          BTmode = RECIEVER_M;
          disp_menu_BTMode();
        }
        break;
      default:
        break;
      }
    }
  }
}

/**
 * @brief Task dedicated to core of ESP32 for controlling via WiFi. Gives user remote access of speaker
 * from the comfort of wifi.
 * 
 * @param param 
 */
void WiFiloop(void *param)
{
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;   // change the param to prevent watchdog from crashing
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;

  while (1)
  {
    WiFiClient client = server.available(); // Listen for incoming clients
    if (client)
    {                          // If a new client connects,
      String currentLine = ""; // make a String to hold incoming data from the client
      while (client.connected())
      { // loop while the client's connected
        if (client.available())
        {                         // if there's bytes to read from the client,
          char c = client.read(); // read a byte, then
          header += c;
          if (c == '\n')
          { // if the byte is a newline character
            if (currentLine.length() == 0)
            {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              // detect if a button was pressed
              int buttonPress = -1;
              // get the button pressed
              if (header.indexOf("GET /+/push") >= 0)
              {
                buttonPress = VOL_UP_B;
                Serial.println("Volume + pushed");
              }
              else if (header.indexOf("GET /-/push") >= 0)
              {
                buttonPress = VOL_DOWN_B;
                Serial.println("Volume - pushed");
              }
              else if (header.indexOf("GET /mute/push") >= 0)
              {
                buttonPress = MUTE_B;
                Serial.println("Mute pushed");
              }
              else if (header.indexOf("GET /pair/push") >= 0)
              {
                buttonPress = PAIR_B;
                Serial.println("pair pushed");
              }
              else if (header.indexOf("GET /tx/push") >= 0)
              {
                buttonPress = TX_B;
                Serial.println("Tx pushed");
              }
              else if (header.indexOf("GET /rx/push") >= 0)
              {
                buttonPress = RX_B;
                Serial.println("Rx pushed");
              }

              if (buttonPress != -1)    // check if default value has changed so we can pass to other core!
              {
                xQueueSend(queue, &buttonPress, portMAX_DELAY);
              }

              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // CSS to style the on/off buttons
              // Feel free to change the background-color and font-size attributes to fit your preferences
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 10px auto; text-align: center;}");

              client.println(".volBut { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 50px; cursor: pointer; line-height: 2;}");

              client.println(".muteBut { background-color: #9900cc; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 50px; cursor: pointer; line-height: 2;}");

              client.println(".pairBut { background-color: #ebcf39; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 50px; cursor: pointer; line-height: 2;}");

              client.println(".txBut { background-color: #ff0000; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 50px; cursor: pointer; line-height: 2;}");

              client.println(".rxBut { background-color: #0066ff; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 50px; cursor: pointer; line-height: 2;}</style></head>");

              // Web Page Heading
              client.println("<body><h1>Bluetooth Speaker</h1>");

              client.println("<p><a href=\"/+/push\"><volBut class=\"volBut\">+</volBut></a></p>");
              client.println("<p><a href=\"/mute/push\"><muteBut class=\"muteBut\">Mute</muteBut></a></p>");
              client.println("<p><a href=\"/-/push\"><volBut class=\"volBut\">-</volBut></a></p>");

              client.println("<p><a href=\"/pair/push\"><pairBut class=\"pairBut\">Pair</pairBut></a></p>");
              client.println("<p><a href=\"/tx/push\"><txBut class=\"txBut\">Tx</txBut></a></p>");
              client.println("<p><a href=\"/rx/push\"><rxBut class=\"rxBut\">Rx</rxBut></a></p>");

              client.println("</body></html>");

              // The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            }
            else
            { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          }
          else if (c != '\r')
          {                   // if you got anything else but a carriage return character,
            currentLine += c; // add it to the end of the currentLine
          }
        }
      }
      // Clear the header variable
      header = "";
      client.stop();
    }
  }
}