/*
	UART module, CAUTION, modifying any of the bits other than TXEIE in UARTx->CR1
	must be atomic as this register is accessed in ISR to disable TX
*/

#include "..\Header\includes.h"

//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

//********************************************************************************************************
// Local defines
//********************************************************************************************************


//reduced version of fifo_write_char in macro form to speed up UART service time
#define UART_FIFO_WRITE_CHAR(fifo_ptr,data) 	\
do{												\
	*((char*)fifo_ptr->head_ptr) = data;		\
	fifo_ptr->head_ptr++;						\
	if(fifo_ptr->head_ptr == fifo_ptr->end)		\
		fifo_ptr->head_ptr = fifo_ptr->start;	\
												\
	fifo_ptr->bytes_free --;					\
	fifo_ptr->bytes_used ++;					\
	if(fifo_ptr->bytes_free == 0)				\
		fifo_ptr->full=TRUE;					\
	if(fifo_ptr->bytes_used != 0)				\
		fifo_ptr->empty=FALSE;					\
												\
	if(fifo_ptr->post_fptr)						\
			fifo_ptr->post_fptr();				\
}while(0)										\

//macro version of fifo_read_char to speed up UART ISR
#define UART_FIFO_READ_CHAR(target, source_fifo)	\
do{													\
	target=*((char*)source_fifo.tail_ptr);			\
	source_fifo.tail_ptr++;							\
													\
	if(source_fifo.tail_ptr == source_fifo.end)		\
		source_fifo.tail_ptr = source_fifo.start;	\
													\
	source_fifo.bytes_free ++;						\
	source_fifo.bytes_used --;						\
	if(source_fifo.bytes_used == 0)					\
		source_fifo.empty=TRUE;						\
	if(source_fifo.bytes_free != 0)					\
		source_fifo.full=FALSE;						\
													\
}while(0)											\


//********************************************************************************************************
// Public variables
//********************************************************************************************************

	struct fifo_struct	uart1_fifo_tx;
	struct fifo_struct	*uart1_fifo_rx_ptr=NULL;

//********************************************************************************************************
// Private variables
//********************************************************************************************************

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

//********************************************************************************************************
// Public functions
//********************************************************************************************************

void uart1_isrtx_enable(void)
{
    	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}

void USART1_IRQHandler(void)
{
	char temp;

	if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
	{
		temp = (char)USART_ReceiveData(USART1);
		if(uart1_fifo_rx_ptr)
		{
			if(!uart1_fifo_rx_ptr->full)
				UART_FIFO_WRITE_CHAR(uart1_fifo_rx_ptr, temp);
		};
	};

	if(USART1->CR1 & (1<<7))	//TX ISR source enabled?
	{
		if(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == SET)
		{
			//read next byte if available, else disable this isr source
			if(!uart1_fifo_tx.empty)
			{
				UART_FIFO_READ_CHAR(temp, uart1_fifo_tx);
				USART_SendData(USART1, temp);
			}
			else
			{
				USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			};
		};
	};
}

void uart1_init(int baud, int fifo_tx_size, void(*wait_fptr)(void), void* fifo_tx_data)
{
	GPIO_InitTypeDef	gpio_init;
	USART_InitTypeDef	uart_init;

	fifo_init(&uart1_fifo_tx, fifo_tx_size, uart1_isrtx_enable, wait_fptr, fifo_tx_data);

	// Enable USART and I/O clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // Configure USART1 TX (PA9) as alternate function push-pull
    gpio_init.GPIO_Pin   = GPIO_Pin_9;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio_init);

    // Configure USART1 RX (PA10) as input floating
    gpio_init.GPIO_Pin   = GPIO_Pin_10;
    gpio_init.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio_init);

    // Initialize USART1
    USART_StructInit(&uart_init);
    uart_init.USART_BaudRate = baud;
    USART_Init(USART1, &uart_init);

    // Enable only RX ISR's (TX is enabled after writing to tx fifo)
    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_SetPriority(USART1_IRQn, 0x0F);	//0x00=high 0x0F=lowest
    NVIC_EnableIRQ(USART1_IRQn);

    // Enable USART1
    USART_Cmd(USART1, ENABLE);
}

//********************************************************************************************************
// Private functions
//********************************************************************************************************
