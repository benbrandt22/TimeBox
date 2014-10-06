/*
    3-4-09
    Copyright Spark Fun Electronics© 2009
    Nathan Seidle
	
	A basic alarm clock that uses a 4 digit 7-segment display. Includes alarm and snooze.
	Alarm will turn back on after 9 minutes if alarm is not disengaged. 
	
	Alarm is through a piezo buzzer.
	Three input buttons (up/down/snooze)
	1 slide switch (engage/disengage alarm)
	
	Display is with PWM of segments - no current limiting resistors!
	
	Uses external 16MHz clock as time base.
	
	Set fuses:
	avrdude -p m168 -P lpt1 -c stk200 -U lfuse:w:0xE6:m
	
	program hex:
	avrdude -p m168 -P lpt1 -c stk200 -U flash:w:clockit-v10.hex

*/

#define NORMAL_TIME
//#define DEBUG_TIME

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define sbi(port, pin)   ((port) |= (uint8_t)(1 << pin))
#define cbi(port, pin)   ((port) &= (uint8_t)~(1 << pin))

#define FOSC 16000000 //16MHz internal osc
//#define FOSC 1000000 //1MHz internal osc
//#define BAUD 9600
//#define MYUBRR (((((FOSC * 10) / (16L * BAUD)) + 5) / 10) - 1)

#define STATUS_LED	5 //PORTB

#define TRUE	1
#define FALSE	0

#define SEG_A	PORTC3
#define SEG_B	PORTC5
#define SEG_C	PORTC2
#define SEG_D	PORTD2
#define SEG_E	PORTC0
#define SEG_F	PORTC1

#define DIG_1	PORTD0
#define DIG_2	PORTD1
#define DIG_3	PORTD4
#define DIG_4	PORTD6

#define DP		PORTD5
#define COL		PORTD3

#define BUT_UP		PORTB5
#define BUT_DOWN	PORTB4
#define BUT_SNOOZE	PORTD7
#define BUT_ALARM	PORTB0

#define BUZZ1	PORTB1
#define BUZZ2	PORTB2

#define AM	1
#define PM	2

//Declare functions
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void ioinit (void);
void delay_ms(uint16_t x); // general purpose delay
void delay_us(uint16_t x);

void display_number(uint8_t number, uint8_t digit);
void display_time(uint16_t time_on);

void display_brightlevel_number_adjusted(uint8_t number, uint16_t bright_level);

void clear_display(void);
void check_buttons(void);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Declare global variables
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
uint8_t hours, minutes, seconds, ampm, flip;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

ISR (SIG_OVERFLOW1) 
{
	//Prescalar of 1024
	//Clock = 16MHz
	//15,625 clicks per second
	//64us per click	
    
    TCNT1 = 49911; //65536 - 15,625 = 49,911 - Preload timer 1 for 49,911 clicks. Should be 1s per ISR call

	//Debug with faster time!
    //TCNT1 = 63581; //65536 - 1,953 = 63581 - Preload timer 1 for 63581 clicks. Should be 0.125s per ISR call - 8 times faster than normal time
	
	if(flip == 0)
	{
		flip = 1;
	}
	else
	{
		flip = 0;
	}
	
	seconds++;
	if(seconds == 60)
	{
		seconds = 0;
		minutes++;
		if(minutes == 60)
		{
			minutes = 0;
			hours++;

			if(hours == 12)
			{
				if(ampm == AM)
					ampm = PM;
				else
					ampm = AM;
			}

			if(hours == 13) hours = 1;
		}
	}
}

ISR (SIG_OVERFLOW2) 
{
	display_time(10); //Display current time for 1ms
}


int main (void)
{
	ioinit(); //Boot up defaults

	while(1)
	{
		check_buttons(); //See if we need to set the time or snooze
	}
	
    return(0);
}

