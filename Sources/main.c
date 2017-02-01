/*
***********************************************************************
 ECE 362 - Experiment 8 - Fall 2015
***********************************************************************
	 	   			 		  			 		  		
 Completed by: < Shenwei Wang >
               < your class number >
               < 7 >


 Academic Honesty Statement:  In entering my name above, I hereby certify
 that I am the individual who created this HC(S)12 source file and that I 
 have not copied the work of any other student (past or present) while 
 completing it. I understand that if I fail to honor this agreement, I will 
 receive a grade of ZERO and be subject to possible disciplinary action.

***********************************************************************

 The objective of this experiment is to implement a reaction time assessment
 tool that measures, with millisecond accuracy, response to a visual
 stimulus -- here, both a YELLOW LED and the message "Go Team!" displayed on 
 the LCD screen.  The TIM module will be used to generate periodic 
 interrupts every 1.000 ms, to serve as the time base for the reaction measurement.  
 The RTI module will provide a periodic interrupt at a 2.048 ms rate to serve as 
 a time base for sampling the pushbuttons and incrementing the variable "random" 
 (used to provide a random delay for starting a reaction time test). The SPI
 will be used to shift out data to an 8-bit SIPO shift register.  The shift
 register will perform the serial to parallel data conversion for the LCD.

 The following design kit resources will be used:

 - left LED (PT1): indicates test stopped (ready to start reaction time test)
 - right LED (PT0): indicates a reaction time test is in progress
 - left pushbutton (PAD7): starts reaction time test
 - right pushbutton (PAD6): stops reaction time test (turns off right LED
                    and turns left LED back on, and displays test results)
 - LCD: displays status and result messages
 - Shift Register: performs SPI -> parallel conversion for LCD interface

 When the right pushbutton is pressed, the reaction time is displayed
 (refreshed in place) on the first line of the LCD as "RT = NNN ms"
 followed by an appropriate message on the second line 
 e.g., 'Ready to start!' upon reset, 'Way to go HAH!!' if a really 
 fast reaction time is recorded, etc.). The GREEN LED should be turned on
 for a reaction time less than 250 milliseconds and the RED LED should be
 turned on for a reaction time greater than 1 second.

***********************************************************************
*/

#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

/* All funtions after main should be initialized here */
char inchar(void);
void outchar(char x);
void tdisp();
void shiftout(char x);
void lcdwait(void);
void send_byte(char x);
void send_i(char x);
void chgline(char x);
void print_c(char x);
void pmsglcd(char[]);

void gamemode(void);
void song_star(void);
void song_Hail(void);
void ledshift(unsigned char x);
void papawait(void);
void ydo(void);
void yre(void);
void ymi(void);
void yfa(void);
void yso(void);
void yla(void);
void yti(void);
void musicplay(void);
void waitgame(int sup);
/* Variable declarations */  	   			 		  			 		       
char goteam 	= 0;  // "go team" flag (used to start reaction timer)
char leftpb	= 0;  // left pushbutton flag
char pb1	= 0;  //pushbutton 1 flag
char pb2	= 0;  //pushbutton 2 flag
char pb3	= 0;  //pushbutton 3 flag
char pb4	= 0;  //pushbutton 4 flag
char pb5	= 0;  //pushbutton 5 flag
char pb6	= 0;  //pushbutton 6 flag
char pb7	= 0;  //pushbutton 7 flag

char pvpb1 = 0; //previous status of pb1
char pvpb2 = 0; //previous status of pb2
char pvpb3 = 0; //previous status of pb3
char pvpb4 = 0; //previous status of pb4
char pvpb5 = 0; //previous status of pb5
char pvpb6 = 0; //previous status of pb6
char pvpb7 = 0; //previous status of pb7

long int score = 0;
long int scoremax = 0;
                               
int runstp = 0; // run/stop flag
char modeslc = 0; // mode select flag for menu
int random	= 0;  // random variable (2 bytes)
int react	= 0;  // reaction time (3 packed BCD digits)
int pai = 300;
int clearled = 0;

int oct = 0;//0-oct5,1-oct6,2-oct4
int plus = 0;
/* ASCII character definitions */
#define CR 0x0D	// ASCII return character   

/* LCD COMMUNICATION BIT MASKS */
#define RS 0x04		// RS pin mask (PTT[2])
#define RW 0x08		// R/W pin mask (PTT[3])
#define LCDCLK 0x10	// LCD EN/CLK pin mask (PTT[4])

