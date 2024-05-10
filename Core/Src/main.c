/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"
#include "bmp280.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// As a style guide, all private vars here should be prefaced with gpv_ for Global Private Variable

int gpv_UART2_DMA_RX_Complete = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

// This will redirect printf() to our UART instance, huart2 in this case
// This, along with #include <stdio.h> up top, allows us to easily print to UART
// from https://forum.digikey.com/t/easily-use-printf-on-stm32/20157
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  /* A note on when to use blocking or nonblocking UART RX
  //when we want data received,
  // just define a buffer in the size of data we want to receive
  // then call a dma receive for that number of bytes
  // and wait for the finished flag to be set
  // the finish flag gets set inside the tx complete callback
  // This strategy is good for getting data for changing settings for example
  //printf("give me 4 bytes of data.\r\n");
  //HAL_UART_Receive_DMA(&huart2, buf, 4);
  //HAL_UART_Receive(&huart2, buf, 4, HAL_MAX_DELAY);
  //int number = atoi(buf);
  //while (g_UART2RXCmplt == 0);
  //g_UART2RXCmplt = 0;
  // we should now have the 4 bytes in buf

  // If we were to use DMA for like the GPS,
  // we would set up that DMA to be cyclic, and make the buffer just as big
  // as the packets that come from the GPS
  // so that we can just grab the data from the buffer whenever we want to use it
  // so that our program loop isnt getting interrupted by the GPS sending new data.
  // whereas for getting user input (like above) we could just use blocking statements
   */

  // All UART RXs in this code expect a single "\n" control character
  // from the serial terminal, so give a warning to anyone trying to communicate
  printf("Send only newlines, not also carriage returns.\r\n");

  /* Add I2C and scan the bus for all addresses that respond */
  // TODO: Replace generic i2c scanner with a scanner that specifically looks for our modules only
  // 20x4 LCD: 0x27, TODO: get addresses for other modules
  printf("Scanning I2C bus.\r\n");
  int ret = 0;
  char i2cBuffer[5] = {0};
  // Scan all 128 available i2c addresses
  for (uint8_t testAddr = 1; testAddr < 128; testAddr++)
  {
	  ret = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(testAddr<<1), 3, 5);
	  if (ret == HAL_OK) // If an ACK was received at address testAddr
	  {
		  sprintf(i2cBuffer, "0x%X", testAddr);
		  printf("Device at: ");
		  printf(i2cBuffer);
		  printf("\r\n");
	  }
  }
  printf("Done.\r\n");

  /* Interface with the BMP280 */
  uint8_t retval = 0;
  retval = BMP280_Init(&hi2c1, (uint8_t)0x76);
  static uint32_t i, j, k;

  // RAW temperature and pressure values
  int32_t UT, UP;

  // Human-readable temperature and pressure
  int32_t temperature;
  uint32_t pressure;

  while (1) {
  		// Check status of chip
  		i = BMP280_GetStatus();
  		printf("Status: [%02X] %s %s\r\n",
  				i,
  				(i & BMP280_STATUS_MEASURING) ? "MEASURING" : "READY",
  				(i & BMP280_STATUS_IM_UPDATE) ? "NVM_UPDATE" : "NVM_READY"
  			);

  		// Get raw readings from the chip
  		i = BMP280_ReadUTP(&UT, &UP);
  		printf("Raw: T=0x%05X P=0x%05X [R=%s]\r\n",
  				UT,
  				UP,
  				i ? "OK" : "ERROR"
  			);

  		if (UT == 0x80000) {
  			// Either temperature measurement is configured as 'skip' or first conversion is not completed yet
  			printf("Temperature: no data\r\n");
  			// There is no sense to calculate pressure without temperature readings
  			printf("Pressure: no temperature readings\r\n");
  		} else {
  			// Temperature (must be calculated first)
  			//  UT = 0x84D3C; // test raw value: 25.90C
  			temperature = BMP280_CalcT((int32_t)0x84DC3);
  			int t = temperature / 100.0f;
  			printf("Temperature: %.2iC\r\n", t);

  			if (UP == 0x80000) {
  				// Either pressure measurement is configured as 'skip' or first conversion is not completed yet
  				printf("Pressure: no data\r\n");
  			} else {
  				// Pressure
  				pressure = BMP280_CalcP(UP);
  				printf("Pressure: %.3uPa [%.3uhPa]\r\n",
  						pressure,
  						pressure / 100
  					);

  				printf("mmHg: %.3u\r\n",
  						BMP280_Pa_to_mmHg(pressure)
  					);

  			}
  		}

  		printf("------------------------\r\n");

  		// Invert state of the Nucleo LED
  		//GPIO_PIN_INVERT(GPIOA, GPIO_PIN_5);

  		HAL_Delay(1000);
  	}









  /* Display a splash screen on the LCD */
  // LCD_Init() must be called AFTER I2C and TIM1 Inits!
  //LCD_Init(&hi2c1, (uint8_t)0x27, 4, 20);
  //LCD_Backlight(LCD_BIT_BACKLIGHT_ON);
  //LCD_PrintString((uint8_t*)"Hello,", 6);
  //LCD_SetCursorPosition(0, 1);
  //LCD_PrintString((uint8_t*)"World!", 6);
  //HAL_Delay(5000);
  //LCD_Command(LCD_CLEAR, LCD_PARAM_SET);	// note LCD_CLEAR sets cursor to 0,0 too
  //LCD_PrintString((uint8_t*)"TEST", 4);

  /* Get and set the RTC module */
  // Setting RTC is done following the procedure in UM1725 section 57.2
  // Enable backup domain access for the RTC first (according to documentation UM1725 57.2.3)
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);
  __HAL_RCC_RTC_ENABLE();

  char timeString[25];
  char dateString[25];
  uint8_t uartBuffer[10] = {0};
  RTC_DateTypeDef dateRTC;
  RTC_TimeTypeDef timeRTC;

  printf("Current date and time: ");
  HAL_RTC_GetTime(&hrtc, &timeRTC, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &dateRTC, RTC_FORMAT_BIN);

  sprintf(timeString, "%02d:%02d:%02d", timeRTC.Hours, timeRTC.Minutes, timeRTC.Seconds);
  printf(timeString);
  printf(" ");
  sprintf(dateString, "%02d/%02d/%02d", dateRTC.Month, dateRTC.Date, dateRTC.Year);
  printf(dateString);

  printf("\r\nSet the time? (y/n)\r\n");
  HAL_UART_Receive(&huart2, uartBuffer, 2, HAL_MAX_DELAY);

  if (uartBuffer[0] == 'y' || uartBuffer[0] == 'Y')
  {
	  // ask the user to set the time and date
	  printf("Enter the time in 24hr format (HH:MM)\r\n");
	  HAL_UART_Receive(&huart2, uartBuffer, 6, HAL_MAX_DELAY);

	  char charHrs[2] = {uartBuffer[0], uartBuffer[1]};
	  char charMins[2] = {uartBuffer[3], uartBuffer[4]};
	  timeRTC.Hours = atoi(charHrs);
	  timeRTC.Minutes = atoi(charMins);

	  uint8_t dst = 0;
	  printf("Daylight savings time? (y/n)\r\n");
	  HAL_UART_Receive(&huart2, uartBuffer, 2, HAL_MAX_DELAY);

	  if (uartBuffer[0] == 'y' || uartBuffer[0] == 'Y') dst = RTC_DAYLIGHTSAVING_ADD1H;
	  else dst = RTC_DAYLIGHTSAVING_NONE;

	  // The daylight savings and store operation interfaces have been
	  // deprecated but we will worry about that later
	  // TODO: Update interface for RTC daylight savings time
	  timeRTC.Seconds = 0;
	  timeRTC.TimeFormat = RTC_HOURFORMAT12_PM;
	  timeRTC.DayLightSaving = dst;
	  timeRTC.StoreOperation = RTC_STOREOPERATION_RESET;

	  if (HAL_RTC_SetTime(&hrtc, &timeRTC, RTC_FORMAT_BIN) != HAL_OK)
	  {
		printf("INVALID TIME.\r\n");
		Error_Handler();
	  }

	  printf("Enter the date (MM-DD-YY)\r\n");
	  HAL_UART_Receive(&huart2, uartBuffer, 8, HAL_MAX_DELAY);

	  char charMM[2] = {uartBuffer[0], uartBuffer[1]};
	  char charDD[2] = {uartBuffer[3], uartBuffer[4]};
	  char charYY[2] = {uartBuffer[6], uartBuffer[7]};
	  dateRTC.Month = atoi(charMM);
	  dateRTC.Date = atoi(charDD);
	  dateRTC.Year = atoi(charYY);

	  if (HAL_RTC_SetDate(&hrtc, &dateRTC, RTC_FORMAT_BIN) != HAL_OK)
	  {
		  printf("INVALID DATE.\r\n");
		  Error_Handler();
	  }

	  // Update the backup register too as part of setting RTC
	  // from https://controllerstech.com/internal-rtc-in-stm32/
	  // The hex number was chosen randomly
	  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);

	  // We'll confirm the new date and time by reading it out
	  printf("Current date and time: ");
	  HAL_RTC_GetTime(&hrtc, &timeRTC, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &dateRTC, RTC_FORMAT_BIN);

	  sprintf(timeString, "%02d:%02d:%02d", timeRTC.Hours, timeRTC.Minutes, timeRTC.Seconds);
	  printf(timeString);
	  printf(" ");
	  sprintf(dateString, "%02d/%02d/%02d", dateRTC.Month, dateRTC.Date, dateRTC.Year);
	  printf(dateString);
	  printf("\r\n");
  }
  else printf("Skipping time set.\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // The goal for now is to transmit the now set time from the RTC, once a second.
	  RTC_DateTypeDef getDate;
	  RTC_TimeTypeDef getTime;
	  HAL_RTC_GetTime(&hrtc, &getTime, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &getDate, RTC_FORMAT_BIN);
	  sprintf(timeString, "%02d:%02d:%02d", getTime.Hours, getTime.Minutes, getTime.Seconds);
	  printf(timeString);
	  printf(" ");
	  sprintf(dateString, "%02d/%02d/%02d", getDate.Month, getDate.Date, getDate.Year);
	  printf(dateString);
	  printf("\r\n");

	  // For now we just print the RTC time to the LCD
	  //LCD_SetCursorPosition(0,2);
	  //LCD_PrintString((uint8_t*)timeString, 8);
	  //LCD_SetCursorPosition(0,3);
	  //LCD_PrintString((uint8_t*)dateString, 8);

	  HAL_Delay(1000);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief UART 2 DMA RX complete callback
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	gpv_UART2_DMA_RX_Complete = 1;
	// this function from https://deepbluembedded.com/how-to-receive-uart-serial-data-with-stm32-dma-interrupt-polling/
    // This was commented out because we are currently using a circular DMA buffer
    // which runs continuously, so there is no need to restart the DMA RX process after one is completed
    //HAL_UART_Receive_DMA(&huart2, UART2_rxBuffer, RXBUFSIZE);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
	  printf("Error encountered.");
	  while (1);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
