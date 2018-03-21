Overview
========
This demo demonstrates how the board NXP OM40007|LPC54018 IoT Module can be controlled by Amazon Developer Services.  
We are going to set everything up and running to operate:
	- The user speaks to the Echo device to activate an Alexa skill;
	- The Alexa Skill triggers an AWS-Lambda function which sends a message to the AWS-IoT;
	- The NXP IoT Module is registered on the AWS-IoT and receives the message to perform the targeted action (turn on/off LED or read temperature);
	- The NXP Module sends a feedback message to the AWS-IoT platform; 
	- After retrieving the message the AWS-Lambda function sends it back to the Alexa platform;
	- This makes the Echo device respond to user: "The LED is on/off !" or "The actual temperature is x degrees ".
In this demo is also possible to drive the LED of the board with an NXP android example application.

Toolchain supported
===================
- MCUXpresso10.1.1

Hardware requirements
=====================
- Micro USB cable
- LPC54018-IoT-Module
- Personal Computer

Board settings
==============
No special settings are required.

Prepare the Demo
================
Before running the demo it is need to configure AWS IoT Console and update some of project files:

1.  Import the SDK SDK_2.3.0_LPC54018-IoT-Module (download from https://mcuxpresso.nxp.com/en/welcome) into the MCUXpresso IDE, follow the https://www.nxp.com/docs/en/user-guide/MCUXSDKGSUG.pdf getting started guide.

2.  Create AWS Account: https://console.aws.amazon.com/console/home (see aws-nxp readme.md, amazon project for configuration of amazon services tool)

3.  Configure device in the AWS IoT Console base on this guide: https://docs.aws.amazon.com/iot/latest/developerguide/iot-sdk-setup.html

    Make note of example's "Thing name" and "REST API endpoint". These strings need to be set in the "aws_clientcredential.h".

	Example:
		static const char clientcredentialMQTT_BROKER_ENDPOINT[] = "abcdefgh123456.iot.us-west-2.amazonaws.com";
		#define clientcredentialIOT_THING_NAME "MyExample"

    In the next step you will get the "device certificate" and the "primary key". Each of the certificates needs to be opened in text editor and its content copied into the "aws_clientcredential_keys.h".
    Or you can use the CertificateConfigurator.html (mcu-sdk-2.0\rtos\amazon-freertos\demos\common\devmode_key_provisioning\CertificateConfigurationTool) to generate the "aws_clientcredential_keys.h".

    Example:
        static const char clientcredentialCLIENT_CERTIFICATE_PEM[] = "";

        Needs to be changed to:

        static const char clientcredentialCLIENT_CERTIFICATE_PEM[] =
            "-----BEGIN CERTIFICATE-----\n"
            "MIIDWTCCAkGgAwIBAgIUPwbiJBIJhO6eF498l1GZ8siO/K0wDQYJKoZIhvcNAQEL\n"
            .
            .
            "KByzyTutxTeI9UKcIPFxK40s4qF50a40/6UFxrGueW+TzZ4iubWzP7eG+47r\n"
            "-----END CERTIFICATE-----\n";

    In the same way update the private key array.

4.  This demo needs WiFi network with internet access.
    Update these macros in "aws_clientcredential.h" based on your WiFi network configuration:
        #define clientcredentialWIFI_SSID       "Paste WiFi SSID here."
        #define clientcredentialWIFI_PASSWORD   "Paste WiFi password here."

5.  Open example's project and build it.

6.  Connect a USB cable between the PC host and the J8 port on the target board.

9.  Download the program to the target board with a J-Link debugger or a LPC-LINK 2.

10.  Either press the reset button on your board or launch the debugger in your IDE to begin running the demo.

7.  Open a serial terminal on PC for serial device with these settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
	
11. Use the shell of the serial terminal to config the WIFI parameters, list of all command here:
	- "help": Lists all the registered commands
	- "exit": Exit program
	- "led arg1 arg2": Power on/off a LED (OM400007 LED on board = LED 3)
	  Usage:
		arg1: 1|2|3|4...            Led index
		arg2: on|off                Led status
	- "ssid arg1": Change the WIFI SSID
	  Usage:
		arg1: ssid
	- "password arg1": Change the WIFI PASSWORD
	  Usage:
		arg1: password
	- "security arg1": Change the WIFI SECURITY
	  Usage:
		arg1: 0-eWiFiSecurityOpen 1-eWiFiSecurityWPE 2-eWiFiSecurityWPA 3-eWiFiSecurityWPA2
	- "readwifi" Read the WIFI parameters
	- "writewifi" Write the WIFI parameters in Flash, after every writewifi commands the board needs a HW reset for update the application parameters.

Prepare the Android application
The Android application requires Cognito service to authorize to AWS IoT in order to access device shadows. Use Amazon Cognito to create a new identity pool:

1.  In the Amazon Cognito Console https://console.aws.amazon.com/cognito/ select "Manage Federated Identities" and "Create new identity pool".

2.  Ensure Enable access to unauthenticated identities is checked. This allows the sample application to assume the unauthenticated role associated with this identity pool.
    Note: to keep this example simple it makes use of unauthenticated users in the identity pool. This can be used for getting started and prototypes but unauthenticated users should typically only be given read-only permissions in production applications. More information on Cognito identity pools including the Cognito developer guide can be found here: http://aws.amazon.com/cognito/.

3.  To obtain the Pool ID constant, select "Edit identity pool" and copy Identity pool ID (it will look like <REGION>:<ID>). This Identity pool ID (<COGNITO POOL ID>) will be used in the application (policy and configuration file).

4.  To obtain Account ID, select account name in webpage menu bar and select "My account" from drop down menu. Make note of "Account ID" under "Account Settings".

5. As part of creating the identity pool Cognito will setup two roles in Identity and Access Management (IAM) https://console.aws.amazon.com/iam/home#roles. These will be named something similar to: "Cognito_PoolNameAuth_Role" and "Cognito_PoolNameUnauth_Role".
Create policy to be attached into "Cognito_PoolNameUnauth_Role" though "Policies" menu, selecting "Create policy", "Create Your Own Policy" and copying example policy below into "Policy Document" field and naming it for example "<THING NAME>Policy". Replace <REGION>, <ACCOUNT ID> and <THING NAME> with your respective values. This policy allows the application to get and update the two thing shadows used in this sample.

    {
        "Version": "2012-10-17",
        "Statement": [
            {
                "Effect": "Allow",
                "Action": [
                    "iot:Connect"
                ],
                "Resource": [
                    "*"
                ]
            },
            {
                "Effect": "Allow",
                "Action": [
                    "iot:Publish"
                ],
                "Resource": [
                    "arn:aws:iot:<REGION>:<ACCOUNT ID>:topic/$aws/things/<THING NAME>/shadow/update",
                    "arn:aws:iot:<REGION>:<ACCOUNT ID>:topic/$aws/things/<THING NAME>/shadow/get"
                ]
            },
            {
                "Effect": "Allow",
                "Action": [
                    "iot:Subscribe",
                    "iot:Receive"
                ],
                "Resource": [
                    "*"
                ]
            }
        ]
    }

6.  Newly created policy now needs to be attached to the unauthenticated role which has permissions to access the required AWS IoT APIs by opening "Cognito_PoolNameUnauth_Role" under "Roles" menu. Then in "Permissions" tab select "Attach policy" to view list of all AWS policies where example's policy "<THING NAME>Policy" needs to be selected though checking its checkbox and clicking on "Attach policy" button.

    More information on AWS IAM roles and policies can be found here: http://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage.html
    More information on AWS IoT policies can be found here: http://docs.aws.amazon.com/iot/latest/developerguide/authorization.html

7.  Prepare "AwsLedWifiPreferences.properties" file with yours AWS credentials. It's structure looks like this:

    customer_specific_endpoint=<REST API ENDPOINT>
    cognito_pool_id=<COGNITO POOL ID>
    thing_name=<THING NAME>
    region=<REGION>

    Then move properties file into your Android device (application will ask for properties file though file browser dialog during first run).

8.  To run Android application do either:
    a) install and run pre-build apk on Android device (<SDK_Repository>\<board_name>\src\aws_examples\led_wifi_android\AwsLedWifi.apk)
    b) open project in Android Studio, build it, attach Android device and Run application

    Application requires at least Android version 5.1 (Android SDK 22).

    Then in both cases when asked select AwsLedWifiPreferences.properties file with AWS IoT preferences. Then application will establish MQTT connection to AWS server, download last state of thing's shadow and will be ready for user input.


