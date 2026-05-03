/*
 * main.c
 * pa5 to clock pa6 to data
 *  Created on: Apr 14, 2026
 *      Author: Jerem
 */
#include "main.h"
#include "stm32f4xx_hal_conf.h"



#define BUFFER_SIZE 700
#define DECIMATION 32
#define BUFFER_PER_SECOND 45
#define TOTAL_PCM_SAMPLES (BUFFER_SIZE * BUFFER_PER_SECOND)
#define DC_ALPHA 32440

volatile int buffer_ready;
volatile int start_sampling;
volatile int buffer_count = 0;
volatile int sampling_active = 0;
volatile int uart_dma_busy = 0;
volatile uint32_t last_press_time = 0;
volatile int print_ready = 0;
volatile int sample_index = 0;
uint32_t bufferA[BUFFER_SIZE];
uint32_t bufferB[BUFFER_SIZE];
uint32_t *buffer_to_drain = NULL;
int16_t pcm[BUFFER_SIZE * BUFFER_PER_SECOND];

uint32_t i1 = 0, i2 = 0, i3 = 0;
uint32_t prev1 = 0, prev2 = 0, prev3 = 0;
int bit_count = 0;
static int32_t hp_x = 0;
static int32_t hp_y = 0;
static int32_t lp_out = 0;

/*
 * Pre-calculated integration phase bit sums.
 * 2^8 possible values for each stage
 * since we calculate each integration in 8
 * bits at a time
 * */


const int8_t LUT_i1[256] = {
    -8, -6, -6, -4, -6, -4, -4, -2, -6, -4, -4, -2, -4, -2, -2,  0,
    -6, -4, -4, -2, -4, -2, -2,  0, -4, -2, -2,  0, -2,  0,  0,  2,
    -6, -4, -4, -2, -4, -2, -2,  0, -4, -2, -2,  0, -2,  0,  0,  2,
    -4, -2, -2,  0, -2,  0,  0,  2, -2,  0,  0,  2,  0,  2,  2,  4,
    -6, -4, -4, -2, -4, -2, -2,  0, -4, -2, -2,  0, -2,  0,  0,  2,
    -4, -2, -2,  0, -2,  0,  0,  2, -2,  0,  0,  2,  0,  2,  2,  4,
    -4, -2, -2,  0, -2,  0,  0,  2, -2,  0,  0,  2,  0,  2,  2,  4,
    -2,  0,  0,  2,  0,  2,  2,  4,  0,  2,  2,  4,  2,  4,  4,  6,
    -6, -4, -4, -2, -4, -2, -2,  0, -4, -2, -2,  0, -2,  0,  0,  2,
    -4, -2, -2,  0, -2,  0,  0,  2, -2,  0,  0,  2,  0,  2,  2,  4,
    -4, -2, -2,  0, -2,  0,  0,  2, -2,  0,  0,  2,  0,  2,  2,  4,
    -2,  0,  0,  2,  0,  2,  2,  4,  0,  2,  2,  4,  2,  4,  4,  6,
    -4, -2, -2,  0, -2,  0,  0,  2, -2,  0,  0,  2,  0,  2,  2,  4,
    -2,  0,  0,  2,  0,  2,  2,  4,  0,  2,  2,  4,  2,  4,  4,  6,
    -2,  0,  0,  2,  0,  2,  2,  4,  0,  2,  2,  4,  2,  4,  4,  6,
    0,  2,  2,  4,  2,  4,  4,  6,  2,  4,  4,  6,  4,  6,  6,  8
};
const int16_t LUT_i2[256] = {
    -36, -20, -22, -6, -24, -8, -10, 6, -26, -10, -12, 4, -14, 2, 0, 16,
    -28, -12, -14, 2, -16, 0, -2, 14, -18, -2, -4, 12, -6, 10, 8, 24,
    -30, -14, -16, 0, -18, -2, -4, 12, -20, -4, -6, 10, -8, 8, 6, 22,
    -22, -6, -8, 8, -10, 6, 4, 20, -12, 4, 2, 18, 0, 16, 14, 30,
    -32, -16, -18, -2, -20, -4, -6, 10, -22, -6, -8, 8, -10, 6, 4, 20,
    -24, -8, -10, 6, -12, 4, 2, 18, -14, 2, 0, 16, -2, 14, 12, 28,
    -26, -10, -12, 4, -14, 2, 0, 16, -16, 0, -2, 14, -4, 12, 10, 26,
    -18, -2, -4, 12, -6, 10, 8, 24, -8, 8, 6, 22, 4, 20, 18, 34,
    -34, -18, -20, -4, -22, -6, -8, 8, -24, -8, -10, 6, -12, 4, 2, 18,
    -26, -10, -12, 4, -14, 2, 0, 16, -16, 0, -2, 14, -4, 12, 10, 26,
    -28, -12, -14, 2, -16, 0, -2, 14, -18, -2, -4, 12, -6, 10, 8, 24,
    -20, -4, -6, 10, -8, 8, 6, 22, -10, 6, 4, 20, 2, 18, 16, 32,
    -30, -14, -16, 0, -18, -2, -4, 12, -20, -4, -6, 10, -8, 8, 6, 22,
    -22, -6, -8, 8, -10, 6, 4, 20, -12, 4, 2, 18, 0, 16, 14, 30,
    -24, -8, -10, 6, -12, 4, 2, 18, -14, 2, 0, 16, -2, 14, 12, 28,
    -16, 0, -2, 14, -4, 12, 10, 26, -6, 10, 8, 24, 6, 22, 20, 36
};

