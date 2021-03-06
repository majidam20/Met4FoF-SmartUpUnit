/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * Copyright (c) 2018 STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
// STM32 Hardware drivers
#include "dma.h"
#include "stm32f7xx_hal.h"
#include "cmsis_os.h"
#include "adc.h"
#include "lwip.h"
#include "spi.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

//Networkinterface and Webserver
#include "lwip/api.h"
#include "lwip/udp.h"
#include "lwip.h"
#include "httpserver-netconn.h"

#define USE_L3GD20 1
#define USE_BMA280 0
// Sensors
//#include "ADXL345.h"
#include "bma280.h"
#if USE_BMA280
#include "bma280.h"
#endif
// FOR MPU6050
//#include "I2Cdev.h"
//#include "MPU6050.h"

//FOR L3GD20
#if USE_L3GD20
#include "L3GD20.h"
#endif
//LCD
#include "ILI9341/ILI9341_STM32_Driver.h"
#include "ILI9341/ILI9341_GFX.h"

//GPS Time Snyc
#include "GPSTimesyn.h"
#include "NMEAPraser.h"

#include "SEGGER_RTT.h"

//Protobuff
#include "pb.h"
#include "message.pb.h"
#include "pb_common.h"
#include "pb_encode.h"

osThreadId WebServerTID;
osThreadId blinkTID;
osThreadId DataProcessingTID;
osThreadId DataStreamingTID;
osThreadId LCDTID;

//I2Cdev I2CdevIface(&hi2c1);
//MPU6050 MPU6050Acc(I2CdevIface);

#if USE_L3GD20
L3GD20 Gyro(SENSOR_CS1_GPIO_Port, SENSOR_CS1_Pin, &hspi3);
//MemPool For the data
osMailQDef(GyroMail, DATABUFFESEIZE, ProtoIMUStamped);
osMailQId GyroMail;
#endif
#if USE_BMA280

//MemPool For the data
BMA280 Acc(SENSOR_CS1_GPIO_Port, SENSOR_CS1_Pin, &hspi3);
osMailQDef(AccMail, DATABUFFESEIZE, AccelDataStamped);
osMailQId AccMail;

#endif
//TODO update website
AccelDataStamped ACCData;


// Network interface Ip
uint8_t ETH_IP_ADDRESS[4] = { 192, 168, 0, 10 };
// Target IP for udp straming
uint8_t UDP_TARGET_IP_ADDRESS[4] = { 192, 168, 0, 1 };

//Top 32 bit for timer2 inputcapture values
static uint32_t tim2_update_counts;
static uint64_t tim2_upper_bits_mask;
#ifdef __cplusplus

extern "C" {

#endif

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void StartWebserverThread(void const * argument);
void StartBlinkThread(void const * argument);
void StartDataProcessingThread(void const * argument);
void StartDataStreamingThread(void const * argument);
void StartLCDThread(void const * argument);
void _Error_Handler(char * file, int line);
//TODO remove this getter functions and implement it in the dataprocessing
float getGVal(int index);
float getBMATemp();
void Check_LWIP_RETURN_VAL(err_t retVal);
#ifdef __cplusplus

}

#endif

/**
 * @brief  The application entry point.
 *
 * @retval None
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */
	/* Enable I-Cache-------------------------------------------------------------*/
	SCB_EnableICache();

	/* Enable D-Cache-------------------------------------------------------------*/
	SCB_EnableDCache();
	/* MCU Configuration----------------------------------------------------------*/

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
    MX_UART7_Init();
    MX_USART3_UART_Init();
    MX_USART6_UART_Init();
	MX_DMA_Init();
	//MX_USB_OTG_FS_PCD_Init();
	MX_ADC1_Init();
	MX_SPI1_Init();
	MX_SPI3_Init();
	MX_TIM2_Init();
	MX_I2C1_Init();
	MX_I2C2_Init();
