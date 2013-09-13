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

gaussian function for 256 element envelope
	=INT(4095*EXP(-((x-center)^2)/(2*width^2)))


*/

#include "..\Header\includes.h"


//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

	#define		SAMPLE_RATE	33075

	//F & P for 20Hz & 10240Hz
	#define		P_MIN		771632
	#define		P_MAX		1180467
	#define		F_MIN		129855
	#define		F_MAX		66485973

	// (p-P_MIN)/1604 gives 0-255 for indexing envelope
	#define		P_SCALE		1604

//********************************************************************************************************
// Local defines
//********************************************************************************************************

	struct	tone_struct
	{
			uint32_t	q;	//angle
			uint32_t	f;	//frequency 1=154.0174uHz ... 			129855 = 20Hz
			uint32_t	p;	//pitch = ln(f)<<16  1 octave = 45426	771632 = 20Hz 1180467=10240Hz
			uint8_t		active;
	//		f=exp(p/65536)
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Private variables
//********************************************************************************************************

	enum sheppard_style_enum	style=STYLE_MINOR;

	// (p-P_MIN)/P_SCALE gives 0-255 for indexing envelope
	static uint16_t		envelope_final[256];	//final amplitude values 0-4095
	static uint16_t		envelope[256];			//amplitude values 0-4095

	static struct tone_struct tone[9];
	static uint8_t	root=0;	//which tone is the root tone (lowest F)

	static uint32_t	update_f=F_MIN, update_p=P_MIN, speed=0;
	static int32_t envelope_volume=32760;
	static uint8_t	update_flag, update_pulldown, update_pushup, entering=TRUE;

	//offset=0, amplitude==32767 (as 32.0)
	static const int16_t minor_table[16384]={
	#include "Minor.csv"
	};

	static const int16_t major_table[16384]={
	#include "Major.csv"
	};

	static const int16_t tone_table[4096]={
	#include "Tone5.csv"
	};

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

//********************************************************************************************************
// Public functions
//********************************************************************************************************

void sheppard_styleset(enum sheppard_style_enum x )
{
	style=x;
}

//if entering or exiting, complete entry (all tones on), or exit (all tones off)
void sheppard_finalize(void)
{
	if(entering)
	{
		tone[0].active=TRUE;
		tone[1].active=TRUE;
		tone[2].active=TRUE;
		tone[3].active=TRUE;
		tone[4].active=TRUE;
		tone[5].active=TRUE;
		tone[6].active=TRUE;
		tone[7].active=TRUE;
		tone[8].active=TRUE;
	}
	else
	{
		tone[0].active=FALSE;
		tone[1].active=FALSE;
		tone[2].active=FALSE;
		tone[3].active=FALSE;
		tone[4].active=FALSE;
		tone[5].active=FALSE;
		tone[6].active=FALSE;
		tone[7].active=FALSE;
		tone[8].active=FALSE;
	};
};

void sheppard_enter(void)
{
	entering=TRUE;
}

void sheppard_exit(void)
{
	entering=FALSE;
}

void TIM6_DAC_IRQHandler(void)
{
	int32_t 			x;
	uint32_t			f,p;
	uint8_t 			temp;
	static uint16_t		update_count=0;

	GPIO_SetBits(GPIOC, GPIO_Pin_8);

	TIM_ClearITPendingBit(TIM6, TIM_IT_Update);

	//update new root tone every 330 samples (~10mS @33075s/S)
	update_count++;
	if(update_count == 330)
	{
		update_count=0;
		if(update_pulldown)
		{
			update_pulldown=FALSE;
			if(entering)
				tone[root].active=TRUE;
			else
				tone[root].active=FALSE;
			root++;
			if(root==9)
				root=0;
		}
		else if(update_pushup)
		{
			update_pushup=FALSE;
			if(entering)
				tone[root].active=TRUE;
			else
				tone[root].active=FALSE;

			if(root)
				root--;
			else
				root=8;
		};

		tone[root].f = update_f;
		tone[root].p = update_p;
		update_flag=FALSE;
	};

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

	//sum outputs
	x=0;
	if(style == STYLE_MINOR)
	{
		x += tone[0].active * minor_table[tone[0].q>>18]*envelope[(tone[0].p-P_MIN)/P_SCALE];
		x += tone[1].active * minor_table[tone[1].q>>18]*envelope[(tone[1].p-P_MIN)/P_SCALE];
		x += tone[2].active * minor_table[tone[2].q>>18]*envelope[(tone[2].p-P_MIN)/P_SCALE];
		x += tone[3].active * minor_table[tone[3].q>>18]*envelope[(tone[3].p-P_MIN)/P_SCALE];
		x += tone[4].active * minor_table[tone[4].q>>18]*envelope[(tone[4].p-P_MIN)/P_SCALE];
		x += tone[5].active * minor_table[tone[5].q>>18]*envelope[(tone[5].p-P_MIN)/P_SCALE];
		x += tone[6].active * minor_table[tone[6].q>>18]*envelope[(tone[6].p-P_MIN)/P_SCALE];
		x += tone[7].active * minor_table[tone[7].q>>18]*envelope[(tone[7].p-P_MIN)/P_SCALE];
		x += tone[8].active * minor_table[tone[8].q>>18]*envelope[(tone[8].p-P_MIN)/P_SCALE];
	}
	else if(style == STYLE_MAJOR)
	{
		x += tone[0].active * major_table[tone[0].q>>18]*envelope[(tone[0].p-P_MIN)/P_SCALE];
		x += tone[1].active * major_table[tone[1].q>>18]*envelope[(tone[1].p-P_MIN)/P_SCALE];
		x += tone[2].active * major_table[tone[2].q>>18]*envelope[(tone[2].p-P_MIN)/P_SCALE];
		x += tone[3].active * major_table[tone[3].q>>18]*envelope[(tone[3].p-P_MIN)/P_SCALE];
		x += tone[4].active * major_table[tone[4].q>>18]*envelope[(tone[4].p-P_MIN)/P_SCALE];
		x += tone[5].active * major_table[tone[5].q>>18]*envelope[(tone[5].p-P_MIN)/P_SCALE];
		x += tone[6].active * major_table[tone[6].q>>18]*envelope[(tone[6].p-P_MIN)/P_SCALE];
		x += tone[7].active * major_table[tone[7].q>>18]*envelope[(tone[7].p-P_MIN)/P_SCALE];
		x += tone[8].active * major_table[tone[8].q>>18]*envelope[(tone[8].p-P_MIN)/P_SCALE];
	}
	else if(style == STYLE_TONE)
	{
		x += tone[0].active * tone_table[(tone[0].q>>18)&4095]*envelope[(tone[0].p-P_MIN)/P_SCALE];
		x += tone[1].active * tone_table[(tone[1].q>>18)&4095]*envelope[(tone[1].p-P_MIN)/P_SCALE];
		x += tone[2].active * tone_table[(tone[2].q>>18)&4095]*envelope[(tone[2].p-P_MIN)/P_SCALE];
		x += tone[3].active * tone_table[(tone[3].q>>18)&4095]*envelope[(tone[3].p-P_MIN)/P_SCALE];
		x += tone[4].active * tone_table[(tone[4].q>>18)&4095]*envelope[(tone[4].p-P_MIN)/P_SCALE];
		x += tone[5].active * tone_table[(tone[5].q>>18)&4095]*envelope[(tone[5].p-P_MIN)/P_SCALE];
		x += tone[6].active * tone_table[(tone[6].q>>18)&4095]*envelope[(tone[6].p-P_MIN)/P_SCALE];
		x += tone[7].active * tone_table[(tone[7].q>>18)&4095]*envelope[(tone[7].p-P_MIN)/P_SCALE];
		x += tone[8].active * tone_table[(tone[8].q>>18)&4095]*envelope[(tone[8].p-P_MIN)/P_SCALE];
	};

	x /= envelope_volume;
	x /= 19; //gives +/- ~~2000 I think

	DAC->DHR12R1 = x+2048;
	DAC->DHR12R2 = 2048-x;

	GPIO_ResetBits(GPIOC, GPIO_Pin_8);
}

//realistic width ranges from 5-100
void sheppard_envelopeset(int32_t center, double width)
{
	int32_t x=0;
	double y;
	if(0 <= center && center <= 255)
	{
		do
		{
			y = 4095*exp(-((x-center)*(x-center))/(2*width*width));
			envelope[x] = y;
			x++;
			main_fly();
		}while(x!=256);
	}
	//if center is out of range, flatten envelope
	else
	{
		do
		{
			envelope[x++] = 4095;
		}while(x!=256);
	};

	//to evaluate volume of envelope 29 points (~1octave) apart, aligned with center
	x=0;
	while(center > 0)
		center-=28;
	center+=28;
	do
	{
		x+=envelope[center];
		center+=28;
	}while(center < 256);

	//volume will be 4095 - 32760 (8x4095)
	envelope_volume = x;
}

void sheppard_speedset(int32_t x)
{
	if(-45426 < x && x < 45426 )
	speed=x;
}

void sheppard_process(void)
{
	if(!update_flag)
	{
		update_p += speed;
		if(update_p<P_MIN)	//below 20Hz?
		{
			update_p += 45426;
			update_pulldown=TRUE;
		};
		if(update_p>P_MIN+45425)	//above 40Hz (first octave)?
		{
			update_p -= 45426;
			update_pushup=TRUE;
		};

		update_f=exp(((double)update_p)/65536);
		update_flag=TRUE;
	};
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
		envelope_final[tempint]=4095;
		envelope[tempint]=4095;
		tempint++;
	}while(tempint!=256);

	tempint=0;
	do
	{
		tone[tempint].active=TRUE;
		tempint++;
	}while(tempint!=9);

	tone[0].f = F_MIN;
	tone[0].p =  P_MIN;

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