const int16_t LUT_i3[256] = {
    -120, -48, -64, 8, -78, -6, -22, 50, -90, -18, -34, 38, -48, 24, 8, 80,
    -100, -28, -44, 28, -58, 14, -2, 70, -70, 2, -14, 58, -28, 44, 28, 100,
    -108, -36, -52, 20, -66, 6, -10, 62, -78, -6, -22, 50, -36, 36, 20, 92,
    -88, -16, -32, 40, -46, 26, 10, 82, -58, 14, -2, 70, -16, 56, 40, 112,
    -114, -42, -58, 14, -72, 0, -16, 56, -84, -12, -28, 44, -42, 30, 14, 86,
    -94, -22, -38, 34, -52, 20, 4, 76, -64, 8, -8, 64, -22, 50, 34, 106,
    -102, -30, -46, 26, -60, 12, -4, 68, -72, 0, -16, 56, -30, 42, 26, 98,
    -82, -10, -26, 46, -40, 32, 16, 88, -52, 20, 4, 76, -10, 62, 46, 118,
    -118, -46, -62, 10, -76, -4, -20, 52, -88, -16, -32, 40, -46, 26, 10, 82,
    -98, -26, -42, 30, -56, 16, 0, 72, -68, 4, -12, 60, -26, 46, 30, 102,
    -106, -34, -50, 22, -64, 8, -8, 64, -76, -4, -20, 52, -34, 38, 22, 94,
    -86, -14, -30, 42, -44, 28, 12, 84, -56, 16, 0, 72, -14, 58, 42, 114,
    -112, -40, -56, 16, -70, 2, -14, 58, -82, -10, -26, 46, -40, 32, 16, 88,
    -92, -20, -36, 36, -50, 22, 6, 78, -62, 10, -6, 66, -20, 52, 36, 108,
    -100, -28, -44, 28, -58, 14, -2, 70, -70, 2, -14, 58, -28, 44, 28, 100,
    -80, -8, -24, 48, -38, 34, 18, 90, -50, 22, 6, 78, -8, 64, 48, 120
};

/*
 * ACCUMULATE ALL PCM
 * */




