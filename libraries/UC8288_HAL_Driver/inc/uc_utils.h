#ifndef _UTILS_H_
#define _UTILS_H_

#include "uc_event.h"
#include "uc_aon.h"

/* For getting core ID. */
static inline int get_core_id()
{
    return 0;
}

// get number of cores
static inline int get_core_num()
{
    return 0;
}

/**
 * @brief Write to CSR.
 * @param CSR register to write.
 * @param Value to write to CSR register.
 * @return void
 *
 * Function to handle CSR writes.
 *
 */
#define csrw(csr, value)  asm volatile ("csrw\t\t" #csr ", %0" : /* no output */ : "r" (value));

/**
 * @brief Read from CSR.
 * @param void
 * @return 32-bit unsigned int
 *
 * Function to handle CSR reads.
 *
 */
#define csrr(csr, value)  asm volatile ("csrr\t\t%0, " #csr "": "=r" (value));

/**
 * @brief Request to put the core to sleep.
 * @param void
 *
 * Set the core to sleep state and wait for events/interrupt to wake up.
 *
 */
static inline void sleep(void)
{
    SCR = 0x01;
    asm volatile ("nop;nop;wfi");
}

/* Loops/exits simulation */
void exit(int i);

/* end of computation */
void eoc(int i);

// sleep some cycles
void sleep_busy(volatile int);


#endif
