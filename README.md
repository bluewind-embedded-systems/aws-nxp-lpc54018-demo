Overview
========
This demo demonstrates how the board NXP OM40007|LPC54018 IoT Module can be controlled by Amazon Developer Services.  
We are going to set everything up and running to operate:

- The user speaks to the Echo device to activate an Alexa skill;
- The Alexa Skill triggers an AWS-Lambda function which sends a message to the AWS-IoT;
- The NXP IoT Module is registered on the AWS-IoT and receives the message to perform the targeted action (turn on/off LED or read temperature);
- The NXP Module sends a feedback message to the AWS-IoT platform; 
- After retrieving the message the AWS-Lambda function sends it back to the Alexa platform;
- This makes the Echo device answer to user: "The LED is on/off !" or "The actual temperature is x degrees ".

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

Install
=======
After cloning this repository please follow the recipe contained [in this README](NXP/README.md) in order to start using it.