int main(void) {
	SPI_Enable();
	USART_Enable();
	TIM2_Enable();
	GPIOA_Enable();
	GPIOB_Enable();
	DMA1_Enable();
	DMA2_Enable();

//	uint32_t apb1_freq = HAL_RCC_GetPCLK2Freq();

	while(1) {
		if (start_sampling) {
			GPIOA->ODR |= (GPIO_ODR_OD9); // Turn on LED to show user that it's time to start speaking
			delay_ms(250); // Slight delay because it takes a bit of time to react and start talking
			start_sampling = 0;
			buffer_count = 0;
			sample_index=0;

			i1 = 0;
			i2 = 0;
			i3 = 0;
			prev1 = 0;
			prev2 = 0;
			prev3 = 0;
			bit_count = 0;
			hp_x = 0;
			hp_y = 0;
			lp_out = 0;

			sampling_active = 1;

			volatile uint16_t dummy = SPI1->DR;
			(void)dummy;

			DMA2->LIFCR |= (DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0);
			DMA2_Stream0->CR |= DMA_SxCR_EN;

			SPI1->CR1 |= SPI_CR1_RXONLY;
			SPI1->CR1 |= SPI_CR1_SPE;

		}
		if (buffer_ready) {
			uint32_t *processing_ptr = buffer_to_drain;
			buffer_ready = 0;
			pdm_to_pcm(processing_ptr);
		}
		if(!sampling_active && !uart_dma_busy && sample_index > 0) {
			UART_DMA_send(sample_index*2);
			while(uart_dma_busy);
			print_ready = 0;
		}
	}
	return 0;
}