#if USE_BMA280
	AccMail = osMailCreate(osMailQ(AccMail), NULL);
	Acc.init(AFS_2G, BW_1000Hz, normal_Mode, sleep_0_5ms);
#endif
#if USE_L3GD20
//	GyroPool = osPoolCreate(osPool(GyroPool));
//	GyroMsgBuffer = osMessageCreate(osMessageQ(GyroMsgBuffer), NULL);
	GyroMail = osMailCreate(osMailQ(GyroMail), NULL);
	Gyro.init(GYRO_RANGE_2000DPS, GYRO_UPDATE_200_HZ);
#endif

	// ADXL345
#if USE_ADXL345
	Go into standby mode to configure the device.
	Acc.setPowerControl(0x00);
	Acc.setResolution(ADXL345_AFS_FULL_RANGE);
	Acc.setDataRate(ADXL345_3200HZ);
	Activate DataRdy Interrupt
	Acc.setInterruptEnableControl(0x80);
	Acc.setInterruptMappingControl(0x00);
	Measurement mode.
	Acc.setPowerControl(0x08);
#endif

	/* USER CODE BEGIN 2 */
	//create the defined Buffer and Pool for ACC and GPS data
	/* Create the thread(s) */
	/* definition and creation of defaultTask */
	osThreadDef(WebserverTherad, StartWebserverThread, osPriorityNormal, 0,
			128);
	WebServerTID = osThreadCreate(osThread(WebserverTherad), NULL);

	osThreadDef(blinkThread, StartBlinkThread, osPriorityLow, 0, 16);
	blinkTID = osThreadCreate(osThread(blinkThread), NULL);

	osThreadDef(DataProcessingThread, StartDataProcessingThread,
			osPriorityNormal, 0, 256);
	DataProcessingTID = osThreadCreate(osThread(DataProcessingThread), NULL);

	osThreadDef(DataStreamingThread, StartDataStreamingThread, osPriorityNormal ,
			0, 8192);
	DataStreamingTID = osThreadCreate(osThread(DataStreamingThread), NULL);

	osThreadDef(LCDThread, StartLCDThread, osPriorityNormal, 0, 256);

	LCDTID = osThreadCreate(osThread(LCDThread), NULL);
	/* USER CODE END 2 */


	initGPSTimesny();
    SEGGER_SYSVIEW_Conf();
	/* Start scheduler */
	osKernelStart();
	/* We should never get here as control is now taken by the scheduler */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

	}
	/* USER CODE END 3 */

}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

	/**Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE()
	;

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 400;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 8;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Activate the Over-Drive mode
	 */
	if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_USART6|RCC_PERIPHCLK_UART7
                              |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_SDMMC2
                              |RCC_PERIPHCLK_CLK48;
	PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
	PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
	PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
	PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
	PeriphClkInitStruct.PLLSAIDivQ = 1;
	PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
        PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
        PeriphClkInitStruct.Usart6ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
        PeriphClkInitStruct.Uart7ClockSelection = RCC_UART7CLKSOURCE_PCLK1;
        PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
        PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
        PeriphClkInitStruct.Sdmmc2ClockSelection = RCC_SDMMC2CLKSOURCE_CLK48;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_SYSCLK, RCC_MCODIV_1);

	/**Configure the Systick interrupt time
	 */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

	/**Configure the Systick
	 */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/* USER CODE BEGIN 4 */

/* StartDefaultTask function */
void StartWebserverThread(void const * argument) {
        osDelay(4000);
	http_server_netconn_init();
	/* Infinite loop */
	for (;;) {
		osThreadTerminate(NULL);
	}
}

void StartBlinkThread(void const * argument) {
	while (1) {
		HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		osDelay(100);
	}
	osThreadTerminate(NULL);
}

void StartDataProcessingThread(void const * argument) {
	while (1) {
		osDelay(1000);
	}
	osThreadTerminate(NULL);
}

