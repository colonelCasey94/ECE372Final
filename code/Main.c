// ******************************************************************************************* //
// Include file for PIC24FJ64GA002 microcontroller. This include file defines
// MACROS for special function registers (SFR) and control bits within those
// registers.

#include "p24fj64ga002.h"
#include <stdio.h>
#include "lcd.h"
//#include "keypad.h"

#define XTFREQ          7372800         	  // On-board Crystal frequency
#define PLLMODE         4               	  // On-chip PLL setting (Fosc)
#define FCY             (XTFREQ*PLLMODE)/2    // Instruction Cycle Frequency (Fosc/2)

#define BAUDRATE         115200
#define BRGVAL          ((FCY/BAUDRATE)/16)-1
// ******************************************************************************************* //
// Configuration bits for CONFIG1 settings.
//
// Make sure "Configuration Bits set in code." option is checked in MPLAB.
//
// These settings are appropriate for debugging the PIC microcontroller. If you need to
// program the PIC for standalone operation, change the COE_ON option to COE_OFF.

_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF &
		 BKBUG_ON & COE_OFF & ICS_PGx1 &
		 FWDTEN_OFF & WINDIS_OFF & FWPSA_PR128 & WDTPS_PS32768 )

// ******************************************************************************************* //
// Configuration bits for CONFIG2 settings.
// Make sure "Configuration Bits set in code." option is checked in MPLAB.

_CONFIG2( IESO_OFF & SOSCSEL_SOSC & WUTSEL_LEG & FNOSC_PRIPLL & FCKSM_CSDCMD & OSCIOFNC_OFF &
		 IOL1WAY_OFF & I2C1SEL_PRI & POSCMOD_XT )

// ******************************************************************************************* //

// Varible used to indicate that the current configuration of the keypad has been changed,
// and the KeypadScan() function needs to be called.


// ******************************************************************************************* //

enum scanState{NOSCAN, WAIT, WHITE, READ, PRINT, PRINTWAIT};
enum roboState{FIND, FOLLOW, TURNAROUND};
enum lastSeen{LEFT,RIGHT,CENTER};

volatile int state = 0;
volatile int count = 0;


