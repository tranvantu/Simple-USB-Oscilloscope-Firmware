#include <33fj16gs502.h>
#DEVICE ADC=10
#include <stdlib.h>
#fuses NOWRT, NOWRTB, NOBSS, NOPROTECT, HS, PR_PLL, NOWINDIS, NOWDT, NOPUT, ICSP1, NOJTAG, NODEBUG
#use delay(crystal= 8MHz,clock=100 MHz)      
#PIN_SELECT U1TX=PIN_B8 
#PIN_SELECT U1RX=PIN_B15
#use RS232(UART1,baud=230400,bits=8,parity=N,stop=1) // hardware UART
#use spi(DO=PIN_B14, CLK=PIN_B12, ENABLE=PIN_A3, BITS=16,stream=CHA_GAIN,MODE=3,BAUD=38400) // software SPI
#use spi(DO=PIN_B11, CLK=PIN_B12, ENABLE=PIN_A3, BITS=16,stream=CHB_GAIN,MODE=3,BAUD=38400) //
#use spi(DO=PIN_A4, CLK=PIN_B12, ENABLE=PIN_A3, BITS=16,stream=CHA_OFFSET,MODE=3,BAUD=38400)//
#use spi(DO=PIN_B13, CLK=PIN_B12, ENABLE=PIN_A3, BITS=16,stream=CHB_OFFSET,MODE=3,BAUD=38400)//
#byte ACLKCON = 0x750
#bit APLLCK = 0x750.14
#bit ASRCSEL = 0x750.7
#bit FRCSEL = 0x750.6
#bit ENAPLL  = 0x750.15
#bit SELACLK = 0x750.13
#bit APSTSCLR2 = 0x750.10
#bit APSTSCLR1 = 0x750.9
#bit APSTSCLR0 = 0x750.8
//====================================
const unsigned int16 SampleCount=400;
const unsigned SampleCountx2 = 800;
unsigned int16 result[2];
unsigned int16 Data[SampleCountx2];
unsigned int16 count=0;
int1 enable = 0;
int1 captured = 0;
int1 cmd_exists = 0;
int1 exit = 0;
unsigned int32 TimerValue;
char c;
char str[25];
char term[1];
char *ptr;
int i = 0;
unsigned int GainA = 255;
unsigned int GainB = 255;
int temp;
unsigned int32 DelayPeriod=0;//uS
//=======================================
void set_delay(char c);
void reset_pot();
void get_data();
void send_data();
void init_adc();
void get_cmd();
void parse_cmd();
//======================================
void parse_cmd()
{
   strcpy(term,",");
   ptr = strtok(str, term);
   i=0;
   while(ptr!=0) 
   {
      if(i==0)
      {
         DelayPeriod = atoi32(ptr);
      }
      if(i==2)
      {
         GainA = atoi(ptr);
         spi_xfer(CHA_GAIN,4352 + GainA);
      }
      if(i==3)
      {
         GainA = atoi(ptr);
         spi_xfer(CHA_OFFSET, 4352 + GainA);
      }
      if(i==5)
      {
         GainB = atoi(ptr);
         spi_xfer(CHB_GAIN,4352 + GainB);
      }
      if(i==6)
      {      
         temp = atoi(ptr);
         spi_xfer(CHB_OFFSET,4352 + temp);
      }
      if(i == 7)
      {
 
      }
      if(i == 8)
      {
 
      }
      i++;
      ptr = strtok(0, term);
   }
}
void reset_pot()
{
   output_high(PIN_A3);
   spi_xfer(CHA_GAIN,4352 + 0);
   output_high(PIN_A3);
   spi_xfer(CHB_GAIN,4352 + 0);
   output_high(PIN_A3);   
   spi_xfer(CHA_OFFSET,4352 + 255);
   output_high(PIN_A3);   
   spi_xfer(CHB_OFFSET,4352 + 255);
}

void get_data()
{
   set_timer23(0);
   count = 0;
   while(count < SampleCount)
   {
      read_high_speed_adc(0, ADC_START_AND_READ, result );
      Data[count] = result[0];
      Data[SampleCount+count] = result[1];
      count++;
      if(DelayPeriod == 100000)//delay_us(int 16); 2^16 = 65536
      {
         delay_us(50000);
         delay_us(50000);         
      }
      else
      {
         delay_us(DelayPeriod);
      }
   }
   TimerValue = get_timer23();
   captured = 1;
}

void send_data()
{
   for(count = 0; count < SampleCountx2; count++)
   {
      printf("%04Lu,",Data[count]);
      if(count == (SampleCountx2-1))
      {
         printf("%010Lu,%010Lu",TimerValue,DelayPeriod);
      }
   }
   captured = 0;
}
void init_adc()
{
   ENAPLL = 1;       //AUX PLL FOR ADC CLOCK
   APSTSCLR2 = 1;
   APSTSCLR1 = 1;
   APSTSCLR0 = 1;
   while(APLLCK !=1);
   setup_high_speed_adc_pair(0,INDIVIDUAL_SOFTWARE_TRIGGER);
   setup_high_speed_adc(ADC_CLOCKED_BY_PRI_PLL);
   setup_timer2(TMR_INTERNAL| TMR_32_BIT);   
}
#INT_RDA
void get_cmd()
{
   c = getc();
   if(c == '!')
   {
      exit = 1;
   }
   else
   {
      str[i] = c;
      i++;
      if(c == '@')
      {
         str[i-1] = "";
         disable_interrupts(INT_RDA);
         cmd_exists = 1;
      }
   }
   c="";
   clear_interrupt(INT_RDA);
}
void main()
{
   set_tris_a(0b00111);
   set_tris_b(0b1000000000000111);
   output_high(PIN_A3);
   reset_pot();
   init_adc();
   clear_interrupt(INT_RDA);
   enable_interrupts(INT_RDA);
   enable_interrupts(GLOBAL);   
   while(TRUE)
   {
      if(cmd_exists == 1)
      {
          parse_cmd();
          cmd_exists = 0;
          enable = 1;
      }
      if(exit == 1)
      {
         enable = 0;
         exit = 0;
      }
      if(enable == 1)
      {
         get_data();
         if(captured == 1)
         {
            send_data();
         }
         enable = 0;
      }
      enable_interrupts(INT_RDA);
   }
}
