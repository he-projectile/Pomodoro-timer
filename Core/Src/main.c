/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_hid.h"
#include "MICROS.h"
#include "ssd1306.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
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
extern USBD_HandleTypeDef  hUsbDeviceFS;
uint8_t dataToSend[5];
uint8_t btn_stat=0;

extern USBD_HandleTypeDef hUsbDeviceFS;

typedef struct
{
	uint8_t MODIFIER;
	uint8_t RESERVED;
	uint8_t KEYCODE1;
	uint8_t KEYCODE2;
	uint8_t KEYCODE3;
	uint8_t KEYCODE4;
	uint8_t KEYCODE5;
	uint8_t KEYCODE6;
}subKeyBoard;

subKeyBoard keyBoardHIDsub = {0,0,0,0,0,0,0,0};

struct timerEntity timeFromStart;

enum POMODORO_INTERVALS{
	POMODORO_PAUSE = 0,
	POMODORO_REST,
	POMODORO_WORK	
} pomodoroInterval = 0;

enum POMODORO_CHANGE_REQUESTS{
	POMODORO_CHANGE_REQUEST_NONE = 0,
	POMODORO_CHANGE_REQUEST_BTN,
	POMODORO_CHANGE_REQUEST_TIMER	
} pomodoroIntervalChangeRequest = 0;

uint8_t pomodoroIntervalChangeRequestBtn = 0;
uint8_t pomodoroIntervalIsChanged = 0;
struct timerEntity pomodoroTimer;
uint16_t intervalCnt = 0;


uint16_t ledBlinkingPeriodInitialMs = 5000;
struct timerEntity ledBlinkingTimer;

struct timerEntity oledUpdate;
uint8_t oledIsIntervalChanged = 0;

