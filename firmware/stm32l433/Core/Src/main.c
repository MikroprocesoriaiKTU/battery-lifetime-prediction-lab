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
#include "math.h"   // log(), pow() for the sensor conversion

#include "sc_types.h"
#include "Statechart.h"
#include "Statechart_required.h"

#include "ssd1306.h"
#include "fonts.h"
#include <stdio.h>  // sprintf()

#include "EventRecorder.h"                        // Keil::Compiler:Event Messaging

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
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c3;

TIM_HandleTypeDef htim6;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C3_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define BUFFERLEN        5     // number of samples averaged before the display is updated
#define NUM_ADC_CHANNELS 3     // ADC scan order (DMA buffer index):
                               // [0] PC2 = ADC1_IN3, divider R5/R7
                               // [1] PC3 = ADC1_IN4, divider R6/R8
                               // [2] PA1 = ADC1_IN6, divider excitation voltage (ADC_DRIVE)
#define R_REF  10e3            // R5 and R6 on the add-on board, ohm (10k or 20k, see board variant)

/* NTC thermistor parameters, needed for the TODO in statechart_processData() */
#define R1_NTC 10e3            // NTC resistance at T1, ohm
#define T1_NTC (25.0+273)      // T1, kelvin
#define B_NTC  3455            // B parameter, kelvin (https://www.electronics-tutorials.ws/io/thermistors.html)

#define A_44 1.1809e-3         // NTC curve 44 (Steinhart-Hart type):
#define B_44 2.1688e-4         //   1/T = A + B*ln(R) + C*ln(R)^2 + D*ln(R)^3,  T in kelvin, R in ohm
#define C_44 1.9006e-6
#define D_44 1.8316e-8

uint16_t DataBufferI2C[BUFFERLEN];                   // raw I2C sensor samples
uint16_t DataBufferADC[BUFFERLEN][NUM_ADC_CHANNELS]; // raw ADC samples

uint32_t SumSensorI2C;
uint32_t SumSensorsADC[NUM_ADC_CHANNELS];
float AverageSensorI2C;                    // averaged I2C sensor temperature, degC
float AverageSensorsADC[NUM_ADC_CHANNELS]; // averaged sensor resistances R7/R8, ohm

Statechart sc_handle; // state machine instance (itemis CREATE)

// =========== I2C temperature sensor LM75AD (START) ===========
// https://www.nxp.com/docs/en/data-sheet/LM75A.pdf
#define TEMPER_REG_POINTER 0   // datasheet: Register list
#define CONFIG_REG_POINTER 1   // datasheet: Register list

#define LM75AD_BASE_ADR  0x90  // datasheet: Slave address (A2..A0 = 0)
#define SENSOR_WRITE_BIT 0     // R/W = 0
#define SENSOR_READ_BIT  1     // R/W = 1

#define LM75AD_CONFIG_NORMAL_MODE 0x00
#define LM75AD_CONFIG_SHUTDOWN    0x01

#define I2C_TIMEOUT 1000       // ms

/* ADC conversion results written by DMA */
#define ADC_CONVERTED_DATA_BUFFER_SIZE ((uint32_t) 32)
static uint16_t aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE];

uint8_t TxBuffer[2];   // I2C transmit buffer
uint8_t TxDataSize;

uint8_t RxBuffer[2];   // I2C receive buffer (temperature register)
uint8_t RxDataSize;
int16_t Temp_reg;      // temperature register value, two's complement
float NTC_Temperature[2];

uint16_t NTC_TemperatureSample[NUM_ADC_CHANNELS]; // latest ADC sample of each channel

/* Build options */
//#define DISABLE_DISPLAY     // define to run without the OLED display
//#define DISABLE_PROCESSING  // define to exclude the state machine from the build
#define ENABLE_EVENT_RECORDER // Keil Event Recorder timestamps (active/sleep timing)
#define ENABLE_SLEEP          // enter SLEEP mode between interrupts

/* DEBUGGING AND PERFORMANCE DECLARATIONS (START) */
#define EVENT_ENTER_ID       0x1
#define EVENT_GOSLEEP_ID     0x2
#define EVENT_TIMER_ID       0x3
#define EVENT_ADC_ID         0x4
#define EVENT_DISPLAY_END_ID 0x5

#ifdef ENABLE_EVENT_RECORDER
#define DEBUG_EVENT_START()    EventRecord2 (EVENT_ENTER_ID+EventLevelAPI, 0, 0)
#define DEBUG_EVENT_GOSLEEP()  EventRecord2 (EVENT_GOSLEEP_ID+EventLevelAPI, 0, 0)
#define DEBUG_EVENT_TIMER()    EventRecord2 (EVENT_TIMER_ID+EventLevelAPI, 0, 0)
#define DEBUG_EVENT_ADC()      EventRecord2 (EVENT_ADC_ID+EventLevelAPI, 0, 0)
#define DEBUG_EVENT_DISPLAY()  EventRecord2 (EVENT_DISPLAY_END_ID+EventLevelAPI, 0, 0)
#else
#define DEBUG_EVENT_START()    __asm("NOP")
#define DEBUG_EVENT_GOSLEEP()  __asm("NOP")
#define DEBUG_EVENT_TIMER()    __asm("NOP")
#define DEBUG_EVENT_ADC()      __asm("NOP")
#define DEBUG_EVENT_DISPLAY()  __asm("NOP")
#endif
/* DEBUGGING AND PERFORMANCE DECLARATIONS (END) */

