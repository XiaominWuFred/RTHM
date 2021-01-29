/*
 * WiFiConnection.h
 *
 *  Created on: Nov 2, 2015
 *      Author: wei
 */

#ifndef NETWORK_WIFICONNECTION_H_
#define NETWORK_WIFICONNECTION_H_

#include "FreeRTOS.h"
#include "wlan.h"


#define WIFI_STATION		ROLE_STA
#define WIFI_AP				ROLE_AP
#define WIFI_P2P			ROLE_P2P

#define WIFI_TX_MAX_POWER	15

#define WIFI_DEVICE_START_ERROR			-1
#define WIFI_DEVICE_STOP_ERROR			-2
#define WIFI_SET_MODE_ERROR				-3
#define WIFI_SET_TX_POWER_ERROR			-4
#define WIFI_TCP_SOCKET_FAILURE			-100
#define WIFI_TCP_REMOTE_ERROR			-101
#define WIFI_TCP_TIMEOUT_ERROR			-102
#define WIFI_SOCKET_OPTION_ERROR		-103
#define WIFI_REMOTE_ADDR_UNRESOLVED		-104
#define WIFI_TCP_READ_ERROR				-105
#define WIFI_TCP_WRITE_ERROR			-106
#define WIFI_SERVER_BIND_ERROR			-107
#define WIFI_LISTEN_ERROR				-108
#define WIFI_LISTEN_TIMEOUT				-109
#define WIFI_TCP_HANDLE_ERROR			-110

#define WIFI_AP_CONNECTION_ERROR		-200
#define WIFI_AP_CONNECTION_TIMEOUT		-201

#define WIFI_SEC_TYPE_OPEN		SL_SEC_TYPE_OPEN
#define WIFI_SEC_TYPE_WEP		SL_SEC_TYPE_WEP
#define WIFI_SEC_TYPE_WPA		SL_SEC_TYPE_WPA
#define WIFI_SEC_TYPE_WPA_ENT	SL_SEC_TYPE_WPA_ENT
#define WIFI_SEC_TYPE_WPS_PBC	SL_SEC_TYPE_WPS_PBC
#define WIFI_SEC_TYPE_WPS_PIN	SL_SEC_TYPE_WPS_PIN

//TCP connection
// functions to change little-endian to big-endian for long and short integer
#define ReverseOrderLong sl_Htonl
#define ReverseOrdeShort sl_Htons

int Start_WiFi_Device(int mode);
int Stop_WiFi_Device(unsigned int timeout);

//functions to connect to AP as a WiFi station

int STA_Connect(char *SSID, char *Passcode, unsigned char SecurityType, TickType_t timeout);
int Set_Tx_Power(int Mode, int Level);

#define TCPHandle_t int

TCPHandle_t TCPOpenConnectionByIP(unsigned long IPAddress, unsigned short int port, TickType_t timeout);
TCPHandle_t TCPOpenConnectionByDN(char *DName, unsigned short int port, TickType_t timeout);
int TCPRead(TCPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout);
int TCPWrite(TCPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout);
TCPHandle_t TCPStartListen(unsigned int short port);
TCPHandle_t TCPListen(TCPHandle_t handle, TickType_t timeout);
int TCPStopListen(TCPHandle_t handle);
int TCPCloseConnection(TCPHandle_t handle);

//UDP
#define WIFI_UDP_HANDLE_ERROR			-120
#define WIFI_UDP_SOCKET_FAILURE			-121
#define WIFI_UDP_READ_ERROR				-122
#define WIFI_UDP_WRITE_ERROR			-123
#define WIFI_UDP_BIND_ERROR				-124

#define UDPHandle_t int

UDPHandle_t UDPOpen(void);
int GetIPFromDN(char *DName, unsigned long *DestinationIP);
void UDPSetAddressPort(unsigned long IPAddress, unsigned short int port);
UDPHandle_t UDPListen(unsigned short int port);
int UDPRead(UDPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout);
int UDPWrite(UDPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout);
int UDPClose(UDPHandle_t handle);

#endif /* NETWORK_WIFICONNECTION_H_ */
