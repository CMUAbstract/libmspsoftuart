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

#include <libmsp/periph.h>
#include <libmsp/clock.h>

#include "pins.h"
#include "config.h"
#include "printf.h"

/**
 * Bit time
 */
#define BIT_TIME        (CONFIG_SMCLK_FREQ / CONFIG_SOFTUART_BAUDRATE)

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

int x = TAOUT;

void printf_init(void)
{
    GPIO(PORT_SOFTUART_TXD, SEL) |= BIT(PIN_SOFTUART_TXD);
    GPIO(PORT_SOFTUART_TXD, DIR) |= BIT(PIN_SOFTUART_TXD);

#ifdef CONFIG_RX
    GPIO(PORT_SOFTUART_RXD, IES) |= BIT(PIN_SOFTUART_RXD); // RXD Hi/lo edge interrupt
    GPIO(PORT_SOFTUART_RXD, IFG) &= ~BIT(PIN_SOFTUART_RXD); // Clear flag before enabling interrupt
    GPIO(PORT_SOFTUART_RXD, IE) |= BIT(PIN_SOFTUART_RXD); // Enable RXD interrupt
#endif
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

    TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) = TAOUT; // TXD Idle as Mark
    TIMER(TIMER_SOFTUART, CTL) = TASSEL_2 + MC_2; // SMCLK, continuous mode

    bitCount = 0xA; 						// Load Bit counter, 8 bits + ST/SP


    // Initialize compare register
    TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCR) = TIMER(TIMER_SOFTUART, R);

    // Set time till first bit
    TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCR) += BIT_TIME; 

    TXByte |= 0x100; 						// Add stop bit to TXByte (which is logical 1)
    TXByte = TXByte << 1; 					// Add start bit (which is logical 0)

    // Set signal, intial value, enable interrupts
    TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) = CCIS_0 + OUTMOD_0 + CCIE + TAOUT; 

    // Wait for previous TX completion
    while ( TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) & CCIE );

    return ch;
}

int puts(const char *str)
{
    while(*str != 0) putchar(*str++);
    putchar('\n'); // semantics of puts say it appends a newline
    return 0;
}

int puts_no_newline(const char *str)
{
    while(*str != 0) putchar(*str++);
    return 0;
}

#ifdef CONFIG_RX
/**
 * ISR for RXD
 */
#ifdef CONFIG_ISR_RX
__attribute__ ((interrupt(GPIO_VECTOR(PORT_SOFTUART_RXD))))
void GPIO_ISR(PORT_SOFTUART_RXD)(void)
#else // !CONFIG_ISR_RX
void softuart_rx_isr(void)
#endif // !CONFIG_ISR_RX
{
    isReceiving = true;

    GPIO(PORT_SOFTUART_RXD, IE) &= ~BIT(PIN_SOFTUART_RXD); // Disable RXD interrupt
    GPIO(PORT_SOFTUART_RXD, IFG) &= ~BIT(PIN_SOFTUART_RXD); // Disable RXD interrupt

    TIMER(TIMER_SOFTUART, CTL) = TASSEL_2 + MC_2; // SMCLK, continuous mode

    // Initialize compare register
    TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCR) = TIMER(TIMER_SOFTUART, R);

    // Set time till first bit
    TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCR) += HALF_BIT_TIME; 

    // Disable TX and enable interrupts
    TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) = OUTMOD_1 + CCIE;

    RXByte = 0; 					// Initialize RXByte
    bitCount = 9; 					// Load Bit counter, 8 bits + start bit
}
#endif // CONFIG_RX

/**
 * ISR for TXD and RXD
 */
#ifdef CONFIG_ISR_TIMER
__attribute__ ((interrupt(TIMER_VECTOR(TIMER_SOFTUART_TYPE, TIMER_SOFTUART_IDX, 0))))
void TIMER_ISR(TIMER_SOFTUART)(void)
#else // !CONFIG_ISR_TIMER
void softuart_timer_isr(void)
#endif // !CONFIG_ISR_TIMER
{
    if(!isReceiving) {
        TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCR) += BIT_TIME; // Add Offset to CCR0
        if ( bitCount == 0) { 					// If all bits TXed
            TIMER(TIMER_SOFTUART, CTL) = TASSEL_2; // SMCLK, timer off (for power consumption)
            TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) &= ~CCIE; // Disable interrupt
        } else {
            if (TXByte & 0x01) {
                //OUTMOD_7 defines the 'window' of the field.
                TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) =
                    (TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) & ~OUTMOD_7 ) |
                    OUTMOD_1;
            } else {
                //OUTMOD_7 defines the 'window' of the field.
                TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) =
                    (TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) & ~OUTMOD_7 ) |
                    OUTMOD_5;
            }

            TXByte = TXByte >> 1;
            bitCount --;
        }
    } else {
#ifdef CONFIG_RX
        TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCR) += BIT_TIME; // Add Offset to CCR0

        if ( bitCount == 0) {

            TIMER(TIMER_SOFTUART, CTL) = TASSEL_2; // SMCLK, timer off (for power consumption)
            TIMER_CC(TIMER_SOFTUART, TIMER_SOFTUART_CC, CCTL) &= ~CCIE; // Disable interrupt

            isReceiving = false;

            GPIO(PORT_SOFTUART_RXD, IFG) &= ~BIT(PIN_SOFTUART_RXD); // clear RXD IFG (interrupt flag)
            GPIO(PORT_SOFTUART_RXD, IE) |= BIT(PIN_SOFTUART_RXD); // enabled RXD interrupt

            if ( (RXByte & 0x201) == 0x200) { 	// Validate the start and stop bits are correct
                RXByte = RXByte >> 1; 			// Remove start bit
                RXByte &= 0xFF; 				// Remove stop bit
                hasReceived = true;
            }
        } else {
            // If bit is set?
            if ( (GPIO(PORT_SOFTUART_RXD, IN) & BIT(PIN_SOFTUART_RXD)) == BIT(PIN_SOFTUART_RXD)) {
                RXByte |= 0x400; 				// Set the value in the RXByte
            }
            RXByte = RXByte >> 1; 				// Shift the bits down
            bitCount --;
        }
#endif // CONFIG_RX
    }
}

