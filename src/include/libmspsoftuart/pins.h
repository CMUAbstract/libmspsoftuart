#ifndef LIBMSPSOFTUART_PINS_H
#define LIBMSPSOFTUART_PINS_H

/**
 * TXD pin
 * NOTE: must be the output pin of TIMER_SOFTUART_CC register of TIMER_SOFTUART timer
 */
#define PORT_SOFTUART_TXD 3
#define PIN_SOFTUART_TXD  1

/* RXD pin */
#define PORT_SOFTUART_RXD 3
#define PIN_SOFTUART_RXD  2

/* The output of this CC register and timer must be routed to TXD pin
 * NOTE: Only timers of Type A are supported.
 * NOTE: Only CC=0 is supported (because int vector differs between 0 and 1-7)
 **/
#define TIMER_SOFTUART_TYPE A
#define TIMER_SOFTUART_IDX  0
#define TIMER_SOFTUART_CC   0
#define TIMER_SOFTUART CONCAT(TIMER_SOFTUART_TYPE, TIMER_SOFTUART_IDX)

#endif // LIBMSPSOFTUART_PINS_H