/* LCD INSTRUCTION CHARACTERS */
#define LCDON 0x0F	// LCD initialization command
#define LCDCLR 0x01	// LCD clear display command
#define TWOLINE 0x38	// LCD 2-line enable command
#define CURMOV 0xFE	// LCD cursor move instruction
#define LINE1 0x80	// LCD line 1 cursor position
#define LINE2 0xC0	// LCD line 2 cursor position

/* LED BIT MASKS */
#define GREEN 0x20
#define RED 0x40
#define YELLOW 0x80
	 	   		
/*
***********************************************************************
 Initializations
***********************************************************************
*/

void  initializations(void) {

/* Set the PLL speed (bus clock = 24 MHz) */
  CLKSEL = CLKSEL & 0x80; // disengage PLL from system
  PLLCTL = PLLCTL | 0x40; // turn on PLL
  SYNR = 0x02;            // set PLL multiplier
  REFDV = 0;              // set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; // engage PLL

/* Disable watchdog timer (COPCTL register) */
  COPCTL = 0x40;   //COP off, RTI and COP stopped in BDM-mode

/* Initialize asynchronous serial port (SCI) for 9600 baud, no interrupts */
  SCIBDH =  0x00; //set baud rate to 9600
  SCIBDL =  0x9C; //24,000,000 / 16 / 156 = 9600 (approx)  
  SCICR1 =  0x00; //$9C = 156
  SCICR2 =  0x0C; //initialize SCI for program-driven operation
  DDRB   =  0x10; //set PB4 for output mode
  PORTB  =  0x10; //assert DTR pin on COM port
         
         
/* Add additional port pin initializations here */
  DDRAD = 0;

/* Initialize SPI for baud rate of 6 Mbs */
  DDRM   = 0x30;
  SPICR1 = 0x50;
  SPICR2 = 0x00;
  SPIBR  = 0x01;

/* Initialize digital I/O port pins */
  ATDDIEN = 0x7F;
  DDRT=0xFF;

/* Initialize the LCD
     - pull LCDCLK high (idle)..
     - pull R/W' low (write state)..
     - turn on LCD (LCDON instruction)..
     - enable two-line mode (TWOLINE instruction)..
     - clear LCD (LCDCLR instruction)
     - wait for 2ms so that the LCD can wake up     
*/
  PTT_PTT5 = 0;
  PTT_PTT6 = 1;
  
  send_i(LCDON);
  send_i(TWOLINE);
  send_i(LCDCLR);
  lcdwait();   

/* Initialize RTI for 2.048 ms interrupt rate */
  RTICTL = 0x45;
  CRGINT = CRGINT | 0x80;	

/* Initialize TIM Ch 7 (TC7) for periodic interrupts every 1.000 ms
     - enable timer subsystem
     - set channel 7 for output compare
     - set appropriate pre-scale factor and enable counter reset after OC7
     - set up channel 7 to generate 1 ms interrupt rate
     - initially disable TIM Ch 7 interrupts      
*/
  TSCR1 = 0x80;
  TSCR2 = 0x0C;
  TIOS  = 0x80;
  TIE   = 0x00;
  TC7   = 1500;

//PWM initialization
  MODRR = 1;
  PWMPOL = 1;
  PWMCTL = 0;
  PWMCAE = 0;
  PWMPER0 = 2;
  PWMDTY0 = 1;
  PWMCLK = 1; 
}
	 		  			 		  		
/*
***********************************************************************
 Main
***********************************************************************
*/

void main(void) {
  DisableInterrupts;
	initializations(); 		  			 		  		
	EnableInterrupts;
  
  
  for(;;) {
    for(clearled = 0; clearled <= 8; clearled ++){
    long long i = 0;
  for (i = 6; i >= 0; i--) {
   PTT_PTT1 = 0;
   PTT_PTT3 = 0;
   PTT_PTT3 = 1;
   PTT_PTT3 = 0; 
 }
  PTT_PTT2 = 0;
  PTT_PTT2 = 1;
  PTT_PTT2 = 0;
      lcdwait();
    }
 
  

/* write your code here */
  if(pb1 == 1 && modeslc == 0){
    pb1 = 0;  
    chgline(LINE1);
    pmsglcd("1. Game Mode");
    chgline(LINE2);
    pmsglcd("Select - Press 7");
    runstp = 1;    
  }
  
  if(pb2 == 1 && modeslc == 0){
    pb2 = 0 ;
    chgline(LINE1);
    pmsglcd("2. Free Style");
    chgline(LINE2);
    pmsglcd("Select - Press 7");
    runstp = 2;
  }
  
  
   if(pb7 == 1 && modeslc == 0){
    pb7 = 0;
    modeslc = 1;    
  }
/* 
*/
  if(runstp == 1 && modeslc == 1){
    modeslc = 0; 
    gamemode();
    
  }
  if(runstp == 2 && modeslc == 1){     
    modeslc = 0;
    send_i(LCDCLR);
    chgline(LINE1);
    pmsglcd("Ready, Set...");    
    papawait();
    chgline(LINE2);
    pmsglcd("GO!!!");    
    for(;;){
      musicplay();
    }
  }

  
  } /* loop forever */
  
}  /* do not leave main */

