/*
 * uart.h
 *
 *  Created on: 15 sept. 2018
 *      Author: javi
 */

#ifndef UART_H_
#define UART_H_

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "main.h"

typedef enum {
	RESULT_OK,
	PHY_FAILURE,
	UART_FAILURE
} UART_Result_t;

#define UART_TIMEOUT_MILLISECONDS 1000

UART_Result_t uart_init();
UART_Result_t uart_write(const void *buf, size_t len);

#endif /* UART_H_ */
