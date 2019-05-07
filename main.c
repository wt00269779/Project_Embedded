#define RW PORTC0
#define RS PORTC1
#define E  PORTC2
#define CTRL_DDR  DDRC
#define CTRL_PORT PORTC
#define DATA_DDR  DDRD
#define DATA_PORT PORTD

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL
#include <util/delay.h>

//set time
int min=0;
int sec=0;
int nap=0;
int timeWait;
int refWait,initMin = 0;
int tcount = 0,count =0;

void sendCommand(unsigned char command)
{
	CTRL_PORT &= ~(1 << RW) & ~(1 << RS);  // set RS to 0 to send command
	DATA_PORT = command;
	CTRL_PORT |= (1 << E); // send short pulse to finish transfer
	_delay_ms(1);
	CTRL_PORT &= ~(1 << E);
	_delay_ms(1);
}

void sendData(char data[])
{
	int i;
	for(i = 0; i<strlen(data);i++){
		CTRL_PORT &= ~(1 << RW);
		CTRL_PORT |= (1 << RS);	// set RS to 1 to send data
		DATA_PORT = data[i];
		CTRL_PORT |= (1 << E); // send short pulse to finish transfer
		_delay_ms(1);
		CTRL_PORT &= ~(1 << E);
		_delay_ms(1);
		CTRL_PORT &= ~(1 << RS);
	}
	
}

ISR(TIMER1_OVF_vect){
	sendCommand(0x01);
	sendCommand(0x80);
	TCNT1 = 49911; // 65535-15625 = 49911
	sec++;
	if(sec == 60) {
		sec = 0;
		min++;
	}
	refWait = timeWait + initMin;
	int i=0;
	char bufferSec[5];
	char bufferMin[5];
	char bufferNap[5];
	char bufferTimewait[5];
	
	//send sec and min
	sprintf(bufferSec, "%d", sec);
	sprintf(bufferMin, "%d", min);
	sendData("Time(m):");
	if(min<10) sendData("0");
	sendData(bufferMin);
	sendData(":");
	if(sec < 10) sendData("0");
	sendData(bufferSec);
	sendCommand(0xC0);
	
	//send nap
	sendData("Nap:");
	sprintf(bufferNap, "%d", nap); //wait to change timeWait to snap
	//sprintf(bufferNap, "%d", count);
	sendData(bufferNap);
	
	//send time wait
	sendData("   SET:");
	sprintf(bufferTimewait, "%d", timeWait);
	//sprintf(bufferTimewait, "%d", refWait);
	sendData(bufferTimewait);
}

ISR(ADC_vect){
	
	timeWait = ADC/205 + 1;
	ADCSRA |= (1<<ADSC);
}

int main(void)
{
	// initialize LCD
	CTRL_DDR |= (1 << RW) | (1 << RS) | (1 << E);
	CTRL_PORT &= ~(1 << RW) & ~(1 << RS) & ~(1 << E);
	DATA_DDR = 0xFF;
	DATA_PORT = 0;
	
	sendCommand(0x38);
	sendCommand(0x0E);
	sendCommand(0x01);  // clear screen
	sendCommand(0x06);	// shift cursor to right
	sendCommand(0x80);	// set cursor to first character on line 1
	
	//set timer interrupt
	DDRC |= (1<<PORTC0);
	PORTC &= ~(1<<PORTC0);
	TCCR1B |= (1<<CS11)|(1<<CS10);
	TIMSK1 |= (1<<TOIE1);
	TCNT1 = 49911;
	
	
	//set ADC ตัวต้านทานปรับค่า
	ADMUX |= (1<<MUX2)|(1<<MUX0)|(1<<REFS0)|(0<<REFS1);
	ADCSRA |= (1<<ADEN)|(1<<ADSC)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADIE);
	
	//set input PIR
	PORTB |= (1<<PORTB0);
	
	//set
	DDRC |= (1<<PORTC4);
	PORTC |= (1<<PORTC4);
	
	sei();
	
	int refMin = 1,refSec = 0;
	refWait = 1;
	while (1)
	{
		while(!(PINB&(1<<PORTB0))==1){
			if(refWait <= min){
				if(refSec <= sec){
					if(tcount < 6*timeWait){
						PORTC = PORTC^(1<<PORTC4);
						while(!(PINC&(1<<PORTC3))==1);
						refSec = sec;
						nap++;
						PORTC = PORTC^(1<<PORTC4);
					}
					initMin = min;
					tcount = 0;
					count = 0;
					refMin = min + 1;
					_delay_ms(1000);
				}
			}
			else if(refMin <= min){ //check every min
				if(refSec <= sec){
					if(count > 7){
						initMin = min;
						tcount = 0;
					}
					count = 0;
					refMin = min + 1;
				}
			}
		}
		count++;
		tcount++;
		 //นับจำนวนครั้งที่ motion sensor ตรวจจับได้
		
		_delay_ms(1600); // ตั้งไว้ดีเลย์ให้ตรงกับ motion เฉยๆ
	}
}