/*
*********************************************************************** 
game mode
***********************************************************************
*/
void gamemode(){
    send_i(LCDCLR);
    chgline(LINE1);
    pmsglcd("1.Little Star");
    chgline(LINE2);
    pmsglcd("2.Hail Purdue");
  for(;;){
    if(pb1 == 1){
      pb1 = 0;
      send_i(LCDCLR);
      chgline(LINE1);
      pmsglcd("Little Star");
      chgline(LINE2);
      pmsglcd("1.E 2.M 3.H 4.I");
      while(pb1 == 0 && pb2 == 0 && pb3 == 0 && pb4 == 0){
      }
      if(pb1 == 1){
        pai = 340;
      }
      if(pb2 == 1){
        pai = 290;
      }
      if(pb3 == 1){
        pai = 240;
      }
      if(pb4 == 1){
        pai = 190;
      }
      send_i(LCDCLR);
      pmsglcd("Start - Press 7");      
      while(pb7 != 1){
      }
      pb7 = 0;

      song_star();
    }
    if(pb2 == 1){
      pb2 = 0;
      send_i(LCDCLR);
      chgline(LINE1);
      pmsglcd("Hail Purdue");
      chgline(LINE2);      pmsglcd("1.E 2.M 3.H 4.I");
      while(pb1 == 0 && pb2 == 0 && pb3 == 0 && pb4 == 0){
      }
      if(pb1 == 1){
        pai = 340;
      }
      if(pb2 == 1){
        pai = 290;
      }
      if(pb3 == 1){
        pai = 240;
      }
      if(pb4 == 1){
        pai = 190;
      }
      send_i(LCDCLR);
      pmsglcd("Start - Press 7");
   
      while(pb7 != 1){
      }
      pb7 = 0;
      
      song_Hail();
    }
  }
} //end of game mode




/*
***********************************************************************
 RTI interrupt service routine: RTI_ISR

  Initialized for 2.048 ms interrupt rate

  Samples state of pushbuttons (PAD7 = left, PAD6 = right)

  If change in state from "high" to "low" detected, set pushbutton flag
     leftpb (for PAD7 H -> L), rghtpb (for PAD6 H -> L)
     Recall that pushbuttons are momentary contact closures to ground

  Also, increments 2-byte variable "random" each time interrupt occurs
  NOTE: Will need to truncate "random" to 12-bits to get a reasonable delay 
***********************************************************************
*/

