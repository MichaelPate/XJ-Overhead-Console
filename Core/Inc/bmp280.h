/*
 * bmp280.h
 *
 *  Created on: May 6, 2024
 *      Author: MPate
 *
 *      BMP280 library adapted fom https://github.com/LonelyWolf/stm32/tree/master
 */

#ifndef INC_BMP280_H_
#define INC_BMP280_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


// These defines configure how to use the BMP280 library
// and they control what functions are implemented.
// This was done to save space
#define BMP280_LIBCONFIG_CALCTYPE	1		// 0 = 32b values, 1 = 64b values, 2 = float values
#define BMP280_LIBCONFIG_FLOATS		0		// 0 = implement float functions, 1 = no float functions

#if (BMP280_LIBCONFIG_CALCTYPE < 0) || (BMP280_LIBCONFIG_CALCTYPE > 2)
#error "Please define correct value for BMP280_LIBCONFIG_CALCTYPE"
#endif

#if (BMP280_LIBCONFIG_FLOATS < 0) || (BMP280_LIBCONFIG_FLOATS > 1)
#error "Please define correct value for BMP280_LIBCONFIG_FLOATS"
#endif


// Registers
#define BMP280_REG_CALIB00          ((uint8_t)0x88) 	// Calibration data calib00
#define BMP280_REG_CALIB25          ((uint8_t)0xA1) 	// Calibration data calib25
#define BMP280_REG_ID               ((uint8_t)0xD0) 	// Chip ID
#define BMP280_REG_RESET            ((uint8_t)0xE0) 	// Software reset control register
#define BMP280_REG_STATUS           ((uint8_t)0xF3) 	// Device status register
#define BMP280_REG_CTRL_MEAS        ((uint8_t)0xF4) 	// Pressure and temperature measure control register
#define BMP280_REG_CONFIG           ((uint8_t)0xF5) 	// Configuration register
#define BMP280_REG_PRESS_MSB        ((uint8_t)0xF7) 	// Pressure readings MSB
#define BMP280_REG_PRESS_LSB        ((uint8_t)0xF8) 	// Pressure readings LSB
#define BMP280_REG_PRESS_XLSB       ((uint8_t)0xF9) 	// Pressure readings XLSB
#define BMP280_REG_TEMP_MSB         ((uint8_t)0xFA) 	// Temperature data MSB
#define BMP280_REG_TEMP_LSB         ((uint8_t)0xFB) 	// Temperature data LSB
#define BMP280_REG_TEMP_XLSB        ((uint8_t)0xFC) 	// Temperature data XLSB


// Register Masks
#define BMP280_MASK_STATUS 			((uint8_t)0x09)		// Unused status bits
#define BMP280_MASK_OSRST			((uint8_t)0xE0)		// osrs_t in control reg
#define BMP280_MASK_OSRSP			((uint8_t)0x1C)		// osrs_p in control reg
#define BMP280_MASK_MODE			((uint8_t)0x03)		// mode in control reg
#define BMP280_MASK_STBY			((uint8_t)0x03)		// t_sb in config reg
#define BMP280_MASK_FILTER			((uint8_t)0x1C)		// filter in config reg


// Register Bits Used for Initialization Configuration
// Device state bits
#define BMP280_STATUS_IM_UPDATE     ((uint8_t)0x01) 	// [0] NVM data being copied to image registers
#define BMP280_STATUS_MEASURING     ((uint8_t)0x08) 	// [3] conversion is running

//Temperature oversampling (reg ctrl_meas:osrs_t [7:5])
#define BMP280_OSRST_SKIP  	        ((uint8_t)0x00) 	// Skipped
#define BMP280_OSRST_x1             ((uint8_t)0x20) 	// x1
#define BMP280_OSRST_x2             ((uint8_t)0x40) 	// x2
#define BMP280_OSRST_x4             ((uint8_t)0x60) 	// x4
#define BMP280_OSRST_x8             ((uint8_t)0x80) 	// x8
#define BMP280_OSRST_x16            ((uint8_t)0xA0) 	// x16

// Pressure oversampling (reg ctrl_meas:osrs_p [4:2])
#define BMP280_OSRSP_SKIP           ((uint8_t)0x00) 	// Skipped
#define BMP280_OSRSP_x1             ((uint8_t)0x04) 	// x1
#define BMP280_OSRSP_x2             ((uint8_t)0x08) 	// x2
#define BMP280_OSRSP_x4             ((uint8_t)0x0C) 	// x4
#define BMP280_OSRSP_x8             ((uint8_t)0x10) 	// x8
#define BMP280_OSRSP_x16            ((uint8_t)0x14) 	// x16