//Checks buttons for system settings
void check_buttons(void)
{
	uint8_t i;
	uint8_t sling_shot = 0;
	uint8_t minute_change = 1;
	uint8_t previous_button = 0;

	//Check for set time
	if ( (PINB & ((1<<BUT_UP)|(1<<BUT_DOWN))) == 0)
	{
		delay_ms(2000);

		if ( (PINB & ((1<<BUT_UP)|(1<<BUT_DOWN))) == 0)
		{
			//You've been holding up and down for 2 seconds
			//Set time!

			while( (PINB & ((1<<BUT_UP)|(1<<BUT_DOWN))) == 0) //Wait for you to stop pressing the buttons
				display_time(1000); //Display current time for 1000ms

			while(1)
			{
				if ( (PIND & (1<<BUT_SNOOZE)) == 0) //All done!
				{
					for(i = 0 ; i < 3 ; i++)
					{
						display_time(250); //Display current time for 100ms
						clear_display();
						delay_ms(250);
					}
					
					while((PIND & (1<<BUT_SNOOZE)) == 0) ; //Wait for you to release button
					
					break; 
				}

				if ( (PINB & (1<<BUT_UP)) == 0)
				{
					//Ramp minutes faster if we are holding the button
					//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
					if(previous_button == BUT_UP) 
						sling_shot++;
					else
					{	
						sling_shot = 0;
						minute_change = 1;
					}
						
					previous_button = BUT_UP;
					
					if (sling_shot > 5)
					{
						minute_change++;
						if(minute_change > 30) minute_change = 30;
						sling_shot = 0;
					}
					//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
					
					minutes += minute_change;
					if (minutes > 59)
					{
						minutes -= 60;
						hours++;

						if(hours == 13) hours = 1;

						if(hours == 12)
						{
							if(ampm == AM) 
								ampm = PM;
							else
								ampm = AM;
						}
					}
					delay_ms(100);
				}
				
				if ( (PINB & (1<<BUT_DOWN)) == 0)
				{
					//Ramp minutes faster if we are holding the button
					//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
					if(previous_button == BUT_DOWN) 
						sling_shot++;
					else
					{
						sling_shot = 0;
						minute_change = 1;
					}
						
					previous_button = BUT_DOWN;
					
					if (sling_shot > 5)
					{
						minute_change++;
						if(minute_change > 30) minute_change = 30;
						sling_shot = 0;
					}
					//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


					minutes -= minute_change;
					if(minutes > 60)
					{
						minutes = 59;
						hours--;
						if(hours == 0) hours = 12;

						if(hours == 11)
						{
							if(ampm == AM) 
								ampm = PM;
							else
								ampm = AM;
						}
					}
					delay_ms(100);
				}
				
				//clear_display(); //Blink display
				//delay_ms(100);
			}
		}
	}
}

void clear_display(void)
{
	PORTC = 0;
	PORTD &= 0b10000000;
}

void display_number(uint8_t number, uint8_t digit)
{
	//Set the digit
	PORTD |= (1<<DIG_1)|(1<<DIG_2)|(1<<DIG_3)|(1<<DIG_4)|(1<<COL);
	
	switch(digit)
	{
		case 1:
			PORTD &= ~(1<<DIG_1);//Select Digit 1
			break;
		case 2:
			PORTD &= ~(1<<DIG_2);//Select Digit 2
			break;
		case 3:
			PORTD &= ~(1<<DIG_3);//Select Digit 3
			break;
		case 4:
			PORTD &= ~(1<<DIG_4);//Select Digit 4
			break;
		case 5:
			PORTD &= ~(1<<COL);//Select Digit COL
			break;
		default: 
			break;
	}
	
	PORTC = 0; //Clear all segments
	PORTD &= ~((1<<2)|(1<<5)); //Clear D segment and decimal point
	
	switch(number)
	{
		case 0:
			PORTC |= 0b00101111; //Segments A, B, C, D, E, F
			PORTD |= 0b00000100;
			break;
		case 1:
			PORTC |= 0b00100100; //Segments B, C
			//PORTD |= 0b00000000;
			break;
		case 2:
			PORTC |= 0b00111001; //Segments A, B, D, E, G
			PORTD |= 0b00000100;
			break;
		case 3:
			PORTC |= 0b00111100; //Segments ABCDG
			PORTD |= 0b00000100;
			break;
		case 4:
			PORTC |= 0b00110110; //Segments BCGF
			PORTD |= 0b00000000;
			break;
		case 5:
			PORTC |= 0b00011110; //Segments AFGCD
			PORTD |= 0b00000100;
			break;
		case 6:
			PORTC |= 0b00011111; //Segments AFGDCE
			PORTD |= 0b00000100;
			break;
		case 7:
			PORTC |= 0b00101100; //Segments ABC
			PORTD |= 0b00000000;
			break;
		case 8:
			PORTC |= 0b00111111; //Segments ABCDEFG
			PORTD |= 0b00000100;
			break;
		case 9:
			PORTC |= 0b00111110; //Segments ABCDFG
			PORTD |= 0b00000100;
			break;

		case 10:
			//Colon
			PORTC |= 0b00101000; //Segments AB
			//PORTD |= 0b00000000;
			break;

		case 11:
			//Alarm dot
			PORTD |= 0b00100000;
			break;

		case 12:
			//AM dot
			PORTC |= 0b00000100; //Segments C
			break;

		default: 
			PORTC = 0;
			break;
	}
	
}

