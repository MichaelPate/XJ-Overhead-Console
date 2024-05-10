/*
 * lcd.c
 *
 *  Created on: May 4, 2024
 *      Author: Michael Pate
 *
 *      This library was adapted from Nikita Bulaev's HD44780 LCD library on github, MIT License
 *      https://github.com/firebull/STM32-LCD-HD44780-I2C/tree/master
 */

#include "lcd.h"
#include "tim.h"


static LCD_HandleTypeDef lcd1;
uint8_t lcd1_CommandBuffer[6] = {0};

// Local write byte function prototype
static bool __LCD_WriteByte(uint8_t rsBits, uint8_t * data);


/**
 * @brief	Turn the LCD on and initialize its parameters
 * @note	Following datasheet page 46. There are 4 steps
 * 			to enable 4-bit mode, and then the rest of the
 * 			parameters can be sent
 * @param  	hi2c    I2C struct to which display is connected
 * @param  	address LCD controller's I2C 7-bit address
 * @param  	rows    Number of rows of display
 * @param  	columns Number of columns of display
 * @return         	True if success, false if HAL error
 */
bool LCD_Init(I2C_HandleTypeDef *hi2c, uint8_t address, uint8_t rows, uint8_t cols)
{
	uint8_t lcdData = LCD_BIT_5x8DOTS;

	lcd1.hi2c = hi2c;
	lcd1.address = address << 1;	// typical address is 0x27
	lcd1.rows = rows;
	lcd1.cols = cols;
	lcd1.backlight = LCD_BIT_BACKLIGHT_ON;


    lcd1_CommandBuffer[0] = LCD_BIT_E | (0x03 << 4);
    lcd1_CommandBuffer[1] = lcd1_CommandBuffer[0];
    lcd1_CommandBuffer[2] = (0x03 << 4);

    // The first 3 steps of initialization are the same
    for (uint8_t i = 0; i < 3; i++)
    {
    	//while (HAL_I2C_GetState(lcd1.hi2c) != HAL_I2C_STATE_READY);

        if (HAL_I2C_Master_Transmit_DMA(lcd1.hi2c, lcd1.address, (uint8_t*)lcd1_CommandBuffer, 3) != HAL_OK)
        {
            return false;
        }

        // Wait for the LCD to ack
        while (HAL_I2C_GetState(lcd1.hi2c) != HAL_I2C_STATE_READY);

        if (i == 2)
        {
        	// The final cycle requires a 100us delay from the datasheet
        	// these steps come from https://controllerstech.com/create-1-microsecond-delay-stm32/
        	__HAL_TIM_SET_COUNTER(&htim1, 0);
        	while (__HAL_TIM_GET_COUNTER(&htim1) < 100);

        }
        else
        {
        	// The first two cycles require a 4100us delay from the datasheet
        	__HAL_TIM_SET_COUNTER(&htim1, 0);
        	while (__HAL_TIM_GET_COUNTER(&htim1) < 4100);
        }
    }

    // Now we need to set 4 bit operation
    lcd1_CommandBuffer[0] = LCD_BIT_BACKLIGHT_ON | LCD_BIT_E | (LCD_MODE_4BITS << 4);
    lcd1_CommandBuffer[1] = lcd1_CommandBuffer[0];
    lcd1_CommandBuffer[2] = LCD_BIT_BACKLIGHT_ON | (LCD_MODE_4BITS << 4);

    if (HAL_I2C_Master_Transmit_DMA(lcd1.hi2c, lcd1.address, (uint8_t*)lcd1_CommandBuffer, 3) != HAL_OK)
    {
        return false;
    }

    while (HAL_I2C_GetState(lcd1.hi2c) != HAL_I2C_STATE_READY);

    // Set display parameters
    lcdData |= LCD_MODE_4BITS;

    if (lcd1.rows == 1)
    {
    	lcdData |= LCD_BIT_1LINE;
    }
    else if (lcd1.rows == 2)
    {
    	lcdData |= LCD_BIT_2LINE;
    }
    else if (lcd1.rows == 4)
    {
    	lcdData |= LCD_BIT_4LINE;
    }
    else
    {
    	lcdData |= LCD_BIT_2LINE;
    }

    __LCD_WriteByte((uint8_t)0x00, &lcdData);

    // For the last init step, turn display, cursor, and blink all on
    LCD_DisplayOn();

    // Specify moving cursor to the right
    LCD_CursorDirToRight();

    // Clear display and home cursor
    LCD_DisplayClear();
    LCD_CursorHome();

    return true;
}


