#include "stm32f4xx.h"                  // Device header
#include "serial_stdio.h"
#include "retarget_stm32f4.h"
#include <string.h>
/*Led PA5, Button PC13*/

void delay_ms(int delay_time);
void led_init(void);
void button_init(void);
void spi_master_init(void);
uint16_t spi_xfer(uint16_t tx_data);

Serial_t USART2_Serial={USART2_getChar,USART2_sendChar};

char mybf[80];/*Input buffer*/
char wordBuffer[80];

#define DAC_CONF	(0x3<<12) 

unsigned short dac_val=0x8FF;
unsigned short receivedData=0xFFFF;
int main(){
	int lineCounter=1;
	//led_init();
	USART2_init(9600);
	serial_puts(USART2_Serial,"\nSystem ready\n");
	spi_master_init();
	
	while(1){
		serial_printf(USART2_Serial,"%d$ ",lineCounter);
		serial_gets(USART2_Serial,mybf,80);
		serial_printf(USART2_Serial,"%s\n",mybf);
		if(sscanf(mybf,"%hx",&dac_val) > 0){
			receivedData=spi_xfer((DAC_CONF|(dac_val)));
			serial_printf(USART2_Serial,"new dac value = 0x%03hX\n",dac_val);
			serial_printf(USART2_Serial,"received value = 0x%04hX\n",receivedData);
		}
		lineCounter++;
		delay_ms(0xFFFFF);
	}
}

void delay_ms(int delay_time){
	for(int i=0; i<delay_time; i++);
}

void led_init(void){
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef myGPIO;
	GPIO_StructInit(&myGPIO);
	myGPIO.GPIO_Mode=GPIO_Mode_OUT;
	myGPIO.GPIO_Pin=GPIO_Pin_5;
	GPIO_Init(GPIOA,&myGPIO);
}

void button_init(void){
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);
	GPIO_InitTypeDef myGPIO;
	GPIO_StructInit(&myGPIO);
	myGPIO.GPIO_Mode=GPIO_Mode_IN;
	myGPIO.GPIO_Pin=GPIO_Pin_13;
	GPIO_Init(GPIOC,&myGPIO);
}
/* SPI1
 * PA4=NSS PA5=SCK PA6=MISO PA7=MOSI*/
void spi_master_init(void){
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 ,ENABLE);
	
	GPIO_InitTypeDef myGPIO;
	GPIO_StructInit(&myGPIO);
	myGPIO.GPIO_Pin=GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	myGPIO.GPIO_Mode=GPIO_Mode_AF;
	myGPIO.GPIO_OType=GPIO_OType_PP;
	myGPIO.GPIO_Speed=GPIO_Medium_Speed;
	myGPIO.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA,&myGPIO);
	
	/*Remap pins to SPI1*/
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource4,GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1);
	
	SPI_InitTypeDef mySPI;
	SPI_StructInit(&mySPI);
	mySPI.SPI_Mode=SPI_Mode_Master;
	mySPI.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_128;//16MHz/128=12.5KHz
	mySPI.SPI_Direction=SPI_Direction_2Lines_FullDuplex;//1 CLK line two data lines
	mySPI.SPI_DataSize=SPI_DataSize_16b;
	mySPI.SPI_CPOL = SPI_CPOL_Low;//Clock steady state = low
	mySPI.SPI_CPHA = SPI_CPHA_1Edge;//CPHA=0;
	mySPI.SPI_NSS = SPI_NSS_Hard;	
	mySPI.SPI_FirstBit=SPI_FirstBit_MSB;
	SPI_Init(SPI1,&mySPI);
	SPI_SSOutputCmd(SPI1,ENABLE);//SSOE = 1
}

uint16_t spi_xfer(uint16_t tx_data){
	uint16_t rx_data;
	SPI_Cmd(SPI1,ENABLE);
	SPI_I2S_SendData(SPI1,tx_data);
	while(!SPI_I2S_GetFlagStatus(SPI1,SPI_FLAG_TXE));//Wait until tx buffer empty
	while(!SPI_I2S_GetFlagStatus(SPI1,SPI_FLAG_RXNE));//Wait until rx buffer full
	rx_data=SPI_I2S_ReceiveData(SPI1);
	while(SPI_I2S_GetFlagStatus(SPI1,SPI_FLAG_BSY));//Wait until transactions completed
	SPI_Cmd(SPI1,DISABLE);//
	return rx_data;
}
