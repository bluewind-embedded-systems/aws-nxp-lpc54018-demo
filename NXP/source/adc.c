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

#include "fsl_adc.h"
#include "fsl_power.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define TICKRATE_HZ (1000) /* 1000 ticks per second */

#define ADC_BASE ADC0
#define ADC_SAMPLE_CHANNEL_NUMBER 0U
#define ADC_IRQ_ID ADC0_SEQA_IRQn
#define ADC_IRQ_HANDLER_FUNC ADC0_SEQA_IRQHandler


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static uint8_t Adc_getValue(uint32_t* zValue);
static void ADC_Configuration(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static adc_result_info_t gAdcResultInfoStruct;
adc_result_info_t *volatile gAdcResultInfoPtr = &gAdcResultInfoStruct;
volatile bool gAdcConvSeqAIntFlag;
uint8_t ADC_Offset = 16;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief ISR for ADC conversion sequence A done.
 */
void ADC_IRQ_HANDLER_FUNC(void)
{
    if (kADC_ConvSeqAInterruptFlag == (kADC_ConvSeqAInterruptFlag & ADC_GetStatusFlags(ADC_BASE)))
    {
        ADC_GetChannelConversionResult(ADC_BASE, ADC_SAMPLE_CHANNEL_NUMBER, gAdcResultInfoPtr);
        ADC_ClearStatusFlags(ADC_BASE, kADC_ConvSeqAInterruptFlag);

        gAdcConvSeqAIntFlag = true;
    }
}

/*
 * Configure the ADC as normal converter in polling mode.
 */
void ADC_Configuration(void)
{
    adc_config_t adcConfigStruct;
    adc_conv_seq_config_t adcConvSeqConfigStruct;

    /* Configure the converter. */
    adcConfigStruct.clockMode = kADC_ClockSynchronousMode; /* Using sync clock source. */
    adcConfigStruct.clockDividerNumber = 1;                /* The divider for sync clock is 2. */
    adcConfigStruct.resolution = kADC_Resolution12bit;
    adcConfigStruct.enableBypassCalibration = false;
    adcConfigStruct.sampleTimeNumber = 0U;
    ADC_Init(ADC_BASE, &adcConfigStruct);

    /* Use the temperature sensor input to channel 0. */
    ADC_EnableTemperatureSensor(ADC_BASE, true);

    /* Enable channel DEMO_ADC_SAMPLE_CHANNEL_NUMBER's conversion in Sequence A. */
    adcConvSeqConfigStruct.channelMask =
        (1U << ADC_SAMPLE_CHANNEL_NUMBER); /* Includes channel DEMO_ADC_SAMPLE_CHANNEL_NUMBER. */
    adcConvSeqConfigStruct.triggerMask = 0U;
    adcConvSeqConfigStruct.triggerPolarity = kADC_TriggerPolarityPositiveEdge;
    adcConvSeqConfigStruct.enableSingleStep = false;
    adcConvSeqConfigStruct.enableSyncBypass = false;
    adcConvSeqConfigStruct.interruptMode = kADC_InterruptForEachSequence;
    ADC_SetConvSeqAConfig(ADC_BASE, &adcConvSeqConfigStruct);
    ADC_EnableConvSeqA(ADC_BASE, true); /* Enable the conversion sequence A. */
}

static void ADC_ClockPower_Configuration(void)
{
    POWER_DisablePD(kPDRUNCFG_PD_VDDA);    /* Power on VDDA. */
    POWER_DisablePD(kPDRUNCFG_PD_ADC0);    /* Power on the ADC converter. */
    POWER_DisablePD(kPDRUNCFG_PD_VD2_ANA); /* Power on the analog power supply. */
    POWER_DisablePD(kPDRUNCFG_PD_VREFP);   /* Power on the reference voltage source. */
    POWER_DisablePD(kPDRUNCFG_PD_TS);      /* Power on the temperature sensor. */

    CLOCK_EnableClock(kCLOCK_Adc0);
}

static uint8_t Adc_getValue(uint32_t* zValue)
{
    uint8_t zRet = false;
    uint32_t zCntMaxWait = 0xFFFFFF;

    gAdcConvSeqAIntFlag = false;
    ADC_DoSoftwareTriggerConvSeqA(ADC_BASE);

    while ( (!gAdcConvSeqAIntFlag ) && (zCntMaxWait--) )
    {
    }

    if( gAdcConvSeqAIntFlag && zCntMaxWait )
    {
        *zValue = gAdcResultInfoStruct.result;

        zRet = true;
    }

    return zRet;
}

uint8_t Adc_getTemperature(float* zTemperature)
{
    uint8_t zRet = true;
    uint8_t zCnt;
    uint32_t zValue;

    float ADCVoltage = 0;
    float TempValue = 0;

    /* Calculate Average of 50 ADC samples */
    for (zCnt = 0; zCnt < 50; zCnt++)
    {
        if( Adc_getValue(&zValue) )
        {
            /* Convert ADC Reading to Voltage. Assuming 3.3V is supply voltage
             * For 12-bit ADC formula is Voltage(in volts) = [ (Decimal Value/4096) * Vsupply ]   */
            ADCVoltage = (((float)zValue / 4096) * 3.3);

            /* Convert Voltage to temperature value. Temp (C) = [ (V (in mV) - LLS intercept at 0C)/ (LLS slope) ]
             * values from LPC540xx Datasheet: LLS slope = -2.04 mV/C ; LLS intercept at 0C = 584 mV   */
            TempValue += (((ADCVoltage * 1000) - 584) / (-2.04));
        }
        else
        {
            zRet = false;
            break;
        }
    }

    if(zRet)
    {
        /* Average of 50 samples*/
        *zTemperature = (TempValue / 50) - ADC_Offset;
    }

    return zRet;
}

void vAdcInit(void)
{
    ADC_ClockPower_Configuration();

    /* ADC Calibration. */
    ADC_DoSelfCalibration(ADC_BASE);

    /* Configure the ADC and Temperature Sensor */
    ADC_Configuration();

    /* Enable the interrupt. */
    /* Enable the interrupt the for sequence A done. */
    ADC_EnableInterrupts(ADC_BASE, kADC_ConvSeqAInterruptEnable);

    NVIC_EnableIRQ(ADC_IRQ_ID);

    ADC_DoSoftwareTriggerConvSeqA(ADC_BASE);
}
