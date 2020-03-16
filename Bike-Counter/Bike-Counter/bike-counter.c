/*
 * bike-counter.c
 *
 * Created: 22.12.2019 21:00:45
 * Author : dmusial98
 
 //timer 0 przerwanie dla wyœwietlania
 */ 

#define F_CPU 1000000UL
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define SEGMENTY_PORT PORTB
#define SEGMENTY_KIERUNEK DDRB

#define ANODY_PORT PORTC
#define ANODY_KIERUNEK DDRC

#define PRZYCISK_KIERUNEK DDRD
#define PRZYCISK_PORT PORTD

#define ANODA_1 (1 << PC0)
#define ANODA_2 (1 << PC1)
#define ANODA_3 (1 << PC2)
#define ANODA_4 (1 << PC3)

#define SEG_A	(1<<0)
#define SEG_B	(1<<1)
#define SEG_C	(1<<2)
#define SEG_D	(1<<3)
#define SEG_E	(1<<4)
#define SEG_F	(1<<5)
#define SEG_G	(1<<6)
#define SEG_DP	(1<<7)

#define WGM21 (1<<3)
#define CS21 (1<<1)
#define OCIE2 (1<<7)

#define LED_DISP_NO 4

volatile uint8_t LED_DIGITS[LED_DISP_NO];  //4 cyfry wyswietlacza LED
//volatile uint16_t counter; 
volatile uint16_t times[5] = {0, 0, 0 ,0 ,0};	//tablica przechowuj¹ca czasy miedzy impulsami
volatile uint8_t number_of_impulse = 0; //numer impulsu w programie
volatile uint32_t velocity = 0; //format to 00,000 000 000
volatile uint16_t perimeter = 1273; //obwód w milimetrach

const uint8_t DIGITS[] PROGMEM = { //wspolna anoda
	~(SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F),
	~(SEG_B | SEG_C),
	~(SEG_A | SEG_B | SEG_D | SEG_E | SEG_G),
	~(SEG_A | SEG_B| SEG_C| SEG_D | SEG_G), //3
	~(SEG_B | SEG_C | SEG_F | SEG_G),
	~(SEG_A | SEG_C | SEG_D | SEG_F | SEG_G),
	~(0xFF & ~SEG_B & ~SEG_DP), //6
	~(SEG_A | SEG_B | SEG_C),
	~(0xFF & ~SEG_DP),
	~(0xFF & ~SEG_E & ~SEG_DP), //9
	0xFF	//pusty wyœwietlacz
	};
	
void count_velocity()
{
	if(number_of_impulse > 1) //jest co zliczac
	{
		uint8_t num_of_times = 0; //wiadomoœæ ile mamy przedzia³ów czasowych, w których policzono czas miedzy impulsami
		
		for(int i = 0; i < 4; i++)
		{
			if(times[i] != 0)
				num_of_times++;
		}
		
		velocity = (perimeter * num_of_times * 100000)/(times[0] + times[1] + times[2] + times[3]) * 36;
		velocity /= 100000;
	}
	else 
		velocity = 0;
}
	
void shift_time_table()
{
	times[0] = times[1];
	times[1] = times[2];
	times[2] = times[3];
	times[3] = times[4];
}
	
void timer_init()
{
	TCCR0  |= 0x02;
	TIMSK |= 0x01;  //przerwanie odpowiedzialne za wyœwietlanie
	
	TCCR2 |= WGM21 | CS21;
	OCR2 = 125;		//8*125 = 1000 czyli 1 ms, co tyle wywoluje sie przerwanie
	TIMSK |= OCIE2; //przerwanie odpowiedzialne za odliczanie czasu
}

void button_init()
{
	PRZYCISK_KIERUNEK &= 0xFB;  //ustawienie PD2 jako wejœcie (dla INT0)	
	PRZYCISK_PORT |= 0x0F; //rezystor podci¹gaj¹cy dla PD3
	
	MCUCR |= 0x03; //poziom niski na PD2 wywo³uje przerwanie
	GICR |= 0x40;
}

void show_on_led(uint8_t value, int decimal_point)
{
	uint8_t tmp = 0xFF;
	
	if(value  < 11) 
		tmp =  pgm_read_byte(&DIGITS[value]);
		
		if(decimal_point == 1)
			SEGMENTY_PORT = tmp & (~SEG_DP);
		else 
			SEGMENTY_PORT = tmp;
}
	
ISR(TIMER0_OVF_vect) //przerwanie obs³ugujace wyœwietlanie
{
	static uint8_t led_no;
	
	ANODY_PORT &= 0x00;
	led_no = (led_no + 1) % LED_DISP_NO;
	
	if(led_no  == 2)
		show_on_led(LED_DIGITS[led_no], 1);
	else 
		show_on_led(LED_DIGITS[led_no], 0);
		
	ANODY_PORT = ~(1 << led_no);
}

ISR(INT0_vect) //obs³uga przerwania na przycisku PD2 -> stan niski
{	
	if(number_of_impulse < 5)
		number_of_impulse++;
	else 
	{
		shift_time_table();
		times[4] = 0;
	}
	
}

ISR(TIMER2_COMP_vect) //odliczenie 1 ms
{
	if(number_of_impulse == 1)
	{
		times[0]++;
	}
	else if(number_of_impulse == 2)
	{
		times[1]++;
	}
	else if(number_of_impulse == 3)
	{
		times[2]++;
	}
	else if(number_of_impulse == 4)
	{
		times[3]++;
	}
	else if(number_of_impulse == 5)
	{
		times[4]++;
	}
	
	for(int i = 0; i < 5; i++)
	{
		if(times[i] >= 2500)
		{
			number_of_impulse = 0;
			for(int j = 0; j < 5; j++)
				times[j] = 0;	
		}
	}
	
	
}
	
void wysw_init()
{
	SEGMENTY_KIERUNEK = 0xFF;
	SEGMENTY_PORT = 0xFF;
	
	ANODY_PORT &= 0x00;
	ANODY_KIERUNEK = 0xFF;
}

int main(void)
{
	wysw_init();
	timer_init();
	button_init();
	
	sei();
	
    while (1) 
    {
		count_velocity();
		uint16_t counter_led = velocity;
		
		volatile uint16_t delay = 0;
		
		for(int8_t i = LED_DISP_NO - 1; i >= 0; i--)
		{
			LED_DIGITS[i] = counter_led % 10;
			counter_led /= 10;			
		}		//zmienianie wartoœci do wyswietlania 
    
		for(uint16_t i = 0; i < 25000; i++)
		{
			delay++;
		}		//petla wydluzajaca czas pomiedzy kolejnym liczeniem predkosci
	}
}