void StartLCDThread(void const * argument) {
	ILI9341_Init(); //initial driver setup to drive ili9341
	ILI9341_Fill_Screen(BLUE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_1);
	char Temp_Buffer_text[40];
	ILI9341_Draw_Text("Met4FoF SmartUpUnit", 0, 0, WHITE, 2, BLUE);
	sprintf(Temp_Buffer_text, "Build.date:%s", __DATE__);
	ILI9341_Draw_Text(Temp_Buffer_text, 0, 20, WHITE, 2, BLUE);
	sprintf(Temp_Buffer_text, "Build.time:%s", __TIME__);
	ILI9341_Draw_Text(Temp_Buffer_text, 0, 40, WHITE, 2, BLUE);
	sprintf(Temp_Buffer_text, "IP      :%d.%d.%d.%d", ETH_IP_ADDRESS[0],
			ETH_IP_ADDRESS[1], ETH_IP_ADDRESS[2], ETH_IP_ADDRESS[3]);
	ILI9341_Draw_Text(Temp_Buffer_text, 0, 60, WHITE, 2, BLUE);
	sprintf(Temp_Buffer_text, "UPD Targ:%d.%d.%d.%d", UDP_TARGET_IP_ADDRESS[0],
			UDP_TARGET_IP_ADDRESS[1], UDP_TARGET_IP_ADDRESS[2],
			UDP_TARGET_IP_ADDRESS[3]);
	ILI9341_Draw_Text(Temp_Buffer_text, 0, 80, WHITE, 2, BLUE);
	//----------------------------------------------------------IMAGE EXAMPLE, Snow Tiger
	while (1) {
		osDelay(1000);
		timespec utc;
		timespec gps_time;
		lgw_gps_get(&utc, &gps_time, NULL, NULL);
		tm* current_time = localtime(&(utc.tv_sec));
		strftime(Temp_Buffer_text, 20, "%Y-%m-%d %H:%M:%S", current_time);
		ILI9341_Draw_Text(Temp_Buffer_text, 0, 100, WHITE, 2, BLUE);
	}
	osThreadTerminate(NULL);
}

