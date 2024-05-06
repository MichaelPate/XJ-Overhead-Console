/*
 * bmp280.c
 *
 *  Created on: May 6, 2024
 *      Author: MPate
 */

#include "bmp280.h"


static BMP280_HandleTypeDef bmp1;

// See the library header file for configuring the library
#if (BMP280_LIBCONFIG_CALCTYPE == 0) || (BMP280_LIBCONFIG_CALCTYPE == 1)
static int32_t t_fine;
#endif

// See the library header file for configuring the library
#if (BMP280_LIBCONFIG_CALCTYPE == 2) || (BMP280_LIBCONFIG_FLOATS)
static float t_fine_f;
#endif


/**
  * @brief  BMP280 device compensation parameters
  */
static struct
{
	uint16_t dig_T1;
	int16_t  dig_T2;
	int16_t  dig_T3;
	uint16_t dig_P1;
	int16_t  dig_P2;
	int16_t  dig_P3;
	int16_t  dig_P4;
	int16_t  dig_P5;
	int16_t  dig_P6;
	int16_t  dig_P7;
	int16_t  dig_P8;
	int16_t  dig_P9;
} BMP280_CalParams;


// Local static function prototypes
static void __BMP280_WriteReg(uint8_t reg, uint8_t data);
static uint8_t __BMP280_ReadReg(uint8_t reg);
static BMP280_Result __BMP280_BulkReadReg(uint8_t reg, uint8_t *buf, uint32_t size);


/**
 * @brief  			 Initialize the BMP280 handle
 * @param  hi2c	 	 I2C handle to which the BMP is connected to
 * @param  address	 I2C address (7 bit)
 * @retval 			 BMP280_Error or BMP280_Success
 */
BMP280_Result BMP280_Init(I2C_HandleTypeDef *hi2c, uint8_t address)
{
	bmp1.hi2c = hi2c;
	bmp1.address = address << 1;

	return BMP280_Check();
}


/**
 * @brief  		Get the version of the BMP280
 * @retval 		BMP280_Error or BMP280_Success
 */
BMP280_Result BMP280_Check(void)
{
	switch (BMP280_GetVersion())
	{
		case BMP280_CHIP_ID1:
			break;
		case BMP280_CHIP_ID2:
			break;
		case BMP280_CHIP_ID3:
			return BMP280_Success;
		default:
			return BMP280_Error;
			break;
	}
	return BMP280_Error;
}


/**
 * @brief  		Write the reset command to the BMP reset register
 */
inline void BMP280_Reset(void)
{
	__BMP280_WriteReg(BMP280_REG_RESET, BMP280_CMD_RESET);
}

/**
 * @brief  		Inline function to get the version from the BMP
 * @retval 		BMP280 version or zero in case of error
 */
inline uint8_t BMP280_GetVersion(void)
{
	return __BMP280_ReadReg(BMP280_REG_ID);
}


/**
 * @brief  		Inline function to get the status from the BMP
 * @retval 		BMP280 status or zero in case of error
 */
inline uint8_t BMP280_GetStatus(void)
{
	return __BMP280_ReadReg(BMP280_REG_STATUS) & BMP280_MASK_STATUS;
}


/**
 * @brief  		Inline function to get the mode from the BMP
 * @retval 		BMP280 power mode or zero in case of error
 */
inline uint8_t BMP280_GetMode(void)
{
	return __BMP280_ReadReg(BMP280_REG_CTRL_MEAS) & BMP280_MASK_MODE;
}


/**
 * @brief  		Set the mode of the BMP280
 * @param 		BMP280 power mode
 */
void BMP280_SetMode(uint8_t mode)
{
	mode &= BMP280_MASK_MODE;
	uint8_t reg = (uint8_t)(__BMP280_ReadReg(BMP280_REG_CTRL_MEAS) & ~BMP280_MASK_MODE);
	__BMP280_WriteReg(BMP280_REG_CTRL_MEAS, reg | mode);
}


/**
 * @brief  		Set the coefficient of the BMP280 IIR filter
 * @param 		BMP280 filter coefficient
 */
void BMP280_SetFilter(uint8_t filter)
{
	filter &= BMP280_MASK_FILTER;
	uint8_t reg = (uint8_t)(__BMP280_ReadReg(BMP280_REG_CONFIG) & ~BMP280_MASK_FILTER);
	__BMP280_WriteReg(BMP280_REG_CONFIG, reg | filter);
}


/**
 * @brief  		Set the inactive duration in normal mode (T_standby)
 * @param 		BMP280 inactive duration
 */
