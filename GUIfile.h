// Definitons of current screen
#define SCREEN_WIDTH  319
#define SCREEN_HEIGHT 239

// #define TAB_ROW 53

#define S_ROW_1 85 // Settings row 1 y pos
#define S_ROW_2 S_ROW_1 + S_B_ROW_SPACE // Settings row 2 y pos

#define S_B_ROW_SPACE 60

#define S_COL_1 88  // Settings col 1 x pos
#define S_COL_2 S_COL_1+S_COL_SPACE // Settings col 2 x pos
#define S_COL_3 S_COL_2+S_COL_SPACE // Settings col 3 x pos
#define S_COL_SPACE 70

#define S_BTN_W 61  // Settings button width
#define S_BTN_H 31  // Settings button height

#define TAB_ROW             22  // Row tab y pos
#define TAB_COL_HOME        112 // Col x pos
#define TAB_COL_SETTINGS    204 // Col x pos

#define CNTRL_COL       11  // Vol up, down and mute col
#define VOL_UP_ROW      36  // Vol up row
#define MUTE_ROW        106 // Mute row
#define VOL_DOWN_ROW    156 // Vol down row
#define VOL_W           52  // Vol width
#define VOL_H           62  // Vol height
#define MUTE_H          42  // Mute height

#define H_GUAGE_COL       115   // Gauge col start
#define H_GUAGE_WIDTH     150   // Guage width
#define H_GUAGE_HEIGHT    25    // Guage height

#define H_GUAGE_ROW_1 73
#define H_GUAGE_ROW_2 H_GUAGE_ROW_1 + H_GUAGE_ROW_SPACE
#define H_GUAGE_ROW_3 H_GUAGE_ROW_2 + H_GUAGE_ROW_SPACE

#define H_GUAGE_ROW_SPACE 60
#define H_GUAGE_DATA_COL 200


// #define SCREEN_BG TFT_BLACK     // Background color of screen



void disp_home();       // displays the home text and information
void disp_settings();   // displays the settings text and buttons
void disp_init_menu();
void disp_menu_BTMode();
void disp_menu_message(String message);
void disp_sleep_mode();
void disp_home_gauges();
void disp_menu_battery(double voltage);


/**
 * @brief Enum of all the buttons in the GUI
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
 * @brief Names for the state of the screen whether its on or off
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

enum BT_MODE{
    TRANSMITTER_M,
    RECIEVER_M,
    NUM_BT_MODE
};

// ENUM FOR SLEEP STATES
enum SLEEP_MODES{
    SLEEP_30S,
    SLEEP_5M,
    SLEEP_OFF,
    NUM_SLEEP_MODE
};