Running the demo
================
This is the configuration for the board OM40007|LPC54018. Also the aws-nxp readme.md, amazon project for configuration of amazon services tool must be completed before trying the Alexa echo dot demo.

The log below shows the output of the demo in the terminal window. The log can be different based on your WiFi network configuration and based on the actions, which you have done in the Android application.

Android application displays status of the LED which are split into Desired and Reported section. Desired value is value wanted by user and Reported value is actual state of the LED on device.

- When you turn on/off the LED in the Android application, the change should be visible on the LED on the board.

Every mentioned action takes approximately 1-3 seconds.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
0 0 [Tmr Svc] Starting key provisioning...
1 0 [Tmr Svc] Write root certificate...
2 0 [Tmr Svc] Write device private key...
3 10 [Tmr Svc] Write device certificate...
4 19 [Tmr Svc] Key provisioning done...
5 19 [Tmr Svc] Starting WiFi...
6 1271 [Tmr Svc] WiFi module initialized.
7 4274 [Tmr Svc] WiFi connected to AP AndroidAP.
8 5278 [Tmr Svc] IP Address acquired 192.168.43.203
9 5283 [AWS-LED] [Shadow 0] MQTT: Creation of dedicated MQTT client succeeded.
10 5291 [AWS-LED] Sending command to MQTT task.
11 5295 [MQTT] Received message 10000 from queue.
12 5598 [MQTT] Looked up a254jqzk0kcvf0.iot.us-west-2.amazonaws.com as 54.187.233.210
13 28345 [MQTT] MQTT Connect was accepted. Connection established.
14 28346 [MQTT] Notifying task.
15 28350 [AWS-LED] Command sent to MQTT task passed.
16 28354 [AWS-LED] [Shadow 0] MQTT: Connect succeeded.
17 28357 [AWS-LED] Sending command to MQTT task.
18 28361 [MQTT] Received message 20000 from queue.
19 28596 [MQTT] MQTT Subscribe was accepted. Subscribed.
20 28597 [MQTT] Notifying task.
21 28601 [AWS-LED] Command sent to MQTT task passed.
22 28605 [AWS-LED] [Shadow 0] MQTT: Subscribe to callback topic succeeded.
23 28608 [AWS-LED] Sending command to MQTT task.
24 28612 [MQTT] Received message 30000 from queue.
25 28847 [MQTT] MQTT Subscribe was accepted. Subscribed.
26 28848 [MQTT] Notifying task.
27 28852 [AWS-LED] Command sent to MQTT task passed.
28 28856 [AWS-LED] [Shadow 0] MQTT: Subscribe to accepted topic succeeded.
29 28859 [AWS-LED] Sending command to MQTT task.
30 28862 [MQTT] Received message 40000 from queue.
31 29097 [MQTT] MQTT Subscribe was accepted. Subscribed.
32 29098 [MQTT] Notifying task.
33 29102 [AWS-LED] Command sent to MQTT task passed.
34 29106 [AWS-LED] [Shadow 0] MQTT: Subscribe to rejected topic succeeded.
35 29109 [AWS-LED] Sending command to MQTT task.
36 29113 [MQTT] Received message 50000 from queue.
37 29136 [MQTT] Notifying task.
38 29140 [AWS-LED] Command sent to MQTT task passed.
39 29142 [AWS-LED] [Shadow 0] MQTT: Publish to operation topic succeeded.
40 29378 [AWS-LED] Sending command to MQTT task.
41 29382 [MQTT] Received message 60000 from queue.
42 29718 [MQTT] MQTT Subscribe was accepted. Subscribed.
43 29719 [MQTT] Notifying task.
44 29723 [AWS-LED] Command sent to MQTT task passed.
45 29727 [AWS-LED] [Shadow 0] MQTT: Subscribe to accepted topic succeeded.
46 29730 [AWS-LED] Sending command to MQTT task.
47 29733 [MQTT] Received message 70000 from queue.
48 29968 [MQTT] MQTT Subscribe was accepted. Subscribed.
49 29969 [MQTT] Notifying task.
50 29973 [AWS-LED] Command sent to MQTT task passed.
51 29977 [AWS-LED] [Shadow 0] MQTT: Subscribe to rejected topic succeeded.
52 29980 [AWS-LED] Sending command to MQTT task.
53 29984 [MQTT] Received message 80000 from queue.
54 30015 [MQTT] Notifying task.
55 30020 [AWS-LED] Command sent to MQTT task passed.
56 30022 [AWS-LED] [Shadow 0] MQTT: Publish to operation topic succeeded.
57 30280 [AWS-LED] AWS LED Demo initialized.
58 30282 [AWS-LED] Use mobile application to turn on/off the LED.
59 40302 [AWS-LED] TempValue        = 16.17
60 40306 [AWS-LED] Sending command to MQTT task.
61 40310 [MQTT] Received message 90000 from queue.
62 40335 [MQTT] Notifying task.
63 40340 [AWS-LED] Command sent to MQTT task passed.
64 40342 [AWS-LED] [Shadow 0] MQTT: Publish to operation topic succeeded.
65 40624 [AWS-LED] [Shadow 0] MQTT: Return MQTT buffer succeeded.
66 50646 [AWS-LED] TempValue        = 16.41
67 50650 [AWS-LED] Sending command to MQTT task.
68 50654 [MQTT] Received message a0000 from queue.
69 50679 [MQTT] Notifying task.
70 50684 [AWS-LED] Command sent to MQTT task passed.
71 50686 [AWS-LED] [Shadow 0] MQTT: Publish to operation topic succeeded.
72 50969 [AWS-LED] [Shadow 0] MQTT: Return MQTT buffer succeeded.
73 60991 [AWS-LED] TempValue        = 16.82
74 60995 [AWS-LED] Sending command to MQTT task.
75 60999 [MQTT] Received message b0000 from queue.
76 61024 [MQTT] Notifying task.
77 61029 [AWS-LED] Command sent to MQTT task passed.
78 61031 [AWS-LED] [Shadow 0] MQTT: Publish to operation topic succeeded.
79 61330 [AWS-LED] [Shadow 0] MQTT: Return MQTT buffer succeeded.
80 71352 [AWS-LED] TempValue        = 17.12
81 71356 [AWS-LED] Sending command to MQTT task.
82 71360 [MQTT] Received message c0000 from queue.
83 71385 [MQTT] Notifying task.
84 71390 [AWS-LED] Command sent to MQTT task passed.
85 71392 [AWS-LED] [Shadow 0] MQTT: Publish to operation topic succeeded.
86 71674 [AWS-LED] [Shadow 0] MQTT: Return MQTT buffer succeeded.
