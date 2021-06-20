/* ---- Code for Digital Clock with Alarm using AVR Microcontroller ------ */

#include <avr/io.h>
#define F_CPU 8000000UL
#define SCL_CLOCK  100000L
#define Device_Write_address	0xD0
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <stdio.h>





static volatile int SEC =0;
static volatile int MIN =0;
static volatile int HOU =0;
#define LCD_Data_Dir DDRB		/* Define LCD data port direction */
#define LCD_Command_Dir DDRC		/* Define LCD command port direction register */
#define LCD_Data_Port PORTB		/* Define LCD data port */
#define LCD_Command_Port PORTC		/* Define LCD data port */
#define RS PC0				/* Define Register Select (data/command reg.)pin */
#define RW PC1				/* Define Read/Write signal pin */
#define EN PC2				/* Define Enable signal pin */
int second,minute,hour,day,date,month,year;


typedef struct
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t weekDay;
	uint8_t date;
	uint8_t month;
	uint8_t year;
}rtc_t;
void LCD_Command(unsigned char cmnd)
{
	LCD_Data_Port= cmnd;
	LCD_Command_Port &= ~(1<<RS);	/* RS=0 command reg. */
	LCD_Command_Port &= ~(1<<RW);	/* RW=0 Write operation */
	LCD_Command_Port |= (1<<EN);	/* Enable pulse */
	_delay_us(1);
	LCD_Command_Port &= ~(1<<EN);
	_delay_ms(3);
}

void LCD_Char (unsigned char char_data)	/* LCD data write function */
{
	LCD_Data_Port= char_data;
	LCD_Command_Port |= (1<<RS);	/* RS=1 Data reg. */
	LCD_Command_Port &= ~(1<<RW);	/* RW=0 write operation */
	LCD_Command_Port |= (1<<EN);	/* Enable Pulse */
	_delay_us(1);
	LCD_Command_Port &= ~(1<<EN);
	_delay_ms(1);
}

void LCD_Init (void)			/* LCD Initialize function */
{
	LCD_Command_Dir = 0xFF;		/* Make LCD command port direction as o/p */
	LCD_Data_Dir = 0xFF;		/* Make LCD data port direction as o/p */
	_delay_ms(20);			/* LCD Power ON delay always >15ms */
	
	LCD_Command (0x38);		/* Initialization of 16X2 LCD in 8bit mode */
	LCD_Command (0x0C);		/* Display ON Cursor OFF */
	LCD_Command (0x06);		/* Auto Increment cursor */
	LCD_Command (0x01);		/* Clear display */
	LCD_Command (0x80);		/* Cursor at home position */
}

void LCD_String (char *str)		/* Send string to LCD function */
{
	int i;
	for(i=0;str[i]!=0;i++)		/* Send each char of string till the NULL */
	{
		LCD_Char (str[i]);
	}
}

void LCD_String_xy (char row, char pos, char *str)/* Send string to LCD with xy position */
{
	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);	/* Command of first row and required position<16 */
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);	/* Command of first row and required position<16 */
	LCD_String(str);		/* Call LCD string function */
}

void LCD_Clear()
{
	LCD_Command (0x01);		/* clear display */
	LCD_Command (0x80);		/* cursor at home position */
}

void i2c_init()           //Initializing i2c Communication
{
	TWSR=0x00;
	TWBR=((F_CPU/SCL_CLOCK)-16)/2;
}


void i2c_start()          //Starting I2C communication
{
	TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while((TWCR &(1<<TWINT))==0);
}

