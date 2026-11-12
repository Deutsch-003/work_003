/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "oledfont.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define red_Pin GPIO_PIN_5
#define red_GPIO_Port GPIOA

#define yellow_Pin GPIO_PIN_6
#define yellow_GPIO_Port GPIOA

#define green_Pin GPIO_PIN_7
#define green_GPIO_Port GPIOA
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile int a = 0;
volatile int run = 1;      //run是A8 表运行还是暂停
volatile int order = 1;    //order是A9 表示正向还是反向

volatile int time = 5;      //初始化时间间隔，为了方便简化了  5表示间隔500ms
volatile int busy = 0;      //检测通道是否繁忙
volatile int transmit = 0;   //检测是否有数据包要发送 1为真 0为假
volatile int receive = 0;    //检测是否有数据包要接受并解析
volatile char messeaget[80];       //t为发送数组
volatile uint8_t  messeager[6];    //r为接收数组;

char oled[30];    //显示哪个oled亮

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)   //时钟系统，每隔100ms触发一次
{
  if (htim == &htim3 )      //检查通道
    if (run == 1) {         //如果是运行

      if (a > (3*time-1)) {    //设置范围0--3倍时间间隔
        a = 0;
      }
      if (a < 0) {
        a= (3*time-1);
      }

      if (a == 0 ){
        HAL_GPIO_WritePin(red_GPIO_Port, red_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(yellow_GPIO_Port, yellow_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(green_GPIO_Port, green_Pin, GPIO_PIN_RESET);
      }
      else if (a == (time)) {
        HAL_GPIO_WritePin(red_GPIO_Port, red_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(yellow_GPIO_Port, yellow_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(green_GPIO_Port, green_Pin, GPIO_PIN_RESET);
      }
      else if (a == (2*time) ) {
        HAL_GPIO_WritePin(red_GPIO_Port, red_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(yellow_GPIO_Port, yellow_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(green_GPIO_Port, green_Pin, GPIO_PIN_SET);
      }
      //例如时间间隔为500ms ,由于固定每100ms断一次, 到0 5 10 时对应三盏灯 ,如果为15则改为0 ,确保每个间隔500ms
      //当然，时间间隔可通过串口调节

      int b = a/time;
      sprintf(oled,"oled  : %d",b);   //反向运算，显示哪个灯亮

      if (order == 1) {
        a++;
      }
      else if (order == 0) {   //这部分是正向走还是反向走
        a-- ;
      }
    }
  };
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
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  HAL_GPIO_WritePin( GPIOA , GPIO_PIN_5 , GPIO_PIN_RESET );
  HAL_GPIO_WritePin( GPIOA , GPIO_PIN_6 , GPIO_PIN_RESET );  //初始灯
  HAL_GPIO_WritePin( GPIOA , GPIO_PIN_7 , GPIO_PIN_RESET );

  HAL_TIM_Base_Start_IT( &htim3);
  OLED_Init() ;
  OLED_Clear() ;

  char zhuangtai1[10] = "run  ";
  char zhuangtai0[10] = "stop ";
  char shunxu1[10] = "forward ";
  char shunxu0[10] = "break   ";   //显示OLED的数组用的

  char zhuangtai[20];     //状态汇总数组
  char shunxu[20];        //顺序汇总数组
  char step[30];          //延时显示数组
  int stept = 500;        //初始延时

  HAL_UART_Receive_DMA(&huart2,(uint8_t *)messeager ,6 );   //开启DMA中断接收

  while (1)
  {
     if (receive == 1) {   //是否有数据待处理

       receive = 0;     //确认收到，可以继续发了

       int all = (messeager[0] + messeager[1] + messeager[2] + messeager[3] + messeager[4]); //算和

       if (all == messeager[5] && messeager [0] == 0xAA) {

         uint8_t messager2 = messeager [2];
         run = messeager[1];
         order = !messager2 ;
         stept = messeager [3] | (messeager [4] << 8);
         time = (stept / 100);    //解析数据包

         transmit = 1;   //解析完了，准备发送结果
       }
     }


    if (run == 1) {
      sprintf(zhuangtai    ,"state : %s",zhuangtai1  );
    }
    else if (run == 0) {
      sprintf(zhuangtai    ,"state : %s",zhuangtai0  );
    }

    if (order == 1) {
      sprintf(shunxu   ,"order : %s",shunxu1 );
    }
    else if (order == 0) {
      sprintf(shunxu   ,"order : %s",shunxu0 );
    }
    //解析顺序，OLED用

    sprintf(step   ,"step  : %dms  ",stept );   //塞进数组

    // char debug[30];
    // sprintf(debug, "%02X %02X %02X", messeager[0], messeager[1], messeager[2]);
    // OLED_ShowString( 0,7,debug ,12,0);
    //调试用的

    OLED_ShowString( 0,0,shunxu ,12,0);
    OLED_ShowString( 0,2,zhuangtai,12,0);
    OLED_ShowString( 0,4,step,12,0);
    OLED_ShowString( 0,6,oled,12,0);        //oled展示

    if (busy == 0 && transmit == 1) {   //如果通道空闲，且有数据要发送
      busy = 1;                          //通道被占用
      sprintf(messeaget,"%s , %s , step:%dms , oled:1,2,3\r\n", shunxu , zhuangtai , stept );  //打包数组
      HAL_UART_Transmit_DMA(&huart2,(uint8_t *)messeaget ,sizeof(messeaget)  );            //DMA发送
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == GPIO_PIN_8 ) {

    static uint32_t last = 0;
    uint32_t now = HAL_GetTick();

    if ((now - last) > 20) {
      run = !run ;
      transmit = 1;
      last = now;
    }
  }
  if (GPIO_Pin == GPIO_PIN_9 ) {

    static uint32_t last = 0;
    uint32_t now = HAL_GetTick();

    if ((now - last) > 20) {
      order = !order ;
      transmit = 1;
      last = now;
    }
  }
}
//上面两个都是防抖-------------------------------


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {   //接收回调
  if (huart == &huart2) {    //通道正确
    receive = 1;             //有一个接收数据待处理
    HAL_UART_Receive_DMA( &huart2,(uint8_t *)messeager ,sizeof(messeager) );   //重新开启DMA
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart2) {    //同上
    transmit = 0;                //发完数据了 改为没有要发送的东西，并且通道空闲了
    busy = 0;
  }
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
