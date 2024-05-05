/*
 * lcd.h
 *
 *  Created on: May 4, 2024
 *      Author: Michael Pate
 *
 *      This library was adapted from Nikita Bulaev's HD44780 LCD library on github, MIT License
 *      https://github.com/firebull/STM32-LCD-HD44780-I2C/tree/master
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_
#endif /* INC_LCD_H_ */

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// LCD bits
#define LCD_BIT_RS                 ((uint8_t)0x01U)
#define LCD_BIT_RW                 ((uint8_t)0x02U)
#define LCD_BIT_E                  ((uint8_t)0x04U)
#define LCD_BIT_BACKLIGHT_ON        ((uint8_t)0x08U)
#define LCD_BIT_BACKLIGHT_OFF       ((uint8_t)0x00U)

#define LCD_MODE_4BITS             ((uint8_t)0x02U)
#define LCD_BIT_1LINE              ((uint8_t)0x00U)
#define LCD_BIT_2LINE              ((uint8_t)0x08U)
#define LCD_BIT_4LINE              LCD_BIT_2LINE
#define LCD_BIT_5x8DOTS            ((uint8_t)0x00U)
#define LCD_BIT_5x10DOTS           ((uint8_t)0x04U)
#define LCD_BIT_SETCGRAMADDR       ((uint8_t)0x40U)
#define LCD_BIT_SETDDRAMADDR       ((uint8_t)0x80U)

#define LCD_BIT_DISPLAY_CONTROL    ((uint8_t)0x08U)
#define LCD_BIT_DISPLAY_ON         ((uint8_t)0x04U)
#define LCD_BIT_CURSOR_ON          ((uint8_t)0x02U)
#define LCD_BIT_CURSOR_OFF         ((uint8_t)0x00U)
#define LCD_BIT_BLINK_ON           ((uint8_t)0x01U)
#define LCD_BIT_BLINK_OFF          ((uint8_t)0x00U)

#define LCD_BIT_DISP_CLEAR         ((uint8_t)0x01U)
#define LCD_BIT_CURSOR_HOME        ((uint8_t)0x02U)

#define LCD_BIT_ENTRY_MODE         ((uint8_t)0x04U)
#define LCD_BIT_CURSOR_DIR_RIGHT   ((uint8_t)0x02U)
#define LCD_BIT_CURSOR_DIR_LEFT    ((uint8_t)0x00U)
#define LCD_BIT_DISPLAY_SHIFT      ((uint8_t)0x01U)

// there are more advanced features for shifting cursor positon
// but those dont need implemented now
// TODO: Implement advanced cursor position functionality

// The LCD controller is a HD44780 using a PCF8574 I2C expander
// See https://dawes.wordpress.com/2010/01/05/hd44780-instruction-set/


typedef struct {
	I2C_HandleTypeDef * hi2c;
	uint8_t address;	// must be shifted left by 1
	uint8_t rows;
	uint8_t cols;
	uint8_t backlight;
	uint8_t modeWord;	// display on/off control bits
	uint8_t entryWord;	// entry mode set bits
} LCD_HandleTypeDef;

typedef enum {
    LCD_BACKLIGHT = 0,
    LCD_DISPLAY,
    LCD_CLEAR,
    LCD_CURSOR,
    LCD_CURSOR_BLINK,
    LCD_CURSOR_HOME,
    LCD_CURSOR_DIR_LEFT,
    LCD_CURSOR_DIR_RIGHT,
    LCD_DISPLAY_SHIFT
} LCDCommands;

#ifndef bool
typedef enum {
    false,
    true
} bool;
#endif

typedef enum {
    LCD_PARAM_UNSET = 0,
    LCD_PARAM_SET
} LCDParamsActions;


#define LCD_BacklightOn()           LCD_Backlight(LCD_BIT_BACKIGHT_ON)
#define LCD_BacklightOff()          LCD_Backlight(LCD_BIT_BACKIGHT_OFF)
#define LCD_AutoscrollOn()          LCD_Command(LCD_DISPLAY_SHIFT, LCD_PARAM_SET)
#define LCD_AutoscrollOff()         LCD_Command(LCD_DISPLAY_SHIFT, LCD_PARAM_UNSET)
#define LCD_DisplayClear()          LCD_Command(LCD_CLEAR, LCD_PARAM_SET)
#define LCD_DisplayOn()             LCD_Command(LCD_DISPLAY, LCD_PARAM_SET)
#define LCD_DisplayOff()            LCD_Command(LCD_DISPLAY, LCD_PARAM_UNSET)
#define LCD_CursorOn()              LCD_Command(LCD_CURSOR, LCD_PARAM_SET)
#define LCD_CursorOff()             LCD_Command(LCD_CURSOR, LCD_PARAM_UNSET)
#define LCD_BlinkOn()               LCD_Command(LCD_CURSOR_BLINK, LCD_PARAM_SET)
#define LCD_BlinkOff()              LCD_Command(LCD_CURSOR_BLINK, LCD_PARAM_UNSET)
#define LCD_CursorDirToRight()      LCD_Command(LCD_CURSOR_DIR_RIGHT, LCD_PARAM_SET)
#define LCD_CursorDirToLeft()       LCD_Command(LCD_CURSOR_DIR_LEFT, LCD_PARAM_SET)
#define LCD_CursorHome()            LCD_Command(LCD_CURSOR_HOME, LCD_PARAM_SET)


bool LCD_Init(I2C_HandleTypeDef *hi2c, uint8_t address, uint8_t lines, uint8_t rows);
bool LCD_Command(LCDCommands command, LCDParamsActions action);
bool LCD_Backlight(uint8_t command);
bool LCD_SetCursorPosition(uint8_t line, uint8_t row);
bool LCD_PrintString(uint8_t * data, uint8_t length);
bool LCD_PrintChar(uint8_t data);
