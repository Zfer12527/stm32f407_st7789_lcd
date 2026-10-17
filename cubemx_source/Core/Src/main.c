#include "main.h"  // 引入主程序头文件，获取 HAL 接口、LCD 引脚宏和函数声明。
SPI_HandleTypeDef hspi1;  // 定义 SPI1 的全局句柄，保存外设实例、配置参数和传输状态。
void SystemClock_Config(void);  // 声明系统时钟配置函数，便于 main 在其定义之前调用。
static void MX_SPI1_Init(void);  // 声明仅在本文件内使用的 SPI1 初始化函数。
static void MX_TIM2_Init(void);  // 声明 PA1 背光 PWM 初始化函数。
int main(void)  // 程序主入口，由启动代码完成底层启动后调用。
{  // 开始当前函数或控制流程的代码块。
  HAL_Init();  // 初始化 HAL 库、毫秒系统滴答和底层 MSP 支持。
  SystemClock_Config();  // 配置系统时钟；按 8 MHz 外部晶振生成 168 MHz 系统时钟。
  MX_GPIO_Init();  // 开启 GPIO 时钟，设置 LCD 控制脚和未使用引脚。
  MX_TIM2_Init();  // 初始化 TIM2_CH2 的 5 kHz PWM，背光先保持关闭。
  MX_SPI1_Init();  // 初始化 SPI1，并通过 MSP 配置 PA5 时钟脚和 PB5 数据脚。
  extern void user_main(void);  // 声明在 app.c 中定义的应用入口函数。
  user_main();  // 进入 LVGL 示例及其主循环；正常运行时不会返回。
  while (1)  // 应用入口意外返回时停留在此兜底循环。
  {  // 开始当前函数或控制流程的代码块。
  }  // 结束当前函数或控制流程的代码块。
}  // 结束当前函数或控制流程的代码块。

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)  // 定义系统时钟配置函数。
{  // 开始当前函数或控制流程的代码块。
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};  // 将振荡器和 PLL 配置结构体清零，避免未初始化字段。
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};  // 将系统时钟和总线分频配置结构体清零。

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();  // 开启电源控制模块时钟，以便配置内部稳压器。
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  // 设置内部稳压器为 Scale 1，支持当前 168 MHz 工作频率。

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;  // 选择本次需要配置的振荡器类型：外部高速时钟 HSE。
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;  // 开启外部高速晶振，当前工程按 8 MHz 晶振配置。
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;  // 开启主 PLL，用于产生系统所需的高频时钟。
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;  // 选择外部高速晶振 HSE 作为 PLL 的输入来源。
  RCC_OscInitStruct.PLL.PLLM = 4;  // PLL 输入先除以 4：8 MHz / 4 = 2 MHz。
  RCC_OscInitStruct.PLL.PLLN = 168;  // PLL VCO 再乘以 168：2 MHz × 168 = 336 MHz。
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  // 系统时钟输出除以 2：336 MHz / 2 = 168 MHz。
  RCC_OscInitStruct.PLL.PLLQ = 4;  // PLLQ 辅助输出除以 4，当前配置对应 84 MHz。
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)  // 应用振荡器和 PLL 参数，并检查启动、锁定是否成功。
  {  // 开始当前函数或控制流程的代码块。
    Error_Handler();  // 初始化失败时进入错误处理函数，停止继续执行。
  }  // 结束当前函数或控制流程的代码块。

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK  // 指定本次配置包含 AHB 时钟 HCLK 和系统时钟 SYSCLK。
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;  // 继续指定需要配置 APB1 时钟 PCLK1 和 APB2 时钟 PCLK2。
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;  // 选择 PLL 的系统时钟输出作为 SYSCLK 来源。
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;  // AHB 不分频，HCLK 和 CPU 时钟均为 168 MHz。
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  // APB1 总线按 HCLK 的四分之一运行，即 42 MHz。
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  // APB2 总线按 HCLK 的二分之一运行，即 84 MHz，供 SPI1 使用。

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)  // 设置总线时钟并将 Flash 等待周期设为 5，检查配置结果。
  {  // 开始当前函数或控制流程的代码块。
    Error_Handler();  // 初始化失败时进入错误处理函数，停止继续执行。
  }  // 结束当前函数或控制流程的代码块。

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();  // 开启 HSE 时钟安全监测，检测到 HSE 故障时触发相应处理。
}  // 结束当前函数或控制流程的代码块。

