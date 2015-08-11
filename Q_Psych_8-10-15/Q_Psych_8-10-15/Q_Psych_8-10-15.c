/*
 * Q_Corp_Psychrometer.c
 *
 * Created: 7/9/2014 3:12:14 PM
 *  Author: Dan
 */ 

#define F_CPU 8000000UL


#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>        // Supplied Watch Dog Timer Macros

#include "lcd.h"
#define F_CPU 8000000UL
void Port_Init()
{
	PORTA = 0b00011111;		DDRA = 0b01000000;
	PORTB = 0b00000000;		DDRB = 0b00000000;
	PORTC = 0b00000000;		DDRC = 0b11110111;
	PORTD = 0b11000000;		DDRD = 0b00001000;
	PORTE = 0b00000000;		DDRE = 0b00110000;
	PORTF = 0b00000000;		DDRF = 0b00000000;
	PORTG = 0b00000000;		DDRG = 0b00000000;
}

void red_light()
{
	PORTA |= (1<<6);
}

void green_light()
{
	if(PORTA & (1<<6))
	{
		PORTA ^= (1<<6);
	}
	
}

void WDT_Init()
{
	wdt_reset();
	wdt_enable(WDTO_2S);
	WDTCR = (1<<WDCE)|(1<<WDE);
	wdt_reset();
}


int errors = 0;
int feedback_speed = 0;
int new_feedback_speed = 0;
int page = 0;
int cal_speed = 50;
int cal_number = 0;
int cal_point_1 = 0;
int cal_point_2 = 0;
int target_feedback = 0;
double pwm_duty_cycle = 0;
float real_speed = 0;
float average_speed = 0;
char avg_speed_string[4];


