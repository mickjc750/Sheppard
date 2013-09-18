/*

PC0 ADC1_IN10 Speed
PC1 ADC1_IN11 Center
PC2 ADC1_IN12 Width

//Logic inputs with pullup, active low
PA0	Select Major
PA1	Select Tone	(neither selects Minor)
PA2 Enter/Exit (pull low for exit)
PA3 Finish

PA6 Output STILL led

*/

#include "..\Header\includes.h"


//********************************************************************************************************
// Configurable defines
//********************************************************************************************************

	//maximum speed (+/-) passed to sheppard module
	#define	SPEED_MAX		100

	//speed pot deadzone (+/-) for still center in terms of 20bit A2D result (524288 is center point)
	#define	SPEED_DEADZONE	10000

	//should match P_MIN & P_MAX in shepard.c
	#define CENTER_MAX		1180467
	#define CENTER_MIN		771632

	#define	WIDTH_MAX		200
	#define WIDTH_MIN		5

	//width pot threshold at which we turn envelope window off, in 20bit A2D value
	#define WIDTH_OFF		998000

//********************************************************************************************************
// Local defines
//********************************************************************************************************

//	A2D readings are scaled up to 20bit (0-1048575)
//	Readings are given +/- this much hysteresis to stop jitter
	#define	HYST	200

//	Error is divided by 2^of AVERGAGE, ie. 6 divides error by 64
	#define	AVERAGE	6

	struct pot_struct
	{
		uint32_t	average;
		uint32_t	hysteresis;
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************

	struct interface_struct	interface_output = {0, 128, 45, TRUE};

	uint8_t	interface_update_speed;
	uint8_t	interface_update_window;

//********************************************************************************************************
// Private variables
//********************************************************************************************************

	static unsigned int	time;

	struct pot_struct speed_pot, width_pot, center_pot;

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

	void pot_process(struct pot_struct *pot_ptr);

//********************************************************************************************************
// Public functions
//********************************************************************************************************

void interface_timer(void)
{
	if(time)
		time--;
}

void interface_process(void)
{
	static uint8_t state=0;
	static uint16_t gpioa_verified, gpioa_verified_count=0;

	int32_t	tempint;

	switch(state)
	{
		case 0:
			ADC_RegularChannelConfig(ADC1,ADC_Channel_10, 1,ADC_SampleTime_239Cycles5);	// define regular conversion config
			ADC_SoftwareStartConvCmd(ADC1, ENABLE);										// start ONE conversion
			state++;
			break;
		case 1:
			if(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==SET)
			{
				//process speed
				state++;
				pot_process(&speed_pot);
				tempint = speed_pot.hysteresis;
				if( 524288-SPEED_DEADZONE < tempint && tempint < 524288+SPEED_DEADZONE)
				{
					GPIO_SetBits(GPIOA, GPIO_Pin_6);
					tempint=0;
				}
				else
				{
					GPIO_ResetBits(GPIOA, GPIO_Pin_6);
					tempint -=524288;
					if(tempint >0)
						tempint-=SPEED_DEADZONE;
					else
						tempint+=SPEED_DEADZONE;
					tempint /= (int32_t)(524288/SPEED_MAX);
				};
				if(tempint != interface_output.speed)
				{
					interface_output.speed=tempint;
					interface_update_speed=TRUE;
					sheppard_speedset(tempint);
				};
			};
			break;
		case 2:
			ADC_RegularChannelConfig(ADC1,ADC_Channel_11, 1,ADC_SampleTime_239Cycles5);	// define regular conversion config
			ADC_SoftwareStartConvCmd(ADC1, ENABLE);										// start ONE conversion
			state++;
			break;
		case 3:
			if(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)== SET)
			{
				//process center
				state++;
				pot_process(&center_pot);
				tempint = center_pot.hysteresis>>12;							//20bit -> 8bit
				if(tempint != interface_output.center)
				{
					interface_output.center=tempint;
					if(interface_output.width_onoff)
						interface_update_window=TRUE;
				};
			};
			break;
		case 4:
			ADC_RegularChannelConfig(ADC1,ADC_Channel_12, 1,ADC_SampleTime_239Cycles5);	// define regular conversion config
			ADC_SoftwareStartConvCmd(ADC1, ENABLE);										// start ONE conversion
			state++;
			break;
		case 5:
			if(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)== SET)
			{
				//process width
				state++;
				pot_process(&width_pot);
				tempint = width_pot.hysteresis;
				if(tempint > WIDTH_OFF)
				{
					if(interface_output.width_onoff)
					{
						interface_output.width_onoff=FALSE;
						interface_update_window=TRUE;
					};
				}
				else
				{
					if(!interface_output.width_onoff)
					{
						interface_output.width_onoff=TRUE;
						interface_update_window=TRUE;
					};
					tempint = width_pot.hysteresis<<10;		//30 bit 0-1073740800
					tempint /=1+(uint32_t)(1073740800/(WIDTH_MAX-WIDTH_MIN)); // 5506364
					tempint += WIDTH_MIN;
					if(tempint != interface_output.width)
					{
						interface_output.width=tempint;
						interface_update_window=TRUE;
					};
				};
			};
			break;
		case 6:
			state=0;
			if(gpioa_verified == (GPIO_ReadInputData(GPIOA)&0x0F))
				gpioa_verified_count++;
			else
			{
				gpioa_verified = (GPIO_ReadInputData(GPIOA)&0x0F);
				gpioa_verified_count=0;
			};
			if(gpioa_verified_count==200)
			{
				if(gpioa_verified & GPIO_Pin_2)
					sheppard_enter();
				else
					sheppard_exit();

				if((gpioa_verified & GPIO_Pin_0) == RESET)
					sheppard_styleset(STYLE_MAJOR);
				else if((gpioa_verified & GPIO_Pin_1) == RESET)
					sheppard_styleset(STYLE_TONE);
				else
					sheppard_styleset(STYLE_MINOR);

				if((gpioa_verified & GPIO_Pin_3) == RESET)
					sheppard_finalize();
			};
			if(gpioa_verified_count > 201)
				gpioa_verified_count=201;

			break;
	};
}

void interface_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//Led output
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//Logic inputs
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 +GPIO_Pin_1 +GPIO_Pin_2 +GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//Analog inputs
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 + GPIO_Pin_1 + GPIO_Pin_2;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//Setup A2D
	RCC_ADCCLKConfig (RCC_PCLK2_Div6);

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	// we work in continuous sampling mode
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1,ADC_Channel_10, 1,ADC_SampleTime_239Cycles5); // define regular conversion config

	ADC_Cmd (ADC1,ENABLE);	//enable ADC1

	//ADC calibration (optional, but recommended at power on)
	ADC_ResetCalibration(ADC1);	// Reset previous calibration
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);	// Start new calibration (ADC must be off at that time)
	while(ADC_GetCalibrationStatus(ADC1));

}

//********************************************************************************************************
// Private functions
//********************************************************************************************************

void pot_process(struct pot_struct *pot_ptr)
{
	int32_t error;
	error = ADC_GetConversionValue(ADC1);
	error <<=4;
	error -= pot_ptr->average;
	pot_ptr->average += error>>AVERAGE;
	if(pot_ptr->average > pot_ptr->hysteresis+HYST)
		pot_ptr->hysteresis = pot_ptr->average-HYST;
	if(pot_ptr->average < pot_ptr->hysteresis-HYST)
		pot_ptr->hysteresis = pot_ptr->average+HYST;
}
