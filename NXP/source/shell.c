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

/* SDK Included Files */
#include "board.h"
#include "fsl_shell.h"
#include "fsl_debug_console.h"

#include "pin_mux.h"

/* Amazon FreeRTOS Demo Includes */
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mflash_drv.h"
#include "mflash_file.h"

#include "shell.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define shell_TASK_STACK_SIZE ((uint16_t)configMINIMAL_STACK_SIZE * (uint16_t)5)
#define LED_NUMBERS 1U
#define LED_3_INIT() LED3_INIT(LOGIC_LED_OFF)
#define LED_3_ON() LED3_ON()
#define LED_3_OFF() LED3_OFF()
#define SHELL_Printf PRINTF

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* SHELL user send data callback */
void SHELL_SendDataCallback(uint8_t *buf, uint32_t len);

/* SHELL user receive data callback */
void SHELL_ReceiveDataCallback(uint8_t *buf, uint32_t len);

static int32_t LedControl(p_shell_context_t context, int32_t argc, char **argv);
static int32_t wifissid(p_shell_context_t context, int32_t argc, char **argv);
static int32_t wifipassword(p_shell_context_t context, int32_t argc, char **argv);
static int32_t wifisecurity(p_shell_context_t context, int32_t argc, char **argv);
static int32_t readwifi(p_shell_context_t context, int32_t argc, char **argv);
static int32_t writewifi(p_shell_context_t context, int32_t argc, char **argv);

static void prvShellMainTask(void *pvParameters);

/*******************************************************************************
 * Variables
 ******************************************************************************/

static const shell_command_context_t xLedCommand =   {"led",
                                                      "\r\n\"led arg1 arg2\":\r\n Usage:\r\n    arg1: 1|2|3|4...         "
                                                      "   Led index\r\n    arg2: on|off                Led status\r\n",
                                                      LedControl, 2};

static const shell_command_context_t xwifissid =     {"ssid",
                                                      "\r\n\"ssid arg1\":\r\n Usage:\r\n    arg1: ssid\r\n",
                                                      wifissid, 1};

static const shell_command_context_t xwifipassword = {"password",
                                                      "\r\n\"password arg1\":\r\n Usage:\r\n    arg1: password\r\n",
                                                      wifipassword, 1};

static const shell_command_context_t xwifisecurity = {"security",
                                                      "\r\n\"security arg1\":\r\n Usage:\r\n    arg1: 0-eWiFiSecurityOpen 1-eWiFiSecurityWPE 2-eWiFiSecurityWPA 3-eWiFiSecurityWPA2 \r\n",
                                                      wifisecurity, 1};

static const shell_command_context_t xreadwifi = {"readwifi",
                                                  "\r\n\"readwifi\"\r\n",
                                                  readwifi, 0};

static const shell_command_context_t xwritewifi = {"writewifi",
                                                  "\r\n\"writewifi\"\r\n",
                                                  writewifi, 0};

shell_context_struct user_context;

/*******************************************************************************
 * Code
 ******************************************************************************/

void SHELL_SendDataCallback(uint8_t *buf, uint32_t len)
{
    while (len--)
    {
        PUTCHAR(*(buf++));
    }
}

void SHELL_ReceiveDataCallback(uint8_t *buf, uint32_t len)
{
    while (len--)
    {
        *(buf++) = GETCHAR();
    }
}

static int32_t LedControl(p_shell_context_t context, int32_t argc, char **argv)
{
    int32_t kLedIndex = ((int32_t)atoi(argv[1]));
    char *kLedCommand = argv[2];

    /* Check second argument to control led */
    switch (kLedIndex)
    {
#if defined(LED_NUMBERS)
        case 3:
            if (strcmp(kLedCommand, "on") == 0)
            {
                LED_3_ON();
            }
            else if (strcmp(kLedCommand, "off") == 0)
            {
                LED_3_OFF();
            }
            else
            {
                SHELL_Printf("Control conmmand is wrong!\r\n");
            }
            break;
#endif
        default:
            SHELL_Printf("LED index is wrong\r\n");
            break;
    }
    return 0;
}

