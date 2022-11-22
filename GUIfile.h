/**
 * @file GUIfile.h
 * @author Mason Lopez (masonlopez@me.com)
 * @brief Helper file for main file, reduces the clutter. This file mainly contains definitons
 * and enums that help create the state machine functionality the project is built around.
 * @version 0.1
 * @date 2022-11-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// Definitons of current screen
#define SCREEN_WIDTH  319
#define SCREEN_HEIGHT 239

// S denoting settings tab 
#define S_ROW_1 85                          // Settings row 1 y pos
#define S_ROW_2 S_ROW_1 + S_B_ROW_SPACE     // Settings row 2 y pos
#define S_B_ROW_SPACE 60                    // spacing between rows

#define S_COL_1 88                          // Settings col 1 x pos
#define S_COL_2 S_COL_1+S_COL_SPACE         // Settings col 2 x pos
#define S_COL_3 S_COL_2+S_COL_SPACE         // Settings col 3 x pos
#define S_COL_SPACE 70

#define S_BTN_W 61                          // Settings button width
#define S_BTN_H 31                          // Settings button height

#define TAB_ROW             22              // Row tab y pos
#define TAB_COL_HOME        112             // Col x pos
#define TAB_COL_SETTINGS    204             // Col x pos

#define CNTRL_COL       11                  // Vol up, down and mute col
#define VOL_UP_ROW      36                  // Vol up row
#define MUTE_ROW        106                 // Mute row
#define VOL_DOWN_ROW    156                 // Vol down row
#define VOL_W           52                  // Vol width
#define VOL_H           62                  // Vol height
#define MUTE_H          42                  // Mute height

// Gauge definitions
#define H_GUAGE_COL       115               // Gauge col start
#define H_GUAGE_WIDTH     150               // Guage width
#define H_GUAGE_HEIGHT    25                // Guage height

#define H_GUAGE_ROW_1 73                    // Starting row of gauges.. change this to change others
#define H_GUAGE_ROW_2 H_GUAGE_ROW_1 + H_GUAGE_ROW_SPACE
#define H_GUAGE_ROW_3 H_GUAGE_ROW_2 + H_GUAGE_ROW_SPACE

#define H_GUAGE_ROW_SPACE 60                // Space between rows
#define H_GUAGE_DATA_COL 200                // Col where data ie vars are printed to


/**
 * @brief Functions definitions for pushing stuff to the screen. 
 * Check main for descriptions of each.
 * 
 */
void disp_home();                         
void disp_settings();                        
void disp_init_menu();
void disp_menu_BTMode();
void disp_menu_message(String message);
void disp_sleep_mode();
void disp_home_gauges();


/**
 * @brief Enum for all the buttons that are used in the project,
 * makes life easier when we init and call functions for buttons since we can 
 * make an array of buttons and access the elements using the enums..
 * 
 */
enum BUTTON_NAMES{
    VOL_UP_B,
    MUTE_B,
    VOL_DOWN_B,
    HOME_B,
    SETTINGS_B,
        PAIR_B,
        TX_B,
        RX_B,
            DISP_OPT_30S_B,
            DISP_OPT_5M_B,
            DISP_OPT_OFF_B,


    NUM_BUTTONS
};

/**
 * @brief Names for the state of the screen whether its on or off.
 * 
 */
enum DISPLAY_STATE{
    DISPLAY_ON, DISPLAY_OFF, NUM_DISPLAY_STATES
};

/**
 * @brief States the GUI can be in whether the home or the settings tab is selected
 * 
 */
enum GUI_VIEW{
    HOME_VIEW,
    SETTINGS_VIEW,
    NUM_GUI_VIEWS
};

/**
 * @brief The current mode of the device, whether it is in transmitter or 
 * reciever mode.
 * 
 */
enum BT_MODE{
    TRANSMITTER_M,
    RECIEVER_M,
    NUM_BT_MODE
};

/**
 * @brief The current sleep mode the device can be set to. 
 * 
 */
enum SLEEP_MODES{
    SLEEP_30S,              // Screen will turn off after 30 seconds
    SLEEP_5M,               // 5 minutes
    SLEEP_OFF,              // will never turn off
    NUM_SLEEP_MODE
};