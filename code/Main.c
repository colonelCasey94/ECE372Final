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

enum scanState{NOSCAN, WAIT, WHITE, READ, PRINT};

volatile int state = 0;
volatile int count = 0;


int main(void)
{

    TRISAbits.TRISA4 = 1; // setting up the push button
    CNEN1bits.CN0IE = 1;
    IFS1bits.CNIF = 0; // set flag low
    IEC1bits.CNIE = 1; // enable interrupt

        // UART1 Setup
        RPINR18bits.U1RXR = 5;
	RPOR5bits.RP11R = 3;
        U1BRG  = BRGVAL;
	U1MODE = 0x8000;
        U1STA  = 0x0440;
	IFS0bits.U1RXIF = 0;

        IFS1bits.CNIF = 0;
        IEC1bits.CNIE = 1;


        PR3 = 1032; // 0.1 second delay
	TMR3 = 0;
        T3CON = 0x8000;
	IFS0bits.T3IF = 0;
//	IEC0bits.T3IE = 1;

        /// ADC

        int ADC_Front_Mid = 0, ADC_Front_Right = 0, ADC_Front_Left = 0;
        int frontBlackWhite = 100;
        int ADC_Barcode;
        int barcodeBlackRed = 100, barcodeRedWhite = 150, i = 0;
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
        int PWM_Period = 1023;
        OC1CON = 0x000E;
        OC1CONbits.OCTSEL = 1;
        OC1R = PWM_Period;
        OC1RS = PWM_Period/2;
        //RPOR2bits.RP4R = 18;
        RPOR5bits.RP11R = 18;   ///// testing
        RPOR5bits.RP10R = 18;
//        TRISBbits.TRISB10 = 0;

        OC2CON = 0x000E;
        OC2CONbits.OCTSEL = 1;
        OC2R = PWM_Period;
        OC2RS = PWM_Period/2;
        RPOR4bits.RP8R = 19;
        RPOR4bits.RP9R = 19;
//        TRISBbits.TRISB9 = 0;

        OC3CON = 0x000E;
        OC3CONbits.OCTSEL = 1;
        OC3R = PWM_Period;
        OC3RS = 0;

	LCDInitialize();

        char* color = "";

        printf("Working\n");

	while(1)
	{
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

///////////////////////////////////////////////////////////////////////////////
//               Following Line                                              //
///////////////////////////////////////////////////////////////////////////////
//            if (ADC_Front_Left > frontBlackWhite && ADC_Front_Mid < frontBlackWhite && ADC_Front_Right >frontBlackWhite){
//               //if on line...
//                //OC1 RIGHT
//                RPOR5bits.RP10R = 18;
//                RPOR5bits.RP11R = 20;
//
//                //OC2 LEFT
//                RPOR4bits.RP8R = 19;
//                RPOR4bits.RP9R = 20;
//
//                OC1RS = PWM_Period;
//                OC2RS = PWM_Period;
//
//            } else if (ADC_Front_Left < frontBlackWhite && ADC_Front_Mid < frontBlackWhite && ADC_Front_Right > frontBlackWhite){
//                //if too far right
//                //OC1 RIGHT
//                RPOR5bits.RP10R = 18;
//                RPOR5bits.RP11R = 20;
//
//                //OC2 LEFT
//                RPOR4bits.RP8R = 19;
//                RPOR4bits.RP9R = 20;
//
//                //turn left
//                OC1RS = PWM_Period;
//                OC2RS = PWM_Period*.9;
//                DelayUs(50000);
//                DelayUs(50000);
//
//                //go straight to pause for possible correction onto line
//                OC1RS = PWM_Period;
//                OC2RS = PWM_Period;
//                DelayUs(50000);
//                DelayUs(50000);
//
//            } else if (ADC_Front_Left > frontBlackWhite && ADC_Front_Mid < frontBlackWhite && ADC_Front_Right < frontBlackWhite){
//                //if too far left
//                //OC1 RIGHT
//                RPOR5bits.RP10R = 18;
//                RPOR5bits.RP11R = 20;
//
//                //OC2 LEFT
//                RPOR4bits.RP8R = 19;
//                RPOR4bits.RP9R = 20;
//
//                //turn right
//                OC1RS = PWM_Period*.9;
//                OC2RS = PWM_Period;
//                DelayUs(50000);
//                DelayUs(50000);
//
//                //go straight to pause for possible correction onto line
//                OC1RS = PWM_Period;
//                OC2RS = PWM_Period;
//                DelayUs(50000);
//                DelayUs(50000);
//
//            } else {
//
//                //all stop
//                 //OC1 RIGHT
//                RPOR5bits.RP10R = 20;
//                RPOR5bits.RP11R = 20;
//
//                //OC2 LEFT
//                RPOR4bits.RP8R = 20;
//                RPOR4bits.RP9R = 20;
//
//                OC2RS = 0;
//                OC1RS = 0;
//            }
///////////////////////////////////////////////////////////////////////////////
//              BARCODE READER                                               //
///////////////////////////////////////////////////////////////////////////////

             AD1CHS = 2; // AN2 pin for reference
            DelayUs(2000);
            while(IFS0bits.AD1IF == 0);
            IFS0bits.AD1IF = 0;
            ADC_Barcode = ADC1BUF0; // digital value

            switch(currentScan){
                case NOSCAN:
                    if (ADC_Barcode < barcodeBlackRed){
                        currentScan = WAIT;
                        i = 0;
                         LCDMoveCursor(1,0);
                LCDPrintString("scan");
                    }
                    break;
                case WAIT:
                    if (ADC_Barcode > barcodeRedWhite){
                        currentScan = WHITE;
                         LCDMoveCursor(1,0);
                LCDPrintString("White");
                    }
                    break;
                case WHITE:
                    if (i == 4){
                        currentScan = PRINT;
                    } else if (ADC_Barcode < barcodeRedWhite){
                        currentScan = READ;
                        LCDMoveCursor(1,0);
                        LCDPrintString("read");
                        DelayUs(50000);
                        DelayUs(50000);
                    }
                    break;
                case READ:
                    if (ADC_Barcode < barcodeBlackRed){
                        value1[i] = 'B';
                        i++;
                        currentScan = WAIT;
                    } else if (ADC_Barcode < barcodeRedWhite){
                        value1[i] = 'R';
                        i++;
                        currentScan = WAIT;
                    } else {
                        currentScan = WHITE;
                    }
                    break;
                case PRINT:
                     LCDMoveCursor(1,0);
                LCDPrintString(value1);
                    break;
            }

///////////////////////////////////////////////////////////////////////////////
        }

	return 0;
}


void __attribute__((interrupt)) _CNInterrupt(void)
{
	IFS1bits.CNIF = 0;

       if(PORTBbits.RB5 == 0){
           while(PORTBbits.RB5 == 0);
           state++;
           if(state == 2){
               state = 0;
           }
       }
}

