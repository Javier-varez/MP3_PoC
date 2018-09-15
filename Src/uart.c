/*
 * uart.c
 *
 *  Created on: 15 sept. 2018
 *      Author: javi
 */

#include "uart.h"

static UART_HandleTypeDef huart;

static UART_Result_t uart_init_phy() {
	GPIO_InitTypeDef gpio;

	__HAL_RCC_GPIOA_CLK_ENABLE();

	// TX Pin USART 2
	gpio.Pin = GPIO_PIN_2;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_MEDIUM;
	gpio.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOA, &gpio);

	return RESULT_OK;
}

UART_Result_t uart_init() {
	if (uart_init_phy() != RESULT_OK) return PHY_FAILURE;

	__HAL_RCC_USART2_CLK_ENABLE();
	huart.Instance = USART2;
	huart.Init.Mode = UART_MODE_TX;
	huart.Init.BaudRate = 115200;
	huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart.Init.Parity = UART_PARITY_NONE;
	huart.Init.StopBits = UART_STOPBITS_1;
	huart.Init.WordLength = UART_WORDLENGTH_8B;
	if (HAL_UART_Init(&huart) != HAL_OK) return UART_FAILURE;

	return RESULT_OK;
}

UART_Result_t uart_write(const void *buf, size_t len) {
	if (HAL_UART_Transmit(&huart, (uint8_t*)buf,
			len, UART_TIMEOUT_MILLISECONDS) != HAL_OK) {
		return UART_FAILURE;
	}

	return RESULT_OK;
}