void StartDataStreamingThread(void const * argument) {
	/* init code for LWIP */
    MX_LWIP_Init();
        //defining Protobuff output stream with Maximum Transfer unit (MTU) size of the networkpackages
	#define MTU_SIZE 500
    uint8_t ProtoBuffer[MTU_SIZE] = { 0 };
	pb_ostream_t ProtoStream = pb_ostream_from_buffer(ProtoBuffer, MTU_SIZE);

	static uint32_t porcessedCount = 0;
	struct netconn *conn;
	struct netbuf *buf;
	//UDP target ip Adress
	ip_addr_t targetipaddr;
	IP4_ADDR(&targetipaddr, UDP_TARGET_IP_ADDRESS[0], UDP_TARGET_IP_ADDRESS[1],
			UDP_TARGET_IP_ADDRESS[2], UDP_TARGET_IP_ADDRESS[3]);
	/* create a new connection */
	conn = netconn_new(NETCONN_UDP);
	/* connect the connection to the remote host */
	err_t net_conn_result=netconn_connect(conn, &targetipaddr, 7000);
	Check_LWIP_RETURN_VAL(net_conn_result);
	/* create a new netbuf */
	buf = netbuf_new();

	osDelay(2000);
	//Start timer and arm inputcapture
	if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK) {
		/* Starting Error */
		_Error_Handler(__FILE__, __LINE__);
	}
	if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3) != HAL_OK) {
		/* Starting Error */
		_Error_Handler(__FILE__, __LINE__);
	}
	if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4) != HAL_OK) {
		/* Starting Error */
		_Error_Handler(__FILE__, __LINE__);
	}
	__HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
          /* Enable ADC1 external trigger */
    HAL_ADC_Start_IT(&hadc1);


	while (1) {
#if USE_BMA280
		AccelDataStamped *Accrptr;
		//Delay =200 ms so the other routine is processed with 5 Hz >>1 Hz GPS PPS
		osEvent Accevt = osMailGet(AccMail,200);
		struct timespec utc;
		if (Accevt.status == osEventMail) {
			Accrptr = (AccelDataStamped*) Accevt.value.p;
			ACCData = *Accrptr;
			osMutexWait(GPS_ref_mutex_id, osWaitForever);
			lgw_cnt2utc(GPS_ref,Accrptr->RawTimerCount,&utc);
			Accrptr->UnixSecs=(uint32_t)(utc.tv_sec);
			Accrptr->NanoSecs=(uint32_t)(utc.tv_nsec);
			osMutexRelease(GPS_ref_mutex_id);
			porcessedCount++;
			uint8_t MSGBuffer[sizeof(ACCData)+4]= {0};
			MSGBuffer[0]=0x41;
			MSGBuffer[1]=0x43;
			MSGBuffer[2]=0x43;
			MSGBuffer[3]=0x33;
			memcpy(&MSGBuffer[4],&*Accrptr, sizeof(ACCData));
			/* reference the data into the netbuf */
			netbuf_ref(buf, &MSGBuffer, sizeof(MSGBuffer));

			/* send the text */
			netconn_send(conn, buf);
			osMailFree(AccMail, Accrptr);
		}
#endif
#if USE_L3GD20
//Delay =200 ms so the other routine is processed with 5 Hz >>1 Hz GPS PPS
		struct timespec utc;
		osEvent Gyroevt = osMailGet(GyroMail, 200);
		if (Gyroevt.status == osEventMail) {
			if(ProtoStream.bytes_written>(MTU_SIZE-ProtoIMUStamped_size)){
				//sending the buffer
				netbuf_ref(buf, &ProtoBuffer, ProtoStream.bytes_written);
				/* send the text */
				err_t net_conn_result =netconn_send(conn, buf);
				Check_LWIP_RETURN_VAL(net_conn_result);
				// reallocating buffer this is maybe performance intensive profile this
				//TODO profile this code
				ProtoStream = pb_ostream_from_buffer(ProtoBuffer, MTU_SIZE);
			}
			ProtoIMUStamped *Gyrorptr;
			Gyrorptr = (ProtoIMUStamped*) Gyroevt.value.p;
			//osMutexWait(GPS_ref_mutex_id, osWaitForever);
			lgw_cnt2utc(GPS_ref, Gyrorptr->raw_timer_count, &utc);
			Gyrorptr->unix_secs = (uint32_t) (utc.tv_sec);
			Gyrorptr->unix_nsecs = (uint32_t) (utc.tv_nsec);
			//osMutexRelease(GPS_ref_mutex_id);
			porcessedCount++;
			pb_encode(&ProtoStream, ProtoIMUStamped_fields, &Gyrorptr);
			/* reference the data into the netbuf */
			osMailFree(GyroMail, Gyrorptr);

		}
#endif
		osEvent GPSMailevnt = osMailGet(GPSDebugMail, 0);
		if (GPSMailevnt.status == osEventMail) {
			GPSDebugMsg *GPSDebugrptr;
			GPSDebugrptr = (GPSDebugMsg*) GPSMailevnt.value.p;
			uint8_t MSGBuffer[sizeof(GPSDebugMsg) + 4] = { 0 }; //added trashbyte so the packet is better visable in wiereshark
			MSGBuffer[0] = 0x47; //GPSD=47 50 53 44
			MSGBuffer[1] = 0x50;
			MSGBuffer[2] = 0x53;
			MSGBuffer[3] = 0x44;
			memcpy(&MSGBuffer[4], &*GPSDebugrptr, sizeof(GPSDebugMsg));
			/* reference the data into the netbuf */
			netbuf_ref(buf, &MSGBuffer, sizeof(MSGBuffer));
			/* send the text */
			osStatus result = osMailFree(GPSDebugMail, GPSDebugrptr);
			netconn_send(conn, buf);
		}
	}
	osThreadTerminate(NULL);
}

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM14 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	// update the upper 32 bits if nothing other has happend this should be the normal case
	if (htim->Instance == TIM2) {
		{
			tim2_update_counts++;
			tim2_upper_bits_mask = uint64_t(tim2_update_counts - 1) << 32;//timer gets initaled with set upodateflag but we want to start at zero therfore -1

		}
	}
	if (htim->Instance == TIM14) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */

	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  file: The file name as string.
 * @param  line: The line in file as a number.
 * @retval None
 */
