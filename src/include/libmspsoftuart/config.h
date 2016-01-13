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

/* SMCLK freq. */
#define FCPU 			8192000

#define BAUDRATE 		115200

#endif // LIBMSPSOFTUART_H
