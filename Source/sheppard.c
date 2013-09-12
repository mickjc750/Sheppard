/*
	9 Octaves
	  20-40
	  40-80
	  80-160
	 160-320
	 320-640
	 640-1280
	1280-2560
	2560-5120
	5120-10240

Minor chord is:
1/1	440
6/5 528
3/2 660

Major chord

*/

#include "..\Header\includes.h"


//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

	#define		SAMPLE_RATE	22050

//********************************************************************************************************
// Local defines
//********************************************************************************************************

	struct	tone_struct
	{
			uint32_t	q;	//angle
			uint32_t	f;	//frequency 1=5.13578uHz ... 			3894249=20Hz
			uint32_t	p;	//pitch = ln(f)<<16  1 octave = 45426	 994510=20Hz 1403344=10240Hz
	//		f=exp(p/65536)
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Private variables
//********************************************************************************************************

	// (p-994510)/1598 gives 0-255 for indexing envelope
	static uint16_t		envelope[256];	//amplitude values 0-4095

	static struct tone_struct tone[9];
	static uint8_t	root=0;	//which tone is the root tone (lowest F)
	
	//offset=0, amplitude==32767 (as 32.0)
	static const int32_t sin_table[1024]={
	#include "sin_table.txt"
	};

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

//********************************************************************************************************
// Public functions
//********************************************************************************************************


void TIM6_DAC_IRQHandler(void)
{
	int32_t 	x;
	uint32_t	f,p;
	uint8_t 	temp;

	GPIO_SetBits(GPIOC, GPIO_Pin_8);

	TIM_ClearITPendingBit(TIM6, TIM_IT_Update);

	//derive upper octaves from root
	f=tone[root].f;
	p=tone[root].p;
	temp=8;
	while(temp--)
	{
		root++;
		if(root==9)
			root=0;
		f<<=1;			//double frequency
		p+=45426;		//increase pitch by 1 octave
		tone[root].f=f;
		tone[root].p=p;
	};
	root++;
	if(root==9)
		root=0;

	//advance angles
	tone[0].q += tone[0].f;
	tone[1].q += tone[1].f;
	tone[2].q += tone[2].f;
	tone[3].q += tone[3].f;
	tone[4].q += tone[4].f;
	tone[5].q += tone[5].f;
	tone[6].q += tone[6].f;
	tone[7].q += tone[7].f;
	tone[8].q += tone[8].f;

	// (p-994510)/1598 gives 0-255 for indexing envelope

	//sum outputs
	x=0;
	x += sin_table[tone[0].q>>22]*envelope[(tone[0].p-994510)/1598];
	x += sin_table[tone[1].q>>22]*envelope[(tone[1].p-994510)/1598];
	x += sin_table[tone[2].q>>22]*envelope[(tone[2].p-994510)/1598];
	x += sin_table[tone[3].q>>22]*envelope[(tone[3].p-994510)/1598];
	x += sin_table[tone[4].q>>22]*envelope[(tone[4].p-994510)/1598];
	x += sin_table[tone[5].q>>22]*envelope[(tone[5].p-994510)/1598];
	x += sin_table[tone[6].q>>22]*envelope[(tone[6].p-994510)/1598];
	x += sin_table[tone[7].q>>22]*envelope[(tone[7].p-994510)/1598];
	x += sin_table[tone[8].q>>22]*envelope[(tone[8].p-994510)/1598];

	x /= 603961; //gives +/- 2000

	DAC->DHR12R1 = x+2048;
	DAC->DHR12R2 = x+2048;

	GPIO_ResetBits(GPIOC, GPIO_Pin_8);
}

void sheppard_timer(void)
{
	static uint32_t p=1014510;
	static uint32_t f;

	GPIO_SetBits(GPIOC, GPIO_Pin_9);

	p=p-45;
	if(p<994510)	//below 20Hz?
	{
		p+=45426;
		f=exp(((double)p)/65536);
		ATOMIC_BLOCK
		{
			root++;
			if(root==9)
				root=0;
			tone[root].f=f;
			tone[root].p=p;
		};
	}
	else
	{
		f=exp(((double)p)/65536);
		ATOMIC_BLOCK
		{
			tone[root].f=f;
			tone[root].p=p;
		};
	};

	GPIO_ResetBits(GPIOC, GPIO_Pin_9);
}

void sheppard_init(void)
{
	GPIO_InitTypeDef		gpio_init;
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseStructure;
	DAC_InitTypeDef		    DAC_InitStructure;

	uint32_t	tempint;

	tempint=0;
	do
	{
		envelope[tempint++]=4095;
	}while(tempint!=256);

	tone[0].f = 3894249;
	tone[0].p =  994510;

	// Enable DAC and I/O and TMR6 clocks
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

    // Configure GPIO for analog
    gpio_init.GPIO_Pin =  GPIO_Pin_4 | GPIO_Pin_5;
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio_init);

    //Configure DAC
    DAC_StructInit(&DAC_InitStructure);
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);

    //Configure timer6
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = (24E6/SAMPLE_RATE);
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
    TIM_ITConfig(TIM6, TIM_IT_Update , ENABLE);

    //Enable DAC channels
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_Cmd(DAC_Channel_2, ENABLE);

    //Enable timer
    TIM_Cmd(TIM6, ENABLE);

    NVIC_SetPriority(TIM6_DAC_IRQn, 0x0F);	//0x00=high 0x0F=lowest
    NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

//********************************************************************************************************
// Private functions
//********************************************************************************************************