interrupt 7 void RTI_ISR(void)
{
  // clear RTI interrupt flag
  CRGFLG = CRGFLG | 0x80;
      if(PORTAD0_PTAD0 == 1 && pvpb1 == 0){
        pb1 = 1;
        /*
        PWMPRCLK = 0x06;    
        if(oct == 1){        
        PWMPRCLK = 0x05;
        }
        if(oct == 2){        
        PWMPRCLK = 0x07;  
        }    
        PWMSCLA = 0xB3;        
        if(plus == 1){
         PWMSCLA = 0xA9;
        }
          
         
        PWME = 0x01; //enable PWM Ch 0   .         
        
        papawait(pai);*/  
      }
      pvpb1 = PORTAD0_PTAD0;
      
      if(PORTAD0_PTAD1 == 1 && pvpb2 == 0){
        pb2 = 1;
        /*PWMPRCLK = 0x06;
        if(oct == 1){        
        PWMPRCLK = 0x05;
        }
        if(oct == 2){        
        PWMPRCLK = 0x07;  
        }
        PWMSCLA = 0xA0; 
        if(plus == 1){
          PWMSCLA = 0x97;
        } 
        PWME = 0x01; //enable PWM Ch 0
        
      papawait(pai);*/
      }
      pvpb2 = PORTAD0_PTAD1;

      if(PORTAD0_PTAD2 == 1 && pvpb3 == 0){
        pb3 = 1;
        /*PWMPRCLK = 0x06;
        if(oct == 1){        
        PWMPRCLK = 0x05;
        }
        if(oct == 2){        
        PWMPRCLK = 0x07;  
        }
        
        PWMSCLA = 0x8E;
        PWME = 0x01; //enable PWM Ch 0
        
      papawait(pai);*/
      }
      pvpb3 = PORTAD0_PTAD2;
      
      if(PORTAD0_PTAD3 == 1 && pvpb4 == 0){
        pb4 = 1;  
        /*PWMPRCLK = 0x06;
        if(oct == 1){        
        PWMPRCLK = 0x05;
        }
        if(oct == 2){        
        PWMPRCLK = 0x07;  
        }
        
        PWMSCLA = 0x86; 
        
        if(plus == 1){
          PWMSCLA = 0x7F;
        }
        PWME = 0x01; //enable PWM Ch 0
        
      papawait(pai);*/
      }
      pvpb4 = PORTAD0_PTAD3;

      if(PORTAD0_PTAD4 == 1 && pvpb5 == 0){
        pb5 = 1;       
        /*PWMPRCLK = 0x05;
        if(oct == 1){        
        PWMPRCLK = 0x04;
        }
        if(oct == 2){        
        PWMPRCLK = 0x06;  
        }
        PWMSCLA = 0xF0;
        if(plus == 1){
          PWMSCLA = 0xE2;
        }
        PWME = 0x01; //enable PWM Ch 0
        
      papawait(pai);*/
      }
      pvpb5 = PORTAD0_PTAD4;
      if(PORTAD0_PTAD5 == 1 && pvpb6 == 0){
        pb6 = 1;
        /*PWMPRCLK = 0x05;
        if(oct == 1){        
        PWMPRCLK = 0x04;
        }
        if(oct == 2){        
        PWMPRCLK = 0x06;  
        }
        PWMSCLA = 0xD5;
        
       if(plus == 1){
          PWMSCLA = 0xC9;
        }  
        PWME = 0x01; //enable PWM Ch 0
        
      papawait(pai);*/
      }
      pvpb6 = PORTAD0_PTAD5;

      if(PORTAD0_PTAD6 == 1 && pvpb7  == 0){
        pb7 = 1;
        /*PWMPRCLK = 0x05;
        if(oct == 1){        
        PWMPRCLK = 0x04;
        }
        if(oct == 2){        
        PWMPRCLK = 0x06;  
        }
        PWMSCLA = 0xBE;
        PWME = 0x01; //enable PWM Ch 0 
        
      papawait(pai);*/
      }
      pvpb7 = PORTAD0_PTAD6;

     // PWME = 0x00; //disable PWM Ch 0  
}

void musicplay(){
  int count = 0;

  if(pb1 == 1){
    if(runstp == 2){
      
    for(count = 1; count < 9; count++){
      ledshift(64);
    }
    }
    ydo();
    pb1 = 0;
  }
  
  if(pb2 == 1){
    if(runstp == 2){
    
    for(count = 1; count < 9; count++){
      ledshift(32);
    }
    }
    yre(); 
    pb2 = 0;
  }
  
  
  
  if(pb3 == 1){
    if(runstp == 2){
      
    for(count = 1; count < 9; count++){
      ledshift(16);
    } 
    }
    ymi();
    pb3 = 0;
  }
  if(pb4 == 1){
    if(runstp == 2){
      
    for(count = 1; count < 9; count++){
      ledshift(8);
    } 
    }
    yfa();
    pb4 = 0;
  }
  if(pb5 == 1){
    if(runstp == 2){
      
    for(count = 1; count < 9; count++){
      ledshift(4);
    } 
    }
    yso();
    pb5 = 0;
  }
  
  if(pb6 == 1){
    if(runstp == 2){
    for(count = 1; count < 9; count++){
      ledshift(2);
    } 
    }
    yla();
    pb6 = 0;
  }
  
  if(pb7 == 1){
  if(runstp == 2){
    for(count = 1; count < 9; count++){
      ledshift(1);
    }             
  }
    yti();
    pb7 = 0;
  }
}