void BMP280_SetStandby(uint8_t tsb)
{
	tsb &= BMP280_MASK_STBY;
	uint8_t reg = (uint8_t)(__BMP280_ReadReg(BMP280_REG_CONFIG) & ~BMP280_MASK_STBY);
	__BMP280_WriteReg(BMP280_REG_CONFIG, reg | tsb);
}


/**
 * @brief  		Set the temperature oversampling setting
 * @param 		oversampling value
 */
void BMP280_SetOSRST(uint8_t osrs)
{
	osrs &= BMP280_MASK_OSRST;
	uint8_t reg = (uint8_t)(__BMP280_ReadReg(BMP280_REG_CTRL_MEAS) & ~BMP280_MASK_OSRST);
	__BMP280_WriteReg(BMP280_REG_CTRL_MEAS, reg | osrs);
}


/**
 * @brief  		Set the pressure oversampling setting
 * @param 		oversampling value
 */
void BMP280_SetOSRSP(uint8_t osrs)
{
	osrs &= BMP280_MASK_OSRSP;
	uint8_t reg = (uint8_t)(__BMP280_ReadReg(BMP280_REG_CTRL_MEAS) & ~BMP280_MASK_OSRSP);
	__BMP280_WriteReg(BMP280_REG_CTRL_MEAS, reg | osrs);
}


/**
 * @brief  		Read the calibration data from the BMP280
 * @retval 		BMP280_Error in case of error on I2C bus, BMP280_Success otherwise
 */
BMP280_Result BMP280_ReadCalibration(void)
{
	// Bulk read from 'calib00' to 'calib25'
	return __BMP280_BulkReadReg(BMP280_REG_CALIB00, (uint8_t *)&BMP280_CalParams, sizeof(BMP280_CalParams));
}


/**
 * @brief  		Read a raw uncompensated pressure value
 * @param	UP	signed 32 bit pointer to store pressure value
 * @note		0x80000 in UP means "no pressure data present"
 * 				which could mean pressure measurement isnt ready or is disabled.
 * @retval 		BMP280_Error in case of error on I2C bus, BMP280_Success otherwise
 */
BMP280_Result BMP280_ReadUP(int32_t *UP)
{
	uint8_t buf[3];

	if (__BMP280_BulkReadReg(BMP280_REG_PRESS_MSB, buf, sizeof(buf)) == BMP280_Success)
	{
		*UP = (int32_t)((buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4));
		return BMP280_Success;
	}

	*UP = BMP280_NO_PRESSURE;
	return BMP280_Error;
}


/**
 * @brief  		Read a raw uncompensated temperature value
 * @param	UT	signed 32 bit pointer to store temperature value
 * @note		0x80000 in UP means "no temperature data present"
 * 				which could mean temperature measurement isnt ready or is disabled.
 * @retval 		BMP280_Error in case of error on I2C bus, BMP280_Success otherwise
 */
BMP280_Result BMP280_ReadUT(int32_t *UT)
{
	uint8_t buf[3];

	if (__BMP280_BulkReadReg(BMP280_REG_TEMP_MSB, buf, sizeof(buf)) == BMP280_Success)
	{
		*UT = (int32_t)((buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4));
		return BMP280_Success;
	}

	*UT = BMP280_NO_TEMPERATURE;
	return BMP280_Error;
}


/**
 * @brief  		Read a raw uncompensated temperature value
 * @param	UT	signed 32 bit pointer to store temperature value
 * @param	UP	signed 32 bit pointer to store pressure value
 * @note		0x80000 in either UT or UP means "no data present"
 * 				which could mean the respective measurement isnt ready or is disabled.
 * @retval 		BMP280_Error in case of error on I2C bus, BMP280_Success otherwise
 */
BMP280_Result BMP280_ReadUTP(int32_t *UT, int32_t *UP)
{
	uint8_t buf[8];

	// Bulk read from 'press_msb' to 'temp_xlsb'
	if (__BMP280_BulkReadReg(BMP280_REG_PRESS_MSB, buf, sizeof(buf)) == BMP280_Success)
	{
		*UP = (int32_t)((buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4));
		*UT = (int32_t)((buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4));
		return BMP280_Success;
	}

	// Default result values
	*UT = BMP280_NO_TEMPERATURE;
	*UP = BMP280_NO_PRESSURE;

	return BMP280_Error;
}


