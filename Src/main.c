/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define START_CHAR '#'
#define END_CHAR '$'
#define MAX_CHARS 35

#define REPORT_PERIOD_MS 2000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t frameActive = 0;
static uint8_t frameCount  = 0;
static uint8_t frameLower  = 0;
static uint8_t frameUpper  = 0;
static char frameBuf[MAX_CHARS + 2];

static volatile uint8_t resultReady = 0;
static uint8_t resultLower = 0;
static uint8_t resultUpper = 0;
static char resultFrame[MAX_CHARS + 2];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void proccesDmaData(const uint8_t* data, uint16_t len);
uint16_t USART2_GetDmaRxOccupied(void);
static void sendValidString(void);
static void sendBufferStatus(void);
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
  
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* System interrupt init*/

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
  /* USER CODE BEGIN 2 */
  USART2_RegisterCallback(proccesDmaData);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
#if POLLING
	//Polling for new data, no interrupts
	USART2_CheckDmaReception();

#endif

	sendValidString();
	sendBufferStatus();

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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  Error_Handler();  
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
    
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  
  }
  LL_Init1msTick(8000000);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
  LL_SetSystemCoreClock(8000000);
}

/* USER CODE BEGIN 4 */
void proccesDmaData(const uint8_t* data, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++)
	{
		char c = (char)data[i];

		if (c == START_CHAR)
		{
			frameActive = 1;
			frameCount  = 0;
			frameLower  = 0;
			frameUpper  = 0;
			frameBuf[0] = START_CHAR;
			continue;
		}

		if (!frameActive)
		{
			continue;
		}

		frameBuf[1 + frameCount] = c;
		frameCount++;

		if (c == END_CHAR)
		{
			frameBuf[1 + frameCount] = '\0';

			strcpy(resultFrame, frameBuf);
			resultLower = frameLower;
			resultUpper = frameUpper;
			resultReady = 1;

			frameActive = 0;
			continue;
		}

		if (c >= 'a' && c <= 'z')
		{
			frameLower++;
		}
		else if (c >= 'A' && c <= 'Z')
		{
			frameUpper++;
		}

		if (frameCount >= MAX_CHARS)
		{
			frameActive = 0;
		}
	}
}


static void sendValidString(void)
{
	if (!resultReady) return;

	char    frame[MAX_CHARS + 2];
	uint8_t lower, upper;
	char    msg[100];

	__disable_irq();
	strcpy(frame, resultFrame);
	lower = resultLower;
	upper = resultUpper;
	resultReady = 0;
	__enable_irq();


	int n = sprintf(msg, "Valid string: %.*s, lower-case: %u, upper-case: %u\r\n",
	                (int)strlen(frame) - 2, frame + 1, lower, upper);
	USART2_PutBuffer((uint8_t *)msg, (uint8_t)n);
}


static void sendBufferStatus(void)
{
	static uint32_t msCounter = 0;

	if (LL_SYSTICK_IsActiveCounterFlag())
	{
		msCounter++;
	}

	if (msCounter < REPORT_PERIOD_MS) return;
	msCounter = 0;

	uint16_t occupied = USART2_GetDmaRxOccupied();
	uint32_t load10   = ((uint32_t)occupied * 1000U) / DMA_USART2_BUFFER_SIZE;
	char     msg[128];

	int n = sprintf(msg, "Buffer capacity: %u bytes, occupied memory: %u bytes, load [in %%]: %lu.%lu%%\r\n",
	                (unsigned)DMA_USART2_BUFFER_SIZE, occupied,
	                (unsigned long)(load10 / 10), (unsigned long)(load10 % 10));
	USART2_PutBuffer((uint8_t *)msg, (uint8_t)n);
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
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