/*
*********************************************************************** 
  TIM Channel 7 interrupt service routine
  Initialized for 1.00 ms interrupt rate
  Increment (3-digit) BCD variable "react" by one
***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{
	// clear TIM CH 7 interrupt flag
 	TFLG1 = TFLG1 | 0x80; 
 	
 	ATDCTL5 = 0x10; //start conversion on ATD Ch 0 (seq. length = 2) NOTE: "mult" (bit 4) needs to be set
  while((ATDSTAT0 & 0x80) != 0x80){
  }
  
  if((runstp && goteam)&& (ATDDR0H < 255/2)){
    react++;
  } 
  
  

}


/*
*********************************************************************** 
  tdisp: Display "RT = NNN ms" on the first line of the LCD and display 
         an appropriate message on the second line depending on the 
         speed of the reaction.  
         
         Also, this routine should set the green LED if the reaction 
         time was less than 250 ms.

         NOTE: The messages should be less than 16 characters since
               the LCD is a 2x16 character LCD.
***********************************************************************
*/
 
void tdisp()
{
  send_i(LCDCLR);
  chgline(LINE1);
  pmsglcd("Score: ");
  print_c((score / 10) %10 + 48);
  print_c((score %10) +48);
  chgline(LINE2);
  if(score > 80) {
    pmsglcd("Excellent!!!");
  }else if(score <= 80 && score > 70){
    pmsglcd("Great Job :D");
  }else if(score <= 70 && score > 60){
    pmsglcd("Come on :O");
  }else{
    pmsglcd("Spicy Chicken :<");
  }
}



/***************Sounds********************/
void ydo()
    {
      PWMPRCLK = 0x06;    
        if(oct == 1){        
        PWMPRCLK = 0x05;
        }
        if(oct == 2){        
        PWMPRCLK = 0x07;  
        }    
        PWMSCLA = 0xB3;        
        if(plus == 1){
         PWMSCLA = 0xA9;
        }
        PWME = 0x01;
        papawait();
        PWME = 0x00; //disable PWM Ch 0
    }
void yre() 
{
        PWMPRCLK = 0x06;
        if(oct == 1){        
        PWMPRCLK = 0x05;
        }
        if(oct == 2){        
        PWMPRCLK = 0x07;  
        }
        PWMSCLA = 0xA0; 
        if(plus == 1){
          PWMSCLA = 0x97;
        } 
        PWME = 0x01; //enable PWM Ch 0
        papawait();
        PWME = 0x00; //disable PWM Ch 0   
}

void ymi(){
  
    PWMPRCLK = 0x06;
        if(oct == 1){        
        PWMPRCLK = 0x05;
        }
        if(oct == 2){        
        PWMPRCLK = 0x07;  
        }
        
        PWMSCLA = 0x8E;
        
        //miss the if plus == 1
        
        PWME = 0x01; //enable PWM Ch 0
        
      papawait();
      PWME = 0x00; //disable PWM Ch 0;
  }

void yfa() {

        PWMPRCLK = 0x06;
        if(oct == 1){        
        PWMPRCLK = 0x05;
        }
        if(oct == 2){        
        PWMPRCLK = 0x07;  
        }
        
        PWMSCLA = 0x86; 
        
        if(plus == 1){
          PWMSCLA = 0x7F;
        }
        PWME = 0x01; //enable PWM Ch 0
        
      papawait();
      PWME = 0x00; //disable PWM Ch 0
  }

void yso()
{
        PWMPRCLK = 0x05;
        if(oct == 1){        
        PWMPRCLK = 0x04;
        }
        if(oct == 2){        
        PWMPRCLK = 0x06;  
        }
        PWMSCLA = 0xF0;
        if(plus == 1){
          PWMSCLA = 0xE2;
        }
        PWME = 0x01; //enable PWM Ch 0
        
      papawait();
      PWME = 0x00; //disable PWM Ch 0
  }
  
void yla() {

    PWMPRCLK = 0x05;
        if(oct == 1){        
        PWMPRCLK = 0x04;
        }
        if(oct == 2){        
        PWMPRCLK = 0x06;  
        }
        PWMSCLA = 0xD5;
        
       if(plus == 1){
          PWMSCLA = 0xC9;
        }  
        PWME = 0x01; //enable PWM Ch 0
        
      papawait();
      PWME = 0x00; //disable PWM Ch 0
  }
  