int main(void)
{
	
	Port_Init();
	DDRE |= (1<<3);
	DDRA |= (1<<6);
	LCDInit(LS_NONE);
	
	DDRC |= (1<<3);
	PORTC |= (1<<3);
	cal_point_1 = eeprom_read_dword(0);
	cal_point_2 = eeprom_read_dword((uint32_t*)5);
	target_feedback = (cal_point_1 + cal_point_2)/2;
	cli();
	TIMSK |= ((1 << TICIE1)| (1 << TOIE1));   //Set capture interrupt
	TCCR1B |= ((1 << ICES1) | (1<<CS10));
	TCCR3A = ((1<<COM3A1));
	TCCR3B = ((1<<CS30) | (1<<WGM33));
	ICR3 = 1000;
	OCR3A = 0;
	sei(); 
	LCDClear();
	//LCDWriteString("Powering On...");
	red_light();
	WDT_Init();
	OCR3A = (int)pwm_duty_cycle;
	_delay_ms(100);
	/*int warm_up_timer = 10;
	while(warm_up_timer > 0)
	{
		if (feedback_speed < (target_feedback - 7))
		{
			if (feedback_speed > (target_feedback - 100))
			{
				pwm_duty_cycle += 1;
			}
		}
		if (feedback_speed > (target_feedback + 7))
		{
			if(feedback_speed < (target_feedback + 100))
			{
				pwm_duty_cycle -= 1;
			}
			
		}
		if (feedback_speed < (target_feedback - 100) )
		{
			pwm_duty_cycle += 3;
		}
		if (feedback_speed > (target_feedback + 100))
		{
			pwm_duty_cycle -= 3;
		}
		if(pwm_duty_cycle > 499){pwm_duty_cycle = 499;}
		if(pwm_duty_cycle < 0 ){pwm_duty_cycle = 0;}
		
		wdt_reset();
		_delay_ms(100);
		LCDClear();
		char feedback_speed_string[4];
		snprintf(feedback_speed_string,4,"%d",feedback_speed);
		LCDWriteString(feedback_speed_string);
		warm_up_timer -= 1;
	}
	*/
	
	
	
	
    while(1)
    {
		feedback_speed = (new_feedback_speed);
        if (page == 0)
		{
			wdt_reset();
			real_speed = ((double) feedback_speed / (double) target_feedback)*4.5f;
			average_speed = (real_speed+average_speed*4.0f)/5.0f;
			int avg_speed_int = (int)average_speed;
			float avg_speed_float = average_speed - avg_speed_int;
			int avg_speed_dec = avg_speed_float*10;
			snprintf(avg_speed_string,4,"%01d.%01d",avg_speed_int,avg_speed_dec);
			LCDClear();
			LCDWriteString("Airflow: ");
			LCDWriteString(avg_speed_string);
			LCDWriteString(" m/s");
			LCDWriteStringXY(0,1,"Target: 4.5 m/s");
			wdt_reset();
			_delay_ms(100);
			wdt_reset();
			
			// speed control section
			if (feedback_speed < (target_feedback - 10))
			{
				if (feedback_speed > (target_feedback - 200))
				{
					pwm_duty_cycle += 0.25;
				}
			}
			if (feedback_speed > (target_feedback + 10))
			{
				if(feedback_speed < (target_feedback + 200))
				{
					pwm_duty_cycle -= 0.25;
				}
				
			}
			if (feedback_speed < (target_feedback - 200) )
			{
				pwm_duty_cycle += 4;
			}
			if (feedback_speed > (target_feedback + 200))
			{
				pwm_duty_cycle -= 4;
			}
			if (feedback_speed < (target_feedback - 60))
			{
				errors += 20;
			}
			if (feedback_speed > (target_feedback + 60))
			{
				errors += 20;
			}
			if (feedback_speed < (target_feedback + 40))
			{
				if (feedback_speed > (target_feedback - 40))
				{
					errors = 0;
				}
			}
			if (errors > 250) 
			{
				errors = 250;
				
			}
			if (pwm_duty_cycle > 999) {pwm_duty_cycle = 999;}
			if (pwm_duty_cycle < 0){pwm_duty_cycle = 0;}
				
		
			
			OCR3A = (int)pwm_duty_cycle;
			wdt_reset();
		}
		
		
		
		
		
		if (page == 1)
		{
			LCDClear();
			LCDWriteString("Calibration Mode");
			if(cal_number == 0)
			{
				LCDWriteStringXY(0,1,"Cal. Point 1");
			}
			if(cal_number == 1)
			{
				LCDWriteStringXY(0,1,"Cal. Point 2");
			}
			if(cal_number == 2)
			{
				LCDWriteStringXY(0,1,"Press %1 To Save");
			    _delay_ms(50);
				LCDWriteStringXY(0,1,"Press %2 To Save");
				_delay_ms(50);
				wdt_reset();
			}
			wdt_reset();
			_delay_ms(150);
			wdt_reset();
			
		}
		
		
		
		
		if (!(PINA & (1<<3)))
		{
			page++;
			while(!(PINA & (1<<3)))
			{
				wdt_reset();
			} //right
			_delay_ms(25);
			if (page > 1)
			{
				page = 0;
			}
		}
		
		
		
		if (!(PINA & (1<<0))) //up
		{
			if (page == 1)
			{
				cal_speed += 5;
				if (cal_speed > 1000)
				{
					cal_speed = 1000;
				}
				OCR3A = cal_speed;
				wdt_reset();
			}	
			_delay_ms(25);	
		}
		
		
		
		
		if (!(PINA & (1<<4))) //down
		{
			if (page == 1)
			{
				cal_speed -= 5 ;
				if (cal_speed < 0)
				{
					cal_speed = 0;
				}
				OCR3A = cal_speed;
				wdt_reset();
			}
			_delay_ms(25);
		}
		
		
		
		
		if (!(PINA & (1<<2))) //center
		{
			wdt_reset();
			if (page == 1)
			{			
				switch (cal_number)
				{
					case 0:
					cal_point_1 = feedback_speed;
					cal_number = 1;
					break;
					
					case 1:
					cal_point_2 = feedback_speed;
					cal_number = 2;
					break;
					
					case 2:
					wdt_reset();
					eeprom_write_dword(0,cal_point_1);
					eeprom_write_dword((uint32_t*)5,cal_point_2);
					wdt_reset();
					page = 0;
					cal_number = 0;
					break;
					
				}
			}
			
			
			while(!(PINA & (1<<2)))
			{
				wdt_reset();
			}
			_delay_ms(25);
		}
		if (!(PINA & (1<<1))) //left
		{
			page--;
			while(!(PINA & (1<<3)))
			{
				wdt_reset();
			}
			_delay_ms(25);
			if (page < 0)
			{
				page = 1;
			}
		}
		if (errors == 0)
		{
			green_light();
		}
		if(errors >= 250)
		{
			red_light();
		}
		
		
    }
}

ISR(TIMER1_CAPT_vect)
{
	new_feedback_speed = 500000/ICR1;

	ICR1 = 0;
	TCNT1 = 0;
	
	
}
ISR(TIMER1_OVF_vect)
{
	TCNT1 = 0;
	ICR1 = 0;
	errors ++;
	if (errors > 250)
	{
		 errors = 250;
	}
	feedback_speed = 0;
	
}

