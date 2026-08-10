/**
 * File: main.c
 * Lab 3: Basic I/O with interrupts and timed LED states
 * Authors: Andrea Taguinod and Shahad
 * Date: 10/15/2025
 */

#include "msp.h"
#include <stdint.h>

// ====== Function Prototypes ======
void init_stuff(void);
void delay(uint32_t t);

// ====== Global Variables ======
volatile int currentLED = 0;       // 0: RED LED (P1.0), 1: RGB LED (P2.0–P2.2)
volatile int rgbColor = 0;         // RGB color index (0–7)
volatile int autoCycleMode = 0;    // 0 = off, 1 = running auto LED behavior

int main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; // Stop watchdog timer

    init_stuff();

    // Configure interrupts for P1.1 and P1.4
    P1->IES |= (BIT1 | BIT4);      // Trigger on falling edge
    P1->IFG &= ~(BIT1 | BIT4);     // Clear interrupt flags
    P1->IE  |= (BIT1 | BIT4);      // Enable interrupts

    // NVIC configuration
    NVIC_SetPriority(PORT1_IRQn, 2);
    NVIC_ClearPendingIRQ(PORT1_IRQn);
    NVIC_EnableIRQ(PORT1_IRQn);

    __enable_irq(); // Global interrupt enable

    while (1)
    {
        if (autoCycleMode)
        {
            if (currentLED == 0)
            {
                // RED LED: Blink every ~4 seconds
                P1->OUT ^= BIT0;
                delay(2000);
            }
            else
            {
                // RGB LED: Cycle through 8 color combinations every ~8 seconds
                rgbColor = (rgbColor + 1) % 8;
                P2->OUT = (P2->OUT & ~0x07) | rgbColor;
                delay(2500);
            }
        }
        else
        {
            __sleep(); // Low-power wait mode when idle
            __no_operation();
        }
    }
}

// ========== Initialization ==========
void init_stuff(void)
{
    // -------- Buttons (P1.1 and P1.4) --------
    P1->SEL0 &= ~(BIT1 | BIT4);
    P1->SEL1 &= ~(BIT1 | BIT4);
    P1->DIR  &= ~(BIT1 | BIT4); // Set as input
    P1->REN  |=  (BIT1 | BIT4); // Enable pull-up/down resistors
    P1->OUT  |=  (BIT1 | BIT4); // Pull-up resistors

    // -------- RED LED (P1.0) --------
    P1->SEL0 &= ~BIT0;
    P1->SEL1 &= ~BIT0;
    P1->DIR  |=  BIT0;  // Output
    P1->OUT  &= ~BIT0;  // OFF initially

    // -------- RGB LED (P2.0 = R, P2.1 = G, P2.2 = B) --------
    P2->SEL0 &= ~(BIT0 | BIT1 | BIT2);
    P2->SEL1 &= ~(BIT0 | BIT1 | BIT2);
    P2->DIR  |=  (BIT0 | BIT1 | BIT2);  // Output
    P2->OUT  &= ~(BIT0 | BIT1 | BIT2);  // All OFF initially
}

// ========== Interrupt Handler ==========
void PORT1_IRQHandler(void)
{
    // ===== Button 1 (P1.1): Toggle RED/RGB LED selection =====
    if (P1->IFG & BIT1)
    {
        P1->IFG &= ~BIT1;           // Clear interrupt flag
        currentLED = !currentLED;   // Toggle selected LED
        autoCycleMode = 0;          // Exit auto-cycle mode
        delay(200);                 // Debounce delay
    }

    // ===== Button 2 (P1.4): Enter auto-cycle mode =====
    if (P1->IFG & BIT4)
    {
        P1->IFG &= ~BIT4;           // Clear interrupt flag
        delay(200);                 // Debounce delay

        autoCycleMode = 1;          // Enter auto-mode

        if (currentLED == 0)
        {
            // RED LED: Turn ON to start blink
            P1->OUT |= BIT0;
        }
        else
        {
            // RGB LED: Start from current color
            P2->OUT = (P2->OUT & ~0x07) | rgbColor;
        }
    }
}

// ========== Delay Function ==========
void delay(uint32_t t)
{
    volatile uint32_t i;
    for (i = 0; i < t * 1000; i++); // Roughly t milliseconds (not accurate)
}