/**
 * @brief  		Get temperature from raw value
 * @note		Temperature resolution is 0.01 *C
 * @note		Sourced from BMP280 datasheet rev 1.19
 * @param   UT	32 bit raw temperature value from BMP280_ReadUT()
 * @retval		temperature in *C: value of 1234 = '12.34*C'
 */
int32_t  BMP280_CalcT(int32_t UT)
{
	// The implementation depends on the library configuration
	#if (BMP280_LIBCONFIG_CALCTYPE != 2)

		// Use integer calculations
		t_fine  = ((((UT >> 3) - ((int32_t)BMP280_CalParams.dig_T1 << 1))) \
				* ((int32_t)BMP280_CalParams.dig_T2)) >> 11;
		t_fine += (((((UT >> 4) - ((int32_t)BMP280_CalParams.dig_T1)) \
				* ((UT >> 4) - ((int32_t)BMP280_CalParams.dig_T1))) >> 12) \
				* ((int32_t)BMP280_CalParams.dig_T3)) >> 14;

		return ((t_fine * 5) + 128) >> 8;

	#else

		// Use floating point calculations
		float v_x1, v_x2;

		v_x1 = (((float)UT) / 16384.0F - ((float)BMP280_CalParams.dig_T1) / 1024.0F) * \
				((float)BMP280_CalParams.dig_T2);
		v_x2 = ((float)UT) / 131072.0F - ((float)BMP280_CalParams.dig_T1) / 8192.0F;
		v_x2 = (v_x2 * v_x2) * ((float)BMP280_CalParams.dig_T3);
		t_fine_f = v_x1 + v_x2;

		return (int32_t)(((v_x1 + v_x2) / 5120.0F) * 100.0F);

	#endif // BMP280_LIBCONFIG_CALCTYPE
}


/**
 * @brief  		Get pressure from raw value
 * @note		Pressure resolution is 0.001 Pa
 * @note		Sourced from BMP280 datasheet rev 1.19
 * @note		BMP_CalcT() should be called before calling this!
 * @param   UP	32 bit raw pressure value from BMP280_ReadUP()
 * @retval		Pressure in mPa: value of 123456789 = '123456.789Pa'
 */