/**
  * @brief SPI1 Initialization Function
  */
static void MX_SPI1_Init(void)  // 定义 SPI1 初始化函数，其作用域限于当前文件。
{  // 开始当前函数或控制流程的代码块。
  hspi1.Instance = SPI1;  // 将句柄绑定到 SPI1 硬件外设。
  hspi1.Init.Mode = SPI_MODE_MASTER;  // 设置 MCU 为 SPI 主机，由 MCU 产生串行时钟。
  hspi1.Init.Direction = SPI_DIRECTION_1LINE;  // 选择单数据线半双工模式；LCD 驱动通过该数据线向屏幕发送数据。
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;  // 每个 SPI 数据帧包含 8 位，即一个字节。
  /* SPI mode 0: idle low, sample MOSI on the rising edge. */
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;  // 设置 CPOL=0，使 SPI 时钟空闲时保持低电平。
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;  // 设置 CPHA=0，在第一个时钟边沿即上升沿采样，组合为模式 0。
  hspi1.Init.NSS = SPI_NSS_SOFT;  // 使用软件管理内部 NSS；屏幕的外部 CS 由 PA4 单独控制。
  /* APB2 = 84 MHz; 10.5 MHz for LVGL partial-area transfers. */
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;  // 将 84 MHz 的 APB2 时钟除以 8，得到 10.5 MHz SPI 时钟。
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;  // 每个字节从最高位开始发送。
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;  // 关闭 TI 帧格式，使用常规 SPI 帧格式。
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // 关闭 SPI 硬件 CRC，当前 LCD 通信不使用 CRC 校验。
  hspi1.Init.CRCPolynomial = 10;  // 填写 CRC 多项式参数；由于 CRC 已关闭，该参数不参与当前传输。
  if (HAL_SPI_Init(&hspi1) != HAL_OK)  // 应用 SPI 参数并调用 MSP 配置外设时钟及引脚，检查初始化结果。
  {  // 开始当前函数或控制流程的代码块。
    Error_Handler();  // 初始化失败时进入错误处理函数，停止继续执行。
  }  // 结束当前函数或控制流程的代码块。
}  // 结束当前函数或控制流程的代码块。

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)  // 定义 GPIO 初始化函数。
{  // 开始当前函数或控制流程的代码块。
  GPIO_InitTypeDef GPIO_InitStruct = {0};  // 清零 GPIO 配置结构体，随后复用它配置不同引脚组。
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();  // 开启 GPIOE 端口时钟，允许访问其配置寄存器。
  __HAL_RCC_GPIOC_CLK_ENABLE();  // 开启 GPIOC 端口时钟，允许访问其配置寄存器。
  __HAL_RCC_GPIOH_CLK_ENABLE();  // 开启 GPIOH 端口时钟，允许访问其配置寄存器。
  __HAL_RCC_GPIOA_CLK_ENABLE();  // 开启 GPIOA 端口时钟，LCD 控制脚和 SPI1 时钟脚位于该端口。
  __HAL_RCC_GPIOB_CLK_ENABLE();  // 开启 GPIOB 端口时钟，SPI1 的 MOSI 数据脚 PB5 位于该端口。
  __HAL_RCC_GPIOD_CLK_ENABLE();  // 开启 GPIOD 端口时钟，允许访问其配置寄存器。

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_CS_Pin|LCD_DC_Pin|LCD_RST_Pin, GPIO_PIN_SET);  // 预置 PA4/CS、PA3/DC、PA2/RST 为高电平，使片选无效并释放复位。
  HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET);  // 预置 PA1/BL 为低电平；背光由引脚控制时先关闭，屏幕初始化后再开启。

  /*Configure GPIO pins : PE2 PE3 PE4 PE5
                           PE6 PE7 PE8 PE9
                           PE10 PE11 PE12 PE13
                           PE14 PE15 PE0 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5  // 开始设置本组引脚掩码，选择编号 2、3、4、5 的引脚。
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9  // 继续合并本组引脚掩码，加入编号 6、7、8、9 的引脚。
                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13  // 继续合并本组引脚掩码，加入编号 10、11、12、13 的引脚。
                          |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;  // 继续合并本组引脚掩码，加入编号 14、15、0、1 的引脚。
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // 将本组未使用引脚设为模拟模式，关闭其数字输入缓冲。
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // 关闭本组引脚的内部上拉和下拉电阻。
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);  // 将本组模拟模式和无上下拉配置应用到 GPIOE 的选定引脚。

  /*Configure GPIO pins : PC13 PC0 PC1 PC2
                           PC3 PC4 PC5 PC6
                           PC7 PC8 PC9 PC10
                           PC11 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2  // 开始设置本组引脚掩码，选择编号 13、0、1、2 的引脚。
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6  // 继续合并本组引脚掩码，加入编号 3、4、5、6 的引脚。
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10  // 继续合并本组引脚掩码，加入编号 7、8、9、10 的引脚。
                          |GPIO_PIN_11|GPIO_PIN_12;  // 继续合并本组引脚掩码，加入编号 11、12 的引脚。
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // 将本组未使用引脚设为模拟模式，关闭其数字输入缓冲。
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // 关闭本组引脚的内部上拉和下拉电阻。
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);  // 将本组模拟模式和无上下拉配置应用到 GPIOC 的选定引脚。

  /*Configure GPIO pins : PA0 PA6 PA7 PA8
                           PA9 PA10 PA11 PA12
                           PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8  // 开始设置本组引脚掩码，选择编号 0、6、7、8 的引脚。
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12  // 继续合并本组引脚掩码，加入编号 9、10、11、12 的引脚。
                          |GPIO_PIN_15;  // 继续合并本组引脚掩码，加入编号 15 的引脚。
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // 将本组未使用引脚设为模拟模式，关闭其数字输入缓冲。
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // 关闭本组引脚的内部上拉和下拉电阻。
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  // 将模拟配置应用到 GPIOA 的未使用引脚；PA7 不再输出 SPI 数据。

  /*Configure LCD control pins : PA2 PA3 PA4; PA1 uses TIM2 PWM. */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_DC_Pin|LCD_CS_Pin;  // 选择 PA2/RST、PA3/DC、PA4/CS；PA1 稍后配置为 PWM 复用功能。
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 将 LCD 控制引脚设置为推挽输出，能够主动输出高、低电平。
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // 关闭本组引脚的内部上拉和下拉电阻。
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // 选择低速输出边沿档位，适合背光、复位、片选等控制信号。
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  // 将推挽输出、无上下拉和低速边沿配置应用到 PA2～PA4。

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB11 PB12 PB13 PB14
                           PB15 PB3 PB4
                           PB6 PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10  // 开始设置本组引脚掩码，选择编号 0、1、2、10 的引脚。
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14  // 继续合并本组引脚掩码，加入编号 11、12、13、14 的引脚。
                          |GPIO_PIN_15|GPIO_PIN_3|GPIO_PIN_4  // 继续合并本组引脚掩码，加入编号 15、3、4 的引脚。
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;  // 继续合并本组引脚掩码，加入编号 6、7、8、9 的引脚。
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // 将本组未使用引脚设为模拟模式，关闭其数字输入缓冲。
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // 关闭本组引脚的内部上拉和下拉电阻。
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  // 将本组模拟配置应用到 GPIOB；PB5 留给后续 SPI1 MSP 初始化。

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD12 PD13 PD14 PD15
                           PD0 PD1 PD2 PD3
                           PD4 PD5 PD6 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11  // 开始设置本组引脚掩码，选择编号 8、9、10、11 的引脚。
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15  // 继续合并本组引脚掩码，加入编号 12、13、14、15 的引脚。
                          |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3  // 继续合并本组引脚掩码，加入编号 0、1、2、3 的引脚。
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;  // 继续合并本组引脚掩码，加入编号 4、5、6、7 的引脚。
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // 将本组未使用引脚设为模拟模式，关闭其数字输入缓冲。
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // 关闭本组引脚的内部上拉和下拉电阻。
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);  // 将本组模拟模式和无上下拉配置应用到 GPIOD 的选定引脚。

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}  // 结束当前函数或控制流程的代码块。