void _Error_Handler(char * file, int line) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

/*
 @startuml
 :DataRDY Interupt from Sensor|
 fork
 :Start ADC1 Conversion|
 :wait for ADC conversion;
 fork again
 :Capture Timer Val|
 :timer ISR;
 :Allocate Sharred membuffer;
 :copy timestamp in membuffer;
 :get data From device and copy to membuffer;
 end fork
 :get data from ADC and copy to membuffer;
 :send membuffer pointer to dataproccesing thread;
 @enduml
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef * htim) {
	//GPS testing change this to an que based aproche in the future
	static uint32_t captureCount = 0;
	static uint32_t GPSEdges = 0;
	static uint16_t ADCValue;
	// RACE CONDITION CECKING !!
	// this code occures as well in  void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim) in stm32f7xx_hal_tim.c
	// this function is called every time an Timer generates an interrupt event and adds it do the Nested Vectored Interrupt Controller (NVIC)
	// the timer generates an global interrupt(IRQ#44 TIM2_IRQHandler) witch is always the same regardless the reason for the the interrupt (input capture, output compare, upadte)
	// so the HAL_TIM_IRQHandler() is called to determin the source of the ISR request, there fore the startus registers of the timer are read.
	// in that order:
	// /* Capture compare 1 event */
	// /* Capture compare 2 event */
	// /* Capture compare 3 event */
	// /* Capture compare 4 event */
	// /* TIM Update event */
	// ...
	// if an update and an inputcapure event occures while jumping or processing  in the HAL_TIM_IRQHandler() then the inputcapure event will be
	// procesed before the update event and the tim2_upper_bits_mask is not set right so we have to check the timer values and decive if they are from befor or after the overflow.
	uint64_t tim2_upper_bits_mask_race_condition = 0;
	#define TIM2OLDTIMERVALMIN 0xFF000000 // if an inputcaputure value is biger than this its prppably an old one
	bool tim2_race_condition_up_date = false;
	if (htim->Instance == TIM2) {
		if (__HAL_TIM_GET_FLAG(htim, TIM_FLAG_UPDATE) != RESET) {
			if (__HAL_TIM_GET_IT_SOURCE(htim, TIM_IT_UPDATE) != RESET) {
				__HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE);
				//the flag gets cleared to prevent HAL_TIM_PeriodElapsedCallback() calling and therfore double increasment
				//DEBUG_MSG("WARNING!!! TIMER OVERFLOW DETECTED OUTSIDE OF UPDATEEVENTHANDLER\n START SPECIAL HANDLING CHECK RESULTS OF THIS MESURMENT CYCLE");
				tim2_race_condition_up_date = true;
				tim2_upper_bits_mask_race_condition = tim2_upper_bits_mask;
				tim2_update_counts++;
				tim2_upper_bits_mask = uint64_t(tim2_update_counts - 1) << 32;//timer gets initaled with set upodateflag but we want to start at zero therfore -1
			}
		}
	}
	if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
		uint64_t timstamp64=0;
		if (tim2_race_condition_up_date == false) {
			timstamp64=tim2_upper_bits_mask+ (uint64_t) HAL_TIM_ReadCapturedValue(&htim2,TIM_CHANNEL_3);
		}
		else {
		uint32_t timestamp_raw = HAL_TIM_ReadCapturedValue(&htim2,
								TIM_CHANNEL_3);
						if (timestamp_raw < TIM2OLDTIMERVALMIN)
						//the timer has overflowen take the updateted bitmask
						{
							timstamp64 = tim2_upper_bits_mask + (uint64_t) timestamp_raw;
						} else {
							//this is an old value using the old bitmask
							timstamp64 = tim2_upper_bits_mask_race_condition
									+ (uint64_t) timestamp_raw;
						}
		}
		HAL_ADC_PollForConversion(&hadc1, 1);
		ADCValue = HAL_ADC_GetValue(&hadc1);
		HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