void DMA1_Enable(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	/* Enable bit enables and disables the stream
	Must be set to 0 in order to set our configurations */
	DMA1_Stream6->CR &= ~DMA_SxCR_EN_Msk;
	while(DMA1_Stream6->CR & DMA_SxCR_EN);

	/* We are specifically transferring from the USART
	 * Which corresponds to Stream6 channel 4 */
	DMA1_Stream6->CR &= ~DMA_SxCR_CHSEL_Msk;
	DMA1_Stream6->CR |= 4UL << DMA_SxCR_CHSEL_Pos;

	/* Next we need to set the DMA's peripheral address to access the data register of USART2*/
	DMA1_Stream6->PAR = (uint32_t)&USART2->DR;

	/* Initialization of our memory that is read from*/
	DMA1_Stream6->M0AR = (uint32_t)pcm;

	DMA1_Stream6->CR |= (1UL << DMA_SxCR_DIR_Pos); //set to "memory to peripheral" mode
	DMA1_Stream6->CR |= DMA_SxCR_MINC; //Memory address pointer is incremented after each transfer
	DMA1_Stream6->CR &= ~(DMA_SxCR_PINC_Msk); //Make sure to not increment the peripheral address
	DMA1_Stream6->CR &= ~(DMA_SxCR_MSIZE_Msk); // 8 bit memory
	DMA1_Stream6->CR &= ~(DMA_SxCR_PSIZE_Msk); // 8 bit peripheral
	DMA1_Stream6->CR |= DMA_SxCR_TCIE; //transfer complete interrupt

	USART2->CR3 |= USART_CR3_DMAT; // DMA mode is enabled for transmission

	NVIC_SetPriority(DMA1_Stream6_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

void DMA2_Enable(void) {
	// Use DMA2 since it allows for Direct Register Access
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

	/* Enable bit enables and disables the stream
	 Must be set to 0 in order to set our configurations */
	DMA2_Stream0->CR &= ~DMA_SxCR_EN_Msk;
	while(DMA2_Stream0->CR & DMA_SxCR_EN); //Wait for it to be set to 0

	/* For our data lines in SPI1 we are specifically receiving --> SPI1_RX
	 * Channel 3 of stream 0 corresponds to SPI1_RX
	 * */
	DMA2_Stream0->CR &= ~(DMA_SxCR_CHSEL_Msk);
	DMA2_Stream0->CR |= 3UL << DMA_SxCR_CHSEL_Pos; //Channel 3 Selection (SPI1_RX)

	/* Next we need to set the DMA's peripheral address to access the data register of our received input (in SPI1) */
	DMA2_Stream0->PAR = (uint32_t)&SPI1->DR;

	/* Initialization of our different buffers for the Double Buffering
	 * */
	DMA2_Stream0->M0AR = (uint32_t)bufferA;
	DMA2_Stream0->M1AR = (uint32_t)bufferB;
	/* Data is collected in 16 bits but we want it to be transfered to a 32 bit array
	 * Therefore we do BUFFER_SIZE * 2*/
	DMA2_Stream0->NDTR = BUFFER_SIZE * 2;

	DMA2_Stream0->CR &= ~(DMA_SxCR_DIR_Msk);
	DMA2_Stream0->CR |= (0 << DMA_SxCR_DIR_Pos); //Set Peripheral-to-memory
	DMA2_Stream0->CR |= DMA_SxCR_MINC; //Memory address pointer is incremented after each data transfer
	DMA2_Stream0->CR &= ~(DMA_SxCR_PINC_Msk); //Make sure to not increment the peripheral address
	DMA2_Stream0->CR &= ~(DMA_SxCR_MSIZE_Msk);
	DMA2_Stream0->CR &= ~(DMA_SxCR_PSIZE_Msk);
	DMA2_Stream0->CR |= (2UL << DMA_SxCR_MSIZE_Pos); //Make data transfered in 32 bit increments
	DMA2_Stream0->CR |= (1UL << DMA_SxCR_PSIZE_Pos); //Collect data from peripheral in 16 bits

	DMA2_Stream0->CR |= DMA_SxCR_DBM; //enable double buffering mode

	DMA2_Stream0->CR |= DMA_SxCR_TCIE; //transfer complete interrupt

	SPI1->CR2 |= SPI_CR2_RXDMAEN; //DMA request made when RXNE flag is set

	NVIC_SetPriority(DMA2_Stream0_IRQn, 0);
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}


void GPIOB_Enable(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable peripheral clock for GPIOB
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Enable SYSCFG for interrupts

	GPIOB->MODER &= ~GPIO_MODER_MODE9_Msk; // (00) input mode for PB9
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPD9_Msk; // (00) Reset PUPDR configure to No pull-up pull-down
	GPIOB->PUPDR |= GPIO_PUPDR_PUPD9_1; // 01 --> Pull down enable, Active High Logic

	SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI9_Msk; // Clear configuration for EXTI9
	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI9_PB; // Map port B to EXTI9

	EXTI->IMR |= EXTI_IMR_IM9; // Un-mask EXTI9 (enable it)
	EXTI->FTSR &= ~EXTI_FTSR_TR9; // Disable Falling Edge Trigger
	EXTI->RTSR |= EXTI_RTSR_TR9; // Rising Edge Enabled

	NVIC_EnableIRQ(EXTI9_5_IRQn); 	//Enable interrupt in NVIC
}

void GPIOA_Enable(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	GPIOA->MODER &= ~(GPIO_MODER_MODE9_Msk);
	GPIOA->MODER |= GPIO_MODER_MODE9_0; // Set to output mode
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT9_Msk); // Set output pin to drive it high or low
}


void DMA1_Stream6_IRQHandler(void) {
	// Check if transfer is complete in stream6
	if (DMA1->HISR & DMA_HISR_TCIF6) {
		DMA1->HIFCR |= DMA_HIFCR_CTCIF6; // Clear transfer complete interrupt flag
		uart_dma_busy = 0; //Not busy during the data transfer
	}
}

void DMA2_Stream0_IRQHandler(void) {
	//Check if transfer is complete in stream0
	if(DMA2->LISR & DMA_LISR_TCIF0) {
		DMA2->LIFCR |= DMA_LIFCR_CTCIF0; //clear transfer flag

		if(DMA2_Stream0->CR & DMA_SxCR_CT) {
			// DMA is now filling M1AR (buffer b) which means that target for transfer is buffer a
			buffer_to_drain = bufferA;
		} else {
			buffer_to_drain = bufferB;
		}

		buffer_ready = 1;

		if(++buffer_count >= BUFFER_PER_SECOND) {
			SPI1->CR1 &= ~SPI_CR1_SPE; // Disable the SPI
			DMA2_Stream0->CR &= ~DMA_SxCR_EN; // Disable the DMA
			GPIOA->ODR &= ~(GPIO_ODR_OD9); //Turn off LED to tell user that sampling has stopped
			sampling_active = 0;
			buffer_count = 0;
			print_ready = 1;
		}
	}
}

void EXTI9_5_IRQHandler(void) {
	if(EXTI->PR & EXTI_PR_PR9) {
		start_sampling = 1;
		/* Prevent multiple interrupt triggers from clock bouncing */
		if ((TIM2->CNT - last_press_time) > 50000) {
			last_press_time = TIM2->CNT;
			start_sampling = 1;
		}
		EXTI->PR |= EXTI_PR_PR9; // Send a bit to signal that the interrupt has been handled
	}
}

void TIM2_Enable(void) {
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable clock
	/* formula: counter clock frequency = fclk/PSC[15:0]+1 TIMx_PSC
	 clock frequency is 16Mhz.*/

	TIM2->PSC = 16-1; // Each clock cycle is 1us
	TIM2->ARR = 0xFFFFFFFF; // Maximum value of counter --> counter can count for a very long time

	TIM2->EGR |= TIM_EGR_UG; // Update Generation bit on --> forces hardware to take on new settings immiedetley
	TIM2->SR &= ~TIM_SR_UIF_Msk; //clear update flag --> ensure that it will run, CPU checks bit
	TIM2->CR1 |= TIM_CR1_CEN; //counter enabled
}

void delay_ms(uint32_t ms) {
	TIM2->CNT = 0;
	//each tick is 1us --> 1000 ticks = 1ms
	while (TIM2->CNT < (1000*ms));
}

/*
 * SPI (Serial Peripheral Interface) is a peripheral used to exchange data between
 * MCU and external components (in our case it's a pdm microphone)
 *
 * The important peripherals here are PA5 (SCK) responsible for the synchronization
 * And PA6 (MISO) which corresponds to receiving data --> SPI1_RX (important for the DMA initialization)
 */
void SPI_Enable(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST; // Clear Reset bit
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; // Enable the SPI1 clock

	// SPI1 must use PA5 (SCK), PA6 (MISO), PA7 (MOSI)
	GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk | GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);
	GPIOA->MODER |= (GPIO_MODER_MODE5_1 | GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);

	// Enable Alternative function (since we're grabbing bits from some IC)
	GPIOA->AFR[0] &= ~((0xF << GPIO_AFRL_AFSEL5_Pos)| (0xF << GPIO_AFRL_AFSEL6_Pos) | (0xF << GPIO_AFRL_AFSEL7_Pos));
	GPIOA->AFR[0] |= ((5UL << GPIO_AFRL_AFSEL5_Pos)| (5UL << GPIO_AFRL_AFSEL6_Pos) | (5UL << GPIO_AFRL_AFSEL7_Pos));

	SPI1->CR1 = 0; // Make sure EVERYTHING is cleared from Control Register
	SPI1->CR1 |= SPI_CR1_RXONLY; // Enable Receive only mode
	SPI1->CR1 |= SPI_CR1_DFF; // Set data register to be in 16 bit format
	SPI1->CR1 |= 3UL << SPI_CR1_BR_Pos; // Clk is set to fpCLK / 16 = 1Mhz
	SPI1->CR1 |= SPI_CR1_MSTR; // Set SPI to be the master

	/* Ignores if NSS pin is pulled low telling the MCU that I don't
	 * want to dedicate a physical GPIO pin to act as a hardware "Slave Select" pin
	 * If I don't do this then there is a chance that the MCU might think
	 * that another master is trying to take over
	 * */
	SPI1->CR1 |= SPI_CR1_SSM;
	SPI1->CR1 |= SPI_CR1_SSI;
	SPI1->CR1 &= ~SPI_CR1_CPOL; // Clk 0 when idle
	SPI1->CR1 &= ~SPI_CR1_CPHA_Msk; // the first clock transition is the first data capture edge

	SPI1->CR1 |= SPI_CR1_SPE; //Enable SPI

}

