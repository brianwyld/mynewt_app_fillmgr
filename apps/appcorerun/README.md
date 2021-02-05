/**
 * Copyright 2020 Brian WYLD
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at
 *    http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, 
 * software distributed under the License is distributed on 
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
 * either express or implied. See the License for the specific 
 * language governing permissions and limitations under the License.
*/

This is a LoRaWAN based fill manager device, which can use an ultrasonic distance ranger unit
to determine the fill level of any container. The fill level is sent to the backend every X minutes
or if the level exceeds a certain point (distance measured is less than a limit)
The default value for the 'full' signal is set in the syscfg, and can be changed by config actions.

This is a MyNewt firmware project, to use it you will require knowledge of MyNewt:
Please refer to the documentation at http://mynewt.apache.org/ or ask questions on dev@mynewt.apache.org

Hardware setup:
The BSP for this project is by default for a Wyres W_BASEV2 card. This provides the MCU (STM32L151CC), LoRa radio (SX1272),
and various leds, gpios and sensors.
The US ranger sensor should have the TRIG and ECHO lines connected to the GPIO pins defined above. Recommendation is to use
the 'SPEAKER' output for the TRIG action, and the 'BUTTON' input for the echo result.

Firmware operation:
This is an appcore application, basic appcore SM operation applies (see Wyres app-core repo for info)

Messages:
The UL packets are formatted as TLV elements, with the environmental information (temp, pressure, battery etc), the 'ack required' flag, and the access request. 
Element         Tag Length  Value
Temp            3   2       Temperature in 1/10 degC
Battery         7   2       Battery voltage in mV
Fill level      240 1        Distance in cm between the US sensor and the detected stuff
Fill status     241 1       Status of the fill ranger : 0=no reading received (error), 1=reading received, 2=FULL 

The DL packets are formatted as TLV actions (see appcore doc for details)
No specific actions are determined for this application.