/* I2C error handler (add error handling if needed) */
void MyErrorHandlerI2C()
{
}
/* ADC error handler (add error handling if needed) */
void MyErrorHandlerADC()
{
}

// =========== I2C temperature sensor LM75AD (END) ===========

/* Read the LM75AD temperature sensor over I2C in blocking mode */
void statechart_readI2CSensor( Statechart* handle)
{
  // Set the sensor to normal (continuous conversion) mode
  TxBuffer[0]=CONFIG_REG_POINTER;        // pointer: configuration register
  TxBuffer[1]=LM75AD_CONFIG_NORMAL_MODE; // value: normal mode
  TxDataSize=2;
  if (HAL_I2C_Master_Transmit(&hi2c3,LM75AD_BASE_ADR|SENSOR_WRITE_BIT,TxBuffer,TxDataSize,I2C_TIMEOUT)!=HAL_OK)
    MyErrorHandlerI2C();

  // Select the temperature register for the following read
  TxBuffer[0]=TEMPER_REG_POINTER;        // pointer: temperature register
  TxDataSize=1;
  if (HAL_I2C_Master_Transmit(&hi2c3,LM75AD_BASE_ADR|SENSOR_WRITE_BIT,TxBuffer,TxDataSize,I2C_TIMEOUT)!=HAL_OK)
    MyErrorHandlerI2C();

  /* TODO (student): read the temperature register.
   *  - Receive 2 bytes into RxBuffer with HAL_I2C_Master_Receive(), using the
   *    address LM75AD_BASE_ADR|SENSOR_READ_BIT and I2C_TIMEOUT.
   *  - Combine them into Temp_reg: the first byte is the MSB, the second the LSB.
   *  - Temp_reg is a two's complement value; its upper 11 bits hold the temperature
   *    in 0.125 degC steps, so temperature in degC = Temp_reg / 256.0
   *    (see the LM75A datasheet, temperature register).
   *  - MAX7500 uses the same registers; BMP180/BMP280 boards need the register
   *    sequence from their own datasheet.
   */
}

/* Save the I2C sensor value to the data buffer */
sc_integer statechart_saveI2CSample( Statechart* handle, const sc_integer sample_no)
{
  DataBufferI2C[sample_no]=Temp_reg;
  return 1;
}

/* Start one ADC scan of all channels; the results are written to aADCxConvertedData by DMA */
sc_integer statechart_startConvADC( Statechart* handle, const sc_integer channel)
{
  if (HAL_ADC_Start_DMA(&hadc1,(uint32_t *)aADCxConvertedData,NUM_ADC_CHANNELS) != HAL_OK)
  {
    MyErrorHandlerADC();
    return 0;
  }
  return 1;
}

/* Copy the ADC results from the DMA buffer once all channels are sampled */
sc_integer statechart_readADCSample( Statechart* handle, const sc_integer channel)
{
  if (channel==2)
  {
    NTC_TemperatureSample[0]=aADCxConvertedData[0];
    NTC_TemperatureSample[1]=aADCxConvertedData[1];
    NTC_TemperatureSample[2]=aADCxConvertedData[2];
  }
  return 1;
}

/* Save the ADC samples of all channels to the data buffer */
sc_integer statechart_saveADCSample( Statechart* handle, const sc_integer channel, const sc_integer sample)
{
  for (int i=0;i<NUM_ADC_CHANNELS;i++)
    DataBufferADC[sample][i]=NTC_TemperatureSample[i];
  return 1;
}

/* Average the buffered samples and convert them to physical units */
void statechart_processData(Statechart* handle)
{
  int j;
  uint32_t sum1,sum2[NUM_ADC_CHANNELS];

  sum1=0;
  for(j=0;j<NUM_ADC_CHANNELS;j++)
    sum2[j]=0;

  for(int i=0;i<BUFFERLEN;i++)
  {
    sum1+=DataBufferI2C[i];
    for(j=0;j<NUM_ADC_CHANNELS;j++)
      sum2[j]+=DataBufferADC[i][j];
  }

  // I2C sensor: 1/256 degC per LSB of Temp_reg
  AverageSensorI2C=sum1/BUFFERLEN/256;

  // Analog sensors: resistance of R7/R8 from the divider equation
  // R_sensor = R_REF * V_sensor / (V_excitation - V_sensor), V_excitation is channel [2]
  for(j=0;j<NUM_ADC_CHANNELS-1;j++)
    AverageSensorsADC[j]=sum2[j]*R_REF/(sum2[2]-sum2[j]);

  /* TODO (student): convert the sensor resistances AverageSensorsADC[0..1] (ohm)
   * to temperature in degC and store them in NTC_Temperature[0..1].
   *  - NTC variants: use the B-parameter model (R1_NTC, T1_NTC, B_NTC) or the more
   *    accurate curve 44 (A_44..D_44); both give the temperature in kelvin.
   *  - Photoresistor (LDR04) variants: convert the resistance to illuminance (lux)
   *    using the gamma coefficient from the photoresistor datasheet, and change the
   *    display text in statechart_displayInfo() accordingly.
   */
}

