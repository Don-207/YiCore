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
#include "yi_spi.h"
#include "yi_led.h"
#include "yi_log.h"
#include "yi_generated.h"
#include "yi_system.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AT24C02_ADDRESS       0x50U
#define AT24C02_WRITE_TIME_MS 10U
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

static uint8_t at24c02_test_pattern(uint16_t address, uint16_t round)
{
  return (uint8_t)(((address * 37U) + (round * 53U)) ^ 0xA5U);
}

static int at24c02_write_page(yi_device_t *i2c, uint8_t memory_address,
                              uint16_t round)
{
  uint8_t frame[AT24C02_PAGE_SIZE + 1U];
  frame[0] = memory_address;
  for(uint16_t index = 0U; index < AT24C02_PAGE_SIZE; index++)
  {
    frame[index + 1U] = at24c02_test_pattern(memory_address + index, round);
  }
  int result = yi_i2c_master_write(i2c, AT24C02_ADDRESS,
                                   frame, sizeof(frame), 20U);
  if(result == 0)
  {
    uint8_t probe;
    uint32_t started = yi_system_uptime_ms();
    do
    {
      if(yi_i2c_master_read(i2c, AT24C02_ADDRESS, &probe, 1U, 5U) == 0)
      {
        return 0;
      }
    } while((uint32_t)(yi_system_uptime_ms() - started) <
            (AT24C02_WRITE_TIME_MS * 2U));
    return -1;
  }
  return result;
}

static int at24c02_read(yi_device_t *i2c, uint8_t memory_address,
                        uint8_t *data, uint16_t length)
{
  return yi_i2c_master_write_read(i2c, AT24C02_ADDRESS,
                                  &memory_address, 1U,
                                  data, length, 50U);
}

