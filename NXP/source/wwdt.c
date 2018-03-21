/*
 * The Clear BSD License
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_wwdt.h"
#include "fsl_power.h"

#include "pin_mux.h"
#include <stdbool.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_WDT_IRQn WDT_BOD_IRQn
#define WDT_CLK_FREQ CLOCK_GetFreq(kClock_WdtOsc)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static volatile uint8_t wdt_update_count;

/*******************************************************************************
 * Code
 ******************************************************************************/

void WDT_BOD_IRQHandler( void )
{
    uint32_t wdtStatus = WWDT_GetStatusFlags( WWDT );

    /* The chip will reset before this happens */
    if ( wdtStatus & kWWDT_TimeoutFlag )
    {
        /* A watchdog feed didn't occur prior to window timeout */
        /* Stop WDT */
        WWDT_Disable( WWDT );
        WWDT_ClearStatusFlags( WWDT, kWWDT_TimeoutFlag );
        /* Needs restart */
        WWDT_Enable( WWDT );
    }

    /* Handle warning interrupt */
    if ( wdtStatus & kWWDT_WarningFlag )
    {
        /* A watchdog feed didn't occur prior to warning timeout */
        WWDT_ClearStatusFlags( WWDT, kWWDT_WarningFlag );
        if ( wdt_update_count < 5 )
        {
            /* Feed only for the first 5 warnings then allow for a WDT reset to occur */
            WWDT_Refresh( WWDT );
            wdt_update_count++;
        }
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
     exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/*!
 * @brief Main function
 */
void vWatchDogInit( void )
{
    wwdt_config_t config;
    uint32_t wdtFreq;

#if !defined (FSL_FEATURE_WWDT_HAS_NO_PDCFG) || (!FSL_FEATURE_WWDT_HAS_NO_PDCFG)
    POWER_DisablePD( kPDRUNCFG_PD_WDT_OSC );
#endif

    /* The WDT divides the input frequency into it by 4 */
    wdtFreq = WDT_CLK_FREQ / 4;

    NVIC_EnableIRQ( APP_WDT_IRQn );

    WWDT_GetDefaultConfig( &config );

    /* Check if reset is due to Watchdog */
    if ( WWDT_GetStatusFlags( WWDT ) & kWWDT_TimeoutFlag )
    {
        PRINTF( "Watchdog reset occurred\r\n" );
    }

    /*
     * Set watchdog feed time constant to approximately 2s
     * Set watchdog warning time to 512 ticks after feed time constant
     * Set watchdog window time to 1s
     */
    config.timeoutValue = wdtFreq * 80;
    //config.warningValue = 512;
    //config.windowValue = wdtFreq * 1;
    /* Configure WWDT to reset on timeout */
    config.enableWatchdogReset = true;

    /* wdog refresh test in window mode */
    PRINTF( "\r\n--- Window mode refresh test start---\r\n" );
    WWDT_Init( WWDT, &config );

    /* First feed will start the watchdog */
    WWDT_Refresh( WWDT );
}

void vWatchDogRefresh( void )
{
    WWDT_Refresh( WWDT );
}
