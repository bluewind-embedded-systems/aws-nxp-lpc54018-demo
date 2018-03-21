/*
 * Amazon FreeRTOS Shadow Demo V1.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright 2017 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Logging includes. */
#include "aws_logging_task.h"

/* MQTT include. */
#include "aws_mqtt_agent.h"

/* Required to get the broker address and port. */
#include "aws_clientcredential.h"
#include "queue.h"

/* Required for shadow API's */
#include "aws_shadow.h"
#include "jsmn.h"

#include "adc.h"

/* Maximal count of tokens in parsed JSON */
#define MAX_CNT_TOKENS 32

#define shadowDemoTIMEOUT pdMS_TO_TICKS(30000UL)

#define N_DECIMAL_POINTS_PRECISION (100) // n = 2. Two decimal points.

/* Name of the thing */
#define shadowDemoTHING_NAME clientcredentialIOT_THING_NAME

#define shadowBUFFER_LENGTH 128

/** stack size for task that handles shadow delta and updates
 */
#define shadowDemoUPDATE_TASK_STACK_SIZE ((uint16_t)configMINIMAL_STACK_SIZE * (uint16_t)5)
static char pcUpdateBuffer[shadowBUFFER_LENGTH];
static ShadowClientHandle_t xClientHandle;
QueueHandle_t jsonDeltaQueue = NULL;

/* Connectivity state */
uint16_t ConnState = 0;
uint16_t parsedConnState = 0;

/* Actual state of LED */
uint16_t ledState = 0;
uint16_t parsedLedState = 0;
uint16_t TempInt = 0;
uint16_t TempDecimal = 0 ;

float tempState = 0;

typedef enum
{
    eOPERATIONAL,
    eDISCONNECT,
    eREINIT,
} t_ShadowStates;

t_ShadowStates yShadowState;

typedef struct
{
    char *pcDeltaDocument;
    uint32_t ulDocumentLength;
    void *xBuffer;
} jsonDelta_t;

static void prvShadowMainTask(void *pvParameters);

extern void turnLedOn(void);
extern void turnLedOff(void);

extern float getTemperature(void);

/* Called when there's a difference between "reported" and "desired" in Shadow document. */
static BaseType_t prvDeltaCallback(void *pvUserData,
                                   const char *const pcThingName,
                                   const char *const pcDeltaDocument,
                                   uint32_t ulDocumentLength,
                                   MQTTBufferHandle_t xBuffer)
{
    (void)pvUserData;
    (void)pcThingName;

    /* add the to queue for processing */
    jsonDelta_t jsonDelta;
    jsonDelta.pcDeltaDocument = (char *)pcDeltaDocument;
    jsonDelta.ulDocumentLength = ulDocumentLength;
    jsonDelta.xBuffer = xBuffer;

    if (jsonDeltaQueue == NULL)
    {
        return pdFALSE;
    }

    if (xQueueSend(jsonDeltaQueue, &jsonDelta, (TickType_t)10) != pdPASS)
    {
        configPRINTF(("Fail to send message to jsonDeltaQueue.\r\n"));
        /* return pdFALSE - don't take ownership */
        return pdFALSE;
    }

    /* return pdTRUE - take ownership of the mqtt buffer - must be returned by SHADOW_ReturnMQTTBuffer */
    return pdTRUE;
}

/* Generate initial shadow document */
static uint32_t prvGenerateShadowJSON()
{
    /* Init shadow document with settings desired and reported state of device. */
    return snprintf(pcUpdateBuffer, shadowBUFFER_LENGTH,
                    "{"
                    "\"state\":{"
                    "\"desired\":{"
                    "\"LEDstate\":%d,"
                    "\"Temp\":%d.%d,"
                    "\"ConnState\":%d"
                    "},"
                    "\"reported\":{"
                    "\"LEDstate\":%d,"
                    "\"Temp\":%d.%d,"
                    "\"ConnState\":%d"
                    "}"
                    "}"
                    "}",
                    ledState, TempInt, TempDecimal, ConnState, ledState, TempInt, TempDecimal, ConnState);
}