/* USER CODE BEGIN 4 */

static void MX_TIM2_Init(void)  // 配置 TIM2 通道 2，在 PA1 输出背光 PWM。
{  // PWM 初始化函数开始。
  GPIO_InitTypeDef gpio = {0};  // 清零 PWM 引脚配置结构体。
  __HAL_RCC_TIM2_CLK_ENABLE();  // 开启 TIM2 时钟；当前 APB1 定时器时钟为 84 MHz。
  __HAL_RCC_TIM2_FORCE_RESET();  // 复位 TIM2，确保从已知寄存器状态开始配置。
  __HAL_RCC_TIM2_RELEASE_RESET();  // 释放 TIM2 复位。
  TIM2->PSC = 83U;  // 84 MHz / (83 + 1) = 1 MHz 计数频率。
  TIM2->ARR = 199U;  // 1 MHz / (199 + 1) = 5 kHz PWM 频率。
  TIM2->CCMR1 = TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;  // 通道 2 使用 PWM 模式 1，并预装载比较值。
  TIM2->CCR2 = 0U;  // 初始占空比为 0%，屏幕初始化完成后再开启背光。
  TIM2->CCER = TIM_CCER_CC2E;  // 开启通道 2 输出，采用高电平有效极性。
  TIM2->EGR = TIM_EGR_UG;  // 产生更新事件，将分频和比较参数装入实际工作寄存器。
  TIM2->SR = 0U;  // 清除初始化产生的更新标志，本例不使用定时器中断。
  TIM2->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;  // 开启周期预装载并启动定时器计数。
  gpio.Pin = LCD_BL_Pin;  // 选择 PA1 背光控制引脚。
  gpio.Mode = GPIO_MODE_AF_PP;  // 使用定时器复用推挽输出，而非普通 GPIO 输出。
  gpio.Pull = GPIO_NOPULL;  // 不启用内部上拉、下拉。
  gpio.Speed = GPIO_SPEED_FREQ_LOW;  // 5 kHz 控制信号使用低速输出边沿即可。
  gpio.Alternate = GPIO_AF1_TIM2;  // 选择 AF1，将 PA1 连接到 TIM2_CH2。
  HAL_GPIO_Init(LCD_BL_GPIO_Port, &gpio);  // 应用 PA1 的 PWM 引脚配置。
}  // PWM 初始化函数结束。