//Displays current time
//Brightness level is an amount of time the LEDs will be in - 200us is pretty dim but visible.
//Amount of time during display is around : [ BRIGHT_LEVEL(us) * 5 + 10ms ] * 10
//Roughly 11ms * 10 = 110ms
//Time on is in (ms)
void display_time(uint16_t time_on)
{
//	uint16_t bright_level = 1000;
	uint16_t bright_level = 50;
	
	//time_on /= 11; //Take the time_on and adjust it for the time it takes to run the display loop below

	for(uint16_t j = 0 ; j < time_on ; j++)
	{

#ifdef NORMAL_TIME
		//Display normal hh:mm time
		if(hours > 9)
		{
			display_number(hours / 10, 1); //Post to digit 1
			display_brightlevel_number_adjusted(hours / 10, bright_level);
		}

		display_number(hours % 10, 2); //Post to digit 2
		display_brightlevel_number_adjusted(hours % 10, bright_level);

		display_number(minutes / 10, 3); //Post to digit 3
		display_brightlevel_number_adjusted(minutes / 10, bright_level);

		display_number(minutes % 10, 4); //Post to digit 4
		display_brightlevel_number_adjusted(minutes % 10, bright_level);
#else
		//During debug, display mm:ss
		display_number(minutes / 10, 1); 
		display_brightlevel_number_adjusted(minutes / 10, bright_level);

		display_number(minutes % 10, 2); 
		display_brightlevel_number_adjusted(minutes % 10, bright_level);

		display_number(seconds / 10, 3); 
		display_brightlevel_number_adjusted(seconds / 10, bright_level);

		display_number(seconds % 10, 4); 
		display_brightlevel_number_adjusted(seconds % 10, bright_level);
#endif
		
		//Flash colon for each second
		if(flip == 1) 
		{
			display_number(10, 5); //Post to digit COL
			delay_us(bright_level);
		}
		
		clear_display();
		delay_us(bright_level);
	}
}

void display_brightlevel_number_adjusted(uint8_t number, uint16_t bright_level)
{
	// Based on the number being displayed, determine how many segments will be lit.
	// Since more segments means a dimmer light, adjust the bright_level to keep
	// them turned on for a longer time when more segments are lit
	uint8_t segments;
	
	switch(number)
	{
		case 0:
			segments = 6;
			break;
		case 1:
			segments = 2;
			break;
		case 2:
			segments = 5;
			break;
		case 3:
			segments = 5;
			break;
		case 4:
			segments = 4;
			break;
		case 5:
			segments = 5;
			break;
		case 6:
			segments = 6;
			break;
		case 7:
			segments = 3;
			break;
		case 8:
			segments = 7;
			break;
		case 9:
			segments = 6;
			break;
		default: 
			segments = 0;
			break;
	}
	
	uint16_t new_bright_level;
	new_bright_level = (bright_level * (1 + (0.2 * segments)));
	
	delay_us(new_bright_level);
}

void ioinit(void)
{
    //1 = output, 0 = input 
    DDRB = 0b11111111 & ~((1<<BUT_UP)|(1<<BUT_DOWN)|(1<<BUT_ALARM)); //Up, Down, Alarm switch  
    DDRC = 0b11111111;
    DDRD = 0b11111111 & ~(1<<BUT_SNOOZE); //Snooze button
	

	PORTB = (1<<BUT_UP)|(1<<BUT_DOWN)|(1<<BUT_ALARM); //Enable pull-ups on these pins
	PORTD = (1<<BUT_SNOOZE); //Enable pull-up on snooze button
	PORTC = 0;

    //Init Timer0 for delay_us
	TCCR0B = (1<<CS01); //Set Prescaler to clk/8 : 1click = 0.5us(assume we are running at external 16MHz). CS01=1 
	
	//Init Timer1 for second counting
	TCCR1B = (1<<CS12)|(1<<CS10); //Set prescaler to clk/1024 :1click = 64us (assume we are running at 16MHz)
	TIMSK1 = (1<<TOIE1); //Enable overflow interrupts
    TCNT1 = 49911; //65536 - 15,625 = 49,911 - Preload timer 1 for 49,911 clicks. Should be 1s per ISR call
	
	//Init Timer2 for updating the display via interrupts
	TCCR2B = (1<<CS22)|(1<<CS21)|(1<<CS20); //Set prescalar to clk/1024 : 1 click = 64us (assume 16MHz)
	TIMSK2 = (1<<TOIE2);
	//TCNT2 should overflow every 16.384 ms (256 * 64us)
	
	hours = 88;
	minutes = 88;
	seconds = 88;
	
	sei(); //Enable interrupts

	hours = 12;
	minutes = 00;
	seconds = 00;
	ampm = AM;

	//Segment test
	/*while(1)
	{
		PORTD = 0;
		PORTC = 0xFF;
		delay_ms(1000);

		PORTD = 0xFF;
		PORTC = 0xFF;
		delay_ms(1000);
	}*/	
}

//General short delays
void delay_ms(uint16_t x)
{
	for (; x > 0 ; x--)
	{
		delay_us(250);
		delay_us(250);
		delay_us(250);
		delay_us(250);
	}
}

//General short delays
void delay_us(uint16_t x)
{
	x *= 2; //Correction for 16MHz
	
	while(x > 256)
	{
		TIFR0 = (1<<TOV0); //Clear any interrupt flags on Timer2
		TCNT0 = 0; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
		while( (TIFR0 & (1<<TOV0)) == 0);
		
		x -= 256;
	}

	TIFR0 = (1<<TOV0); //Clear any interrupt flags on Timer2
	TCNT0 = 256 - x; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
	while( (TIFR0 & (1<<TOV0)) == 0);
}