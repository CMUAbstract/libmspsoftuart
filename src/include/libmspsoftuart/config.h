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

#define CONFIG_SOFTUART_BAUDRATE 		115200

/* NOTE: must match the configuration in main app
 * TODO: figure out a way to have this defined in one place */
#define CONFIG_SMCLK_FREQ 8000000

#endif // LIBMSPSOFTUART_H
