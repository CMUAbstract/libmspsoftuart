#ifndef LIBMSPSOFTUART_H
#define LIBMSPSOFTUART_H

// Enable receiving direction
// #define CONFIG_RX

/* Define an ISR directly instead of a function
 *
 * If libsoftuart is the sole user of the interrupt vector of the corresponding
 * port/timer, then set this flag. Otherwise, unset this flag and call the
 * respective function from the ISR handler in your application.
 */
#define CONFIG_ISR_RX
#define CONFIG_ISR_TIMER

/**
 * TXD pin
 * NOTE: must be the output pin of TIMER_UART_CC register of TIMER_UART timer
 */
#define PORT_TXD 3
#define PIN_TXD  1

/* RXD pin */
#define PORT_RXD 3
#define PIN_RXD  2

/* The output of this CC register and timer must be routed to TXD pin
 * NOTE: Only timers of Type A are supported.
 **/
#define TIMER_UART_TYPE A
#define TIMER_UART_IDX  0
#define TIMER_UART_CC   0
#define TIMER_UART CONCAT(TIMER_UART_TYPE, TIMER_UART_IDX)

/* SMCLK freq. */
#define FCPU 			8192000

#define BAUDRATE 		115200

#endif // LIBMSPSOFTUART_H