static int32_t wifissid(p_shell_context_t context, int32_t argc, char **argv)
{
    int32_t zRet = 0;

    if (argv[1] != NULL)
    {
        strcpy( g_wifi_setup.cWifiSSID, argv[1] );
        SHELL_Printf("%s\r\n", g_wifi_setup.cWifiSSID);

        zRet = 1;
    }

    return zRet;
}

static int32_t wifipassword(p_shell_context_t context, int32_t argc, char **argv)
{
    int32_t zRet = 0;

    if (argv[1] != NULL)
    {
        strcpy( g_wifi_setup.cWifiPassword, argv[1] );
        SHELL_Printf("%s\r\n", g_wifi_setup.cWifiPassword);

        zRet = 1;
    }

    return zRet;
}

static int32_t wifisecurity(p_shell_context_t context, int32_t argc, char **argv)
{
    int32_t zRet = 0;
    int32_t security = 0;

    if (argv[1] != NULL)
    {
        security = ((int32_t)atoi(argv[1]));
        switch(security)
        {
            case 0:
                g_wifi_setup.eWifiSecurity = eWiFiSecurityOpen;
            break;

            case 1:
                g_wifi_setup.eWifiSecurity = eWiFiSecurityWEP;
            break;

            case 2:
                g_wifi_setup.eWifiSecurity = eWiFiSecurityWPA;
            break;

            case 3:
                g_wifi_setup.eWifiSecurity = eWiFiSecurityWPA2;
            break;

        }

        SHELL_Printf("\r\n", argv[1],"\r\n");

        zRet = 1;
    }

    return zRet;
}

static int32_t readwifi(p_shell_context_t context, int32_t argc, char **argv)
{
    int32_t zRet = 0;

    uint8_t * pucWIFIData = NULL;
    uint32_t ulWIFIDataLength;

    if (pdTRUE == mflash_init(g_wifi_file, 0))
    {
        if ( mflash_read_file( WIFI_CONFIGURATION, &pucWIFIData, &ulWIFIDataLength ) )
        {
            memcpy( (void *)&g_wifi_setup, (void *)pucWIFIData, sizeof(g_wifi_setup) );
            SHELL_Printf("WIFI:\r\n");
            SHELL_Printf("SSID: %s\r\n", g_wifi_setup.cWifiSSID);
            SHELL_Printf("Password: %s\r\n", g_wifi_setup.cWifiPassword);
            SHELL_Printf("Security: %d\r\n", g_wifi_setup.eWifiSecurity);

            zRet = 1;
        }
    }

    return zRet;
}

static int32_t writewifi(p_shell_context_t context, int32_t argc, char **argv)
{
    bool zRet = pdFALSE;

    uint32_t ulWIFIDataLength;

    ulWIFIDataLength = sizeof(g_wifi_setup);

    if (pdTRUE == mflash_init(g_wifi_file, 0))
    {
        if( pdFALSE == mflash_save_file( WIFI_CONFIGURATION,
                                       (uint8_t *)&g_wifi_setup,
                                       ulWIFIDataLength ) )
        {
            SHELL_Printf("FLASH: write error\r\n");

            zRet = pdTRUE;
        }
    }

    return zRet;
}

static void prvShellMainTask(void *pvParameters)
{
    SHELL_Main(&user_context);
}

void vInitShell(void)
{
    /* Init SHELL */
    SHELL_Init(&user_context, SHELL_SendDataCallback, SHELL_ReceiveDataCallback, SHELL_Printf, "SHELL>> ");

    /* Add new command to commands list */
    SHELL_RegisterCommand(&xLedCommand);
    SHELL_RegisterCommand(&xwifissid);
    SHELL_RegisterCommand(&xwifipassword);
    SHELL_RegisterCommand(&xwifisecurity);
    SHELL_RegisterCommand(&xreadwifi);
    SHELL_RegisterCommand(&xwritewifi);
}

void vStartShellTask(void)
{
    (void)xTaskCreate(prvShellMainTask, "AWS-SHELL", shell_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
}

