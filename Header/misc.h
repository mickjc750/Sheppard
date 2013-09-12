
//********************************************************************************************************
// Public defines
//********************************************************************************************************

//	used to determine if compiler is padding out structures for data alignment
	#define PACKTEST1_SIZE (sizeof(int)*4+sizeof(char)*6)
	struct packtest1_struct
	{
		int	a;
		char b;
		int c;
		char d;
		char e;
		int f;
		char g;
		char h;
		char i;
		int j;
	};

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Public prototypes
//********************************************************************************************************

// call to print memory to a text buffer as hex data
// option=0, ############
// option=1, ##:##:##:##:##:##
char* hex2ascii(char* textbuf, void* startadd, unsigned char length, unsigned char option);

//write a number to a text buffer, base can be 1-41 (using alpha up to Z)
//number will be right justified within string of length, left padded with char pad
//if length is 0, or padding character is null, string length will be the minimum required
//if fixed length specified is inadequate for value, buffer will contain '#' characters
//note that specified length does not include terminator, or sign character, so buffer must be at least length+2
//if sign_pad is non0, positive values will begin with this character
char* ascii_long(long value, char *text, unsigned char base, unsigned char fixed_length, char digit_pad, char sign_pad);

//write a number to a text buffer, base can be 1-41 (using alpha up to Z)
//number will be right justified within string of length, left padded with char pad
//if length is 0, or padding character is null, string length will be the minimum required
//if fixed length specefied is inadequate for value, buffer will contain '#' characters
//note that specefied length does not include terminator, so buffer must be at least length+1
char* ascii_ulong(unsigned long value, char *text, unsigned char base, unsigned char fixed_length, char pad);