/* Show the averaged sensor values on the OLED display */
void statechart_displayInfo( Statechart* handle)
{
#ifndef DISABLE_DISPLAY
  static char string_display[30];

  // I2C sensor reading
  ssd1306_SetCursor(0, 0);
  sprintf(string_display,"T0=%.1f C    ",AverageSensorI2C);
  ssd1306_WriteString(string_display,Font_11x18, White);

  // Analog sensor readings
  ssd1306_SetCursor(0, 22);
  sprintf(string_display,"T1=%.1f C    ",NTC_Temperature[0]);
  ssd1306_WriteString(string_display,Font_11x18, White);
  ssd1306_SetCursor(0, 44);
  sprintf(string_display,"T2=%.1f C    ",NTC_Temperature[1]);
  ssd1306_WriteString(string_display,Font_11x18, White);

  // Copy the screen buffer to the display
  ssd1306_UpdateScreen(&hi2c3);
#endif

  DEBUG_EVENT_DISPLAY();
  __asm("NOP");  // place a breakpoint here to stop at the end of an iteration
}

/* Sampling timer (TIM6) interrupt callback */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  DEBUG_EVENT_TIMER();
  if (htim==&htim6)
  {
    HAL_ResumeTick();                          // enable SysTick after wake-up
    statechart_raise_ev_GetSample(&sc_handle); // raise event Ev_GetSample in the state machine
  }
}

/* ADC conversion (DMA transfer) completed callback */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  DEBUG_EVENT_ADC();
  HAL_ResumeTick();                               // enable SysTick after wake-up
  statechart_raise_ev_ADCSampleReady(&sc_handle); // raise event Ev_ADCSampleReady in the state machine
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C3_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
#ifdef ENABLE_EVENT_RECORDER
  EventRecorderInitialize(EventRecordAll, 1);  // initialize and start Event Recorder
#endif

  HAL_GPIO_WritePin(ADC_DRIVE_GPIO_Port,ADC_DRIVE_Pin,GPIO_PIN_SET);  // excite the analog sensor dividers

#ifndef DISABLE_DISPLAY
  HAL_GPIO_WritePin(OLED_VCC_GPIO_Port,OLED_VCC_Pin,GPIO_PIN_SET);    // power the OLED display
  HAL_Delay(1000);                                                    // wait until the OLED supply settles

  ssd1306_Init(&hi2c3);
  ssd1306_SetCursor(0,0);
  ssd1306_WriteString("KTU",Font_11x18, White);
  ssd1306_SetCursor(0, 26);
  ssd1306_WriteString("Low Power",Font_11x18, White);
  ssd1306_SetCursor(0, 44);
  ssd1306_WriteString("Embedded",Font_11x18, White);
  ssd1306_UpdateScreen(&hi2c3);  // copy the screen buffer to the display
#endif

#ifndef DISABLE_PROCESSING
  statechart_init(&sc_handle);   // initialize the state machine
  statechart_enter(&sc_handle);  // start the state machine

  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)  // start the sampling timer TIM6
  {
    Error_Handler();
  }
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  DEBUG_EVENT_START();
  while (1)
  {
#ifdef ENABLE_SLEEP
    HAL_SuspendTick();  // stop SysTick, otherwise it wakes the CPU every 1 ms
#endif
    DEBUG_EVENT_GOSLEEP();

#ifdef ENABLE_SLEEP
    // SLEEP mode: the TIM6 and ADC/DMA interrupts wake the CPU, their callbacks resume SysTick
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
#endif

    /* TODO (student, problem task 3): use STOP0 mode instead of SLEEP.
     *  - Enter STOP0 with HAL_PWREx_EnterSTOP0Mode(PWR_STOPENTRY_WFI).
     *  - TIM6 stops in STOP mode: wake the MCU with the RTC wake-up timer clocked
     *    from the LSI oscillator (enable the RTC and its wake-up interrupt in CubeMX).
     *  - The PLL is switched off in STOP mode: after wake-up, restore the system
     *    clock (e.g. call SystemClock_Config()) before using the peripherals.
     *  - Include the wake-up time in the timing model and compare the measured current.
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x10909CEC;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 40000;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 400;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, ADC_DRIVE_Pin|LED1_Pin|LED2_Pin|OLED_VCC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OscTestPoint_GPIO_Port, OscTestPoint_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ADC_DRIVE_Pin LED1_Pin LED2_Pin OLED_VCC_Pin */
  GPIO_InitStruct.Pin = ADC_DRIVE_Pin|LED1_Pin|LED2_Pin|OLED_VCC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD4_Pin */
  GPIO_InitStruct.Pin = LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SWITCH_S1_Pin */
  GPIO_InitStruct.Pin = SWITCH_S1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SWITCH_S1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OscTestPoint_Pin */
  GPIO_InitStruct.Pin = OscTestPoint_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OscTestPoint_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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
