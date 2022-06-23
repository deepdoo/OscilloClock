//*****************************************************************************
// Scope Clock Hardware Demonstration
//
// Version 1.0 2020-06-10
//
//*****************************************************************************
//*****************************************************************************
//
// Pin Usage
//
//
//*****************************************************************************
//
// Timer Usage
//
//*****************************************************************************
//*****************************************************************************
// Includes
//*****************************************************************************
#include <string.h>
#include <intrins.h>
#include "stc12c5a60s2.h"
#include "sc.h"
#include "font.h"
#include "trigonometric.h"

//*****************************************************************************
// Define
//*****************************************************************************
#define VERSION		10
#define TRUE		1
#define FALSE		0
#define XTAL_FREQ		20000000

sbit AD7304_SDI = P2^0;
sbit AD7304_CLK = P2^1;
sbit AD7304_CS = P2^2;
sbit AD7304_LDAC = P2^3;
sbit C_SEL0 = P2^4;
sbit C_SEL1 = P2^5;
sbit BS_G = P2^6;
sbit PWR_EN = P2^7;

#define BS_SEL		P0

sbit KEY_S1 = P1^6;
sbit KEY_S2 = P1^5;
sbit KEY_S3 = P1^7;

sbit PULSE_OUT_0 = P1^0;
sbit PULSE_OUT_1 = P1^1;

//*****************************************************************************
// Constant		 				               
//*****************************************************************************

//*****************************************************************************
// Function Prototype
//*****************************************************************************
void adjust_display(unsigned char);

void ad7304_init(void);
void ad7304_update(unsigned char, unsigned char);

void vect_plot(unsigned char, unsigned char,
								unsigned char, unsigned char,
								unsigned char, unsigned char);

void draw_character(unsigned char, unsigned char, unsigned char, unsigned char);
void character_test(void);

void draw_circle(void);								
void calibration(void);
void display_grid(void);
								
void display_encoder_status(void);

void delay_100us(unsigned int);
void delay_ms(unsigned int);

//*****************************************************************************
// Global Variable
//*****************************************************************************
volatile unsigned char key_s1, key_s2, key_s3;
volatile unsigned char debounce_cnt;
volatile unsigned char seq_a, seq_b;
volatile unsigned char encoder_cnt;
volatile unsigned char encoder_max;
volatile unsigned char display_encoder_pos;

//*****************************************************************************
// Main Function
//*****************************************************************************
void main(void)
{
	P2M0 = 0x08;  // P2.3, Strong Push-Pull
	BS_G = 1;  // Turn off display	
	PWR_EN = 0;
	delay_ms(500);
	debounce_cnt = 0;
	encoder_cnt = 0;
	encoder_max = 3;
	display_encoder_pos = 0;
	ad7304_init();
	AUXR &= 0x3F;
	TMOD = 0xD1;	
	TL0 = 0x7D;  // 1ms
	TH0 = 0xF9;
	TF0 = 0;	
	TL1 = 0xFF;  // 32768
	TH1 = 0x7F;
	ET0 = 1;
	ET1 = 1;
	EA  = 1;
	TR0 = 1;
	TR1 = 1;
	
	BS_G = 0;
	
	while (1)
	{
		if (display_encoder_pos)
			display_encoder_status();
		adjust_display(encoder_cnt);
	}
}

//*****************************************************************************
// Function Implementation
//*****************************************************************************
void adjust_display(unsigned char t)
{
	if (key_s3)
	{
		KEY_S3 = 1;
		if (KEY_S3 == 1)
		{		
			key_s3 = FALSE;
			if (display_encoder_pos)
				display_encoder_pos = 0;
			else
				display_encoder_pos = 1;
			return;
		}
	}

	switch (t)
	{
		case 0:
			calibration();
			break;
		case 1:
			display_grid();
			break;
		case 2:
			character_test();
			break;
		case 3:
			draw_circle();
			break;
		default:
			calibration();
			break;
	}
}

void draw_circle(void)
{
	vect_plot(127, 127, 254, 254, CIR, 0xFF);
}

void draw_character(unsigned char c, unsigned char x_pos, unsigned char y_pos, unsigned char sz)
{
	unsigned int shape_index, length;
	unsigned char tmp, i;

	c -= 32;
	tmp = c << 1; // c * 2	
	shape_index = character_index_table[tmp] * 6; // shape table start position
	tmp++;
	length = character_index_table[tmp]; // shape table length

	for( i = 0; i < length; i++ )
	{
		vect_plot(character_shape_table[shape_index++] * sz + x_pos,
							character_shape_table[shape_index++] * sz + y_pos,
							character_shape_table[shape_index++] * sz,
							character_shape_table[shape_index++] * sz,
							character_shape_table[shape_index++],
							character_shape_table[shape_index++]);
	}	
}

void character_test(void)
{
	unsigned char x, y;
	unsigned char i;

	x = 0;
	y = 0;
	for(i = 0; i < 96 ; i++)
	{
		draw_character(' '+i, x*16, 50+y*30, 1);
		x++;
		if(x == 16)
		{
			x = 0;
			y++;
		}
	}
}

