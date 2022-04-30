/**
 ******************************************************************************
 * File Name          : main.c
 * Description        : Main program body
 ******************************************************************************
 ** This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * COPYRIGHT(c) 2017 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_hal.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tsc.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#define led1On()  HAL_GPIO_WritePin(LED_1_GPIO_Port,LED_1_Pin,GPIO_PIN_SET)
#define led1Off() HAL_GPIO_WritePin(LED_1_GPIO_Port,LED_1_Pin,GPIO_PIN_RESET)
#define led2On()  HAL_GPIO_WritePin(LED_2_GPIO_Port,LED_2_Pin,GPIO_PIN_SET)
#define led2Off() HAL_GPIO_WritePin(LED_2_GPIO_Port,LED_2_Pin,GPIO_PIN_RESET)
#define led3On()  HAL_GPIO_WritePin(LED_3_GPIO_Port,LED_3_Pin,GPIO_PIN_SET)
#define led3Off() HAL_GPIO_WritePin(LED_3_GPIO_Port,LED_3_Pin,GPIO_PIN_RESET)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
uint16_t adcData[2];
// on va off ba sms
int SW = 0;
// noise nashy az gsm va pir
int NSW = 0;
// neshan gar gozasht 3 min
int tim = 1;
int c = 0;
// neshan gar vojod sms
int sms = 0;
// neshan gar peyda kardan shomare mored nazar
int numberDitect = 0;
// neshan gar peyda kardan kalame on dar matn sms
int smsOn = 0;
// neshan gar peyda kardan kalame off dar matn sms
int smsOff = 0;
// time counter har bar 10 s ra mishemarad
int TC = 0;
// baraye pir ke mikhaym bebinim vasle ya na
int pir1 = 1;

char ch[250];
uint8_t in;
int chC = 0;

void sim800_Init() {
	HAL_GPIO_WritePin(Power_Key_GPIO_Port, Power_Key_Pin, 1);
	HAL_Delay(202);
	HAL_GPIO_WritePin(Power_Key_GPIO_Port, Power_Key_Pin, 0);
	HAL_Delay(2000);
	HAL_GPIO_WritePin(Power_Key_GPIO_Port, Power_Key_Pin, 1);
	HAL_Delay(4000);
	HAL_UART_Transmit(&huart1, "ATr", sizeof("ATr"), 100);
	HAL_Delay(7000);
}

int size(char arr[]) {
	int count = 0;
	while (arr[count] != 0) {
		count++;
	}
	return count;
}

//send text massage to tell_num
void SIM800_Send_SMS(char tell[], char sms_text[]) {
	char text[50] = { 0 };
	char sms_con[24] = { 0, 0 }; //andaze dastor + shomare telephone
	char sub = 26;
	char enter = 13;
	strcpy(text, sms_text);
	sprintf(sms_con, "AT+CMGS=\"");
	strcat(sms_con, tell);
	strcat(sms_con, "\"\r");
	HAL_UART_Transmit(&huart1, "AT+CMGF=1\r", sizeof("AT+CMGF=1\r"), 100);
	HAL_Delay(1500);
	HAL_UART_Transmit(&huart1, sms_con, size(sms_con), 100);
	HAL_Delay(500);
	HAL_UART_Transmit(&huart1, sms_text, size(sms_text), 290);
	HAL_Delay(100);
	HAL_UART_Transmit(&huart1, &sub, sizeof(sub), 100);
	HAL_Delay(100);
	return;

}

void SIM800_call(char tell[]) {
	char calling[50];
	sprintf(calling, "ATD");
	strcat(calling, tell);
	strcat(calling, ";\r");
	HAL_UART_Transmit(&huart1, calling, size(calling), 100);
}

//irancell
void SIM800_Check_Credit() {
	HAL_UART_Transmit(&huart1, "AT+CUSD=1,\"*141*1#\",15\r\n",
			sizeof("AT+CUSD=1,\"*141*1#\",15\r\n"), 100);
}

void SIM800_Check() {
	char d[10] = { 0 };
	HAL_UART_Transmit(&huart1, "AT\r", sizeof("AT\r"), 100);
	HAL_Delay(100);
	HAL_Delay(1000);

}

void SIM800_Signal() {
	HAL_UART_Transmit(&huart1, "AT+CSQ\r", sizeof("AT+CSQ\r"), 100);
}

void SIM800_delet_all_sms() {
	HAL_UART_Transmit(&huart1, "AT+CMGD=4,4\r\n", sizeof("AT+CMGD=4,4\r\n"),
			100);
}

void SIM800_read_SMS() {

	if (sms == 1) {

		HAL_UART_Transmit(&huart1, "AT+CMGF=1\r", sizeof("AT+CMGF=1\r"), 100);
		HAL_Delay(700);

		HAL_UART_Transmit(&huart1, "AT+CMGL=\"REC UNREAD\"\r",
				sizeof("AT+CMGL=\"REC UNREAD\"\r"), 100);
		//HAL_UART_Receive (&huart1,con,80,100);

		sms = 0;
		NSW = 1;

	} else if (smsOn == 1) {
		SW = 1;
		SIM800_Send_SMS("09******606", "I'm On \n");
		smsOn = 0;
		SIM800_Check();
		HAL_Delay(100);
		SIM800_Check();
		SIM800_delet_all_sms();
		NSW = 1;
		HAL_Delay(20000);
	} else if (smsOff == 1) {
		SW = 0;
		SIM800_Send_SMS("09******606", "I'm Off \n");
		smsOff = 0;
		SIM800_Check();
		HAL_Delay(100);
		SIM800_Check();
		SIM800_delet_all_sms();
		NSW = 1;
		HAL_Delay(20000);
	}

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

	__HAL_UART_CLEAR_FEFLAG(huart);

	if (chC >= 250)
		chC = 0;
	if (in != 0x0A) {

		ch[chC++] = (char) in;
		ch[chC] = 0;
	} else {
		if (strstr(ch, "CMTI:")) {

			sms = 1;
			NSW = 1;
		} else if (strstr(ch, "9******606")) {
			numberDitect = 1;

		} else if ((strcasestr(ch, "on")) && numberDitect == 1) {
			smsOn = 1;
			numberDitect = 0;
		} else if ((strcasestr(ch, "off")) && numberDitect == 1) {
			smsOff = 1;
			numberDitect = 0;

		}
		ch[0] = 0;
		chC = 0;
	}

	HAL_UART_Receive_IT(&huart1, &in, 1);

}

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
//	TC++;
////	HAL_GPIO_WritePin(LED2_GPIO_Port,LED2_Pin,TC%2);
//	if(TC==36)
//	{
//		tim = 1;
//		TC=0;
//	}
//
//}

/* USER CODE END 0 */

