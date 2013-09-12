
//********************************************************************************************************
// Public defines
//********************************************************************************************************

	enum sheppard_style_enum
	{
		STYLE_MAJOR,
		STYLE_MINOR,
		STYLE_TONE
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Public prototypes
//********************************************************************************************************

	void sheppard_styleset(enum sheppard_style_enum x );
	void sheppard_enter(void);
	void sheppard_exit(void);
	void sheppard_finalize(void);
	void sheppard_envelopeset(int32_t center, double width);
	void sheppard_speedset(int32_t x);
	void sheppard_process(void);
	void sheppard_init(void);