void calibration(void)
{
	vect_plot(127, 127, 254, 254, 1, 0x55);

	vect_plot(0,   127, 0, 254, 0, 0x1E);
	vect_plot(127, 127, 0, 254, 0, 0x1E);
	vect_plot(254, 127, 0, 254, 0, 0x1E);	

	vect_plot(127, 0,   254, 0, 0, 0x1E);	
	vect_plot(127, 127, 254, 0, 0, 0x1E);
	vect_plot(127, 254, 254, 0, 0, 0x1E);

	vect_plot(127, 127, 254, 254, 0, 0x1E);
	vect_plot(127, 127, 254, 254, 2, 0x1E);	
	
	vect_plot(37,   37, 254, 254, POS, 0x0C);
	vect_plot(217,  37, 254, 254, NEG, 0x0C);
	vect_plot(217, 217, 254, 254, POS, 0x0C);
	vect_plot(37,  217, 254, 254, NEG, 0x0C);
}

void display_grid(void)
{
	unsigned char i;
	
	for(i = 0; i < 11; i++)
	{
		vect_plot(i*25, 127, 0, 255, 0, 0xFF);
		vect_plot(127, i*25, 255, 0, 0, 0xFF);		
	}	
}

void display_encoder_status(void)
{
	draw_character('0' + encoder_cnt/100, 80, 108, 2);
	draw_character('0' + (encoder_cnt%100)/10, 112, 108, 2);
	draw_character('0' + encoder_cnt%10, 144, 108, 2);	
}

/*
 * Plot a vector segment
 * Positon x, positon y, x size, y size, shape, segment
 */
void vect_plot(unsigned char x, unsigned char y,
								unsigned char xs, unsigned char ys,
								unsigned char s, unsigned char seg)
{
	ad7304_update(2, x);
	ad7304_update(3, y);
	ad7304_update(0, xs);
	ad7304_update(1, ys);	
	BS_G = 1; // Turn off display
	AD7304_LDAC = 0;
	_nop_();
	AD7304_LDAC = 1;
	
	C_SEL0 = s&0x1;
	C_SEL1 = (s&0x2)>>1;
	
	BS_SEL = seg;
	BS_G = 0; // Turn on display
}

void ad7304_init(void)
{
	AD7304_CS = 1;
	AD7304_CLK = 1;
	AD7304_LDAC = 1;		
}

void ad7304_update(unsigned char ch, unsigned char value)
{
	unsigned char i;
	unsigned char tmp, mask;
	
	mask = 0x08;
	tmp = 0x0C | ch;  

	AD7304_CS = 0;	
	for( i = 0; i < 4; i++ )
	{
		AD7304_CLK = 0;
		if( tmp & mask )
			AD7304_SDI = 1;
		else
			AD7304_SDI = 0;
		mask >>= 1;
		AD7304_CLK = 1;					
	}
	mask = 0x80;
	tmp = value;
	for( i = 0; i < 8; i++ )
	{
		AD7304_CLK = 0;
		if( tmp & mask )
			AD7304_SDI = 1;
		else
			AD7304_SDI = 0;
		mask >>= 1;
		AD7304_CLK = 1;				
	}
	AD7304_CS = 1;
	AD7304_SDI = 1;
}

/*
void delay_100us(unsigned int n)		//@20.000MHz
{
	unsigned char i, j;
	unsigned char k;
	
	for(k = 0; k < n; k++)
	{
		_nop_();
		_nop_();
		i = 2;
		j = 238;
		do
		{
			while (--j);
		} while (--i);
	}	
}
*/

void delay_ms(unsigned int ms)
{
	unsigned int cnt;
	unsigned char i, j;
	
	for (cnt = 0; cnt < ms; cnt++)
	{
		_nop_();
		_nop_();
		i = 20;
		j = 112;
		do
		{
			while (--j);
		} while (--i);
	}		
}

void Timer0_Routine(void) interrupt 1
{
	TL0 = 0x7D; //1ms
	TH0 = 0xF9;
	PULSE_OUT_0 = ~PULSE_OUT_0;
	if (key_s3 == FALSE)
	{
		KEY_S3 = 1;
		if (KEY_S3 == 0)
		{
			debounce_cnt++;
			if (debounce_cnt == 50)
			{
				key_s3 = TRUE;
				debounce_cnt = 0;
			}
		}
		else 
		{
			debounce_cnt = 0;
		}
	}
	
	KEY_S1 = 1;
	KEY_S2 = 1;
	if ((KEY_S1 != key_s1) || (KEY_S2 != key_s2))
	{
		key_s1 = KEY_S1;
		key_s2 = KEY_S2;
		seq_a <<= 1;
		seq_a |= KEY_S1;
			
		seq_b <<= 1;
		seq_b |= KEY_S2;
			
		// Mask the MSB four bits
		seq_a &= 0x0F;		// 0b00001111
		seq_b &= 0x0F;		// 0b00001111;
			
		// Compare the recorded sequence with the expected sequence
		if (seq_a == 0x09 && seq_b == 0x03) 
		{
			encoder_cnt++;
			if (encoder_cnt > encoder_max)
				encoder_cnt = 0;
		}
		 
		if (seq_a == 0x03 && seq_b == 0x09) 
		{
			if (encoder_cnt == 0)
				encoder_cnt = encoder_max;
			else
				encoder_cnt--;
		}
	}	
}

// Timer 1
// Counter mode
void Timer1_Routine(void) interrupt 3
{
	TL1  = 0xFF;
	TH1  = 0x7F;
	PULSE_OUT_1 = ~PULSE_OUT_1;
}
