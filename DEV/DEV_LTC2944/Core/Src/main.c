/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DEV_LTC2944.h"
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
GAS_Gauge_Data_t Batt_Data;
float CurrentFiltered = 0 ;
float JouleEnergy     = 0 ;

uint32_t TimeSimulation = 10000, ReadQnt, ReadCounter = 0 ,ConvDelay = 2;
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */
	static enum { STEP_START_MANUAL_CONVERTION, STEP_CAPTURE_DATA, STEP_SLEEP, STEP_FAULT } GAS_GAUGE_STATE_MACHINE = STEP_START_MANUAL_CONVERTION;
	Dev_Gas_Gauge_Configuration_t Config_Parameters = {
			.ADC_Mode = Manual_Mode,
			.Alert_Pin_Mode = ALCC_Disabled,
			.Prescaler_Factor = Factor_16,
			.Shutdown_Status = false,
			.RSense_Value	= 0.1
	};


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
	//MX_I2C2_Init();
	/* USER CODE BEGIN 2 */
	Dev_Gas_Gauge_IntHwInit( 0 );

	Dev_Gas_Gauge_ExtDevConfig( 0, Config_Parameters);
	ReadQnt = TimeSimulation/(50+ConvDelay);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	Dev_Gas_Gauge_Set_Battery_Capacity(0, 0);

	uint32_t st = HAL_GetTick();


	while (1)
	{

		if(HAL_GetTick() - st >=TimeSimulation)
			st = 0;


		switch( GAS_GAUGE_STATE_MACHINE )
		{
		case STEP_START_MANUAL_CONVERTION:
			if( Dev_Gas_Gauge_Manual_Convert(0) == ANSWERED_REQUEST ) // Inicia a conversão ADC manual. Leva Aproximadamente 50ms
				GAS_GAUGE_STATE_MACHINE = STEP_CAPTURE_DATA;
			else
			{
				GAS_GAUGE_STATE_MACHINE = STEP_FAULT;
				/*
				 * Possibilidade 1: E Provavel que o dispositivo nao foi encontrado na rede ( Desconectado?!?! )
				 * Possibilidade 2: Ultrapassou o DEV_GASG_MAX_MANUAL_CONVERTION_RETRY (Maximo de tentativas)
				 *
				 */
			}
			break;
		case STEP_CAPTURE_DATA:
			if( Dev_Gas_Gauge_Get_Battery_Data( 0, &Batt_Data) == ANSWERED_REQUEST ) // Captura os dados convertidos
			{
				GAS_GAUGE_STATE_MACHINE = STEP_SLEEP;

				CurrentFiltered = CurrentFiltered*0.8 + Batt_Data.Current*0.2;
                                                                                                       //(W)*(s) = joule
				JouleEnergy = Batt_Data.ACC_Charge*3600*Batt_Data.Voltage; // conversao mAh -> j ( (A)*U *(3600) -> W*s  )

				if( ReadCounter >=  ReadQnt)
				{
					JouleEnergy = 0;
				}
			}
			else
			{
				GAS_GAUGE_STATE_MACHINE = STEP_FAULT;
				/*
				 * Possibilidade 1: E Provavel que o dispositivo nao foi encontrado na rede ( Desconectado?!?! )
				 * Possibilidade 2: Ponteiro de saida dos dados e NULL???
				 *
				 */
			}
			break;
		case STEP_SLEEP:
			HAL_Delay(ConvDelay); // Aguarda nova conversão
			GAS_GAUGE_STATE_MACHINE = STEP_START_MANUAL_CONVERTION;
			ReadCounter++;


			break;

		case STEP_FAULT:
			/*
			 * O codigo preso aqui significa que ocorreu algum problema com o DEVICE ou com o BARRAMENTO.
			 *
			 * O RECOMENDADO e:
			 *
			 * 1 - Reiniciar o barramento (ou reconfigura-lo) usando a funcao do DEVICE Dev_XXX_XXXX_DeintHwInit( 0 ) e Dev_XXX_XXXX_IntHwInit( 0 ). OBs.: Pode afetar outros dispositivos do barramento.
			 * 2 - Refazer as configuracoes do DEVICE usando Dev_XXX_XXXX_ExtDevConfig( 0, Config_Parameters)
			 */
			break;
		}

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
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 60;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
	PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