void yti() {
        PWMPRCLK = 0x05;
        if(oct == 1){        
        PWMPRCLK = 0x04;
        }
        if(oct == 2){        
        PWMPRCLK = 0x06;  
        }
        PWMSCLA = 0xBE;
        PWME = 0x01; //enable PWM Ch 0 
        
      papawait();
      PWME = 0x00; //disable PWM Ch 0
  } 

/*
***********************************************************************
  shiftout: Transmits the character x to external shift 
            register using the SPI.  It should shift MSB first.  
             
            MISO = PM[4]
            SCK  = PM[5]
***********************************************************************
*/
 
void shiftout(char x)

{
 
  while(!SPISR_SPTEF)// read the SPTEF bit, continue if bit is 1
  {
  }
  SPIDR = x;// write data to SPI data register
  lcdwait();// wait for 30 cycles for SPI data to shift out 

}

/*
***********************************************************************
  lcdwait: Delay for approx 2 ms
***********************************************************************
*/

void lcdwait()
{
  asm {
  pshx
  pshd
  pshc
  ldx #2
loopo: 
  ldy #7996
loopi:  
  dbne y,loopi
  
  dbne x,loopo
  
  pulc
  puld
  pulx
  }   
}
/*pai pai wait*/
void papawait() {
  int i;
  int o;
  for (o = 0; o < pai; o++){
      for (i = 0; i < 7996; i++){
    }
  }
}
/*waittime for game mode*/
void waitgame(int sup){
  int i = 0;
  int o = 0;
  int mark = 0;
  
  if (sup == 64){
    while(o < pai && pb1 != 1){
      i = 0; 
      while(i < 3500){
        i++;
      }
      o++;
    }
    
    scoremax += 40;
    mark = pai / 4;
    if( o < mark) {
      score += 40;
    }else if(o >= mark && o < 2 * mark){
      score += 30;
    }else if (o >= 2 * mark && o < 3 * mark){
      score += 20;
    }else{
      score += 10;
    }
   
      
    if(pb1 == 1) {
    pb1 = 0;  
    ydo();
    }
  }
  
  
  if (sup == 32){
    while(o < pai && pb2 != 1){
      i = 0;
      while(i < 3500){
        i++;
      }
      o++;
    }
    
    scoremax += 40;
    mark = pai / 4;
    if( o < mark) {
      score += 40;
    }else if(o >= mark && o < 2 * mark){
      score += 30;
    }else if (o >= 2 * mark && o < 3 * mark){
      score += 20;
    }else{
      score += 10;
    }
    
    if(pb2 == 1) {
    pb2 = 0;  
    yre();
    }
  }  

  if (sup == 16){
    while(o < pai && pb3 != 1){
      i = 0;
      while(i < 3500){
        i++;
      }
      o++;
    }
    scoremax += 40;
    mark = pai / 4;
    if( o < mark) {
      score += 40;
    }else if(o >= mark && o < 2 * mark){
      score += 30;
    }else if (o >= 2 * mark && o < 3 * mark){
      score += 20;
    }else{
      score += 10;
    }
    if(pb3 == 1) {
    pb3 = 0;  
    ymi();
    }
  }

  if (sup == 8){
    while(o < pai && pb4 != 1){
      i = 0;
      while(i < 3500){
        i++;
      }
      o++;
    }
    scoremax += 40;
    mark = pai / 4;
    if( o < mark) {
      score += 40;
    }else if(o >= mark && o < 2 * mark){
      score += 30;
    }else if (o >= 2 * mark && o < 3 * mark){
      score += 20;
    }else{
      score += 10;
    }
    if(pb4 == 1) {
    pb4 = 0;  
    yfa();
    }
  }

  if (sup == 4){
    while(o < pai && pb5 != 1){
      i = 0;
      while(i < 3500){
        i++;
      }
      o++;
    }
    scoremax += 40;
    mark = pai / 4;
    if( o < mark) {
      score += 40;
    }else if(o >= mark && o < 2 * mark){
      score += 30;
    }else if (o >= 2 * mark && o < 3 * mark){
      score += 20;
    }else{
      score += 10;
    }
    if(pb5 == 1) {
    pb5 = 0;  
    yso();
    }
  }
  if (sup == 2){
    while(o < pai && pb6 != 1){
      i = 0;
      while(i < 3500){
        i++;
      }
      o++;
    }
    scoremax += 40;
    mark = pai / 4;
    if( o < mark) {
      score += 40;
    }else if(o >= mark && o < 2 * mark){
      score += 30;
    }else if (o >= 2 * mark && o < 3 * mark){
      score += 20;
    }else{
      score += 10;
    }
    if(pb6 == 1) {
    pb6 = 0;  
    yla();
    }
  }
  
  if (sup == 1){
    while(o < pai && pb7 != 1){
      i = 0;
      while(i < 3500){
        i++;
      }
      o++;
    }
    scoremax += 40;
    mark = pai / 4;
    if( o < mark) {
      score += 40;
    }else if(o >= mark && o < 2 * mark){
      score += 30;
    }else if (o >= 2 * mark && o < 3 * mark){
      score += 20;
    }else{
      score += 10;
    }
    if(pb7 == 1) {
    pb7 = 0;  
    yti();
    }
  }
  
  
}