void USART_Enable(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
	GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1); //set to alternate mode

	GPIOA->AFR[0] &= ~((0xF << GPIO_AFRL_AFSEL3_Pos) | (0xF << GPIO_AFRL_AFSEL2_Pos));
	GPIOA->AFR[0] |= (0x7 << GPIO_AFRL_AFSEL3_Pos) | (0x7 << GPIO_AFRL_AFSEL2_Pos);

	USART2->BRR = 0x8A; //16Mhz / 115200 --> 921600 baud at 16Mhz
	USART2->CR1 &= ~USART_CR1_M_Msk;
	USART2->CR1 |= (USART_CR1_UE | USART_CR1_TE | USART_CR1_RE); // usart enabled, transmitter enabled, receiver enabled

}

/* We pass a uint16_t because by definition the number of data registers to be transfered
 * is a 16bit unsigned number */
void UART_DMA_send(uint16_t len) {
	while(uart_dma_busy);

	DMA1_Stream6->CR &= ~DMA_SxCR_EN_Msk;
	while(DMA1_Stream6->CR & DMA_SxCR_EN);

	DMA1_Stream6->NDTR = len;
	uart_dma_busy = 1;
	DMA1_Stream6->CR |= DMA_SxCR_EN;
}

/*
 * Third order CIC FILTER to convert PDM to PCM
 * I used this website as a reference: https://tomverbeure.github.io/2020/09/30/Moving-Average-and-CIC-Filters.html
 */

