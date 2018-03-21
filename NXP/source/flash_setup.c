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

#include "FreeRTOS.h"
#include "fsl_debug_console.h"

#include "flash_setup.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

wifi_setup g_wifi_setup;
static wifi_setup yWIFIParameter;

mflash_file_t g_wifi_file[] = {
    { .path = WIFI_CONFIGURATION,
      .flash_addr = MFLASH_FILE_BASEADDR + (2 * MFLASH_FILE_SIZE),
      .max_size = MFLASH_FILE_SIZE
    },
    { 0 }
};

WIFINetworkParams_t pxNetworkParams = {
    .pcSSID = clientcredentialWIFI_SSID,
    .pcPassword = clientcredentialWIFI_PASSWORD,
    .xSecurity = clientcredentialWIFI_SECURITY,
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

void vFlashReadWifiSetup(void)
{
   uint8_t * pucWIFIData = NULL;
   uint32_t ulWIFIDataLength;

   if (pdTRUE == mflash_init(g_wifi_file, 1))
   {
       if( pdFALSE == mflash_read_file( WIFI_CONFIGURATION,
                                       &pucWIFIData,
                                       &ulWIFIDataLength ) )
       {
           strcpy(g_wifi_setup.cWifiSSID, clientcredentialWIFI_SSID);
           strcpy(g_wifi_setup.cWifiPassword, clientcredentialWIFI_PASSWORD);
           g_wifi_setup.eWifiSecurity = clientcredentialWIFI_SECURITY;

           ulWIFIDataLength = sizeof(g_wifi_setup);

           if( pdFALSE == mflash_save_file( WIFI_CONFIGURATION,
                                            (uint8_t *)&g_wifi_setup,
                                            ulWIFIDataLength ) )
           {
              PRINTF("FLASH: write error\n");
           }
       }
       else
       {
           memcpy( (void *)&g_wifi_setup, (void *)pucWIFIData, sizeof(g_wifi_setup) );
           PRINTF("WIFI:\r\n");

           PRINTF("SSID: %s\r\n", g_wifi_setup.cWifiSSID);
           PRINTF("Password: %s\r\n", g_wifi_setup.cWifiPassword);
           PRINTF("Security: %d\r\n", g_wifi_setup.eWifiSecurity);
       }
   }

   strcpy(yWIFIParameter.cWifiSSID, g_wifi_setup.cWifiSSID);
   strcpy(yWIFIParameter.cWifiPassword, g_wifi_setup.cWifiPassword);
   yWIFIParameter.eWifiSecurity = g_wifi_setup.eWifiSecurity;

   pxNetworkParams.pcSSID =     yWIFIParameter.cWifiSSID;
   pxNetworkParams.pcPassword = yWIFIParameter.cWifiPassword;
   pxNetworkParams.xSecurity =  yWIFIParameter.eWifiSecurity;

}