/* Reports current connectivity state to shadow */
static uint32_t prvReportConnStateShadowJSON()
{
    return snprintf(pcUpdateBuffer, shadowBUFFER_LENGTH,
                    "{"
                    "\"state\":{"
                    "\"reported\":{"
                    "\"ConnState\":%d"
                    "}"
                    "}"
                    "}",
                    ConnState);
}

/* Reports current state to shadow */
static uint32_t prvReportLEDStateShadowJSON()
{
    return snprintf(pcUpdateBuffer, shadowBUFFER_LENGTH,
                    "{"
                    "\"state\":{"
                    "\"reported\":{"
                    "\"LEDstate\":%d"
                    "}"
                    "}"
                    "}",
                    ledState);
}

/* Reports current temperature to shadow */
static uint32_t prvUpdateTempShadowJSON()
{
    return snprintf(pcUpdateBuffer, shadowBUFFER_LENGTH,
                    "{"
                    "\"state\":{"
                    "\"reported\":{"
                    "\"Temp\":%d.%d"
                    "}"
                    "}"
                    "}",
                    TempInt, TempDecimal);
}

/* Initialize Shadow Client. */
static ShadowReturnCode_t prvShadowClientInit(void)
{
    ShadowCreateParams_t sCreateParams;
    MQTTAgentConnectParams_t sConnectParams;
    ShadowCallbackParams_t sCallbackParams;
    ShadowReturnCode_t sReturn;

    sCreateParams.xMQTTClientType = eDedicatedMQTTClient;
    sReturn = SHADOW_ClientCreate(&xClientHandle, &sCreateParams);

    if (sReturn == eShadowSuccess)
    {
        memset(&sConnectParams, 0x00, sizeof(sConnectParams));
        sConnectParams.pcURL = clientcredentialMQTT_BROKER_ENDPOINT;
        sConnectParams.xURLIsIPAddress = pdFALSE;
        sConnectParams.usPort = clientcredentialMQTT_BROKER_PORT;

        sConnectParams.xSecuredConnection = pdTRUE;
        sConnectParams.pcCertificate = NULL;
        sConnectParams.ulCertificateSize = 0;
        sConnectParams.pxCallback = NULL;
        sConnectParams.pvUserData = &xClientHandle;

        sConnectParams.pucClientId = (const uint8_t *)(clientcredentialIOT_THING_NAME);
        sConnectParams.usClientIdLength = (uint16_t)strlen(clientcredentialIOT_THING_NAME);
        sReturn = SHADOW_ClientConnect(xClientHandle, &sConnectParams, shadowDemoTIMEOUT);

        if (sReturn == eShadowSuccess)
        {
            sCallbackParams.pcThingName = shadowDemoTHING_NAME;
            sCallbackParams.xShadowUpdatedCallback = NULL;
            sCallbackParams.xShadowDeletedCallback = NULL;
            sCallbackParams.xShadowDeltaCallback = prvDeltaCallback;
            sReturn = SHADOW_RegisterCallbacks(xClientHandle, &sCallbackParams, shadowDemoTIMEOUT);

            if (sReturn != eShadowSuccess)
            {
                if ( SHADOW_ClientDisconnect( xClientHandle ) == eShadowSuccess )
                {
                    SHADOW_ClientDelete( xClientHandle );
                }
                configPRINTF(("Shadow_RegisterCallbacks unsuccessful, returned %d.\r\n", sReturn));
            }
        }
        else
        {
            SHADOW_ClientDelete( xClientHandle );

            configPRINTF(("Shadow_ClientConnect unsuccessful, returned %d.\r\n", sReturn));
        }
    }
    else
    {
        SHADOW_ClientDelete( xClientHandle );

        configPRINTF(("Shadow_ClientCreate unsuccessful, returned %d.\r\n", sReturn));
    }

    return sReturn;
}

int parseStringValue(char *val, char *json, jsmntok_t *token)
{
    if (token->type == JSMN_STRING)
    {
        int len = token->end - token->start;
        memcpy(val, json + token->start, len);
        val[len] = '\0';
        return 0;
    }
    return -1;
}

