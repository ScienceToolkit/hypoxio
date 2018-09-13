/******************************************************************************
* Project:   Hypoxio Controller Firmware                                      *
* Professor: Jeff Stuart                                                      *
* Author:    Mike De Lange                                                    *
* Version:   1.0                                                              *
* IDE:       Microchip MPLAB X IDE v4.15                                      *
* Compiler:  Microchip XC16 v1.33                                             * 
* Last Mod:  August 29, 2018                                                  *
******************************************************************************/
/******************************************************************************
* Processor Configuration                                                     *
******************************************************************************/
// FOSC
#pragma config FOSFPR = FRC_PLL4        // Oscillator (FRC w/PLL 4x)
#pragma config FCKSMEN = CSW_FSCM_OFF   // Clock Switching and Monitor (Sw Disabled, Mon Disabled)

// FWDT
#pragma config FWPSB = WDTPSB_16        // WDT Prescaler B (1:16)
#pragma config FWPSA = WDTPSA_512       // WDT Prescaler A (1:512)
#pragma config WDT = WDT_OFF            // Watchdog Timer (Disabled)

// FBORPOR
#pragma config FPWRT = PWRT_64          // POR Timer Value (64ms)
#pragma config BODENV = BORV_27         // Brown Out Voltage (2.7V)
#pragma config BOREN = PBOR_ON          // PBOR Enable (Enabled)
#pragma config MCLRE = MCLR_EN          // Master Clear Enable (Enabled)

// FBS
#pragma config BWRP = WR_PROTECT_BOOT_OFF// Boot Segment Program Memory Write Protect (Boot Segment Program Memory may be written)
#pragma config BSS = NO_BOOT_CODE       // Boot Segment Program Flash Memory Code Protection (No Boot Segment)
#pragma config EBS = NO_BOOT_EEPROM     // Boot Segment Data EEPROM Protection (No Boot EEPROM)
#pragma config RBS = NO_BOOT_RAM        // Boot Segment Data RAM Protection (No Boot RAM)

// FSS
#pragma config SWRP = WR_PROT_SEC_OFF   // Secure Segment Program Write Protect (Disabled)
#pragma config SSS = NO_SEC_CODE        // Secure Segment Program Flash Memory Code Protection (No Secure Segment)
#pragma config ESS = NO_SEC_EEPROM      // Secure Segment Data EEPROM Protection (No Segment Data EEPROM)
#pragma config RSS = NO_SEC_RAM         // Secure Segment Data RAM Protection (No Secure RAM)

// FGS
#pragma config GWRP = GWRP_OFF          // General Code Segment Write Protect (Disabled)
#pragma config GCP = GSS_OFF            // General Segment Code Protection (Disabled)

// FICD
#pragma config ICS = ICS_PGD           // Comm Channel Select (Use PGC and PGD)

/******************************************************************************
* Header Files                                                                *
******************************************************************************/
#include "p30f6014A.h"
#include <p30fxxxx.h>
#include <uart.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <timer.h>
#include <outcompare.h>
#include <ports.h>
#include <math.h>

#include "main.h"

/******************************************************************************
* Defines                                                                     *
******************************************************************************/
#define BAUD38400       (7372800/38400/16)-1

#define TILT_CENTER     0
//#define TILT_LEFT       -675
//#define TILT_RIGHT      675

// H-Bridge A Pins
#define HBRIDGEA_IN1    PORTDbits.RD0
#define HBRIDGEA_IN2    PORTDbits.RD1
#define HBRIDGEA_ENA    PORTDbits.RD4
#define HBRIDGEA_IN1_T  TRISDbits.TRISD0
#define HBRIDGEA_IN2_T  TRISDbits.TRISD1
#define HBRIDGEA_ENA_T  TRISDbits.TRISD4

