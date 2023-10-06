/* Board Support Package */
#include "TM4C123GH6PM.h"
#include "bsp.h"
#include "4dig7seg_driver.h"

extern const uint8_t numbers[];

void io_init_tmp36() {
    // TIMER0 init with ADC trigger
    SYSCTL->RCGCTIMER = 1u;  // clock enable to TIMER0
    TIMER0->CTL |= 0;  // TAEN disable timer A during config
    TIMER0->CFG |= 0x04u;  // 16-bit
    TIMER0->TAMR |= 0x2u;  // TAMR periodic mode
    TIMER0->CTL |= P5;  // TAOTE trigger ADC event on timeout
    TIMER0->TAV = 0;  // ensure timers 0
    TIMER0->TAILR = 0xFFFFu;
    TIMER0->TAPR = 0xFFu;  // max period

    // PE3 for AIN0
    SYSCTL->RCGCADC |= 1u;  // enable clock to ADC
    GPIOE_AHB->AFSEL |= P3;  // alternate func for PE3
    GPIOE_AHB->DEN &= ~P3;  // ensure digital disabled
    GPIOE_AHB->AMSEL |= P3;  // enable analog mode

    // configure sample sequencer
    ADC0->ACTSS &= ~P0;  // disable ASEN0
    ADC0->EMUX &= ~(0xFu << 0);  // clear EM0 MUX
    ADC0->EMUX |= (0x5u << 0);  // use timer to trigger sequence
    ADC0->SSMUX0 = 0;  // sample AIN0 for all 8 samples in sequence
    ADC0->SSCTL0 = 0x6u << 0;  // set 1st sample as end w/interrupt
    ADC0->SAC = 0x4u;  // average 16 samples per FIFO entry
    ADC0->IM |= P0;  // sample seq 0 interrupt
    ADC0->ACTSS |= P0;  // enable sequencer, timer will trigger
    // ADC0->SSOP0 leave as 0 to send res to FIFO, not to comparator
    // read from ADC0->SSFIFO0 x times

    // SS0 interrupt#: 14
    NVIC->ISER[0] |= (1u << 14);
}

void start_timers() {
    TIMER0->CTL |= P0;
}

void ADCSeq0_IRQHandler(void) {
    ADC0->ISC |= P0;  // clear interrupt status
    // calculate and set display value
    // 750 mV at 25 C, 10mV/deg C (500 mV at 0 C)
    // conversion results in value that doesn't make sense, too high
    int32_t digit = (int) (((ADC0->SSFIFO0 * 3.3 * 1000 / 4096) - 500) / 10);
    uint8_t digits[NUM_DIGITS];
    for (int i = 0; i < NUM_DIGITS; i++) {
        digits[i] = digit % 10;
        digit = digit / 10;
    }

    // TODO: allow calculated number to be negative
    // don't display leading 0's
    uint8_t to_display[NUM_DIGITS] = {0};
    to_display[0] = numbers[digits[0]];  // always display least sig digit
    if (digits[NUM_DIGITS-1]) {
        to_display[NUM_DIGITS-1] = numbers[digits[NUM_DIGITS-1]];
    }
    for (int i = NUM_DIGITS - 2; i > 0; i--) {
        if (to_display[i+1] || digits[i]) {
            to_display[i] = numbers[digits[i]];
        } else {
            to_display[i] = 0;
        }
    }
    set_display(to_display);
}

__attribute__((naked)) void assert_failed (char const *file, int line) {
    /* TBD: damage control */
    NVIC_SystemReset(); /* reset the system */
}