int parseUInt16Value(uint16_t *val, char *json, jsmntok_t *token)
{
    if (token->type == JSMN_PRIMITIVE)
    {
        if (sscanf(json + token->start, "%hu", val) == 1)
        {
            return 0;
        }
    }
    return -1;
}

int compareString(char *json, jsmntok_t *token, char *string)
{
    if (token->type == JSMN_STRING)
    {
        int len = strlen(string);
        int tokLen = token->end - token->start;
        if (len > tokLen)
        {
            len = tokLen;
        }
        return strncmp(string, json + token->start, len);
    }
    return -1;
}

int findTokenIndex(char *json, jsmntok_t *tokens, uint32_t length, char *name)
{
    int i;
    for (i = 0; i < length; i++)
    {
        if (compareString(json, &tokens[i], name) == 0)
        {
            return i;
        }
    }
    return -1;
}

/* Process shadow delta JSON */
void processShadowDeltaJSON(char *json, uint32_t jsonLength)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    jsmntok_t tokens[MAX_CNT_TOKENS];

    int tokenCnt = 0;
    tokenCnt = jsmn_parse(&parser, json, jsonLength, tokens, MAX_CNT_TOKENS);
    /* the token with state of device is at 6th positin in this delta JSON:
     * {"version":229,"timestamp":1510062270,"state":{"LEDstate":1},"metadata":{"LEDstate":{"timestamp":1510062270}}} */
    if (tokenCnt < 6)
    {
        return;
    }

    /* find index of token "state" */
    int stateTokenIdx;
    stateTokenIdx = findTokenIndex(json, tokens, tokenCnt, "state");
    if (stateTokenIdx < 0)
    {
        return;
    }

    int i = stateTokenIdx + 1;

    /* end position of "state" object in JSON */
    int stateTokenEnd = tokens[i].end;

    char key[20] = {0};
    int err = 0;

    while (i < tokenCnt)
    {
        if (tokens[i].end > stateTokenEnd)
        {
            break;
        }

        err = parseStringValue(key, json, &tokens[i++]);
        if (err == 0)
        {
            if (strstr(key, "LEDstate"))
            {
                /* found "LEDstate" keyword, parse value of next token */
                err = parseUInt16Value(&parsedLedState, json, &tokens[i]);
            }
            if (strstr(key, "ConnState"))
            {
                /* found "ConnState" keyword, parse value of next token */
                err = parseUInt16Value(&parsedConnState, json, &tokens[i]);
            }
            i++;
        }
    }
}

