/*
 * WiFiEvent.h
 *
 *  Created on: Oct 9, 2015
 *      Author: wei
 */

#include "uart_if.h"

#ifndef NETWORK_WIFIEVENT_H_
#define NETWORK_WIFIEVENT_H_

#define SSID_LEN_MAX        32
#define BSSID_LEN_MAX       6
#define SIMPLELINK_EVENT_MESSAGE	//Allow event handler to output message to UART for debug

// Status bits - These are used to set/reset the corresponding bits in given variable
#define WIFI_CONNECTION 		0x0001 // If this bit is set: the device is connected to the AP or client is connected to device (AP)
#define WIFI_SMARTCONFIG_START 	0x0002 // If this bit is set: the SmartConfiguration. process is started from SmartConfig app
#define WIFI_P2P_DEV_FOUND 		0x0004 // If this bit is set: the device (P2P mode). found any p2p-device in scan
#define WIFI_P2P_REQ_RECEIVED 	0x0008 // If this bit is set: the device (P2P mode). found any p2p-negotiation request
#define WIFI_CONNECTION_FAILED 	0x0010 // If this bit is set: the device(P2P mode). connection to client(or reverse way) is failed
#define WIFI_IP_LEASED 			0x0020 // If this bit is set: the device has leased IP to any connected client
#define WIFI_IP_AQUIRED 		0x0040 // If this bit is set: the device has acquired an IP
#define WIFI_NWP_INIT  			0x8000 // If this bit is set: Network Processor is powered up
//#define WIFI_PING_DONE 			0x0100 // If this bit is set: the device has completed the ping operation.

void InitializeWiFiVariables();
long ConfigureSimpleLinkToDefaultState();

//variables
unsigned long getWiFi_Status();
void setWiFi_Status(unsigned long a);
unsigned long getWiFi_IP();
unsigned long getConnectedDevice_IP();

//Manage network status
void SetNetworkStatus(long status);		//set network status flag
void ClearNetworkStatus(long status);	//clear network status flag
long CheckAnyNetworkStatus(long status);//Check if any status flag is set
long CheckAllNetworkStatus(long status);//Check if all status flags are set
unsigned long GetGatewayIP();

#ifdef NOTERM
#define UART_PRINT(x,...)
#define DBG_PRINT(x,...)
#define ERR_PRINT(x)
#else
#define UART_PRINT Report
#define DBG_PRINT  Report
#define ERR_PRINT(x) Report("Error [%d] at line [%d] in function [%s]  \n\r",x,__LINE__,__FUNCTION__)
#endif

// Loop forever, user can change it as per application's requirement
#define LOOP_FOREVER() \
            {\
                while(1); \
            }

// check the error code and handle it
#define ASSERT_ON_ERROR(error_code)\
            {\
                 if(error_code < 0) \
                   {\
                        ERR_PRINT(error_code);\
                        return error_code;\
                 }\
            }

#endif /* NETWORK_WIFIEVENT_H_ */