// Power mode of the device (reg ctrl_meas:mode [1:0])
#define BMP280_MODE_SLEEP           ((uint8_t)0x00) 	// Sleep mode
#define BMP280_MODE_FORCED          ((uint8_t)0x01) 	// Forced mode
#define BMP280_MODE_FORCED2         ((uint8_t)0x02) 	// Forced mode
#define BMP280_MODE_NORMAL          ((uint8_t)0x03) 	// Normal mode

// Standby duration in normal mode (reg config:t_sb [7:5])
// Specifies tsb, the inactuve duration
#define BMP280_STBY_0p5ms           ((uint8_t)0x00) 	// 0.5ms
#define BMP280_STBY_62p5ms          ((uint8_t)0x20) 	// 62.5ms
#define BMP280_STBY_125ms           ((uint8_t)0x40) 	// 125ms
#define BMP280_STBY_250ms           ((uint8_t)0x60) 	// 250ms
#define BMP280_STBY_500ms           ((uint8_t)0x80) 	// 500ms
#define BMP280_STBY_1s              ((uint8_t)0xA0) 	// 1s
#define BMP280_STBY_2s              ((uint8_t)0xC0) 	// 2s
#define BMP280_STBY_4s              ((uint8_t)0xE0) 	// 4s

// Time constant for the internal IIR filter (reg config:filter [4:2])
#define BMP280_FILTER_OFF           ((uint8_t)0x00) 	// Off
#define BMP280_FILTER_2             ((uint8_t)0x04) 	// 2
#define BMP280_FILTER_4             ((uint8_t)0x08) 	// 4
#define BMP280_FILTER_8             ((uint8_t)0x0C) 	// 8
#define BMP280_FILTER_16            ((uint8_t)0x10) 	// 16


// Status values indicating a lack of readings
#define BMP280_NO_TEMPERATURE       ((int32_t)0x80000)
#define BMP280_NO_PRESSURE          ((int32_t)0x80000)


// Writing this to the reset register will complete a POR routine
// See datasheet
#define BMP280_CMD_RESET			((uint8_t)0xB6)


// Chip IDs for samples and mass production parts
#define BMP280_CHIP_ID1             ((uint8_t)0x56)
#define BMP280_CHIP_ID2             ((uint8_t)0x57)
#define BMP280_CHIP_ID3             ((uint8_t)0x58)


/**
  * @brief  BMP280 device handle structure definition
  */
typedef struct
{
	I2C_HandleTypeDef * hi2c;
	uint8_t address;
} BMP280_HandleTypeDef;

/**
  * @brief  Enumeration for ease of reading
  */
typedef enum
{
	BMP280_Error = 0,
	BMP280_Success = 1
} BMP280_Result;


// Function Prototypes
BMP280_Result BMP280_Init(I2C_HandleTypeDef *hi2c, uint8_t address);
BMP280_Result BMP280_Check(void);
void BMP280_Reset(void);
uint8_t BMP280_GetVersion(void);
uint8_t BMP280_GetStatus(void);
uint8_t BMP280_GetMode(void);
void BMP280_SetMode(uint8_t mode);
void BMP280_SetFilter(uint8_t filter);
void BMP280_SetStandby(uint8_t tsb);
void BMP280_SetOSRST(uint8_t osrs);
void BMP280_SetOSRSP(uint8_t osrs);
BMP280_Result BMP280_ReadCalibration(void);
BMP280_Result BMP280_ReadUP(int32_t *UP);
BMP280_Result BMP280_ReadUT(int32_t *UT);
BMP280_Result BMP280_ReadUTP(int32_t *UT, int32_t *UP);
int32_t  BMP280_CalcT(int32_t UT);
uint32_t BMP280_CalcP(int32_t UP);
uint32_t BMP280_Pa_to_mmHg(uint32_t p_pa);

// To save memory, these are only implemented when the library config is set to
// see top of file
#if (BMP280_LIBCONFIG_FLOATS)

	float BMP280_FloatCalcT(int32_t UT);
	float BMP280_FloatCalcP(uint32_t UP);
	float BMP280_Float_Pa_to_mmHg(float p_pa);

#endif // BMP280_LIBCONFIG_FLOATS


#endif /* INC_BMP280_H_ */