void prvShadowMainTask(void *pvParameters)
{
    (void)pvParameters;
    uint8_t zRet;
    ShadowReturnCode_t ret = eShadowSuccess;

    jsonDeltaQueue = xQueueCreate(8, sizeof(jsonDelta_t));
    if (jsonDeltaQueue == NULL)
    {
        configPRINTF(("Failed to create jsonDeltaQueue queue.\r\n"));
        vTaskDelete(NULL);
    }

    if (prvShadowClientInit() != eShadowSuccess)
    {
        configPRINTF(("Failed to initialize, stopping demo.\r\n"));
        vTaskDelete(NULL);
    }

    ShadowOperationParams_t xOperationParams;
    xOperationParams.pcThingName = shadowDemoTHING_NAME;
    xOperationParams.xQoS = eMQTTQoS0;
    xOperationParams.pcData = NULL;
    /* subscribe to the accepted and rejected topics */
    xOperationParams.ucKeepSubscriptions = pdTRUE;

    /* Delete the device shadow before initial update */
    if (SHADOW_Delete(xClientHandle, &xOperationParams, shadowDemoTIMEOUT) != eShadowSuccess)
    {
        configPRINTF(("Failed to delete device shadow, stopping demo.\r\n"));
        vTaskDelete(NULL);
    }

    xOperationParams.pcData = pcUpdateBuffer;
    xOperationParams.ulDataLength = prvGenerateShadowJSON();

    /* create initial shadow document for the thing */
    if (SHADOW_Update(xClientHandle, &xOperationParams, shadowDemoTIMEOUT) != eShadowSuccess)
    {
        configPRINTF(("Failed to update device shadow, stopping demo.\r\n"));
        vTaskDelete(NULL);
    }

    configPRINTF(("AWS LED Demo initialized.\r\n"));
    configPRINTF(("Use mobile application to turn on/off the LED.\r\n"));

    jsonDelta_t jsonDelta;

    yShadowState = eOPERATIONAL;

    for (;;)
    {
        switch ( yShadowState )
        {
            case eOPERATIONAL:
                /* process delta shadow JSON received in prvDeltaCallback() every 5 minutes */
                if ( xQueueReceive( jsonDeltaQueue, &jsonDelta, ( 300 * ( 1000 * portTICK_PERIOD_MS ) ) ) == pdTRUE )
                {
                    parsedConnState = ConnState;
                    parsedLedState = ledState;
                    /* process item from queue */
                    processShadowDeltaJSON( jsonDelta.pcDeltaDocument, jsonDelta.ulDocumentLength );
                    /* process connectivity check */
                    if ( parsedConnState != ConnState )
                    {
                        ConnState = parsedConnState;

                        /* update device shadow */
                        xOperationParams.ulDataLength = prvReportConnStateShadowJSON();
                        SHADOW_Update( xClientHandle, &xOperationParams, shadowDemoTIMEOUT );
                    }
                    /* process LED request*/
                    if ( parsedLedState != ledState )
                    {
                        ledState = parsedLedState;
                        if ( ledState == 1 )
                        {
                            configPRINTF( ("Turn LED on\r\n") );
                            /* turn LED on */
                            turnLedOn();
                        }
                        else
                        {
                            configPRINTF( ("Turn LED off\r\n") );
                            /* turn LED off */
                            turnLedOff();
                        }

                        /* update device shadow */
                        xOperationParams.ulDataLength = prvReportLEDStateShadowJSON();
                        SHADOW_Update( xClientHandle, &xOperationParams, shadowDemoTIMEOUT );
                    }
                    /* return mqtt buffer */
                    SHADOW_ReturnMQTTBuffer( xClientHandle, jsonDelta.xBuffer );
                }
                else
                {
                    zRet = Adc_getTemperature( &tempState );

                    if ( zRet )
                    {
                        TempInt = (int)tempState;
                        TempDecimal = ( (int)( tempState * N_DECIMAL_POINTS_PRECISION ) % N_DECIMAL_POINTS_PRECISION );

                        configPRINTF( ("TempValue        = %d.%d\r\n", (uint32_t)TempInt, (uint32_t)TempDecimal ) );

                        /* update device shadow */
                        xOperationParams.ulDataLength = prvUpdateTempShadowJSON();

                        ret = SHADOW_Update( xClientHandle, &xOperationParams, shadowDemoTIMEOUT );
                        if ( eShadowSuccess != ret )
                        {
                            yShadowState = eDISCONNECT;
                        }
                    }
                    else
                    {
                        configPRINTF( ("Failed to get adc tempreture.\r\n") );
                    }
                }

            break;

            case eDISCONNECT:
                if ( SHADOW_ClientDisconnect( xClientHandle ) == eShadowSuccess )
                {
                    SHADOW_ClientDelete( xClientHandle );

                    yShadowState = eREINIT;
                }
                else
                {
                    configPRINTF( ("Failed reinitialize, stopping demo.\r\n") );
                }
            break;

            case eREINIT:
                if ( prvShadowClientInit() == eShadowSuccess )
                {
                    yShadowState = eOPERATIONAL;
                }
                else
                {
                    configPRINTF( ("Failed reinitialize, stopping demo.\r\n") );
                }
            break;
        }
    }
}

void vStartLedDemoTask(void)
{
    (void)xTaskCreate(prvShadowMainTask, "AWS-LED", shadowDemoUPDATE_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
}
