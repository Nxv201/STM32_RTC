/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c-lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define DS1307_I2C_ADDR 		0x68
#define DS1307_REG_SECOND 	0x00
#define DS1307_REG_MINUTE 	0x01
#define DS1307_REG_HOUR  		0x02
#define DS1307_REG_DOW    	0x03
#define DS1307_REG_DATE   	0x04
#define DS1307_REG_MONTH  	0x05
#define DS1307_REG_YEAR   	0x06
#define DS1307_REG_CONTROL 	0x07
#define DS1307_REG_UTC_HR		0x08
#define DS1307_REG_UTC_MIN	0x09
#define DS1307_REG_CENT    	0x10
#define DS1307_REG_RAM   		0x11
#define DS1307_TIMEOUT			1000

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t dow;
    uint8_t date;
    uint8_t month;
    uint16_t year;
}DS1307_STRUCT;
DS1307_STRUCT    ds1307;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */
char time_buff[10];
char date_buff[20];
const char *DAYS_OF_WEEK[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
uint8_t DS1307_DecodeBCD(uint8_t bin);
uint8_t DS1307_EncodeBCD(uint8_t dec);
void DS1307_SetClockHalt(uint8_t halt);
void DS1307_SetRegByte(uint8_t regAddr, uint8_t val);
void DS1307_SetTimeZone(int8_t hr, uint8_t min);
uint8_t DS1307_GetClockHalt(void);
uint8_t DS1307_GetRegByte(uint8_t regAddr);
void DS1307_config(void);
void DS1307_gettime(void);
void DS1307_settime(uint8_t date, uint8_t month, uint16_t year, uint8_t dayOfWeek,uint8_t hour_24mode,uint8_t min, uint8_t sec);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void DS1307_config(void){
    DS1307_SetClockHalt(0);
}

uint8_t DS1307_GetClockHalt(void) {
	return (DS1307_GetRegByte(DS1307_REG_SECOND) & 0x80) >> 7;
}
void DS1307_SetClockHalt(uint8_t halt) {
	uint8_t ch = (halt ? 1 << 7 : 0);
	DS1307_SetRegByte(DS1307_REG_SECOND, ch | (DS1307_GetRegByte(DS1307_REG_SECOND) & 0x7f));
}

void DS1307_SetTimeZone(int8_t hr, uint8_t min) {
	DS1307_SetRegByte(DS1307_REG_UTC_HR, hr);
	DS1307_SetRegByte(DS1307_REG_UTC_MIN, min);
}

void DS1307_SetRegByte(uint8_t regAddr, uint8_t val) {
	uint8_t bytes[2] = { regAddr, val };
	HAL_I2C_Master_Transmit(&hi2c1, DS1307_I2C_ADDR << 1, bytes, 2, DS1307_TIMEOUT);
}

uint8_t DS1307_GetRegByte(uint8_t regAddr) {
	uint8_t val;
	HAL_I2C_Master_Transmit(&hi2c1, DS1307_I2C_ADDR << 1, &regAddr, 1, DS1307_TIMEOUT);
	HAL_I2C_Master_Receive(&hi2c1, DS1307_I2C_ADDR << 1, &val, 1, DS1307_TIMEOUT);
	return val;
}

uint8_t DS1307_DecodeBCD(uint8_t bin) {
	return (((bin & 0xf0) >> 4) * 10) + (bin & 0x0f);
}

uint8_t DS1307_EncodeBCD(uint8_t dec) {
	return (dec % 10 + ((dec / 10) << 4));
}


//(DD/MM/YY DOW HH:MM:SS)
void DS1307_settime(uint8_t date, uint8_t month, uint16_t year, uint8_t dayOfWeek,uint8_t hour_24mode,uint8_t min, uint8_t sec){
		DS1307_SetTimeZone(+7, 00);
		DS1307_SetRegByte(DS1307_REG_SECOND, DS1307_EncodeBCD(sec | DS1307_GetClockHalt())); // sec, 0 to 59
    DS1307_SetRegByte(DS1307_REG_MINUTE, DS1307_EncodeBCD(min)); // min , 0 to 59
    DS1307_SetRegByte(DS1307_REG_HOUR, DS1307_EncodeBCD(hour_24mode & 0x3f));//hour_24mode Hour in 24h format, 0 to 23.
    DS1307_SetRegByte(DS1307_REG_DOW, DS1307_EncodeBCD(dayOfWeek));//dayOfWeek, 0 to 6.
    DS1307_SetRegByte(DS1307_REG_DATE, DS1307_EncodeBCD(date));//date Day of month, 1 to 31.
    DS1307_SetRegByte(DS1307_REG_MONTH, DS1307_EncodeBCD(month));//month , 1 to 12.
		// set Year
    DS1307_SetRegByte(DS1307_REG_CENT, year / 100); 
    DS1307_SetRegByte(DS1307_REG_YEAR, DS1307_EncodeBCD(year % 100));//2000 to 2099.
}
void DS1307_gettime(){
    ds1307.sec=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_SECOND) & 0x7f);
    ds1307.min=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_MINUTE));
    ds1307.hour=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_HOUR) & 0x3f);
    ds1307.dow=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_DOW));
    ds1307.date=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_DATE));
    ds1307.month=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_MONTH));
    uint16_t cen = DS1307_GetRegByte(DS1307_REG_CENT) * 100;
    ds1307.year=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_YEAR)) + cen;
		/* Display time Format: hh:mm:ss */ 
		 sprintf((char*)time_buff,"%02d:%02d:%02d",ds1307.hour, ds1307.min, ds1307.sec); 
		/* Display date Format: dd-mm-yy */ 
		 sprintf((char*)date_buff,"%02d-%02d-%04d  %s",ds1307.date, ds1307.month, ds1307.year, DAYS_OF_WEEK[ds1307.dow]); 		

}

void display_time (void) 
{ 
 lcd_send_cmd (0x80); // send cursor to 0,0 
 lcd_send_string (time_buff);
 lcd_send_cmd (0xc0); // send cursor to 1,0 
 lcd_send_string (date_buff);
}
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
  MX_I2C2_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
	lcd_init();
	DS1307_config();
//	DS1307_settime(4, 10, 2023, 3, 19, 21, 00);
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		DS1307_gettime();
		display_time();
    HAL_Delay(500);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

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