/*
*********************************************************************** 
  send_byte: writes character x to the LCD
***********************************************************************
*/

void send_byte(char x)
{
     shiftout(x);// shift out character
     PTT_PTT6 = 0;// pulse LCD clock line low->high->low
     PTT_PTT6 = 1;
     PTT_PTT6 = 0;
     lcdwait();// wait 2 ms for LCD to process data
}

/*
***********************************************************************
  send_i: Sends instruction byte x to LCD  
***********************************************************************
*/

void send_i(char x)
{
  PTT_PTT4 = 0;// set the register select line low (instruction data)
  send_byte(x);// send byte
}

/*
***********************************************************************
  chgline: Move LCD cursor to position x
  NOTE: Cursor positions are encoded in the LINE1/LINE2 variables
***********************************************************************
*/

void chgline(char x)
{
  send_i(CURMOV);
  send_i(x);
}

/*
***********************************************************************
  print_c: Print (single) character x on LCD            
***********************************************************************
*/
 
void print_c(char x)
{
  PTT_PTT4 = 1;
  send_byte(x);
}

/*
***********************************************************************
  pmsglcd: print character string str[] on LCD
***********************************************************************
*/

void pmsglcd(char str[])
{
  int ct = 0;
  while(str[ct] != '\0') {
    print_c(str[ct]);
    ct += 1;
  }
}

/*
***********************************************************************
 Character I/O Library Routines for 9S12C32 (for debugging only)
***********************************************************************
 Name:         inchar
 Description:  inputs ASCII character from SCI serial port and returns it
 Example:      char ch1 = inchar();
***********************************************************************
*/

char inchar(void) {
  /* receives character from the terminal channel */
        while (!(SCISR1 & 0x20)); /* wait for input */
    return SCIDRL;
}

/*
***********************************************************************
 Name:         outchar
 Description:  outputs ASCII character x to SCI serial port
 Example:      outchar('x');
***********************************************************************
*/

void outchar(char x) {
  /* sends a character to the terminal channel */
    while (!(SCISR1 & 0x80));  /* wait for output buffer empty */
    SCIDRL = x;
}
/*LED SHIFT */
void ledshift(unsigned char x){
 long long i = 0;
 for (i = 6; i >= 0; i--) {
   PTT_PTT1 = ((x >> i) & 0x01);
   PTT_PTT3 = 0;
   PTT_PTT3 = 1;
   PTT_PTT3 = 0; 
 }
  PTT_PTT2 = 0;
  PTT_PTT2 = 1;
  PTT_PTT2 = 0;
  pb1 = 0;
  pb2 = 0;
  pb3 = 0;
  pb4 = 0;
  pb5 = 0;
  pb6 = 0;
  pb7 = 0;
}
/*Little Star*/

