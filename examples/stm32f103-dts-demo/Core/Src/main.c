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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "yi_device.h"
#include "yi_gpio.h"
#include "yi_i2c.h"
#include "yi_eeprom.h"
#include "yi_flash.h"
#include "yi_led.h"
#include "yi_log.h"
#include "yi_soft_timer.h"
#include "yi_generated.h"
#include "yi_system.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AT24C02_CAPACITY      256U
#define AT24C02_PAGE_SIZE     8U
#define AT24C02_READ_CHUNK    16U
#define AT24C02_STRESS_ROUNDS 100U
#define W25Q64_TEST_ADDRESS    0x007FF000UL
#define W25Q64_SECTOR_SIZE     4096U
#define W25Q64_PAGE_SIZE       256U
#define W25Q64_STRESS_ROUNDS   100U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static volatile uint8_t key0_event;
static yi_gpio_callback_t key0_callback;
static yi_soft_timer_t hello_timer;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void Error_Handler(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void key0_pressed(yi_device_t *dev,
                         yi_gpio_callback_t *callback,
                         uint16_t pins)
{
  (void)dev;
  (void)callback;
  (void)pins;
  key0_event = 1U;
}

static void hello_timer_expired(yi_soft_timer_t *timer, void *user_data)
{
  yi_device_t *led = (yi_device_t *)user_data;

  (void)timer;
  (void)yi_led_toggle(led);
  (void)yi_log_info("Hello YiCore");
}

static uint8_t at24c02_test_pattern(uint16_t address, uint16_t round)
{
  return (uint8_t)(((address * 37U) + (round * 53U)) ^ 0xA5U);
}

static int at24c02_stress_test(yi_device_t *eeprom)
{
  uint8_t write_buffer[AT24C02_PAGE_SIZE];
  uint8_t readback[AT24C02_READ_CHUNK];

  if(!yi_device_is_ready(eeprom) ||
     (yi_eeprom_get_size(eeprom) != AT24C02_CAPACITY) ||
     (yi_eeprom_get_page_size(eeprom) != AT24C02_PAGE_SIZE))
  {
    return -1;
  }
  for(uint16_t round = 0U; round < AT24C02_STRESS_ROUNDS; round++)
  {
    for(uint16_t address = 0U; address < AT24C02_CAPACITY;
        address += AT24C02_PAGE_SIZE)
    {
      for(uint16_t index = 0U; index < sizeof(write_buffer); index++)
      {
        write_buffer[index] = at24c02_test_pattern(address + index, round);
      }
      if(yi_eeprom_write(eeprom, address,
                          write_buffer, sizeof(write_buffer)) != 0)
      {
        (void)yi_log_error("AT24C02 stress failure: page write or ACK polling");
        return -1;
      }
    }
    for(uint16_t address = 0U; address < AT24C02_CAPACITY;
        address += AT24C02_READ_CHUNK)
    {
      if(yi_eeprom_read(eeprom, address, readback, sizeof(readback)) != 0)
      {
        (void)yi_log_error("AT24C02 stress failure: sequential read");
        return -1;
      }
      for(uint16_t index = 0U; index < sizeof(readback); index++)
      {
        if(readback[index] != at24c02_test_pattern(address + index, round))
        {
          (void)yi_log_error("AT24C02 stress failure: data mismatch");
          return -1;
        }
      }
    }
    if(((round + 1U) % 10U) == 0U)
    {
      (void)yi_log_info("AT24C02 stress progress: 10 more rounds passed");
    }
  }
  return 0;
}

static uint8_t w25q64_pattern(uint32_t address, uint16_t round)
{
  return (uint8_t)(((address * 37UL) + ((uint32_t)round * 53UL)) ^
                   (address >> 8) ^ 0xA5U);
}

static int w25q64_stress_test(yi_device_t *flash)
{
  uint8_t write_buffer[W25Q64_PAGE_SIZE];
  uint8_t read_buffer[W25Q64_PAGE_SIZE];

  if(!yi_device_is_ready(flash) ||
     (yi_flash_get_size(flash) != 0x00800000U) ||
     (yi_flash_get_erase_block_size(flash) != W25Q64_SECTOR_SIZE))
  {
    return -1;
  }
  (void)yi_log_info("W25Q64 JEDEC ID EF4017 detected");
  for(uint16_t round = 0U; round < W25Q64_STRESS_ROUNDS; round++)
  {
    if(yi_flash_erase(flash, W25Q64_TEST_ADDRESS,
                      W25Q64_SECTOR_SIZE) != 0)
    {
      return -1;
    }
    for(uint32_t offset = 0U; offset < W25Q64_SECTOR_SIZE;
        offset += W25Q64_PAGE_SIZE)
    {
      for(uint16_t index = 0U; index < W25Q64_PAGE_SIZE; index++)
      {
        write_buffer[index] =
          w25q64_pattern(W25Q64_TEST_ADDRESS + offset + index, round);
      }
      if(yi_flash_write(flash, W25Q64_TEST_ADDRESS + offset,
                        write_buffer, sizeof(write_buffer)) != 0)
      {
        return -1;
      }
    }
    for(uint32_t offset = 0U; offset < W25Q64_SECTOR_SIZE;
        offset += W25Q64_PAGE_SIZE)
    {
      if(yi_flash_read(flash, W25Q64_TEST_ADDRESS + offset,
                       read_buffer, sizeof(read_buffer)) != 0)
      {
        return -1;
      }
      for(uint16_t index = 0U; index < W25Q64_PAGE_SIZE; index++)
      {
        if(read_buffer[index] !=
           w25q64_pattern(W25Q64_TEST_ADDRESS + offset + index, round))
        {
          return -1;
        }
      }
    }
    if(((round + 1U) % 10U) == 0U)
    {
      (void)yi_log_info("W25Q64 stress progress: 10 more rounds passed");
    }
  }
  return 0;
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
  if(yi_system_init() != 0)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */
	yi_device_init_all();
	yi_device_t *led0 = YI_DT_GET(LED0);
  yi_device_t *led1 = YI_DT_GET(LED1);

	yi_device_t *key = YI_DT_GET(KEY0);
	yi_device_t *at24c02 = YI_DT_GET(AT24C02);
	yi_device_t *w25q64 = YI_DT_GET(W25Q64);
	uint32_t last_key_tick = 0U;
	yi_gpio_callback_init(&key0_callback, key0_pressed, YI_GPIO_PIN(13));
	if((key == NULL) || (yi_gpio_add_callback(key, &key0_callback) != 0))
	{
		Error_Handler();
	}
	yi_soft_timer_init(&hello_timer, hello_timer_expired, led1);
	if(yi_soft_timer_start(&hello_timer, 500U, 500U) != 0)
	{
		Error_Handler();
	}
	(void)yi_log_info("YiCore initialized");
	(void)w25q64;
	(void)yi_log_info("AT24C02 stress started: 100 rounds");
	if(at24c02_stress_test(at24c02) == 0)
	{
		(void)yi_led_on(led1);
		(void)yi_log_info("AT24C02 stress test passed");
	}
	else
	{
		(void)yi_led_off(led1);
		(void)yi_log_error("AT24C02 stress test failed");
	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		yi_soft_timer_process();
		uint32_t now = yi_system_uptime_ms();
		if(key0_event != 0U)
		{
			key0_event = 0U;
			if((last_key_tick == 0U) || ((now - last_key_tick) >= 50U))
			{
				last_key_tick = now;
				yi_led_toggle(led0);
        
				(void)yi_log_info("PC13 key pressed");
			}
		}
		yi_system_delay_ms(10U);
  }
  /* USER CODE END 3 */
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
  yi_system_irq_lock();
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
