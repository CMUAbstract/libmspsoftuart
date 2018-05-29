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
#include <stdint.h>

#include <libmsp/periph.h>

#include "uart.h"

#define CONFIG_RX (defined(LIBMSPSOFTUART_RX_PORT) && defined(LIBMSPSOFTUART_RX_PIN))
#define CONFIG_TX (defined(LIBMSPSOFTUART_TX_PORT) && defined(LIBMSPSOFTUART_TX_PIN))

#if !CONFIG_TX
#error RX-only configuration not supported
#endif // !CONFIG_TX

#define TIMER_SOFTUART CONCAT(LIBMSPSOFTUART_TIMER_TYPE, LIBMSPSOFTUART_TIMER_IDX)

/* GPIO function select register differs among chips
 * Technically, this is documented as differing per device, but probably
 * consistent for whole family */
#if defined(__MSP430FR5949__) || \
    defined(__MSP430FR5994__)
// P*SEL0,P*SEL1 define a 2-bit value
#define SEL_REG SEL0 // we use only one of the bits
#else
#define SEL_REG SEL
#endif

/**
 * Bit time
 */
#define BIT_TIME        (LIBMSPSOFTUART_CLOCK_FREQ / LIBMSPSOFTUART_BAUDRATE)

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

void mspsoftuart_init(void)
{
    // default to high, for when we turn on the output pin (by setting SEL register)
    TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) = TOUT;

    GPIO(LIBMSPSOFTUART_TX_PORT, SEL_REG) |= BIT(LIBMSPSOFTUART_TX_PIN);
    GPIO(LIBMSPSOFTUART_TX_PORT, DIR) |= BIT(LIBMSPSOFTUART_TX_PIN);

#if CONFIG_RX
    GPIO(LIBMSPSOFTUART_RX_PORT, IES) |= BIT(LIBMSPSOFTUART_RX_PIN); // RXD Hi/lo edge interrupt
    GPIO(LIBMSPSOFTUART_RX_PORT, IFG) &= ~BIT(LIBMSPSOFTUART_RX_PIN); // Clear flag before enabling interrupt
    GPIO(LIBMSPSOFTUART_RX_PORT, IE) |= BIT(LIBMSPSOFTUART_RX_PIN); // Enable RXD interrupt
#endif
}

char mspsoftuart_receive_byte_sync(void)
{
    char ch;
    while (!hasReceived);

    ch = RXByte;
    hasReceived = false;

    return ch;
}

void mspsoftuart_send_byte_sync(char ch)
{
    uint8_t c = ch;

    TXByte = c;

    while(isReceiving); 					// Wait for RX completion

    TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) = TOUT; // TXD Idle as Mark
    // continuous mode
    TIMER(TIMER_SOFTUART, CTL) = TIMER_CLK_SOURCE_BITS(LIBMSPSOFTUART_TIMER_TYPE, SMCLK) + MC_2;

    bitCount = 0xA; 						// Load Bit counter, 8 bits + ST/SP


    // Initialize compare register
    TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCR) = TIMER(TIMER_SOFTUART, R);

    // Set time till first bit
    TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCR) += BIT_TIME; 

    TXByte |= 0x100; 						// Add stop bit to TXByte (which is logical 1)
    TXByte = TXByte << 1; 					// Add start bit (which is logical 0)

    // Set signal, intial value, enable interrupts
    TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) = CCIS_0 + OUTMOD_0 + CCIE + TOUT;

    // Wait for previous TX completion
    while ( TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) & CCIE );
}

#if CONFIG_RX
/**
 * ISR for RXD
 */
#ifdef LIBMSPSOFTUART_RX_HANDLER__ISR
ISR(GPIO_VECTOR(LIBMSPSOFTUART_RX_PORT))
#else // !LIBMSPSOFTUART_RX_HANDLER__ISR
void softuart_rx_isr(void)
#endif // !LIBMSPSOFTUART_RX_HANDLER__ISR
{
    isReceiving = true;

    GPIO(LIBMSPSOFTUART_RX_PORT, IE) &= ~BIT(LIBMSPSOFTUART_RX_PIN); // Disable RXD interrupt
    GPIO(LIBMSPSOFTUART_RX_PORT, IFG) &= ~BIT(LIBMSPSOFTUART_RX_PIN); // Disable RXD interrupt

    // continuous mode
    TIMER(TIMER_SOFTUART, CTL) = TIMER_CLK_SOURCE_BITS(LIBMSPSOFTUART_TIMER_TYPE, SMCLK) + MC_2;

    // Initialize compare register
    TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCR) = TIMER(TIMER_SOFTUART, R);

    // Set time till first bit
    TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCR) += HALF_BIT_TIME; 

    // Disable TX and enable interrupts
    TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) = OUTMOD_1 + CCIE;

    RXByte = 0; 					// Initialize RXByte
    bitCount = 9; 					// Load Bit counter, 8 bits + start bit
}
#endif // CONFIG_RX