#if USE_L3GD20
		ProtoIMUStamped *mptr;
		// ATENTION!! if buffer is full the allocation function is blocking aprox 60µs
		mptr = (ProtoIMUStamped *) osMailAlloc(GyroMail,0);//The parameter millisec must be 0 for using this function in an ISR.
		if (mptr != NULL) {
			*mptr = Gyro.GetStampedData(0x00000000,
					timstamp64,
					captureCount, ADCValue);
			//put dater pointer into MSGQ
			osStatus result = osMailPut(GyroMail, mptr);
		}
#endif
#if USE_BMA280
		AccelDataStamped *mptr;
		// ATENTION!! if buffer is full the allocation function is blocking aprox 60µs
		mptr = (AccelDataStamped *) osMailAlloc(AccMail,0);//The parameter millisec must be 0 for using this function in an ISR.
		if (mptr != NULL) {
			*mptr = Acc.GetStampedData(0x00000000,
					timstamp64,
					captureCount,ADCValue);
			//put dater pointer into MSGQ
			osStatus result = osMailPut(AccMail,mptr);
		}
#endif
		captureCount++;
		HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		//osMessagePut(ACCBuffer,(uint32_t)mptr,osWaitForever);
	} else if (htim->Instance == TIM2
			&& htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
		uint64_t timstamp64=0;
		if (tim2_race_condition_up_date == false) {
			timstamp64=tim2_upper_bits_mask+ (uint64_t) HAL_TIM_ReadCapturedValue(&htim2,TIM_CHANNEL_1);
		}
		else {
		uint32_t timestamp_raw = HAL_TIM_ReadCapturedValue(&htim2,
								TIM_CHANNEL_1);
						if (timestamp_raw < TIM2OLDTIMERVALMIN)
						//the timer has overflowen take the updateted bitmask
						{
							timstamp64 = tim2_upper_bits_mask + (uint64_t) timestamp_raw;
						} else {
							//this is an old value using the old bitmask
							timstamp64 = tim2_upper_bits_mask_race_condition
									+ (uint64_t) timestamp_raw;
						}
		}
	}

	else if (htim->Instance == TIM2
			&& htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
		uint64_t timstamp64=0;
		if (tim2_race_condition_up_date == false) {
			timstamp64=tim2_upper_bits_mask+ (uint64_t) HAL_TIM_ReadCapturedValue(&htim2,TIM_CHANNEL_4);
		}
		else {
		uint32_t timestamp_raw = HAL_TIM_ReadCapturedValue(&htim2,
								TIM_CHANNEL_4);
						if (timestamp_raw < TIM2OLDTIMERVALMIN)
						//the timer has overflowen take the updateted bitmask
						{
							timstamp64 = tim2_upper_bits_mask + (uint64_t) timestamp_raw;
						} else {
							//this is an old value using the old bitmask
							timstamp64 = tim2_upper_bits_mask_race_condition
									+ (uint64_t) timestamp_raw;
						}
		}

		//pointer needs to be static otherwiese it would be deletet when jumping out of ISR
		static NMEASTamped *mptr = NULL;
		static uint32_t GPScaptureCount = 0;
		static uint8_t DMA_NMEABUFFER[NMEBUFFERLEN]={0};
		GPSEdges++;
			if (GPScaptureCount > 0) {
                                //HAL_UART_RxCpltCallback(&huart7);
                                if(HAL_UART_DMAStop(&huart7)!=HAL_OK)
                                {
                                SEGGER_RTT_printf(0,"HAL_UART_DMAStop returned not ok");
                                }
                                if(HAL_DMA_Abort(&hdma_uart7_rx)!=HAL_OK)
                                {
                                SEGGER_RTT_printf(0,"HAL_UART_DMAStop returned not ok");
                                }
				mptr = (NMEASTamped *) osMailAlloc(NMEAMail,0);//The parameter millisec must be 0 for using this function in an ISR.
				if (mptr != NULL) {
					mptr->RawTimerCount = timstamp64;
					mptr->CaptureCount = GPScaptureCount;
					memcpy(&(mptr->NMEAMessage[0]),&(DMA_NMEABUFFER[0]),NMEBUFFERLEN);
                                        osStatus result = osMailPut(NMEAMail, mptr);
				}
                                SEGGER_RTT_printf(0,"DMA BUFFER:=%s\n",DMA_NMEABUFFER);
                                memset(DMA_NMEABUFFER, 0, sizeof(DMA_NMEABUFFER));
				HAL_StatusTypeDef DMA_START_result=HAL_UART_Receive_DMA(&huart7,
						DMA_NMEABUFFER,
						NMEBUFFERLEN - 1);
				GPScaptureCount++;
                                if(DMA_START_result!=HAL_OK)
                                {
                                 SEGGER_RTT_printf(0,"DMA start ERROR");
                                }
			}
			else if (GPScaptureCount == 0) {
				HAL_UART_Receive_DMA(&huart7,&(DMA_NMEABUFFER[0]),NMEBUFFERLEN - 1);
					GPScaptureCount++;
				}
	}
}