/**
 * @brief  Send a command to the display
 * @param  command  One of listed in LCDCommands enum
 * @param  action   LCD_PARAM_SET or LCD_PARAM_UNSET
 * @return          True on success, false on HAL error
 */
bool LCD_Command(LCDCommands command, LCDParamsActions action)
{
	uint8_t lcdData = 0x00;

	if (action == LCD_PARAM_SET)
	{
		switch (command)
		{
			case LCD_DISPLAY:
				lcd1.modeWord |= LCD_BIT_DISPLAY_ON;
				break;

			case LCD_CURSOR:
				lcd1.modeWord |= LCD_BIT_CURSOR_ON;
				break;

			case LCD_CURSOR_BLINK:
				lcd1.modeWord |= LCD_BIT_BLINK_ON;
				break;

			case LCD_CLEAR:
				lcdData = LCD_BIT_DISP_CLEAR;

				if (__LCD_WriteByte((uint8_t)0x00, &lcdData) == false)
				{
					return false;
				}
				else
				{
					HAL_Delay(2);
					return true;
				}
				break;

			case LCD_CURSOR_HOME:
				lcdData = LCD_BIT_CURSOR_HOME;

				if (__LCD_WriteByte((uint8_t)0x00, &lcdData) == false)
				{
					return false;
				}
				else
				{
					HAL_Delay(2);
					return true;
				}
				break;

			case LCD_CURSOR_DIR_RIGHT:
				lcd1.entryWord |= LCD_BIT_CURSOR_DIR_RIGHT;
				break;

			case LCD_CURSOR_DIR_LEFT:
				lcd1.entryWord |= LCD_BIT_CURSOR_DIR_LEFT;
				break;

			case LCD_DISPLAY_SHIFT:
				lcd1.entryWord |= LCD_BIT_DISPLAY_SHIFT;
				break;

			default:
				return false;
				break;
		}
	}
	else if (action == LCD_PARAM_UNSET)
	{
		switch (command)
		{
			case LCD_DISPLAY:
				lcd1.modeWord &= ~LCD_BIT_DISPLAY_ON;
				break;

            case LCD_CURSOR:
                lcd1.modeWord &= ~LCD_BIT_CURSOR_ON;
                break;

            case LCD_CURSOR_BLINK:
                lcd1.modeWord &= ~LCD_BIT_BLINK_ON;
                break;

            case LCD_CURSOR_DIR_RIGHT:
                lcd1.entryWord &= ~LCD_BIT_CURSOR_DIR_RIGHT;
                break;

            case LCD_CURSOR_DIR_LEFT:
                lcd1.entryWord &= ~LCD_BIT_CURSOR_DIR_LEFT;
                break;

            case LCD_DISPLAY_SHIFT:
                lcd1.entryWord &= ~LCD_BIT_DISPLAY_SHIFT;
                break;

            default:
            	return false;
            	break;
		}
	}
	else
	{
		return false;
	}

	// Now we can send the command
    switch (command)
    {
        case LCD_DISPLAY:
        case LCD_CURSOR:
        case LCD_CURSOR_BLINK:
            lcdData = LCD_BIT_DISPLAY_CONTROL | lcd1.modeWord;
            break;
        case LCD_CURSOR_DIR_RIGHT:
        case LCD_CURSOR_DIR_LEFT:
        case LCD_DISPLAY_SHIFT:
            lcdData = LCD_BIT_ENTRY_MODE | lcd1.entryWord;
            break;

        default:
            break;
    }

    return __LCD_WriteByte((uint8_t)0x00, &lcdData);
}