void i2c_stop()            //Stopping I2C communication
{
	TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

void i2c_write(char data)       //Writing through I2C
{
	TWDR=data;
	TWCR=(1<<TWINT)|(1<<TWEN);
	while((TWCR &(1<<TWINT))==0);
}

unsigned char i2c_read()        //Read through I2C
{
	TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while(!(TWCR &(1<<TWINT)));
	return TWDR;
}
unsigned char i2c_readNak(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	
	return TWDR;
}
void time_set(void)         //Setting time
{
	i2c_init();
	i2c_start();
	i2c_write(0xD0);       //Setting writing address
	i2c_write(0x07);
	i2c_write(0x00);
	i2c_stop();
	_delay_ms(2);
}

void RTC_set(rtc_t *rtc)
{
	
	i2c_start();
	i2c_write(0xD0);
	i2c_write(0x00);
	i2c_write(rtc->sec);
	i2c_write(rtc->min);
	i2c_write(rtc->hour);
	i2c_write(rtc->weekDay);
	i2c_write(rtc->date);
	i2c_write(rtc->month);
	i2c_write(rtc->year);
	i2c_stop();
	
}
void RTC_Read_Clock(rtc_t *rtc)
{
	i2c_start();/* Start I2C communication with RTC */
	i2c_write(0xD0);
	i2c_write(0x00);
	i2c_stop();
	i2c_start();
	i2c_write(0xD1);	/* Write address to read */
	rtc->sec = i2c_read();                // read second and return Positive ACK
	rtc->min = i2c_read();                 // read minute and return Positive ACK
	rtc->hour= i2c_read();               // read hour and return Negative/No ACK
	rtc->weekDay = i2c_read();           // read weekDay and return Positive ACK
	rtc->date= i2c_read();              // read Date and return Positive ACK
	rtc->month=i2c_read();            // read Month and return Positive ACK
	rtc->year =i2c_readNak();             // read Year and return Negative/No ACK

	
	i2c_stop();			/* Stop i2C communication */

}
char compareArray(char a[],char b[]){
	int i;
	for(i=0;i<20;i++){
		if(a[i]!=b[i]){
			return 1;
		}
	}
	return 0;
}


int main(void)
{
	DDRC |= (1<<PC3);
	
	DDRB = 0xFF;
	DDRD =0x00;
	_delay_ms(50);
	
	TCCR1B |=(1<<CS12)|(1<<CS10)|(1<<WGM12);
	OCR1A=10800;
	sei();
	EIMSK |=(1<<OCIE1A);
	
	rtc_t rtc;
	i2c_init();
	time_set();
	LCD_Init();
	
	
	int ALMIN = 0;
	int ALHOU = 0;
	
	LCD_Clear();
	_delay_ms(50);
	LCD_Command(0x38);
	_delay_ms(50);
	LCD_Command(0b00001111);
	_delay_ms(50);
	char bufer[20];
	char alrm[20];
	char clk[20];
	int a= 0x00;
	int b = 0x00;
	
	
	while(1)
	{
		
		RTC_Read_Clock(&rtc);
		SEC=((uint16_t)rtc.sec);
		MIN=((uint16_t)rtc.min);
		HOU=((uint16_t)rtc.hour);
		
		sprintf(bufer, "%02x:%02x:%02x",(HOU+a),(MIN+b),(SEC));
		sprintf(alrm,"%02d:%02d",ALHOU,ALMIN);
		sprintf(clk, "%02x:%02x",(HOU+a),(MIN+b));
		LCD_String_xy(0,0,bufer);
		LCD_String_xy(1,0,"ALARM");
		LCD_String_xy(1,6,alrm);
		DDRD=0x00;
		PIND=0x00;
		
		
		if (PIND&(1<<PD5)){
			LCD_String_xy(0,9," ALM ON");
			if (compareArray(alrm,clk)==0){
				_delay_ms(100);
				LCD_Clear();
				PORTC = (1<<PORTC3);
				_delay_ms(300);
				
			}
			else{
				PORTC=(0<<PORTC3);
			}
		}
			
		
		if(!(PIND&(1<<PD5))){
			LCD_String_xy(0,8," ALM OFF");
			PORTC = (0<<PORTC3);

		}
		
		if (PIND&(1<<PD5)){
			if ((PIND&(1<<PD2))){
				 if (ALMIN<60)
				 {
					 ALMIN++;
					 _delay_ms(100);
				 }

				 if (ALMIN==60)
				 {
					 if (ALHOU<24)
					 {
						 ALHOU++;
					 }
					 ALMIN=0;
					 _delay_ms(100);
				 }
			}
			if (PIND&(1<<PD0))
			{
				if (ALHOU<24)
				{
					ALHOU++;
				}
				_delay_ms(100);
				if (ALHOU==24)
				{
					ALHOU=0;
				}
			}
			if (PIND&(1<<PD1))
			{
				if (ALHOU>0)
				{
					ALHOU--;
					_delay_ms(100);
				}
			}
			if (PIND&(1<<PD6)){
				if (ALMIN>0){
					ALMIN--;
					_delay_ms(200);
				}
			}
				
		}
		if (PIND&(1<<PD4))
		{
			LCD_String_xy(1,12,"SET");
			if ((PIND&(1<<PD2)))
			{
				if (MIN<90)
				{
					b+=1;
					_delay_ms(220);
				}
				if (MIN==96)
				{
					if (HOU<36)
					{
						a+=1;
					}
					b=-96;
					_delay_ms(220);
				}
			}
			if (PIND&(1<<PD0))
			{
				if (HOU<36)
				{if (HOU =25){
					
					a+=1;
				}
				_delay_ms(220);
				if (HOU==36)
				{
					HOU=0;
				}
			}
			if (PIND&(1<<PD1))
			{
				if (HOU>0)
				{
					a--;
					_delay_ms(220);
				}
			
				
			}
			if (PIND&(1<<PD6)){
				if (MIN>0){
					b--;
					_delay_ms(300);
				}
			}
		}
	}
}