// H-Bridge B Pins
#define HBRIDGEB_IN3    PORTDbits.RD2
#define HBRIDGEB_IN4    PORTDbits.RD3
#define HBRIDGEB_ENA    PORTDbits.RD5
#define HBRIDGEB_IN3_T  TRISDbits.TRISD2
#define HBRIDGEB_IN4_T  TRISDbits.TRISD3
#define HBRIDGEB_ENA_T  TRISDbits.TRISD5

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
int iStep;
Direction TiltDirection;
Mode SystemMode;
unsigned int uiCalibrationStepCount;
unsigned int uiCalibrationLevel;
unsigned int uiCurrentPosition;
unsigned long uiMotorSpeed;
bool bCmdReceived;
char szRxBuffer[BUFFERSIZE];
int iRxBufferPtr;
double dDistancePct;
unsigned int uiStepsFromEnd;
int iTotalCycles;
double dSpeedPct;
bool bLeftLimitSwitchHit;
bool bRightLimitSwitchHit;

/******************************************************************************
* Function Prototypes                                                         *
******************************************************************************/
void MotorStop();

/******************************************************************************
* UART1 Interrupt Routines                                                    *
******************************************************************************/
void __attribute__((__interrupt__,no_auto_psv)) _U1RXInterrupt(void) 
{
    char cRxChar;
    
   	while (DataRdyUART1()) 
	{ 
		cRxChar = ReadUART1();
        if (cRxChar == '<')
        {
            memset(szRxBuffer, 0, BUFFERSIZE);
            iRxBufferPtr = 0;
        }
        szRxBuffer[iRxBufferPtr] = cRxChar;
        iRxBufferPtr++;
        if (iRxBufferPtr >= BUFFERSIZE)
        {
            iRxBufferPtr = 0;
        }
        else
        {
            if (cRxChar == '>')
            {
                bCmdReceived = true;            
            }
        }
    }
        
    IFS0bits.U1RXIF = 0;    // Clear RX Interrupt flag 
}
 
/******************************************************************************
* External Interrupt Routines                                                 *
******************************************************************************/
void __attribute__((__interrupt__,no_auto_psv)) _INT0Interrupt(void) 
{
    MotorStop();
    bLeftLimitSwitchHit = true;
    IFS0bits.INT0IF = 0;
}
void __attribute__((__interrupt__,no_auto_psv)) _INT1Interrupt(void) 
{
    MotorStop();
    bRightLimitSwitchHit = true;
    IFS1bits.INT1IF = 0;
}

void Enable_LimitSwitches()
{
    CloseINT0();
    CloseINT1();
    ConfigINT0(EXT_INT_ENABLE & FALLING_EDGE_INT & GLOBAL_INT_ENABLE & EXT_INT_PRI_4);
    ConfigINT1(EXT_INT_ENABLE & FALLING_EDGE_INT & GLOBAL_INT_ENABLE & EXT_INT_PRI_4);
}

void Disable_LimitSwitches()
{
    CloseINT0();
    CloseINT1();
}

/******************************************************************************
* UART1 Routines                                                              *
******************************************************************************/
void UART1_Init()
{
	CloseUART1();
	ConfigIntUART1(UART_RX_INT_EN & UART_RX_INT_PR4 & UART_TX_INT_DIS & 
                   UART_TX_INT_PR2);

	OpenUART1(UART_EN & UART_IDLE_CON & UART_DIS_WAKE & UART_DIS_LOOPBACK & 
              UART_NO_PAR_8BIT & UART_1STOPBIT & UART_DIS_ABAUD, 
              UART_INT_TX_BUF_EMPTY & UART_TX_PIN_NORMAL & UART_TX_ENABLE & 
              UART_INT_RX_CHAR & UART_ADR_DETECT_DIS & UART_RX_OVERRUN_CLEAR, 
              BAUD38400);
}

/******************************************************************************
* PWM                                                                         *
******************************************************************************/
void PWM_Init()
{
    // Timer2 Config
    T2CONbits.T32 = 1;      // Use Timers 2/3 in 32bit mode
    T2CONbits.TCKPS = 0;    // 1:1 Prescale
    // PWM using OC1
    OC1CON = 0x0000;    // Turn off Output Compare 1 Module
    //OC1R = 368;         // Initialize Compare Register1
    OC1RS = TILT_CENTER;      // Set the pulse width
                                //  3700 = full CCW (0.506ms)
                                // 11200 = center (1.533ms)
                                // 18700 = full CW (2.557ms)
    OC1CON = 0x0006;            // Load new compare mode to OC1CON
    PR2 = 0x3900;       // \ Set the period to be
    PR3 = 0x0002;       // /   approximately 20ms
    //IPC1bits.T2IP = 1; // Setup Output Compare 1 interrupt for
    IFS0bits.T2IF = 0; // Clear Output Compare 1 interrupt flag
    IEC0bits.T2IE = 1; // Enable Output Compare 1 interrupts
    T2CONbits.TON = 1; // Start Timer2 with assumed settings       
}

