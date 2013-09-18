
/*

Differential output on PA4 & PA5

PC0 ADC1_IN10 Speed
PC1 ADC1_IN11 Center
PC2 ADC1_IN12 Width

PA0	Select Major
PA1	Select Tone	(both low selects Minor)
PA2 Enter/Exit (1/0)
PA3 Finish

*/

#include "..\Header\includes.h"

//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

//********************************************************************************************************
// Local defines
//********************************************************************************************************

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Private variables
//********************************************************************************************************

	static volatile uint8_t	tickflag;

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

//********************************************************************************************************
// Public functions
//********************************************************************************************************

void SysTick_Handler(void)
{
	tickflag=TRUE;
	console_timer();
	interface_timer();
}

void main_fly(void)
{
	sheppard_process();
	interface_process();
}

int main(void)
{
	GPIO_InitTypeDef	gpio_init;
	int	tempint;

	__disable_irq();
	SCB->VTOR = 0x08004000;
	__enable_irq();

	uart1_init(38400, 16, main_fly, NULL);

	//Enable the two led outputs for debugging
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	gpio_init.GPIO_Pin   = GPIO_Pin_8 + GPIO_Pin_9;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio_init);

	console_init();
	sheppard_init();
	interface_init();

	//10mS system tick
	SysTick_Config(SystemCoreClock * 0.01); //system_stm32f10x.c


	while(1)
	{
		if(interface_update_window)
		{
			interface_update_window=FALSE;
			if(interface_output.width_onoff)
				sheppard_envelopeset(interface_output.center, interface_output.width);
			else
				sheppard_envelopeset(-1, 0);
		};
		main_fly();
	};


/*
	while(1)
    {
		uart1_fifo_rx_ptr = &console_fifo_tx;
		console_device = CONSOLE_DEVICE_UART1;
		while(1)
			console_main();
    };

*/
	return 0;
}

//********************************************************************************************************
// Private functions
//********************************************************************************************************