float getGVal(int index) {
	switch (index) {
	case 0:
		return ACCData.Data.x;
	case 1:
		return ACCData.Data.y;
	case 2:
		return ACCData.Data.z;
	default:
		int nan = 0x7F800001;
		return *(float*) &nan;
	}

}

float getBMATemp() {
	return ACCData.Data.temperature;
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

void Check_LWIP_RETURN_VAL(err_t retVal)
{
				if (retVal!=ERR_OK){
					switch(retVal){
					case -1: SEGGER_RTT_printf(0,"LWIP ERR_MEM: Out of memory error.");break;
					case -2: SEGGER_RTT_printf(0,"LWIP ERR_BUF: Buffer error. ");break;
					case -3: SEGGER_RTT_printf(0,"LWIP ERR_TIMEOUT: Time Out. ");break;
					case -4: SEGGER_RTT_printf(0,"LWIP ERR_RTE: Routing problem. ");break;
					case -5: SEGGER_RTT_printf(0,"LWIP ERR_INPROGRESS: Operation in progress ");break;
					case -6: SEGGER_RTT_printf(0,"LWIP ERR_VAL: Illegal value");break;
					case -7: SEGGER_RTT_printf(0,"LWIP ERR_WOULDBLOCK: Operation would block.");break;
					case -8: SEGGER_RTT_printf(0,"LWIP ERR_USE: Address in use.");break;
					case -9: SEGGER_RTT_printf(0,"LWIP ERR_ALREADY: Already connecting.");break;
					case -10: SEGGER_RTT_printf(0,"LWIP ERR_ISCONN: Conn already established.");break;
					case -11: SEGGER_RTT_printf(0,"LWIP ERR_CONN: Not connected.");break;
					case -12: SEGGER_RTT_printf(0,"LWIP ERR_IF: Low-level netif error.");break;
					case -13: SEGGER_RTT_printf(0,"LWIP ERR_ABRT: Connection aborted.");break;
					case -14: SEGGER_RTT_printf(0,"LWIP ERR_RST: Connection reset.");break;
					case -15: SEGGER_RTT_printf(0,"LWIP ERR_CLSD: Connection closed. ");break;
					case -16: SEGGER_RTT_printf(0,"LWIP ERR_ARG: Illegal argument.");break;
					}
				}
}
/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