void LCD_Backlight_SetDuty(uint8_t percent)  // 设置背光 PWM 占空比，允许 0～100%。
{  // 背光占空比设置函数开始。
  if (percent > 100U) percent = 100U;  // 将超过 100% 的输入限制到 100%。
  TIM2->CCR2 = ((TIM2->ARR + 1U) * percent) / 100U;  // 周期为 200 个计数，50% 对应 CCR2=100。
}  // 背光占空比设置函数结束。

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)  // 定义初始化失败等异常情况的统一错误处理函数。
{  // 开始当前函数或控制流程的代码块。
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();  // 关闭可屏蔽中断，防止错误状态下继续响应普通中断。
  while (1)  // 发生错误后停留在无限循环中，等待复位或调试器检查。
  {  // 开始当前函数或控制流程的代码块。
  }  // 结束当前函数或控制流程的代码块。
  /* USER CODE END Error_Handler_Debug */
}  // 结束当前函数或控制流程的代码块。
#ifdef USE_FULL_ASSERT  // 仅在启用 USE_FULL_ASSERT 时编译下面的完整断言回调。
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)  // 定义断言失败回调，file 是出错源文件名，line 是出错行号。
{  // 开始当前函数或控制流程的代码块。
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}  // 结束当前函数或控制流程的代码块。
#endif /* USE_FULL_ASSERT */  // 结束 USE_FULL_ASSERT 条件编译区域。
