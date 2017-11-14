/**
ATmega128 Login Service Project
FILE	LoginService.c
DATE	2017-05-18
AUTHOR	이영현(Lee Young Hyun)
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>
#include "lcd.h"
#include <math.h>

#define SEI()	sei()
#define CLI()	cli()
#define MAX_SIZE 1024
#define	MaxLine	2
#define	MaxCol	16

#define ON	1
#define	OFF	2
#define	DELAYTIME	1
#define	DELAYTIME1	0
#define	C1	523	//도
#define	C1_	554
#define	D1	587	//레
#define	D1_	622
#define	E1	659	//미
#define	F1	699	//파
#define	F1_	740	
#define G1	784	//솔
#define G1_	831
#define A1	880	//라
#define A1_	932
#define B1	988	//시

#define C2	C1*2	//도
#define	C2_	C1_*2
#define	D2	D1*2	//레
#define D2_	D1_*2
#define E2	E1*2	//미
#define F2	F1*2	//파
#define F2_	F1_*2
#define G2	G1*2	//솔
#define G2_	G1_*2
#define A2	A1*2	//라
#define A2_	A1_*2
#define B2	B1*2	//시

#define DLY_1	DLY_4*4	//온음표
#define DLY_2	DLY_4*2	//2분음표
#define DLY_4	400	//4분음표
#define DLY_8	DLY_4/2	//8분음표
#define DLY_16	DLY_8/2	//16분음표

#define EX_LCD_DATA 	(*(volatile unsigned char *)0x8000)
#define EX_LCD_CONTROL	(*(volatile unsigned char *)0x8001)
#define EX_SS_DATA		(*(volatile unsigned char *)0x8002)
#define EX_SS_SEL		(*(volatile unsigned char *)0x8003)
#define EX_DM_SEL		(*(volatile unsigned int *)0x8004)
#define EX_DM_DATA		(*(volatile unsigned int *)0x8006)
#define EX_LED			(*(volatile unsigned char *)0x8008)

volatile long T1HIGHCNT=0xFD, T1LOWCNT=0x66;
volatile int SoundState=ON;
volatile int Soundonoff=ON;
unsigned int noteon;

int dot_heart[10] = {0x000, 0x048, 0x0b4, 0x102, 0x102, 0x084, 0x048, 0x030, 0x000, 0x000}; // 하트 도트매트릭스 (저항 342~683)
int dot_triangle[10] = {0x030, 0x078, 0x048, 0x0cc, 0x084, 0x186, 0x102, 0x303, 0x3ff, 0x1fe}; // 세모 도트매트릭스 (저항 684~1023)
int dot_xxx[10] = {0x201, 0x102, 0x084, 0x048, 0x030, 0x030, 0x048, 0x084, 0x102, 0x201}; // 엑스 도트매트릭스 (저항 0~341)
int dot_data[MAX_SIZE]; // 저항 0~1023
int dmi =0;	// 도트매트릭스 행/열 숫자 표시하기위한 변수

unsigned char segment_data[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f}; // 세그먼트 데이타 (0~9)
unsigned char display_num[4] = {0,1,2,3}; // 세그먼트 설정 (digit 0~3)
unsigned char led, count_int;
unsigned char digit_num=0;
unsigned int i=0;		// segment 카운트하기위한 변수
unsigned int adc_data;	// adc_data 저장할 변수
unsigned int temp = 0;	// segclear에 쓰는 변수
unsigned int a=1;		// 일정시간동안 저항을 유지하면 스피커발생시키는데 필요한 변수

// 범위가 바뀔때 스피커출력
void soundon(void){


PORTA = 0x00;
DDRA = 0xFF;
PORTB = 0x00;
DDRB = 0x00;
PORTC = 0x00;
DDRC = 0xFF;
PORTD = 0x00;
DDRD = 0xFF;
PORTE = 0x00;
DDRE = 0x00;
PORTF = 0x00;
DDRF = 0x00;
PORTG = 0x00;
DDRG = 0x1f;

init_devicetwo();


	sound(G2);
	delay_ms(DLY_1);
	nosound();
	
	sound(A2);
	delay_ms(DLY_1);
	nosound();
	
	sound(B2);
	delay_ms(DLY_1);
	nosound();
	
	sound(A2);
	delay_ms(DLY_2);
	nosound();
	
	sound(B2);
	delay_ms(DLY_1);
	nosound();

}

// 일정시간지나면 발생하는 스피커
void soundup(void){

PORTA = 0x00;
DDRA = 0xFF;
PORTB = 0x00;
DDRB = 0x00;
PORTC = 0x00;
DDRC = 0xFF;
PORTD = 0x00;
DDRD = 0xFF;
PORTE = 0x00;
DDRE = 0x00;
PORTF = 0x00;
DDRF = 0x00;
PORTG = 0x00;
DDRG = 0x1f;

init_devicetwo();


	sound(B2);
	delay_ms(DLY_1);
	nosound();
	
	sound(A2);
	delay_ms(DLY_2);
	nosound();
	
	sound(B2);
	delay_ms(DLY_1);
	nosound();

	sound(A2);
	delay_ms(DLY_2);
	nosound();
	
}

// sound함수를 호출할때 새로 설정하는 init_device
void init_devicetwo(void){
//stop errant interrupts untill set up
CLI();
XDIV = 0x00;
XMCRA = 0x00;
port_init();
timer1_init();
MCUCR = 0x00;
EICRA = 0x00;
EICRB = 0x00;
EIMSK = 0x00;
TIMSK = 0x04;
ETIMSK = 0x00;
SEI();
//all peripherals are now initialized
}

// timer1 초기화함수
void timer1_init(void){
TCCR1B = 0x00;
TCNT1H = 0xFD;
TCNT1L = 0x66;
OCR1AH = 0x02;
OCR1AL = 0x9A;
OCR1BH = 0x02;
OCR1BL = 0x9A;
OCR1CH = 0x02;
OCR1CL = 0x9A;
ICR1H = 0x02;
ICR1L = 0x9A;
TCCR1A = 0x00;
TCCR1B = 0x02;
}

// 사운드에 관련된 주파수를 파라미터로 입력
void sound(int freq){
Soundonoff=ON;
T1HIGHCNT = (0xFFFF-floor(1000000/freq)) / 0x100;
T1LOWCNT = 0xFFFF-floor(1000000/freq) - 0xFF00;
}

// sound를 끄는 함수
void nosound(void){
Soundonoff = OFF;
delay(10);
}

// delay_us를 새로 작성한 함수
void delay_us(unsigned char time_us){
register unsigned char i;

for(i=0; i<time_us; i++){
asm volatile("PUSH R0");
asm volatile("POP R0");
asm volatile("PUSH R0");
asm volatile("POP R0");
asm volatile("PUSH R0");
asm volatile("POP R0");
}
}


SIGNAL(SIG_OVERFLOW1){
TCNT1H = T1HIGHCNT; //reload counter hight value
TCNT1L = T1LOWCNT; //reload counter low value
if(Soundonoff==ON){
PORTG = PORTG ^ 0x10;
}
}

// delay_ms를 새로 작성한 함수
void delay_ms(unsigned int timer_ms){
unsigned int i;
for(i=0; i<timer_ms; i++){
delay_us(1000);
}
}

// delay 함수
void delay(int delaytime){
int i,j;
for(i=0; i<=1000; i++){
for(j=0; j<=delaytime; j++);
}
}

// 세그먼트에 필요한 인터럽트
ISR(TIMER0_OVF_vect){
TCNT0 = 0x00;	// 타이머/카운터0의 8비트 카운터값을 저장하는 레지스터
digit_num++;	// 세그먼트 자리수
digit_num = digit_num%4;
EX_SS_SEL = 0x0f;	// 0x00001111
EX_SS_DATA = segment_data[display_num[digit_num]];
EX_SS_SEL = ~(0x01 << digit_num);
}

// segment에 출력하는 함수
void segment(void){

PORTA = 0x00;
DDRA = 0xFF;
PORTB = 0x00;
DDRB = 0x00;	//입력으로 설정
PORTC = 0x00;
DDRC = 0x03;
PORTD = 0x00;
DDRD = 0x00;
PORTG = 0x00;
DDRG = 0x03;	// Write, ale신호
MCUCR = 0x80;	// External SRAM/XMEM Enable
EX_SS_SEL = 0x0f;
TCCR0 = 0x05;	// Normal mode, prescale 1024
TCNT0 = 0x00;	
TIMSK = 0x01;	// timer0 OVERFLOW interrupt enable
ASSR = 0x00;

TCNT0 = 0x00;	// 타이머/카운터0의 8비트 카운터값을 저장하는 레지스터
digit_num++;	// 세그먼트 자리수
digit_num = digit_num%4;
EX_SS_SEL = 0x0f;	// 0x00001111
EX_SS_DATA = segment_data[display_num[digit_num]];
EX_SS_SEL = ~(0x01 << digit_num);

// 세그먼트 카운트 원리 
count_int++;
	if(count_int==61){
	i++;
	display_num[0] = (i%10000)/1000;
	display_num[1] = (i%1000)/100;
	display_num[2] = (i%100)/10;
	display_num[3] = (i%10);
	_delay_ms(10);
	count_int=0;
	
	}

}

// init_device설정시 포트초기화
void port_init(void){
PORTA = 0x00;
DDRA = 0x00;
PORTB = 0x00;
DDRB = 0x00;
PORTC = 0x00;
DDRC = 0x00;
PORTD = 0x00;
DDRD = 0x00;
PORTE = 0x00;
DDRE = 0x02;
PORTF = 0x00;
DDRF = 0x00;
PORTG = 0x00;
DDRG = 0x03;
}


//ADC initialize
void adc_init(void){
ADCSRA = 0x00; // disable adc
ADMUX = 0x00; // select adc input 0
ADCSRA = 0x87;
}

//UART0 initialize
void uart0_init(void){
UCSR0B = 0x00; // disable while setting baud rate
UCSR0A = 0x00;
UCSR0C = 0x06;
UBRR0L = 0x67; // set baud rate lo
UBRR0H = 0x00; // set baud rate hi
UCSR0B = 0x18;
}

//call this routine to initialize all peripherals
void init_devices(void){
//stop errant interrupts until set up
CLI();
XDIV = 0x00; // xtal divider
XMCRA = 0x00; // external memory
port_init();
uart0_init();
adc_init();
MCUCR = 0x80;
EICRA = 0x00; // extended ext ints
EICRB = 0x00; // extended ext ints
EIMSK = 0x00;
TIMSK = 0x00; // timer interrupt sources
ETIMSK = 0x00; // extended timer interrupt sources
SEI(); // re-enable interrupts
// all peripherals are now initialized
}

// printf 함수에서 사용할 putchar 에  UART0으로 데이터를 보내도록 함
int PutChar(char c){
tx0Char(c);
return c;
}

// UART0을 이용한 출력
void tx0Char(char message){
while(((UCSR0A>>UDRE0)&0x01) == 0);
// UDRE, data register empty
UDR0 = message;
}

//scanf함수 사용시 추가
int Getchar(void){
while((UCSR0A & 0x80) == 0);
return UDR0;
}

// 입력으로 들어오는 채널의 ADC를 시작시킴
void startConvertion(){
ADCSRA &= 0x07; // 128분주비사용
ADMUX = 0x00;
ADCSRA |= 0xC0; // ADC ON
}

// start하고 변환값 return
unsigned int readConvertData(void){
volatile unsigned int temp;
while((ADCSRA & 0x10)==0);
temp = (int)ADCL+(int)ADCH*256;
return temp;
}

// segment 지우는함수(초기화)
void segclear(void){
	display_num[0] = 0;
	display_num[1] = 0;
	display_num[2] = 0;
	display_num[3] = 0;
	temp = i;
	i=0;
	a=1;
	// a는 일정시간이 지나면 스피커발생시킬때 필요한 변수로
	// 범위가 바뀌면 1로 초기화하여 계속 하는걸로 한다.
	}

// 도트매트릭스 하트모양
void dot_matrixheart(){
EX_DM_SEL = (1<<dmi);
EX_DM_DATA = dot_heart[dmi++];
if(dmi>9){
dmi=0;
}
_delay_ms(1);
}

// 도트매트릭스 삼각형모양
void dot_matrixtri(){
EX_DM_SEL = (1<<dmi);
EX_DM_DATA = dot_triangle[dmi++];
if(dmi>9){
dmi=0;
}
_delay_ms(1);
}

// 도트매트릭스 엑스모양
void dot_matrixxxx(){
EX_DM_SEL = (1<<dmi);
EX_DM_DATA = dot_xxx[dmi++];
if(dmi>9){
dmi=0;
}
_delay_ms(1);
}

// 메인함수
void main(void){

float voltage;
int number=0;
init_devices();
int login=0;
int logout=0;
int away=0;
lcdInit();

while(1){

startConvertion();
adc_data = readConvertData();
voltage = (float) (5. * adc_data)/1024;
lcd_gotoxy(1,2);
lcd_puts(2, "ADC res ");
lcd_putn4(adc_data);
dot_data[number]=adc_data;
number++;
if(number >9){ number=0;}

 
// 가변저항이 684이면 세그먼트 클리어, 스피커 출력
if(adc_data==684){
segclear();
soundon();
}

// 가변저항이 341이면 세그먼트 클리어, 스피커 출력
if(adc_data==341){
segclear();
soundon();
}

// 가변저항이 684보다 크거나 같으면 ( login상태 )
if(adc_data>=684){
dot_matrixtri();
segment();
lcd_puts(1, "Away    ");
lcd_putn4(temp);

// 시간이 30배수일때
if(i==(30*a)){
// 스피커 출력 후 a증가
soundup();
a++;
}
}

// 가변저항이 342보다 크거나같고 683보다 작거나 같을때 ( away상태 )
if(adc_data>=342 && adc_data <=683){

dot_matrixheart();
segment();
lcd_puts(1, "Logtime ");
lcd_putn4(temp);

// 시간이 30배수일때
if(i==(30*a)){
// 스피커 출력 후 a증가
soundup();
a++;
}
}

// 가변저항이 0보다 크거나같고 341보다 작거나 같을때 ( logout상태 )
if(adc_data>=0 && adc_data<=341){
dot_matrixxxx();
segment();
lcd_puts(1, "Away    ");
lcd_putn4(temp);

// 시간이 30배수일때
if(i==(30*a)){
// 스피커 출력 후 a증가
soundup();
a++;
}
}

}

}