void pdm_to_pcm(uint32_t *pdm) {
	/* Create local variables that are copies of the global variable
	 * To speed up how long this function takes
	 * Has to be faster than how long the buffer takes to fill up */

	uint16_t *pdm_temp = (uint16_t *)pdm;

	uint32_t l_i1 = i1;
	uint32_t l_i2 = i2;
	uint32_t l_i3 = i3;
	int l_bit_count = bit_count;

	for (int i = 0; i < BUFFER_SIZE*2; i++) {
		uint16_t word = pdm_temp[i];

		uint8_t bytes[2];
		bytes[0] = (uint8_t)(word >> 8);
		bytes[1] = (uint8_t)(word & 0xFF);

		/* Integrater Stage */

		for(int j = 0; j < 2; j++) {
			uint8_t byte_val = bytes[j];
			l_bit_count += 8;

			l_i3 += (l_i2 << 3) + (l_i1 * 36) + LUT_i3[byte_val];
			l_i2 += (l_i1 << 3) + LUT_i2[byte_val];
			l_i1 += LUT_i1[byte_val];

			/* Comb Stages */

			if (l_bit_count >= DECIMATION) {
				int32_t c1 = l_i3 - prev1;
				prev1 = l_i3;
				int32_t c2 = c1 - prev2;
				prev2 = c1;
				int32_t c3 = c2 - prev3;
				prev3 = c2;
				l_bit_count = 0;

				/* Applying high and low frequency filters */
				if (sample_index < TOTAL_PCM_SAMPLES) {
					int32_t raw_pcm = (int32_t)c3;
					/* IIR High pass Filter to eliminate DC
					 * y[n] = x[n] - x[n-1] + 0.99 * y[n-1] */

					int32_t current_y = raw_pcm - hp_x + hp_y - ((hp_y ) >> 10);
					hp_x = raw_pcm;
					hp_y = current_y;

					/* Simple Exponential Smoothing low pass filter.
					 * a[n] = 0.75 * a[n-1] + 0.25 * y[n]
					 * Smooths out high frequency jitter noise that
					 * the first filter might have emphasized */
					lp_out = lp_out + ((current_y - lp_out) >> 2);

					/* Scale down the amplitude */
					int32_t final_output = lp_out >> 6;
					if (final_output > 32767) final_output = 32767;
					if (final_output < -32768) final_output = -32768;

					pcm[sample_index++] = (int16_t)(final_output);
				}
			}
		}
	}

	i1 = l_i1;
	i2 = l_i2;
	i3 = l_i3;
	bit_count = l_bit_count;
}