int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration----------------------------------------------------------*/

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
	MX_ADC_Init();
	MX_I2C2_Init();
	MX_TSC_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();

	/* USER CODE BEGIN 2 */
	sim800_Init();
	HAL_Delay(500);
	SIM800_Check();
	HAL_Delay(500);
	SIM800_Check();
	HAL_Delay(500);
	SIM800_delet_all_sms();
	HAL_Delay(1000);
	HAL_Delay(5000);
	SIM800_delet_all_sms();
	HAL_Delay(2000);
	HAL_UART_Receive_IT(&huart1, &in, 1);
	HAL_UART_Receive_IT(&huart1, &in, 1);
	HAL_Delay(500);
	HAL_Delay(120000);

	HAL_ADC_Start_DMA(&hadc, adcData, 2);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

			SIM800_read_SMS();
			HAL_UART_Receive_IT(&huart1, &in, 1);
		pir1 = HAL_GPIO_ReadPin(PIR_1_GPIO_Port, PIR_1_Pin);
		if (pir1 == 0 && SW==1)	//
				{
			led1On();
			HAL_Delay(100);
			led1Off();
			HAL_Delay(100);
			SIM800_call("+989******606");
			HAL_Delay(60000);
		}

	}
	/* USER CODE END 3 */

}

/** System Clock Configuration
 */
void SystemClock_Config(void) {

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInit;

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_HSI14;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
	RCC_OscInitStruct.HSICalibrationValue = 16;
	RCC_OscInitStruct.HSI14CalibrationValue = 16;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure the Systick interrupt time
	 */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

	/**Configure the Systick
	 */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void _Error_Handler(char * file, int line) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */

}

#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