/******************************************************************************
* Timer Init                                                                  *
******************************************************************************/
void Init_Timer1(int iSpeed)
{
	WriteTimer1(0);
	OpenTimer1(T1_ON & T1_IDLE_CON & T1_GATE_OFF & T1_PS_1_1 & T1_SYNC_EXT_OFF & T1_SOURCE_INT, iSpeed);
	ConfigIntTimer1(T1_INT_PRIOR_2 & T1_INT_ON);
}

void Init_Timer23(unsigned long ulSpeed)
{
	WriteTimer23(0);
	OpenTimer23(T2_ON & T2_GATE_OFF & T2_PS_1_1 & T2_32BIT_MODE_ON & T2_SOURCE_INT, ulSpeed);
	ConfigIntTimer23(T3_INT_PRIOR_2 & T3_INT_ON);
}

/******************************************************************************
* Motor Control                                                               *
******************************************************************************/
void MotorStop()
{
    CloseTimer23();
    PORTD = 0x0000;
}

void MotorControl(Direction dir, unsigned int uiSpeedPct)
{
    if (uiSpeedPct == 0)
    {
        //MotorStop();
    }
    else
    {
        TiltDirection = dir;       
        uiMotorSpeed = TILT_SPEED_MIN - (unsigned long)((double)(TILT_SPEED_MIN-TILT_SPEED_MAX) * ((double)uiSpeedPct/100.0));
        Init_Timer23(uiMotorSpeed);
        WriteTimer23(0);
    }
}

/******************************************************************************
* Timer Interrupts                                                            *
******************************************************************************/
void __attribute__ ((__interrupt__,no_auto_psv)) _T1Interrupt(void)
{
//    if (PORTAbits.RA6 == 1)
//    {
//        LATAbits.LATA6 = 0;
//    }
//    else
//    {
//        LATAbits.LATA6 = 1;
//    }
    IFS0bits.T1IF = 0;
}

// Outputs a power level between 0.0 and 1.0 based on a step position 
// between 0 and 150.
double GetSpeed(unsigned int uiPosition)
{
    return 1.0-((cos((M_PI/180.0*1.21)*(double)uiPosition)+1)/2.0);
}

// Outputs the number of step from the end of the maximum range.
// Input   0.0 -> 100.0
// Output   50 -> dWorkingRange
unsigned int GetDistanceFromEndInSteps(double dPct)
{
    double dWorkingRange = ((double)uiCalibrationStepCount/2.0) - (double)ACCELERATION_WINDOW - 50.0;
    double dPortion = dWorkingRange * (dPct/100.0);
    double dStepsFromEnd = 50.0 + (dWorkingRange - dPortion);
    return (unsigned int)dStepsFromEnd;
}