const uint16_t wortDurationS = 25*60, restDurationS = 5*60; 
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void pressWinD(void);
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
  MX_USB_DEVICE_Init();
  MX_TIM5_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
	systemTimerInit(&htim5, 84000000);
	timerStart(&timeFromStart, 0, 0);
	
  ssd1306_Init(&hi2c1);

  ssd1306_Fill(Black);
  ssd1306_SetCursor(5, 10);
  ssd1306_WriteString("Pomodoro", Font_11x18, White);
	ssd1306_SetCursor(5, 32);
  ssd1306_WriteString("timer", Font_11x18, White);
  ssd1306_UpdateScreen(&hi2c1);	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	
  while (1)
  {
		static GPIO_PinState pinState = 1;
		
		if ( HAL_GPIO_ReadPin(PCB_BTN_GPIO_Port, PCB_BTN_Pin) != pinState){
			pinState = HAL_GPIO_ReadPin(PCB_BTN_GPIO_Port, PCB_BTN_Pin);
			if (pinState == GPIO_PIN_RESET)
				pomodoroIntervalChangeRequest = POMODORO_CHANGE_REQUEST_BTN;		
		}

		if (isTimerFinished(&pomodoroTimer) == 1 && pomodoroInterval != POMODORO_PAUSE){
			pomodoroIntervalChangeRequest = POMODORO_CHANGE_REQUEST_TIMER;					
		}		
		
		if( pomodoroIntervalChangeRequest != POMODORO_CHANGE_REQUEST_NONE) {	
			if ( pomodoroIntervalChangeRequest == POMODORO_CHANGE_REQUEST_BTN ){
				pomodoroIntervalIsChanged = 1;
				intervalCnt = 0;
				if( pomodoroInterval == POMODORO_PAUSE )
					pomodoroInterval = POMODORO_WORK;
				else
					pomodoroInterval = POMODORO_PAUSE;
			}
			
			if ( pomodoroIntervalChangeRequest == POMODORO_CHANGE_REQUEST_TIMER ){
				pomodoroIntervalIsChanged = 1;
				switch ( (uint8_t) pomodoroInterval){
					case POMODORO_WORK: pomodoroInterval = POMODORO_REST; break;
					case POMODORO_REST: pomodoroInterval = POMODORO_WORK; break;
				}
			}
			pomodoroIntervalChangeRequest = POMODORO_CHANGE_REQUEST_NONE;
			oledIsIntervalChanged = 1;
		}
		
		
		if (pomodoroIntervalIsChanged == 1){
			pomodoroIntervalIsChanged = 0;
			if(pomodoroInterval == POMODORO_WORK){
				timerStart( &pomodoroTimer, wortDurationS, 0);
				timerStart( &ledBlinkingTimer, 0, (uint32_t) ledBlinkingPeriodInitialMs * 1000);
				pressWinD();
				intervalCnt ++;
			}
			if(pomodoroInterval == POMODORO_REST){
				timerStart( &pomodoroTimer, restDurationS, 0);
				pressWinD();
			}			
		}
		
		
		if ( pomodoroInterval == POMODORO_REST ){
			HAL_GPIO_WritePin(PCB_LED_GPIO_Port, PCB_LED_Pin, GPIO_PIN_RESET);	
		}
		
		if ( pomodoroInterval == POMODORO_WORK ){
			if( isTimerFinished( &ledBlinkingTimer) ){
				HAL_GPIO_TogglePin(PCB_LED_GPIO_Port, PCB_LED_Pin);
				timerStart( &ledBlinkingTimer, 0, whatTimeIsToFinish(&pomodoroTimer)*(ledBlinkingPeriodInitialMs/10*9)/(wortDurationS*1000) + ledBlinkingPeriodInitialMs/10 );
			}
		}
		
		if ( pomodoroInterval == POMODORO_PAUSE ){
			HAL_GPIO_WritePin(PCB_LED_GPIO_Port, PCB_LED_Pin, GPIO_PIN_SET);	
		}
		
		if (oledIsIntervalChanged){
			oledIsIntervalChanged = 0;
			char str[15];			
			switch ((uint8_t) pomodoroInterval){
				case POMODORO_WORK:
					ssd1306_SetCursor(5, 10);
					ssd1306_WriteString("WORK HARD!   ", Font_11x18, White);
					ssd1306_UpdateScreen(&hi2c1);

					sprintf(str, "%03d", intervalCnt);
					ssd1306_SetCursor(90, 32);
					ssd1306_WriteString(str, Font_11x18, White);
					ssd1306_UpdateScreen(&hi2c1);							
					break;
				case POMODORO_REST:
					ssd1306_SetCursor(5, 10);
					ssd1306_WriteString("CHILL HARD!   ", Font_11x18, White);
					ssd1306_UpdateScreen(&hi2c1);
					break;
				case POMODORO_PAUSE:
					ssd1306_Fill(Black);
					ssd1306_SetCursor(5, 10);
					ssd1306_WriteString("PAUSED   ", Font_11x18, White);
					ssd1306_UpdateScreen(&hi2c1);						
					break;				
			}		
		}
		
		if( isTimerFinished(&oledUpdate) ){
			char str[15];			
			if (pomodoroInterval != POMODORO_PAUSE){
				uint16_t secondsToFinish = truncf((float)whatTimeIsToFinish(&pomodoroTimer)/1000000);
				ssd1306_SetCursor(5, 32);
				sprintf(str, "%02d:%02d", secondsToFinish/60, secondsToFinish%60);
				ssd1306_WriteString(str, Font_11x18, White);			
				timerStart(&oledUpdate, 0, 100000);
				ssd1306_UpdateScreen(&hi2c1);	
			}
		}				
		
/*		
		if(pinState == GPIO_PIN_RESET){
			HAL_GPIO_WritePin(PCB_LED_GPIO_Port, PCB_LED_Pin, GPIO_PIN_RESET);	
			keyBoardHIDsub.MODIFIER=0x08;  // To press win key
			keyBoardHIDsub.KEYCODE1=7;
			keyBoardHIDsub.KEYCODE2=0;  // Press d key
			USBD_HID_SendReport(&hUsbDeviceFS,&keyBoardHIDsub,sizeof(keyBoardHIDsub));
			HAL_Delay(50);
			keyBoardHIDsub.MODIFIER=0x00;  // To release win key
			keyBoardHIDsub.KEYCODE1=0x00;  // Release d key	
			keyBoardHIDsub.KEYCODE2=0x00;
			USBD_HID_SendReport(&hUsbDeviceFS,&keyBoardHIDsub,sizeof(keyBoardHIDsub));
			HAL_GPIO_WritePin(PCB_LED_GPIO_Port, PCB_LED_Pin, GPIO_PIN_SET);				
		}		
*/		
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
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
void pressWinD(void){
				keyBoardHIDsub.MODIFIER=0x08;  // To press win key
				keyBoardHIDsub.KEYCODE1=7;
				keyBoardHIDsub.KEYCODE2=0;  // Press d key
				USBD_HID_SendReport(&hUsbDeviceFS,&keyBoardHIDsub,sizeof(keyBoardHIDsub));
				HAL_Delay(50);
				keyBoardHIDsub.MODIFIER=0x00;  // To release win key
				keyBoardHIDsub.KEYCODE1=0x00;  // Release d key	
				keyBoardHIDsub.KEYCODE2=0x00;
				USBD_HID_SendReport(&hUsbDeviceFS,&keyBoardHIDsub,sizeof(keyBoardHIDsub));	
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
