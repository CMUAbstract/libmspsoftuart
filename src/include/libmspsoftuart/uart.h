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
 *****************************************************************************
 * Includes also improvements from Joby Taffey and fixes from Colin Funnell
 * as posted here:
 *
 * http://blog.hodgepig.org/tag/msp430/
 *****************************************************************************/

#ifndef __UART_H
#define __UART_H

#include <stdbool.h>
#include <stdint.h>

void mspsoftuart_init(void);
void mspsoftuart_send_byte_sync(uint8_t b);
uint8_t mspsoftuart_receive_byte_sync();

#endif