void __attribute__ ((__interrupt__,no_auto_psv)) _T3Interrupt(void)
{
//    if (PORTAbits.RA6 == 1)
//    {
//        LATAbits.LATA6 = 0;
//    }
//    else
//    {
//        LATAbits.LATA6 = 1;
//    }

    double dSpeed;
    int a;
    
    // Step Control
    //
    switch (iStep)
    {
        case 0: 
            if (TiltDirection == Left) { LATD=0x0035; } else { LATD=0x0039; }
            iStep = 1;
            break;
        case 1: 
            if (TiltDirection == Left) { LATD=0x0036; } else { LATD=0x003A; }
            iStep = 2;
            break;
        case 2: 
            if (TiltDirection == Left) { LATD=0x003A; } else { LATD=0x0036; }
            iStep = 3;
            break;
        case 3: 
            if (TiltDirection == Left) { LATD=0x0039; } else { LATD=0x0035; }
            iStep = 0;
            break;
    }
    
    // Keep track of the current position
    //
    if (TiltDirection == Left)
    {
        uiCurrentPosition--;
    }
    else /* if (TiltDirection == Right) */
    {
        uiCurrentPosition++;
    }

    if (SystemMode == Calibrate)
    {
        uiCalibrationStepCount++;
    }
    else if (SystemMode == Normal)
    {
        if (TiltDirection == Left)
        {
            // Check to see when we pass level so we can decrement the 
            // total number of cycles we need to do.
            if (uiCurrentPosition == uiCalibrationLevel)
            {
                iTotalCycles--;
                if (iTotalCycles <= 0)
                {
                    MotorStop();
                }
            }

            if (uiCurrentPosition < uiCalibrationLevel)
            {
                // [LEFT HALF] Deacceleration window
                if ((uiCurrentPosition >= 0+uiStepsFromEnd) && (uiCurrentPosition < ACCELERATION_WINDOW+uiStepsFromEnd))
                {
                    dSpeed = GetSpeed(uiCurrentPosition-uiStepsFromEnd)*dSpeedPct;
                    MotorControl(Left, dSpeed);
                }
            }
            else if (uiCurrentPosition > uiCalibrationLevel)
            {
                // [RIGHT HALF] Acceleration window
                if ((uiCurrentPosition <= uiCalibrationStepCount-uiStepsFromEnd) && (uiCurrentPosition > uiCalibrationStepCount-ACCELERATION_WINDOW-uiStepsFromEnd))
                {
                    dSpeed = GetSpeed((uiCalibrationStepCount-uiCurrentPosition)-uiStepsFromEnd)*dSpeedPct;
                    MotorControl(Left, dSpeed);
                }
            }

            if (uiCurrentPosition < uiStepsFromEnd)
            {
                TiltDirection = Right;
            }
        }
        else if (TiltDirection == Right)
        {
            if (uiCurrentPosition < uiCalibrationLevel)
            {
                // [LEFT HALF] Acceleration window
                if ((uiCurrentPosition >= 0+uiStepsFromEnd) && (uiCurrentPosition < ACCELERATION_WINDOW+uiStepsFromEnd))
                {
                    dSpeed = GetSpeed(uiCurrentPosition-uiStepsFromEnd)*dSpeedPct;
                    MotorControl(Right, dSpeed);
                }
            }
            else if (uiCurrentPosition > uiCalibrationLevel)
            {
                // [RIGHT HALF] Deacceleration window
                if ((uiCurrentPosition > uiCalibrationStepCount-ACCELERATION_WINDOW-uiStepsFromEnd) && (uiCurrentPosition <= uiCalibrationStepCount-uiStepsFromEnd))
                {
                    dSpeed = GetSpeed((uiCalibrationStepCount-uiCurrentPosition)-uiStepsFromEnd)*dSpeedPct;
                    MotorControl(Right, dSpeed);
                }
            }
            if (uiCurrentPosition > uiCalibrationStepCount-uiStepsFromEnd)
            {
                TiltDirection = Left;
            }
        }
    } // End SystemMode == Normal

    WriteTimer23(0);
 
    IFS0bits.T3IF = 0;
}

/******************************************************************************
* Table Control                                                               *
******************************************************************************/
void Table_CalibrateLimits()
{
    /* 1. Move motor to left extreme.
       2. Clear counter.
       3. Move motor to right extreme while counting steps.
       4. Usable range = total steps - 10 (5 for each side)
       5. Center position = Usable range / 2
    */
    SystemMode = FastMove;
    MotorControl(Left, 100);
    while (LIMITSWITCH_LEFT);
    MotorStop();
    uiCalibrationStepCount = 0;
    SystemMode = Calibrate;
    MotorControl(Right, 100);
    while (LIMITSWITCH_RIGHT);
    MotorStop();
    uiCalibrationLevel = (unsigned int)((double)uiCalibrationStepCount / 2.0);
    uiCurrentPosition = uiCalibrationStepCount;
    SystemMode = Normal;
}