static int at24c02_stress_test(yi_device_t *i2c)
{
  uint8_t readback[AT24C02_READ_CHUNK];

  if(!yi_device_is_ready(i2c)) { return -1; }
  for(uint16_t round = 0U; round < AT24C02_STRESS_ROUNDS; round++)
  {
    for(uint16_t address = 0U; address < AT24C02_CAPACITY;
        address += AT24C02_PAGE_SIZE)
    {
      if(at24c02_write_page(i2c, (uint8_t)address, round) != 0)
      {
        (void)yi_log_error("AT24C02 stress failure: page write or ACK polling");
        return -1;
      }
    }
    for(uint16_t address = 0U; address < AT24C02_CAPACITY;
        address += AT24C02_READ_CHUNK)
    {
      if(at24c02_read(i2c, (uint8_t)address,
                      readback, sizeof(readback)) != 0)
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

static int w25q64_command(yi_device_t *spi, yi_device_t *cs,
                          const uint8_t *tx, uint8_t *rx,
                          uint16_t length, uint32_t timeout_ms)
{
  int result;
  if(yi_gpio_set(cs, YI_GPIO_LOW) != 0) { return -1; }
  result = yi_spi_transceive(spi, tx, rx, length, timeout_ms);
  if(yi_gpio_set(cs, YI_GPIO_HIGH) != 0) { result = -1; }
  return result;
}

static int w25q64_write_enable(yi_device_t *spi, yi_device_t *cs)
{
  const uint8_t command = 0x06U;
  return w25q64_command(spi, cs, &command, NULL, 1U, 10U);
}

static int w25q64_wait_ready(yi_device_t *spi, yi_device_t *cs,
                             uint32_t timeout_ms)
{
  const uint8_t tx[2] = { 0x05U, 0xFFU };
  uint8_t rx[2];
  uint32_t started = yi_system_uptime_ms();
  do
  {
    if(w25q64_command(spi, cs, tx, rx, sizeof(tx), 10U) != 0) { return -1; }
    if((rx[1] & 0x01U) == 0U) { return 0; }
  } while((uint32_t)(yi_system_uptime_ms() - started) < timeout_ms);
  return -1;
}

static uint8_t w25q64_pattern(uint32_t address, uint16_t round)
{
  return (uint8_t)(((address * 37UL) + ((uint32_t)round * 53UL)) ^
                   (address >> 8) ^ 0xA5U);
}

static int w25q64_erase_test_sector(yi_device_t *spi, yi_device_t *cs)
{
  const uint8_t command[4] = {
    0x20U, (uint8_t)(W25Q64_TEST_ADDRESS >> 16),
    (uint8_t)(W25Q64_TEST_ADDRESS >> 8), (uint8_t)W25Q64_TEST_ADDRESS
  };
  if(w25q64_write_enable(spi, cs) != 0) { return -1; }
  if(w25q64_command(spi, cs, command, NULL, sizeof(command), 20U) != 0)
  {
    return -1;
  }
  return w25q64_wait_ready(spi, cs, 5000U);
}

static int w25q64_program_page(yi_device_t *spi, yi_device_t *cs,
                               uint32_t address, uint16_t round)
{
  uint8_t frame[W25Q64_PAGE_SIZE + 4U];
  frame[0] = 0x02U;
  frame[1] = (uint8_t)(address >> 16);
  frame[2] = (uint8_t)(address >> 8);
  frame[3] = (uint8_t)address;
  for(uint16_t index = 0U; index < W25Q64_PAGE_SIZE; index++)
  {
    frame[index + 4U] = w25q64_pattern(address + index, round);
  }
  if(w25q64_write_enable(spi, cs) != 0) { return -1; }
  if(w25q64_command(spi, cs, frame, NULL, sizeof(frame), 100U) != 0)
  {
    return -1;
  }
  return w25q64_wait_ready(spi, cs, 100U);
}

static int w25q64_verify_page(yi_device_t *spi, yi_device_t *cs,
                              uint32_t address, uint16_t round)
{
  uint8_t tx[W25Q64_PAGE_SIZE + 4U] = { 0U };
  uint8_t rx[W25Q64_PAGE_SIZE + 4U];
  tx[0] = 0x03U;
  tx[1] = (uint8_t)(address >> 16);
  tx[2] = (uint8_t)(address >> 8);
  tx[3] = (uint8_t)address;
  if(w25q64_command(spi, cs, tx, rx, sizeof(tx), 100U) != 0) { return -1; }
  for(uint16_t index = 0U; index < W25Q64_PAGE_SIZE; index++)
  {
    if(rx[index + 4U] != w25q64_pattern(address + index, round)) { return -1; }
  }
  return 0;
}

static int w25q64_stress_test(yi_device_t *spi, yi_device_t *cs)
{
  const uint8_t id_tx[4] = { 0x9FU, 0xFFU, 0xFFU, 0xFFU };
  uint8_t id_rx[4];
  if(!yi_device_is_ready(spi) || !yi_device_is_ready(cs) ||
     (yi_gpio_set(cs, YI_GPIO_HIGH) != 0)) { return -1; }
  if(w25q64_command(spi, cs, id_tx, id_rx, sizeof(id_tx), 20U) != 0 ||
     id_rx[1] != 0xEFU || id_rx[2] != 0x40U || id_rx[3] != 0x17U)
  {
    (void)yi_log_error("W25Q64 JEDEC ID mismatch");
    return -1;
  }
  (void)yi_log_info("W25Q64 JEDEC ID EF4017 detected");
  for(uint16_t round = 0U; round < W25Q64_STRESS_ROUNDS; round++)
  {
    if(w25q64_erase_test_sector(spi, cs) != 0) { return -1; }
    for(uint32_t offset = 0U; offset < W25Q64_SECTOR_SIZE;
        offset += W25Q64_PAGE_SIZE)
    {
      if(w25q64_program_page(spi, cs, W25Q64_TEST_ADDRESS + offset, round) != 0)
      {
        return -1;
      }
    }
    for(uint32_t offset = 0U; offset < W25Q64_SECTOR_SIZE;
        offset += W25Q64_PAGE_SIZE)
    {
      if(w25q64_verify_page(spi, cs, W25Q64_TEST_ADDRESS + offset, round) != 0)
      {
        return -1;
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
	yi_device_t *soft_i2c = YI_DT_GET(SOFT_I2C0);
	yi_device_t *spi = YI_DT_GET(SPI1);
	yi_device_t *spi_cs = YI_DT_GET(SPI1_CS);
	uint32_t last_key_tick = 0U;
	uint32_t last_hello_tick = yi_system_uptime_ms();
	yi_gpio_callback_init(&key0_callback, key0_pressed, YI_GPIO_PIN(13));
	if((key == NULL) || (yi_gpio_add_callback(key, &key0_callback) != 0))
	{
		Error_Handler();
	}
	(void)yi_log_info("YiCore initialized");
	(void)soft_i2c;
	(void)yi_log_info("W25Q64 HW-SPI1 stress started: 18 MHz, sector 0x7FF000, 100 rounds");
	if(w25q64_stress_test(spi, spi_cs) == 0)
	{
		(void)yi_led_on(led1);
		(void)yi_log_info("W25Q64 HW-SPI1 stress test passed");
	}
	else
	{
		(void)yi_led_off(led1);
		(void)yi_log_error("W25Q64 HW-SPI1 stress test failed");
	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
		if((now - last_hello_tick) >= 500U)
		{
			last_hello_tick = now;
			yi_led_toggle(led1);
			(void)yi_log_info("Hello YiCore");
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
