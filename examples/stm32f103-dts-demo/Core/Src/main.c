/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : UART DMA + LwRB stress test
  ******************************************************************************
  */
/* USER CODE END Header */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "yi_device.h"
#include "yi_generated.h"
#include "yi_led.h"
#include "yi_log.h"
#include "yi_soft_timer.h"
#include "yi_system.h"
#include "yi_uart_dma_lwrb.h"
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_STRESS_DMA_SIZE      512U
#define UART_STRESS_RING_SIZE     8192U
#define UART_STRESS_READ_SIZE     256U
#define UART_STRESS_LOG_PERIOD_MS 1000U
#define UART_STRESS_IDLE_MS       3000U
#define UART_STRESS_BAUDRATE      921600U
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static yi_uart_dma_lwrb_t uart_rx;
static uint8_t uart_dma_buffer[UART_STRESS_DMA_SIZE];
static uint8_t uart_ring_buffer[UART_STRESS_RING_SIZE];
static uint8_t uart_read_buffer[UART_STRESS_READ_SIZE];
static yi_soft_timer_t blink_timer;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void Error_Handler(void);

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void blink_timer_expired(yi_soft_timer_t *timer, void *user_data)
{
  (void)timer;
  (void)yi_led_toggle((yi_device_t *)user_data);
}

static char *append_text(char *out, const char *text)
{
  while(*text != '\0')
  {
    *out++ = *text++;
  }
  return out;
}

static char *append_u32(char *out, uint32_t value)
{
  char digits[10];
  uint32_t count = 0U;

  do
  {
    digits[count++] = (char)('0' + (value % 10U));
    value /= 10U;
  } while(value != 0U);

  while(count > 0U)
  {
    *out++ = digits[--count];
  }
  return out;
}

static void report_progress(uint32_t elapsed_ms,
                            uint32_t bytes,
                            uint32_t errors,
                            uint32_t overrun)
{
  char message[96];
  char *out = message;
  uint32_t bytes_per_second = (elapsed_ms > 0U) ?
    ((bytes * 1000U) / elapsed_ms) : 0U;

  out = append_text(out, "UART DMA LwRB rx=");
  out = append_u32(out, bytes);
  out = append_text(out, " B rate=");
  out = append_u32(out, bytes_per_second);
  out = append_text(out, " B/s err=");
  out = append_u32(out, errors);
  out = append_text(out, " overrun=");
  out = append_u32(out, overrun);
  *out = '\0';
  (void)yi_log_info(message);
}
/* USER CODE END 0 */

int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  if(yi_system_init() != 0)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN 2 */
  if(yi_device_init_all() != 0)
  {
    Error_Handler();
  }

  yi_device_t *led = YI_DT_GET(LED1);
  yi_device_t *uart = YI_DT_GET(USART1);
  uint32_t expected = 0U;
  uint32_t total = 0U;
  uint32_t errors = 0U;
  uint32_t start_tick = yi_system_uptime_ms();
  uint32_t last_log_tick = start_tick;
  uint32_t last_rx_tick = start_tick;
  uint8_t saw_data = 0U;

  if((led == NULL) || (uart == NULL) ||
     (yi_uart_dma_lwrb_start(&uart_rx,
                             uart,
                             uart_dma_buffer,
                             sizeof(uart_dma_buffer),
                             uart_ring_buffer,
                             sizeof(uart_ring_buffer)) != 0))
  {
    Error_Handler();
  }

  yi_soft_timer_init(&blink_timer, blink_timer_expired, led);
  if(yi_soft_timer_start(&blink_timer, 100U, 100U) != 0)
  {
    Error_Handler();
  }

  (void)yi_log_info("UART DMA LwRB stress ready: USART1 921600 baud");
  (void)yi_log_info("Send byte stream pattern byte[n] = n & 0xff");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    yi_soft_timer_process();

    uint32_t read = yi_uart_dma_lwrb_read(&uart_rx,
                                          uart_read_buffer,
                                          sizeof(uart_read_buffer));
    if(read > 0U)
    {
      saw_data = 1U;
      last_rx_tick = yi_system_uptime_ms();
      for(uint32_t i = 0U; i < read; i++)
      {
        uint8_t want = (uint8_t)expected;
        if(uart_read_buffer[i] != want)
        {
          errors++;
          expected = (uint32_t)uart_read_buffer[i] + 1U;
        }
        else
        {
          expected++;
        }
      }
      total += read;
    }

    uint32_t now = yi_system_uptime_ms();
    if((now - last_log_tick) >= UART_STRESS_LOG_PERIOD_MS)
    {
      last_log_tick = now;
      report_progress(now - start_tick,
                      total,
                      errors,
                      yi_uart_dma_lwrb_overrun(&uart_rx, false));
    }

    if((saw_data != 0U) && ((now - last_rx_tick) >= UART_STRESS_IDLE_MS))
    {
      report_progress(now - start_tick,
                      total,
                      errors,
                      yi_uart_dma_lwrb_overrun(&uart_rx, false));
      (void)yi_log_info("UART DMA LwRB stress idle");
      saw_data = 0U;
    }
  }
  /* USER CODE END WHILE */
}

void Error_Handler(void)
{
  yi_system_irq_lock();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
