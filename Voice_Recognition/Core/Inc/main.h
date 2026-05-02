/*
 * main.h
 *
 *  Created on: Apr 14, 2026
 *      Author: Jerem
 */


#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f4xx_hal.h"

#define BUFFER_SIZE 700
#define DECIMATION 32
#define BUFFER_PER_SECOND 45
#define TOTAL_PCM_SAMPLES (BUFFER_SIZE * BUFFER_PER_SECOND)
#define DC_ALPHA 32440



/* --- Constants and Configuration --- */
#define BUFFER_SIZE           700
#define DECIMATION            32
#define BUFFER_PER_SECOND     45
#define TOTAL_PCM_SAMPLES     (BUFFER_SIZE * BUFFER_PER_SECOND)
#define DC_ALPHA              32440

/* --- Global Variables (Extern) --- */
// Volatiles for Interrupt/Main sync
extern volatile int buffer_ready;
extern volatile int start_sampling;
extern volatile int buffer_count;
extern volatile int sampling_active;
extern volatile int uart_dma_busy;
extern volatile uint32_t last_press_time;
extern volatile int print_ready;
extern volatile int sample_index;

// Buffers
extern uint32_t bufferA[BUFFER_SIZE];
extern uint32_t bufferB[BUFFER_SIZE];
extern uint32_t *buffer_to_drain;
extern int16_t pcm[BUFFER_SIZE * BUFFER_PER_SECOND];

// Filter State Variables
extern uint32_t i1, i2, i3;
extern uint32_t prev1, prev2, prev3;
extern int bit_count;


/* --- Function Prototypes --- */
void SPI_Enable(void);
void DMA1_Enable(void);
void DMA2_Enable(void);
void GPIOA_Enable(void);
void GPIOB_Enable(void);
void TIM2_Enable(void);
void USART_Enable(void);

void DMA1_Stream6_IRQHandler(void);
void DMA2_Stream0_IRQHandler(void);
void EXTI9_5_IRQHandler(void);

void UART_DMA_send(uint16_t);
void delay_ms(uint32_t);
void pdm_to_pcm(uint32_t*);

#endif
