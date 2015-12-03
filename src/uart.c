/******************************************************************************
 * Software UART example for MSP430.
 *
 * Stefan Wendler
 * sw@kaltpost.de
 * http://gpio.kaltpost.de
 *
 * ****************************************************************************
 * The original code is taken form Nicholas J. Conn example:
 *
 * http://www.msp430launchpad.com/2010/08/half-duplex-software-uart-on-launchpad.html
 *
 * Origial Description from Nicholas:
 *
 * Half Duplex Software UART on the LaunchPad
 *
 * Description: This code provides a simple Bi-Directional Half Duplex
 * Software UART. The timing is dependant on SMCLK, which
 * is set to 1MHz. The transmit function is based off of
 * the example code provided by TI with the LaunchPad.
 * This code was originally created for "NJC's MSP430
 * LaunchPad Blog".
 *
 * Author: Nicholas J. Conn - http://msp430launchpad.com
 * Email: webmaster at msp430launchpad.com
 * Date: 08-17-10
 * ****************************************************************************
 * Includes also improvements from Joby Taffey and fixes from Colin Funnell
 * as posted here:
 *
 * http://blog.hodgepig.org/tag/msp430/
 ******************************************************************************/

#include <msp430.h>
#include <stdbool.h>

#include "uart.h"

/**
 * TXD on P1.1
 */
#define TXD BIT1

/**
 * RXD on P1.2
 */
#define RXD BIT2

/**
 * CPU freq.
 */
#define FCPU 			1000000

/**
 * Baudrate
 */
#define BAUDRATE 		9600

/**
 * Bit time
 */
#define BIT_TIME        (FCPU / BAUDRATE)

/**
 * Half bit time
 */
#define HALF_BIT_TIME   (BIT_TIME / 2)

/**
 * Bit count, used when transmitting byte
 */
static volatile uint8_t bitCount;

/**
 * Value sent over UART when uart_putc() is called
 */
static volatile unsigned int TXByte;

/**
 * Value recieved once hasRecieved is set
 */
static volatile unsigned int RXByte;

/**
 * Status for when the device is receiving
 */
static volatile bool isReceiving = false;

/**
 * Lets the program know when a byte is received
 */
static volatile bool hasReceived = false;

void printf_init(void)
{
     P1SEL |= TXD;
     P1DIR |= TXD;

     P1IES |= RXD; 		// RXD Hi/lo edge interrupt
     P1IFG &= ~RXD; 		// Clear RXD (flag) before enabling interrupt
     P1IE  |= RXD; 		// Enable RXD interrupt
}

int getchar(void)
{
     int ch;
     while (!hasReceived);

     ch = RXByte;
     hasReceived = false;

     return ch;
}

int putchar(int ch)
{
     uint8_t c = ch;

     TXByte = c;

     while(isReceiving); 					// Wait for RX completion

     TA0CCTL0 = OUT; 							// TXD Idle as Mark
     TA0CTL = TASSEL_2 + MC_2; 				// SMCLK, continuous mode

     bitCount = 0xA; 						// Load Bit counter, 8 bits + ST/SP
     TA0CCR0 = TA0R; 							// Initialize compare register

     TA0CCR0 += BIT_TIME; 						// Set time till first bit
     TXByte |= 0x100; 						// Add stop bit to TXByte (which is logical 1)
     TXByte = TXByte << 1; 					// Add start bit (which is logical 0)

     TA0CCTL0 = CCIS_0 + OUTMOD_0 + CCIE + OUT; // Set signal, intial value, enable interrupts

     while ( TA0CCTL0 & CCIE ); 				// Wait for previous TX completion

     return ch;
}

int puts(const char *str)
{
     while(*str != 0) putchar(*str++);
     putchar('\n'); // semantics of puts say it appends a newline
     return 0;
}

/**
 * ISR for RXD
 */
void __attribute__ ((interrupt(PORT1_VECTOR)))
PORT1_ISR(void)
{
     isReceiving = true;

     P1IE &= ~RXD; 					// Disable RXD interrupt
     P1IFG &= ~RXD; 					// Clear RXD IFG (interrupt flag)

     TA0CTL = TASSEL_2 + MC_2; 		// SMCLK, continuous mode
     TA0CCR0 = TA0R; 					// Initialize compare register
     TA0CCR0 += HALF_BIT_TIME; 			// Set time till first bit
     TA0CCTL0 = OUTMOD_1 + CCIE; 		// Disable TX and enable interrupts

     RXByte = 0; 					// Initialize RXByte
     bitCount = 9; 					// Load Bit counter, 8 bits + start bit
}

/**
 * ISR for TXD and RXD
 */
void __attribute__ ((interrupt(TIMER0_A0_VECTOR)))
TIMERA0_ISR(void)
{
     if(!isReceiving) {
          TA0CCR0 += BIT_TIME; 						// Add Offset to CCR0
          if ( bitCount == 0) { 					// If all bits TXed
               TA0CTL = TASSEL_2; 					// SMCLK, timer off (for power consumption)
               TA0CCTL0 &= ~ CCIE ; 					// Disable interrupt
          } else {
               if (TXByte & 0x01) {
                    TA0CCTL0 = ((TA0CCTL0 & ~OUTMOD_7 ) | OUTMOD_1);  //OUTMOD_7 defines the 'window' of the field.
               } else {
                    TA0CCTL0 = ((TA0CCTL0 & ~OUTMOD_7 ) | OUTMOD_5);  //OUTMOD_7 defines the 'window' of the field.
               }

               TXByte = TXByte >> 1;
               bitCount --;
          }
     } else {
          TA0CCR0 += BIT_TIME; 						// Add Offset to CCR0

          if ( bitCount == 0) {

               TA0CTL = TASSEL_2; 					// SMCLK, timer off (for power consumption)
               TA0CCTL0 &= ~ CCIE ; 					// Disable interrupt

               isReceiving = false;

               P1IFG &= ~RXD; 						// clear RXD IFG (interrupt flag)
               P1IE |= RXD; 						// enabled RXD interrupt

               if ( (RXByte & 0x201) == 0x200) { 	// Validate the start and stop bits are correct
                    RXByte = RXByte >> 1; 			// Remove start bit
                    RXByte &= 0xFF; 				// Remove stop bit
                    hasReceived = true;
               }
          } else {
               if ( (P1IN & RXD) == RXD) { 		// If bit is set?
                    RXByte |= 0x400; 				// Set the value in the RXByte
               }
               RXByte = RXByte >> 1; 				// Shift the bits down
               bitCount --;
          }
     }
}