int main(void)
{
    enum lastSeen last = CENTER;
    enum roboState roboState = FIND;
    int blackLineCounter = 0;

    TRISAbits.TRISA4 = 1; // setting up the push button
    CNEN1bits.CN0IE = 1;
    IFS1bits.CNIF = 0; // set flag low
    IEC1bits.CNIE = 1; // enable interrupt
    
    TRISAbits.TRISA1 = 1;
    AD1PCFGbits.PCFG1 = 1;
    CNPU1bits.CN3PUE = 1;

        TMR1 = 0;
    PR1 = 65000; // 57600
    T1CON = 0x8030;
    IFS0bits.T1IF = 0;
//    IEC0bits.T1IE = 1;
    T1CONbits.TON = 0;


       
        PR3 = 1032; // 0.1 second delay
	TMR3 = 0;
        T3CON = 0x8000;
	IFS0bits.T3IF = 0;
//	IEC0bits.T3IE = 1;

        /// ADC

        int ADC_Front_Mid = 0, ADC_Front_Right = 0, ADC_Front_Left = 0;
        int frontBlackWhite = 150;
        int ADC_Barcode;
        int barcodeBlackRed = 170, barcodeRedWhite = 550, i = 0;
        enum scanState currentScan = NOSCAN;
        char value[8];
        char value1[5] = "    ";

//        AD1PCFG &= 0xFFC3;
        AD1PCFGbits.PCFG2 = 0;
        AD1PCFGbits.PCFG3 = 0;
        AD1PCFGbits.PCFG4 = 0;
       // AD1PCFGbits.PCFG5 = 0;
        AD1CON2 = 0; // reference voltage
        AD1CON3 = 0x0101;  // sample conversion
        AD1CON1 = 0x20E4;  // sample conversion
//        AD1CHS = 5; // AN0 pin for reference
        AD1CSSL = 0;

        IFS0bits.AD1IF = 0; // set flag low
        AD1CON1bits.ADON = 1; // turn on ADC

        // PWM
        float PWM_Period_RIGHT = 1023;
        OC1CON = 0x000E;
        OC1CONbits.OCTSEL = 1;
        OC1R = PWM_Period_RIGHT;
        OC1RS = PWM_Period_RIGHT/2;
        //RPOR2bits.RP4R = 18;
        RPOR5bits.RP11R = 18;   ///// testing
        RPOR5bits.RP10R = 18;
//        TRISBbits.TRISB10 = 0;

        float PWM_Period_LEFT = 1023;
        OC2CON = 0x000E;
        OC2CONbits.OCTSEL = 1;
        OC2R = PWM_Period_LEFT;
        OC2RS = PWM_Period_LEFT/2;
        RPOR4bits.RP8R = 19;
        RPOR4bits.RP9R = 19;
//        TRISBbits.TRISB9 = 0;

        OC3CON = 0x000E;
        OC3CONbits.OCTSEL = 1;
        OC3R = 0;
        OC3RS = 0;

	LCDInitialize();
        int Long_Delay = 60000;

        char* color = "";

        int flag = 1;
        
        
        int ADC_UP, ADC_Side = 0;
        int temp_UP, temp_LEFT, temp_RIGHT, temp_x, temp_y = 0;

        while (1){  
            if(PORTAbits.RA1 == 1)
            {
    ///////////////////////////////////////////////////////////////////////////////
    //              AUTO-MODE                                                    //
    ///////////////////////////////////////////////////////////////////////////////


                LCDInitialize();
                while ((PORTAbits.RA1 == 1)){
///////////////////////////////////////////////////////////////////////////////
//              Scanning Front                                               //
///////////////////////////////////////////////////////////////////////////////

            AD1CHS = 2; // AN2 pin for reference
            DelayUs(2000);
            while(IFS0bits.AD1IF == 0);
            IFS0bits.AD1IF = 0;
            ADC_Front_Mid = ADC1BUF0; // digital value



            AD1CHS = 3; // AN3 pin for reference
            DelayUs(2000);
            while(IFS0bits.AD1IF == 0);
            IFS0bits.AD1IF = 0;
            ADC_Front_Left = ADC1BUF0;



            AD1CHS = 4; // AN4 pin for reference
            DelayUs(2000);
            while(IFS0bits.AD1IF == 0);
            IFS0bits.AD1IF = 0;
            ADC_Front_Right = ADC1BUF0;


            int ADC_BatRate = 0;
            float float_BatRate = 0.0;

            AD1CHS = 5; // AN5 pin for reference
            DelayUs(2000);
            while(IFS0bits.AD1IF == 0);
            IFS0bits.AD1IF = 0;
            ADC_BatRate = ADC1BUF0; // digital value
           // float_BatRate = 1.4 - (ADC_BatRate*3.3/1023); //y = -(1/3)x + 1.4


           PWM_Period_RIGHT = ADC_BatRate;//1023 * (1 - (1.4 - (ADC_BatRate*3.3/1023)*(1/3)));
           PWM_Period_LEFT = ADC_BatRate;//1023 * (1 - (1.4 - (ADC_BatRate*3.3/1023)*(1/3)));

///////////////////////////////////////////////////////////////////////////////
//          Printing Front Values on top line                                //
///////////////////////////////////////////////////////////////////////////////

            sprintf(value, "%d",ADC_Front_Left); // convert digital value to string for LCD
            LCDMoveCursor(0,0);
            LCDPrintString(value);

            sprintf(value, "%d",ADC_Front_Mid); // convert digital value to string for LCD
            LCDMoveCursor(0,4);
            LCDPrintString(value);

            sprintf(value, "%d",ADC_Front_Right); // convert digital value to string for LCD
            LCDMoveCursor(0,8);
            LCDPrintString(value);


            switch(roboState){
                case FIND:
///////////////////////////////////////////////////////////////////////////////
//               Following Line                                              //
///////////////////////////////////////////////////////////////////////////////
                    blackLineCounter = 0;
                    roboState = FOLLOW;
                    currentScan = NOSCAN;
                break;

                case FOLLOW:
///////////////////////////////////////////////////////////////////////////////
//               Following Line                                              //
///////////////////////////////////////////////////////////////////////////////

                   if (ADC_Front_Left > frontBlackWhite && ADC_Front_Mid < frontBlackWhite && ADC_Front_Right >frontBlackWhite){
                       flag = 1;
                       last = CENTER;
                        //if on line...
                        //OC1 RIGHT
                        RPOR5bits.RP10R = 18;
                        RPOR5bits.RP11R = 20;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 19;
                        RPOR4bits.RP9R = 20;

                        OC1RS = PWM_Period_RIGHT;
                        OC2RS = PWM_Period_LEFT;

                    } else if (ADC_Front_Right > frontBlackWhite && ADC_Front_Mid < frontBlackWhite && ADC_Front_Left < frontBlackWhite){
                        flag =1;
                        last = RIGHT;
                        //if too far right
                        //OC1 RIGHT
                        RPOR5bits.RP10R = 18;
                        RPOR5bits.RP11R = 20;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 19;
                        RPOR4bits.RP9R = 20;

                        //turn left
                        OC1RS = PWM_Period_RIGHT;
                        OC2RS = PWM_Period_LEFT*6/10;


                    } else if (ADC_Front_Left > frontBlackWhite && ADC_Front_Mid < frontBlackWhite && ADC_Front_Right < frontBlackWhite){
                        flag = 1;
                        last=LEFT;
                        //if too far left
                        //OC1 RIGHT
                        RPOR5bits.RP10R = 18;
                        RPOR5bits.RP11R = 20;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 19;
                        RPOR4bits.RP9R = 20;

                        //turn right
                        OC1RS = PWM_Period_RIGHT*6/10;
                        OC2RS = PWM_Period_LEFT;


                    } else if (ADC_Front_Right > frontBlackWhite && ADC_Front_Mid > frontBlackWhite && ADC_Front_Left < frontBlackWhite){
                        flag = 1;
                        last = RIGHT;
                        //if way too far right
                        //OC1 RIGHT
                        RPOR5bits.RP10R = 18;
                        RPOR5bits.RP11R = 20;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 20;
                        RPOR4bits.RP9R = 19;

                        //turn left
                        OC1RS = PWM_Period_RIGHT;
                        OC2RS = PWM_Period_LEFT+100;

                    } else if (ADC_Front_Left > frontBlackWhite && ADC_Front_Mid > frontBlackWhite && ADC_Front_Right < frontBlackWhite){
                        flag = 1;
                        last = LEFT;
                        //if way too far left
                        //OC1 RIGHT
                        RPOR5bits.RP10R = 20;
                        RPOR5bits.RP11R = 18;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 19;
                        RPOR4bits.RP9R = 20;

                        //turn right
                        OC1RS = PWM_Period_RIGHT+100;
                        OC2RS = PWM_Period_LEFT;

                    }else if (ADC_Front_Left < frontBlackWhite && ADC_Front_Mid < frontBlackWhite && ADC_Front_Right < frontBlackWhite) {
                        //finds all black
                        blackLineCounter++;
                        //OC1 RIGHT
                        RPOR5bits.RP10R = 18;
                        RPOR5bits.RP11R = 20;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 19;
                        RPOR4bits.RP9R = 20;

                        //turn right
                        OC1RS = PWM_Period_RIGHT;
                        OC2RS = PWM_Period_LEFT;
                        while(ADC_Front_Left < frontBlackWhite && ADC_Front_Mid < frontBlackWhite && ADC_Front_Right < frontBlackWhite){
                            AD1CHS = 2; // AN2 pin for reference
                            DelayUs(2000);
                            while(IFS0bits.AD1IF == 0);
                            IFS0bits.AD1IF = 0;
                            ADC_Front_Mid = ADC1BUF0; // digital value



                            AD1CHS = 3; // AN3 pin for reference
                            DelayUs(2000);
                            while(IFS0bits.AD1IF == 0);
                            IFS0bits.AD1IF = 0;
                            ADC_Front_Left = ADC1BUF0;



                            AD1CHS = 4; // AN4 pin for reference
                            DelayUs(2000);
                            while(IFS0bits.AD1IF == 0);
                            IFS0bits.AD1IF = 0;
                            ADC_Front_Right = ADC1BUF0;
                        }
                    } else if (ADC_Front_Left > frontBlackWhite && ADC_Front_Mid > frontBlackWhite && ADC_Front_Right > frontBlackWhite && blackLineCounter >=2){

                         RPOR5bits.RP10R = 20;
                        RPOR5bits.RP11R = 18;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 19;
                        RPOR4bits.RP9R = 20;

                        OC2RS = 1023;
                        OC1RS = 1023;
                        DelayUs(50000);
                            roboState = TURNAROUND;

                    } else if (last == RIGHT) {

                        RPOR5bits.RP10R = 18;
                        RPOR5bits.RP11R = 20;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 20;
                        RPOR4bits.RP9R = 19;

                        //turn right
                        OC1RS = PWM_Period_RIGHT+100;
                        OC2RS = PWM_Period_LEFT;

                    }else if (last == LEFT) {

                        RPOR5bits.RP10R = 20;
                        RPOR5bits.RP11R = 18;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 19;
                        RPOR4bits.RP9R = 20;

                        //turn right
                        OC1RS = PWM_Period_RIGHT+100;
                        OC2RS = PWM_Period_LEFT;

                    }else {

                        //all stop
                        //OC1 RIGHT
                        RPOR5bits.RP10R = 20;
                        RPOR5bits.RP11R = 20;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 20;
                        RPOR4bits.RP9R = 20;

                        OC2RS = 0;
                        OC1RS = 0;
                    }

                   break;
                case TURNAROUND:
///////////////////////////////////////////////////////////////////////////////
//              turn around at end                                           //
///////////////////////////////////////////////////////////////////////////////

                        //all stop
                        //OC1 RIGHT
                        RPOR5bits.RP10R = 18;
                        RPOR5bits.RP11R = 20;

                        //OC2 LEFT
                        RPOR4bits.RP8R = 20;
                        RPOR4bits.RP9R = 19;

                        OC2RS = 1000;
                        OC1RS = 1000;

                        if (T1CONbits.TON == 0){
                            TMR1 = 0;
                            T1CONbits.TON = 1;
                        }

                        if ((ADC_Front_Left < frontBlackWhite || ADC_Front_Mid < frontBlackWhite || ADC_Front_Right < frontBlackWhite)&&!TMR3<57600){
                            T1CONbits.TON = 0;
                            TMR1 = 0;
                            roboState = FIND;
                        }
                   break;
            }
///////////////////////////////////////////////////////////////////////////////
//              BARCODE READER                                               //
///////////////////////////////////////////////////////////////////////////////

            AD1CHS = 0; // AN2 pin for reference
            DelayUs(2000);
            while(IFS0bits.AD1IF == 0);
            IFS0bits.AD1IF = 0;
            ADC_Barcode = ADC1BUF0; // digital value

            sprintf(value, "%d",ADC_Barcode); // convert digital value to string for LCD
            LCDMoveCursor(0,12);
            LCDPrintString(value);

            switch(currentScan){
                case NOSCAN:
                    LCDClear();
                    if (ADC_Barcode < barcodeBlackRed){
                        currentScan = WAIT;
                        i = 0;
                         LCDMoveCursor(1,0);
                         LCDPrintString("scan");
                    }
                    break;
                case WAIT:
                    if (ADC_Barcode > barcodeRedWhite){
                        TMR1=0;
                        T1CONbits.TON = 1;
                        currentScan = WHITE;
                         LCDMoveCursor(1,0);
                LCDPrintString("White");
                    }
                    break;
                case WHITE:
                    if(IFS0bits.T1IF == 1){
                        IFS0bits.T1IF = 0;
                        currentScan = NOSCAN;
                        T1CONbits.TON = 0;
                    } else if (i == 4){
                        currentScan = PRINT;
                    } else if (ADC_Barcode < barcodeRedWhite){
                        TMR1=0;
                        T1CONbits.TON = 0;
                        currentScan = READ;
                        LCDMoveCursor(1,0);
                        LCDPrintString("read");
                        DelayUs(50000);
                    }
                    break;
                case READ:
                    if (ADC_Barcode < barcodeBlackRed){
                        value1[i] = '0';
                        i++;
                        currentScan = WAIT;
                    } else if (ADC_Barcode < barcodeRedWhite){
                        value1[i] = '1';
                        i++;
                        currentScan = WAIT;
                    } else {
                        currentScan = WHITE;
                    }
                    break;
                case PRINT:
                     LCDMoveCursor(1,0);
                LCDPrintString(value1);
                i = 0;
                TMR1 = 0;
                PR1 = 0xFFFF;
                IFS0bits.T1IF = 0;
                T1CONbits.TON = 1;
                currentScan = PRINTWAIT;
                    break;
                case PRINTWAIT:
                    if(IFS0bits.T1IF == 1){
                        IFS0bits.T1IF = 0;
                        T1CONbits.TON = 0;
                        //PR1 = PR1 / 4;
                        currentScan = NOSCAN;
                    }
            }
    ///////////////////////////////////////////////////////////////////////////////
    //            END AUTO-MODE                                                  //
    ///////////////////////////////////////////////////////////////////////////////
                }
            }
            else
            {
    ///////////////////////////////////////////////////////////////////////////////
    //              CONTROLLER                                                   //
    ///////////////////////////////////////////////////////////////////////////////

                AD1PCFGbits.PCFG10 = 0;
                AD1PCFGbits.PCFG9 = 0;

                TRISBbits.TRISB15 = 1;
                TRISBbits.TRISB14 = 1;

                temp_UP = 0, temp_LEFT = 0, temp_RIGHT = 0, temp_x = 0, temp_y = 0;

                AD1CHS = 10; // AN2 pin for reference
                DelayUs(2000);
                while(IFS0bits.AD1IF == 0);
                IFS0bits.AD1IF = 0;
                ADC_UP = ADC1BUF0; // digital value

                AD1CHS = 9; // AN2 pin for reference
                DelayUs(2000);
                while(IFS0bits.AD1IF == 0);
                IFS0bits.AD1IF = 0;
                ADC_Side = ADC1BUF0; // digital value




                if(ADC_Side > 510){
                    temp_LEFT = (ADC_Side - 512)*2 - 175;
                    temp_RIGHT = 0;
                }
                if(ADC_Side < 500){
                    temp_RIGHT = 1023 - (ADC_Side * 2);
                    temp_LEFT = 0;
                }

                if(ADC_UP > 510){
                    temp_UP = (ADC_UP - 512)*2;
                }
                if(ADC_UP < 500){
                    temp_UP = 1023 - (ADC_UP * 2);
                }

                if(ADC_UP >= 510){
                    //OC1 RIGHT
                    RPOR5bits.RP10R = 18;
                    RPOR5bits.RP11R = 20;

                    //OC2 LEFT
                    RPOR4bits.RP8R = 19;
                    RPOR4bits.RP9R = 20;
                }

                else if(ADC_UP < 510){
                    //OC1 RIGHT
                    RPOR5bits.RP10R = 20;
                    RPOR5bits.RP11R = 18;

                    //OC2 LEFT
                    RPOR4bits.RP8R = 20;
                    RPOR4bits.RP9R = 19;
                }

                temp_x = temp_UP + temp_RIGHT;
                if(temp_x > 1023){
                    temp_x = 1023;
                }

                temp_y = temp_UP + temp_LEFT;
                if(temp_y > 1023){
                    temp_y = 1023;
                }

                OC1RS = temp_x; // RIGHT
                OC2RS = temp_y;  // LEFT


                //**********************************//
                //**********************************//
            }
        }



	return 0;
}


void __attribute__((interrupt)) _CNInterrupt(void)
{
	IFS1bits.CNIF = 0;
}




 