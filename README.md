# voltronic_ESP8266_MQTT
Home Assistant addon using MQTT to get data from Voltronic Solar Inverters using ESP8266

This project is intended to interface with the Voltronics 'InfiniSolar V' Solar inverters https://voltronicpower.com/en-US/Product/Detail/InfiniSolar-V-1K-5K, sold in India by Flin Energy https://flinenergy.com/solar_inverters/_FlinInfini_Lite_Smart_Hybrid_Inverter.

This inverter follows protocol version 1.8 that starts with ^ (^P005PI) unlike previous starting with Q (QPI).
___________________________________________________________________________________________
Comman Format: ^TnnnXXXX,XXXX,XXXX,,<CRC><cr>
___________________________________________________________________________________________
Character       Description         Remark

^               Start bit

T               Type                P: PC Query command, S: Set command, D: Device Response

nnn             Data length         Include CRC and ending character, except"^Tnnn"

XXXXX           Data                If the data is reserved, they will be filled nothing,  
                                      so you would see double "," connected.

,               Seperator           Separate each data, please use "," to recognize the 
                                    length of data. If double "," continuing, that means this data is reserved.

<CRC>           Two byte of CRC result, the first byte is high 8 bits, second byte is low 
                8 bits.
___________________________________________________________________________________________

Before compiling be sure to change the SSID, password, aqnd MQTT server IP in include/passwd.h 
The binaries can be built by importing the project in PlatformIO editor or Arduino.