void song_Hail(){
ledshift(4);
papawait();
ledshift(0);
papawait();
ledshift(2);
papawait();
ledshift(1);
papawait();
ledshift(64);
papawait();
ledshift(32);
papawait();
ledshift(16);
papawait();
ledshift(16);
oct = 2;
waitgame(4);
ledshift(8);
papawait();
ledshift(8);
waitgame(2);
ledshift(64);
waitgame(1);
oct = 0;
ledshift(32);
waitgame(64);
ledshift(16);
waitgame(32);
ledshift(0);
waitgame(16);


ledshift(16);
waitgame(16);
ledshift(0);
waitgame(8);
ledshift(16);
waitgame(8);
ledshift(32);
waitgame(64);
ledshift(64);
waitgame(32);
ledshift(32);
waitgame(16);
ledshift(16);
papawait();
ledshift(16);
waitgame(16);
ledshift(32);
papawait();
ledshift(2);
waitgame(16);
ledshift(64);
waitgame(32);
ledshift(2);
waitgame(64);
ledshift(32);
waitgame(32);
ledshift(0);
waitgame(16);

ledshift(4);
waitgame(16);
ledshift(0);
waitgame(32);
ledshift(2);
oct = 2;
waitgame(2);
oct = 0;
ledshift(1);
waitgame(64);
ledshift(64);
oct = 2;
waitgame(2);
oct = 0;
ledshift(32);
waitgame(32);
ledshift(16);
papawait();
ledshift(16);
oct = 2;
waitgame(4);
ledshift(8);
papawait();
ledshift(8);
waitgame(2);
ledshift(64);
waitgame(1);
oct = 0;
ledshift(32);
waitgame(64);
ledshift(16);
waitgame(32);
ledshift(0);
waitgame(16);

ledshift(2);
waitgame(16);
ledshift(1);
waitgame(8);
ledshift(64);
waitgame(8);
ledshift(2);
waitgame(64);
ledshift(4);
waitgame(32);
ledshift(64);
waitgame(16);
ledshift(16);
papawait();
ledshift(4);
oct = 2;
waitgame(2);
ledshift(2);
waitgame(1);
ledshift(16);
oct = 0;
waitgame(64);
ledshift(32);
oct = 2;
waitgame(2);
ledshift(64);
waitgame(4);
ledshift(64);
oct = 0;
waitgame(64);
ledshift(0);
waitgame(16);


oct = 2;    
ledshift(0);
waitgame(4);
ledshift(0);
waitgame(2);
oct = 0;

ledshift(0);
waitgame(16);
ledshift(0);
waitgame(32);
ledshift(0);
waitgame(64);
ledshift(0);
waitgame(64);
ledshift(0);
papawait();
score = score * 100 / scoremax ;
tdisp();
papawait();
papawait();
papawait();
papawait();
papawait();  

}
void song_star(){
  //line1
  ledshift(64);
  papawait();
  ledshift(64); 
  papawait(); 
  ledshift(4);
  papawait();
  ledshift(4);
  papawait();
  ledshift(2);
  papawait();
  ledshift(2);
  papawait();
  ledshift(4);
  papawait();
  ledshift(0);    
  waitgame(64);
     
  ledshift(8);
  waitgame(64);
  ledshift(8);
  waitgame(4);
  ledshift(16);
  waitgame(4);
  ledshift(16);
  waitgame(2);
  ledshift(32);
  waitgame(2);
  ledshift(32);
  waitgame(4);
  ledshift(64);
  papawait();
  ledshift(0);
  waitgame(8);
  
  
  //line 2
  ledshift(4);
  waitgame(8);
  ledshift(4);
  waitgame(16);
  ledshift(8);
  waitgame(16);
  ledshift(8);
  waitgame(32);
  ledshift(16);
  waitgame(32);
  ledshift(16);
  waitgame(64);
  ledshift(32);
  papawait();
  ledshift(0);
  waitgame(4);
  
  ledshift(4);
  waitgame(4);
  ledshift(4);
  waitgame(8);
  ledshift(8);
  waitgame(8);
  ledshift(8);
  waitgame(16);
  ledshift(16);
  waitgame(16);
  ledshift(16);
  waitgame(32);
  ledshift(32);
  papawait();
  ledshift(0);
  waitgame(4);  
  
  //line 3
  ledshift(64);
  waitgame(4);
  ledshift(64);
  waitgame(8);
  ledshift(4);
  waitgame(8);
  ledshift(4);
  waitgame(16);
  ledshift(2);
  waitgame(16);
  ledshift(2);
  waitgame(32);
  ledshift(4);
  papawait();
  ledshift(0);
  waitgame(64);
  
  ledshift(8);
  waitgame(64);
  ledshift(8);
  waitgame(4);
  ledshift(16);
  waitgame(4);
  ledshift(16);
  waitgame(2);
  ledshift(32);
  waitgame(2);
  ledshift(32);
  waitgame(4);
  ledshift(64);
  papawait();
  ledshift(0);
  waitgame(8);
  ledshift(0);
  waitgame(8);
  ledshift(0);
  waitgame(16);
  ledshift(0);
  waitgame(16);
  ledshift(0);
  waitgame(32);
  ledshift(0);
  waitgame(32);
  ledshift(0);
  waitgame(64);
  ledshift(0);
  papawait();
  score = score * 100 / scoremax ;
tdisp();
papawait();
papawait();
papawait();
papawait();
papawait();  
}