/**
 * @brief  Turn display's Backlight on or off
 * @param  command LCD_BIT_BACKIGHT_ON to turn display backlight on
 *                 LCD_BIT_BACKIGHT_OFF (or 0x00) to turn display backlight off
 * @return         True if success or false if HAL error
 */
bool LCD_Backlight(uint8_t command)
{
    lcd1.backlight = command;

    if (HAL_I2C_Master_Transmit_DMA(lcd1.hi2c, lcd1.address, &lcd1.backlight, 1) != HAL_OK)
    {
        return false;
    }

    while (HAL_I2C_GetState(lcd1.hi2c) != HAL_I2C_STATE_READY);

    return true;
}


/**
 * @brief  Set cursor position on the display
 * @param  column counting from 0
 * @param  row    counting from 0
 * @return        True if sucess or false if HAL error
 */
bool LCD_SetCursorPosition(uint8_t column, uint8_t row)
{
    // We will setup offsets for 4 lines maximum
    static const uint8_t lineOffsets[4] = { 0x00, 0x40, 0x14, 0x54 };

    if (row >= lcd1.rows)
    {
        row = lcd1.rows - 1;
    }

    uint8_t lcdCommand = LCD_BIT_SETDDRAMADDR | (column + lineOffsets[row]);

    return __LCD_WriteByte(0x00, &lcdCommand);
}


/**
 * @brief  Print a string from cursor position
 * @param  data   Pointer to string
 * @param  length Size of data
 * @return        True on success, false on HAL error
 */
bool LCD_PrintString(uint8_t * data, uint8_t length)
{
    for (uint8_t i = 0; i < length; ++i)
    {
        if (__LCD_WriteByte(LCD_BIT_RS, &data[i]) == false)
        {
            return false;
        }
    }

    return true;
}


/**
 * @brief  Print a single char at the cursor position
 * @param  data Symbol to print
 * @return      True on success, false on HAL error
 */
bool LCD_PrintChar(uint8_t data)
{
    return __LCD_WriteByte(LCD_BIT_RS, &data);
}


/**
 * @brief  Local function to send data to display
 * @param  rsBits   State of RS and R/W bits
 * @param  data     Pointer to data to be sent
 * @return          True if success, false on HAL error
 */
static bool __LCD_WriteByte(uint8_t rsBits, uint8_t *data)
{
    // High 4 bits
    lcd1_CommandBuffer[0] = rsBits | LCD_BIT_E | lcd1.backlight | (*data & 0xF0);  // Send data and set strobe
    lcd1_CommandBuffer[1] = lcd1_CommandBuffer[0];                                          // Strobe turned on
    lcd1_CommandBuffer[2] = rsBits | lcd1.backlight | (*data & 0xF0);              // Turning strobe off

    // Low 4 bits
    lcd1_CommandBuffer[3] = rsBits | LCD_BIT_E | lcd1.backlight | ((*data << 4) & 0xF0);  // Send data and set strobe
    lcd1_CommandBuffer[4] = lcd1_CommandBuffer[3];                                                 // Strobe turned on
    lcd1_CommandBuffer[5] = rsBits | lcd1.backlight | ((*data << 4) & 0xF0);              // Turning strobe off

    if (HAL_I2C_Master_Transmit_DMA(lcd1.hi2c, lcd1.address, (uint8_t*)lcd1_CommandBuffer, 6) != HAL_OK)
    {
        return false;
    }

    while (HAL_I2C_GetState(lcd1.hi2c) != HAL_I2C_STATE_READY);

    return true;
}