/**
 * ISR for TXD and RXD
 */
#ifdef LIBMSPSOFTUART_TIMER_HANDLER__ISR
ISR(TIMER_VECTOR(LIBMSPSOFTUART_TIMER_TYPE, LIBMSPSOFTUART_TIMER_IDX, LIBMSPSOFTUART_TIMER_CC))
#else // !LIBMSPSOFTUART_TIMER_HANDLER__ISR
void softuart_timer_isr(void)
#endif // !LIBMSPSOFTUART_TIMER_HANDLER__ISR
{
#if LIBMSPSOFTUART_TIMER_CC > 0 // CC0 has a dedicate IV that is auto-cleared upon servicing
    // This is crucial: reading IV register clears the interrupt flag
    if (!(TIMER_INTVEC(LIBMSPSOFTUART_TIMER_TYPE, LIBMSPSOFTUART_TIMER_IDX) &
            TIMER_INTFLAG(LIBMSPSOFTUART_TIMER_TYPE, LIBMSPSOFTUART_TIMER_IDX, LIBMSPSOFTUART_TIMER_CC)))
        return;
#endif // LIBMSPSOFTUART_TIMER_CC > 0

    if(!isReceiving) {
        TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCR) += BIT_TIME; // Add Offset to CCR0
        if ( bitCount == 0) { 					// If all bits TXed
            // timer off (for power consumption)
            TIMER(TIMER_SOFTUART, CTL) = TIMER_CLK_SOURCE_BITS(LIBMSPSOFTUART_TIMER_TYPE, SMCLK);
            TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) &= ~CCIE; // Disable interrupt
        } else {
            if (TXByte & 0x01) {
                //OUTMOD_7 defines the 'window' of the field.
                TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) =
                    (TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) & ~OUTMOD_7 ) |
                    OUTMOD_1;
            } else {
                //OUTMOD_7 defines the 'window' of the field.
                TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) =
                    (TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) & ~OUTMOD_7 ) |
                    OUTMOD_5;
            }

            TXByte = TXByte >> 1;
            bitCount --;
        }
    } else {
#if CONFIG_RX
        TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCR) += BIT_TIME; // Add Offset to CCR0

        if ( bitCount == 0) {

            // timer off (for power consumption)
            TIMER(TIMER_SOFTUART, CTL) = TIMER_CLK_SOURCE_BITS(LIBMSPSOFTUART_TIMER_TYPE, SMCLK);
            TIMER_CC(TIMER_SOFTUART, LIBMSPSOFTUART_TIMER_CC, CCTL) &= ~CCIE; // Disable interrupt

            isReceiving = false;

            GPIO(LIBMSPSOFTUART_RX_PORT, IFG) &= ~BIT(LIBMSPSOFTUART_RX_PIN); // clear RXD IFG (interrupt flag)
            GPIO(LIBMSPSOFTUART_RX_PORT, IE) |= BIT(LIBMSPSOFTUART_RX_PIN); // enabled RXD interrupt

            if ( (RXByte & 0x201) == 0x200) { 	// Validate the start and stop bits are correct
                RXByte = RXByte >> 1; 			// Remove start bit
                RXByte &= 0xFF; 				// Remove stop bit
                hasReceived = true;
            }
        } else {
            // If bit is set?
            if ( (GPIO(LIBMSPSOFTUART_RX_PORT, IN) & BIT(LIBMSPSOFTUART_RX_PIN)) == BIT(LIBMSPSOFTUART_RX_PIN)) {
                RXByte |= 0x400; 				// Set the value in the RXByte
            }
            RXByte = RXByte >> 1; 				// Shift the bits down
            bitCount --;
        }
#endif // CONFIG_RX
    }
}