uint32_t BMP280_CalcP(int32_t UP)
{
	// Pressure computation depends on library configuration
	#if (BMP280_LIBCONFIG_CALCTYPE == 0)	// 32 bit calculations
		// 32-bit only calculations
		int32_t v1, v2;
		uint32_t p;

		v1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
		v2 = (((v1 >> 2) * (v1 >> 2)) >> 11 ) * ((int32_t)BMP280_CalParams.dig_P6);
		v2 = v2 + ((v1 * ((int32_t)BMP280_CalParams.dig_P5)) << 1);
		v2 = (v2 >> 2) + (((int32_t)BMP280_CalParams.dig_P4) << 16);
		v1 = (((BMP280_CalParams.dig_P3 * (((v1 >> 2) * (v1 >> 2)) >> 13 )) >> 3) + \
				((((int32_t)BMP280_CalParams.dig_P2) * v1) >> 1)) >> 18;
		v1 = (((32768 + v1)) * ((int32_t)BMP280_CalParams.dig_P1)) >> 15;
		if (v1 == 0) {
			// avoid exception caused by division by zero
			return 0;
		}
		p = (((uint32_t)(((int32_t)1048576) - UP) - (uint32_t)(v2 >> 12))) * 3125U;
		if (p < 0x80000000U) {
			p = (p << 1) / ((uint32_t)v1);
		} else {
			p = (p / (uint32_t)v1) << 1;
		}
		v1 = (((int32_t)BMP280_CalParams.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
		v2 = (((int32_t)(p >> 2)) * ((int32_t)BMP280_CalParams.dig_P8)) >> 13;
		p = (uint32_t)((int32_t)p + ((v1 + v2 + BMP280_CalParams.dig_P7) >> 4));

		return p * 1000U;
	#elif (BMP280_LIBCONFIG_CALCTYPE == 1)
		// 64-bit calculations
		int64_t v1, v2, p;

		v1 = (int64_t)t_fine - 128000;
		v2 = v1 * v1 * (int64_t)BMP280_CalParams.dig_P6;
		v2 = v2 + ((v1 * (int64_t)BMP280_CalParams.dig_P5) << 17);
		v2 = v2 + ((int64_t)BMP280_CalParams.dig_P4 << 35);
		v1 = ((v1 * v1 * (int64_t)BMP280_CalParams.dig_P3) >> 8) + \
				((v1 * (int64_t)BMP280_CalParams.dig_P2) << 12);
		v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)BMP280_CalParams.dig_P1) >> 33;
		if (v1 == 0) {
			// avoid exception caused by division by zero
			return 0;
		}
		p = 1048576 - UP;
		p = (((p << 31) - v2) * 3125) / v1;
		v1 = (((int64_t)BMP280_CalParams.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
		v2 = (((int64_t)BMP280_CalParams.dig_P8) * p) >> 19;
		p = ((p + v1 + v2) >> 8) + ((int64_t)BMP280_CalParams.dig_P7 << 4);

		return (uint32_t)((p * 1000) >> 8);
	#else // BMP280_LIBCONFIG_CALCTYPE == 2
		// Float calculations
		float v_x1, v_x2, p_f;

		v_x1 = (t_fine_f / 2.0F) - 64000.0F;
		v_x2 = v_x1 * v_x1 * ((float)BMP280_CalParams.dig_P6) / 32768.0F;
		v_x2 = v_x2 + v_x1 * ((float)BMP280_CalParams.dig_P5) * 2.0F;
		v_x2 = (v_x2 / 4.0F) + (((float)BMP280_CalParams.dig_P4) * 65536.0F);
		v_x1 = (((float)BMP280_CalParams.dig_P3) * v_x1 * v_x1 / 524288.0F + \
				((float)BMP280_CalParams.dig_P2) * v_x1) / 524288.0F;
		v_x1 = (1.0F + v_x1 / 32768.0F) * ((float)BMP280_CalParams.dig_P1);
		p_f = 1048576.0F - (float)UP;
		if ((uint32_t)v_x1 == 0U) {
			// Avoid exception caused by division by zero
			return 0;
		}
		p_f = (p_f - (v_x2 / 4096.0F)) * 6250.0F / v_x1;
		v_x1 = ((float)BMP280_CalParams.dig_P9) * p_f * p_f / 2147483648.0F;
		v_x2 = p_f * ((float)BMP280_CalParams.dig_P8) / 32768.0F;
		p_f += (v_x1 + v_x2 + ((float)BMP280_CalParams.dig_P7)) / 16.0F;

		return (uint32_t)(p_f * 1000.0F);
	#endif // BMP280_LIBCONFIG_CALCTYPE
}


/**
 * @brief  			Convert Pa to mmHg
 * @param   p_pa	Pressure in mPa
 * @retval  		Pressure in mmHg: 123456 = '123.456mmHg'
 */
uint32_t BMP280_Pa_to_mmHg(uint32_t p_pa)
{
	// Implementation depends on library configuration
	#if (BMP280_LIBCONFIG_CALCTYPE == 0)
		// 32-bit fixed point calculations

		// Convert milli-Pascals to Pascals (Q32.0),
		// multiply by the magic constant (Q0.22),
		// the product is a pressure in mmHg (Q10.22)
		// note: 1Pa = ~0.00750061683mmHg
		uint32_t p = p_pa / 1000U;
		p *= 31460U; // 31460 is the ~0.00750061683 in Q0.22 format

		// (p_mmHg >> 22) -> integer part from Q10.22 value
		// (p_mmHg << 10) >> 18 -> fractional part truncated down to 14 bits
		// (XX * 61039) / 1000000 is rough integer equivalent of float (XX / 16383.0F) * 1000
		return ((p >> 22) * 1000U) + ((((p << 10) >> 18) * 61039U) / 1000000U);
	#elif (BMP280_LIBCONFIG_CALCTYPE == 1)
		// 64-bit integer calculations

		// A bit more precision but noticeable slower on a 32-bit MCU
		uint64_t p_mmHg = ((uint64_t)p_pa * 1000000ULL) / 133322368ULL;
		return (uint32_t)p_mmHg;
	#else // BMP280_LIBCONFIG_CALCTYPE == 2
		// Float calculations
		return (uint32_t)((float)p_pa / 133.322368F);
	#endif // BMP280_LIBCONFIG_CALCTYPE
}


/**
 * @brief  		Write data to a given register in the BMP280
 * @param  reg	Address of the register to write to
 * @param  data	Data to be written
 */
static void __BMP280_WriteReg(uint8_t reg, uint8_t data)
{
	uint8_t buf[2] = {reg, data};
	HAL_I2C_Master_Transmit_DMA(bmp1.hi2c, bmp1.address, buf, 2);
}


/**
 * @brief  		Read data from a given register in the BMP280
 * @param  reg  Address of the register to read from
 * @retval      Data read from the register
 */
static uint8_t __BMP280_ReadReg(uint8_t reg)
{
	uint8_t retval = 0;
	HAL_I2C_Master_Transmit_DMA(bmp1.hi2c, bmp1.address, &reg, 1);
	while (HAL_I2C_GetState(bmp1.hi2c) != HAL_I2C_STATE_READY);
	HAL_I2C_Master_Receive_DMA(bmp1.hi2c, bmp1.address, &retval, 1);
	return retval;
}


/**
 * @brief  		Read a sequential series of registers from the BMP280
 * @param  reg  Address of the first register
 * @param  buf  Buffer to store read registers
 * @param  size Number of registers to be read
 * @retval      BMP280_Error or BMP280_Success
 */
static BMP280_Result __BMP280_BulkReadReg(uint8_t reg, uint8_t *buf, uint32_t size)
{
	if (HAL_I2C_Master_Transmit_DMA(bmp1.hi2c, bmp1.address, &reg, 1) == HAL_OK)
	{
		while (HAL_I2C_GetState(bmp1.hi2c) != HAL_I2C_STATE_READY);
		if (HAL_I2C_Master_Receive_DMA(bmp1.hi2c, bmp1.address, buf, size) == HAL_OK)
		{
			return BMP280_Success;
		}
	}

	return BMP280_Error;
}


// The following functions are only implemented when the library is configured
// for floating point functions, see the library header file
#if (BMP280_LIBCONFIG_FLOATS)


/**
 * @brief  		Calculate temperature from a raw value
 * @note		Procedure from BMP280 datasheet rev 1.19
 * @note		Temperature resolution is 0.01*C float
 * @param  UT   32 bit raw temperature value
 * @retval      Temperature in degrees celsius
 */
float BMP280_FloatCalcT(int32_t UT)
{
	float v_x1, v_x2;

	v_x1 = (((float)UT) / 16384.0F - ((float)BMP280_CalParams.dig_T1) / 1024.0F) * \
			((float)BMP280_CalParams.dig_T2);
	v_x2 = ((float)UT) / 131072.0F - ((float)BMP280_CalParams.dig_T1) / 8192.0F;
	v_x2 = (v_x2 * v_x2) * ((float)BMP280_CalParams.dig_T3);
	t_fine_f = v_x1 + v_x2;

	return (v_x1 + v_x2) / 5120.0F;
}


/**
 * @brief  		Calculate pressure from a raw value
 * @note		Procedure from BMP280 datasheet rev 1.19
 * @note		Pressure resolution is 0.001Pa float
 * @note		BMP280_CalcT or BMP280_FloatCalcT must be called first!
 * @param  UP   32 bit raw pressure value
 * @retval      Pressure in Pa
 */
float BMP280_FloatCalcP(uint32_t UP)
{
	float v_x1, v_x2, p;

	v_x1 = (t_fine_f / 2.0F) - 64000.0F;
	v_x2 = v_x1 * v_x1 * ((float)BMP280_CalParams.dig_P6) / 32768.0F;
	v_x2 = v_x2 + v_x1 * ((float)BMP280_CalParams.dig_P5) * 2.0F;
	v_x2 = (v_x2 / 4.0F) + (((float)BMP280_CalParams.dig_P4) * 65536.0F);
	v_x1 = (((float)BMP280_CalParams.dig_P3) * v_x1 * v_x1 / 524288.0F + \
			((float)BMP280_CalParams.dig_P2) * v_x1) / 524288.0F;
	v_x1 = (1.0F + v_x1 / 32768.0F) * ((float)BMP280_CalParams.dig_P1);
	p = 1048576.0F - (float)UP;
	if ((uint32_t)v_x1 == 0U) {
		// Avoid exception caused by division by zero
		return 0.0F;
	}
	p = (p - (v_x2 / 4096.0F)) * 6250.0F / v_x1;
	v_x1 = ((float)BMP280_CalParams.dig_P9) * p * p / 2147483648.0F;
	v_x2 = p * ((float)BMP280_CalParams.dig_P8) / 32768.0F;
	p += (v_x1 + v_x2 + ((float)BMP280_CalParams.dig_P7)) / 16.0F;

	return p;
}


/**
 * @brief  			Convert pressure in Pa to mmHg (float)
 * @note			Float calculations with float result
 * @param  p_pa   	Pressure in Pa
 * @retval      	Pressure in mmHg
 */
float BMP280_Float_Pa_to_mmHg(float p_pa)
{
	return p_pa / 133.322368F;
}


#endif //BMP280_LIBCONFIG_FLOATS