void Table_GotoLevel()
{
    SystemMode = FastMove;
    if (uiCurrentPosition < uiCalibrationLevel)
    {
        MotorControl(Right, 100);
    }
    else if (uiCurrentPosition > uiCalibrationLevel)
    {
        MotorControl(Left, 100);
    }
    while (uiCurrentPosition != uiCalibrationLevel);
    MotorStop();
    SystemMode = Normal;
}


/******************************************************************************
* Main                                                                        *
******************************************************************************/
int main(int argc, char** argv)
 {
    char szParam[8];
    int iDelay;
    char szTXBuffer[32];
    int iTXBufferLength;
    int iIdx;
    
    // Motor control lines
    LATD = 0x0000;
    TRISD = 0xFFC0;         // 1111 1111 1100 0000
    // Limit switches
    TRISFbits.TRISF6 = 1;
    TRISAbits.TRISA12 = 1;
     
    UART1_Init();
    
    TiltDirection = Left;
    SystemMode = Normal;
    uiCurrentPosition = 0;
    bCmdReceived = false;
    memset(szRxBuffer, 0, BUFFERSIZE*sizeof(char));
    iRxBufferPtr = 0;
    iStep = 0;
    dDistancePct = 100.0;
    uiStepsFromEnd = 50;
    iTotalCycles = 1;
    dSpeedPct = 100.0;
    bLeftLimitSwitchHit = false;
    bRightLimitSwitchHit = false;
       
    // CALIBRATION PROCEDURE
    //
    // Disable the limit switch interrupts so we can calibrate for 
    // full range.
    Disable_LimitSwitches();
    // Calibrate the table's limits
    Table_CalibrateLimits();
    // Move to level
    Table_GotoLevel();
    // Calibration complete.  Enable the limit switches to protect 
    // from crashing.
    Enable_LimitSwitches();
    //
    // END CALIBRATION PROCEDURE
    
    while (1)
    {
        if (bCmdReceived == true)
        {
            // Tilt Table Commands:
            //       <R>    Reset table to center position
            //  <T$$$###@@>    Run tilt cycle
            //  0123456789A
            //      $$$  Tilt Distance % 000-100
            //      ###  Speed % 000-100
            //       @@  Number of tilts in a cycle
            if (szRxBuffer[1] == 'R')
            {
                Table_GotoLevel();
            }
            else if (szRxBuffer[1] == 'T')
            {
                // Grab tilt distance parameter
                memset(szParam, 0, 8);
                memcpy(szParam, &szRxBuffer[2], 3);
                dDistancePct = atof(szParam);
                uiStepsFromEnd = GetDistanceFromEndInSteps(dDistancePct);            
                        
                // Grab count parameter
                memset(szParam, 0, 8);
                memcpy(szParam, &szRxBuffer[8], 2);
                iTotalCycles = atoi(szParam);
                
                // Grab speed parameter
                memset(szParam, 0, 8);
                memcpy(szParam, &szRxBuffer[5], 3);
                dSpeedPct = atof(szParam);
                
                SystemMode = Normal;
                MotorControl(Left, dSpeedPct);
            }
            bCmdReceived = false;
        } // if (bCmdReceived == true)
        
        if (bLeftLimitSwitchHit == true)
        {
            memset(szTXBuffer, 0, 32*sizeof(char));
            strcpy(szTXBuffer, "[LIMITSWITCHLEFT]");
            iTXBufferLength = strlen(szTXBuffer);
            for (iIdx=0; iIdx<iTXBufferLength; iIdx++)
            {
                while (BusyUART1());
                WriteUART1(szTXBuffer[iIdx]);
            }
            bLeftLimitSwitchHit = false;
        }
        else if (bRightLimitSwitchHit == true)
        {
            memset(szTXBuffer, 0, 32*sizeof(char));
            strcpy(szTXBuffer, "[LIMITSWITCHRIGHT]");
            iTXBufferLength = strlen(szTXBuffer);
            for (iIdx=0; iIdx<iTXBufferLength; iIdx++)
            {
                while (BusyUART1());
                WriteUART1(szTXBuffer[iIdx]);
            }
            bRightLimitSwitchHit = false;   
        }

    } // while (1)

    return (EXIT_SUCCESS);
}

