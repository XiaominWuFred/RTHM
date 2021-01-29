/*
 * WiFiConnection.c
 *
 *  Created on: Nov 2, 2015
 *      Author: wei
 */

#include "FreeRTOS.h"
#include "task.h"
#include "socket.h"
#include "SimpleLink.h"
#include "WiFiEvent.h"
#include "WiFiConnection.h"
#include "string.h"

// remote IP address and port for UDPWrite
SlSockAddrIn_t UDPRemoteAddr;

/************************************************************************************
 *  int Start_WiFi_Device(int iMode)
 *
 *  start WiFI device and return the WiFi status
 *
 *  input:
 *  	iMode: the WiFi mode
 *  	WIFI_STATION		station mode
 *  	WIFI_AP				AP mode
 *  	WIFI_P2P			P2P mode
 *
 *  output:
 *  	WIFI_STATION		WiFi is in station mode
 *  	WIFI_AP				WiFi is in AP mode
 *  	WIFI_P2P			WiFI is in P2P mode
 *
 ***********************************************************************************/

int Start_WiFi_Device(int Mode)
{
	int iMode = -1;
	int iStatus;

	iMode = sl_Start(0,0,0);
	if (iMode < 0)
		return WIFI_DEVICE_START_ERROR;
	if (Mode == iMode)
		return iMode;
	iStatus = sl_WlanSetMode((_u8)Mode);
	if (iStatus >= 0)
		return Mode;
	else
		return WIFI_SET_MODE_ERROR;
}

/************************************************************************************
 *  int WiFi_Stop_Device(unsigned int timeout)
 *
 *  Stop WiFi device.
 *
 *  input:
 *  	timeout: time for WiFi module to enter hibernation
 *  output
 *  	0 if successful and negative in error occures.
 *
 *************************************************************************************/
int Stop_WiFi_Device(unsigned int timeout)
{
	return sl_Stop(timeout);
}

/************************************************************************************
 *  int Set_Tx_Power(int Mode, int Level)
 *
 *  Set the Tx power for the Mode
 *
 *  input:
 *  	Mode : the WiFi mode
 *  			WIFI_STATION
 *  			WIFI_AP
 *  	level : the Tx power level
 *  output:
 *  	Zero if successful negative value if error
 ************************************************************************************/
int Set_Tx_Power(int Mode, int Level)
{
	int iStatus;
	_u8 uLevel;

	if ((Mode != WIFI_STATION) && (Mode != WIFI_AP))
		return WIFI_SET_TX_POWER_ERROR;
	if (Level > WIFI_TX_MAX_POWER)
		Level = WIFI_TX_MAX_POWER;
	uLevel = (_u8)Level;
	if (Mode == WIFI_STATION)
		iStatus = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, sizeof(uLevel), &uLevel);
	else
		iStatus = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,WLAN_GENERAL_PARAM_OPT_AP_TX_POWER, sizeof(uLevel), &uLevel);
	if (iStatus == 0)
		return 0;
	else
		return WIFI_SET_TX_POWER_ERROR;
}
/*********************************************************************************************************
 *  int STA_Connect(char *SSID, char *Passcode, unsigned char SecurityType, TickType_t timeout)
 *
 *  This function connects CC3200 to an AP as a station
 *
 *  input:
 *  	1: SSID: SSID of the AP
 *  	2: Passcode: the Passcode to connect AP if applicable
 *  	3: SecurityType: The Security type of the connection that holds one of the following value
 *  			WIFI_SEC_TYPE_OPEN -- Open connection without security
 *  			WIFI_SEC_TYPE_WEP -- WEP security
 *  			WIFI_SEC_TYPE_WPA -- WPA security
 *  	4: timeout: Timeout for the connection. Should be multiples of 500
 *
 *  output:
 *  	error code if negative.
 *
 **********************************************************************************************************/
int STA_Connect(char *SSID, char *Passcode, unsigned char SecurityType, TickType_t timeout)
{
	SlSecParams_t secParams;
	long lRetVal = -1;
	TickType_t TimeCounter = 0;

	secParams.Key = (signed char*)Passcode;
	secParams.KeyLen = strlen(Passcode);
	secParams.Type = SecurityType;

	lRetVal = sl_WlanConnect((signed char*)SSID, strlen(SSID), 0, &secParams, 0);

	if (lRetVal < 0)
	{
		return WIFI_AP_CONNECTION_ERROR;
	}
	while (!CheckAllNetworkStatus(WIFI_CONNECTION | WIFI_IP_AQUIRED) && (TimeCounter<timeout))
	{
		vTaskDelay(500);
		TimeCounter += 500;
   	}
	if (CheckAllNetworkStatus(WIFI_CONNECTION | WIFI_IP_AQUIRED))
	{
		return 0;
	}
	else
	{
		return WIFI_AP_CONNECTION_TIMEOUT;
	}

}

/***********************************************************************************************************************
 *
 *  TCPHandle_t TCPOpenConnectionByIP(unsigned long IPAddress, unsigned short int port, TickType_t timeout)
 *
 *  Create a TCP connection with the remote server and return the connected handle
 *
 *  input:
 *  	1. IPAddress: the IP address of the remote device
 *  	2. port: the TCP port of the remote device for connection
 *  	3. timeout: the timeout in system tick for the establishment of connection. Wait forever
 *  		if the timeout is portMAX_DELAY
 *  output:
 *  	It returns the handle to the connection if positive or error code if negative.
 *
 ***********************************************************************************************************************/
TCPHandle_t TCPOpenConnectionByIP(unsigned long IPAddress, unsigned short int port, TickType_t timeout)
{
	int iSocketID;
	SlSockAddrIn_t sAddr;
	int iAddrSize;
	int iStatus;
	long  lNonBlocking = 1;
	TickType_t timeoutcount = 0;


	// Set up remote server IP and port number for connection
	sAddr.sin_family = SL_AF_INET;
	sAddr.sin_port = sl_Htons(port);
	sAddr.sin_addr.s_addr = sl_Htonl(IPAddress);
	iAddrSize = sizeof(sAddr);

	// creating a TCP socket
	iSocketID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
	if( iSocketID < 0 )
	{
	    return  WIFI_TCP_SOCKET_FAILURE;
	}

	// make the socket function calls non blocking
	iStatus = sl_SetSockOpt(iSocketID, SL_SOL_SOCKET, SL_SO_NONBLOCKING,&lNonBlocking,sizeof(lNonBlocking));
	if (iStatus < 0)
	{
	    // error
	    sl_Close(iSocketID);
	    return WIFI_SOCKET_OPTION_ERROR;
	}

	// connecting to TCP server
	iStatus = SL_EALREADY;
	while (iStatus <0)
	{
		iStatus = sl_Connect(iSocketID, ( SlSockAddr_t *)&sAddr, iAddrSize);
		if (iStatus < 0)
		{
			// Check if the error is SL_EALREADY, delay one tick and try again
			if ((iStatus == SL_EALREADY) && (timeoutcount < timeout))
			{
				// delay 1 tick to connect again till the predefined delay
				if (timeout < portMAX_DELAY)
					//increment the time out counter if the preset timeout is portMAX_DELAY
					timeoutcount++;
				vTaskDelay(1);
			}
			else
			{
				// error during connection
				sl_Close(iSocketID);
				if (timeoutcount>=timeout)
					return WIFI_TCP_TIMEOUT_ERROR;
				else
					return WIFI_TCP_REMOTE_ERROR;
			}
		}

	}
	// connect successfully.
	return iSocketID;
}

/****************************************************************************************************************
 *
 *  TCPHandle_t TCPOpenConnectionByDN(char *DName, unsigned short int port, TickType_t timeout)
 *
 *  Create a TCP connection with the remote server using FQDN and return the connected handle
 *
 *  input:
 *  	1. IPAddress: the IP address of the remote device
 *  	2. port: the TCP port of the remote device for connection
 *  	3. timeout: the timeout in system tick for the establishment of connection. Wait forever
 *  		if the timeout is portMAX_DELAY
 *  output:
 *  	It returns the handle to the connection if positive or error code if negative.
 *
 ***************************************************************************************************************/
TCPHandle_t TCPOpenConnectionByDN(char *DName, unsigned short int port, TickType_t timeout)
{
	int iStatus;
	int iSocketID;
	unsigned long DestinationIP;

	// retrieve IP address from FQDN
	iStatus = sl_NetAppDnsGetHostByName((_i8 *)DName, strlen(DName), &DestinationIP,SL_AF_INET);
	if (iStatus < 0)
	{
		// error -- cannot get the remote IP address
		return WIFI_REMOTE_ADDR_UNRESOLVED;
	}
	else
	{
		// connect using the IP address
		iSocketID = TCPOpenConnectionByIP(DestinationIP, port, timeout);
		return iSocketID;
	}
}

/***************************************************************************************************
 *
 * int TCPRead(TCPHandle_t handle, void *Buffer, int BufferSize,  TickType_t timeout)
 *
 * Read data from the opened connection within the timeout period. It returns the actual number of
 * bytes read.
 *
 * input:
 *   1. handle: the handle of the existing TCP connection
 *   2. Buffer: buffer to hold the incoming data
 *   3. BufferSize: the size of data in bytes to be read
 *   4. timeout: time out period. The function will not return when all bytes are received if it is
 *   	portMAX_DELAY
 * output:
 * 		the amount of data actually read if positive. Otherwise the error code.
 *
 ***************************************************************************************************/
int TCPRead(TCPHandle_t handle, void *Buffer, int BufferSize,  TickType_t timeout)
{
	char *ptr;
	TickType_t counter=0;
	int BytesRead;
	int TotalBytes = 0;

	ptr = (char *)Buffer;
	while ((BufferSize >0) && (counter < timeout))
	{
		// read data from socket until all byte are read or timed out
		BytesRead = sl_Recv(handle,ptr,BufferSize,0);
		if ((BytesRead < 0) && (BytesRead != SL_EAGAIN))
			return WIFI_TCP_READ_ERROR;
		else
		{
			// adjust the size of data to be read
			if (BytesRead >=0 )
			{
				BufferSize -= BytesRead;
				TotalBytes += BytesRead;
				ptr +=BytesRead;
			}
			if (BufferSize > 0)
			{
				// if there are still data to read, delay one tick
				vTaskDelay(1);
				if (timeout < portMAX_DELAY)
					counter++; 	//increment counter if the timeout is not portMAX_DELAY
			}
		}
	}
	return TotalBytes;
}

/***************************************************************************************************
 *
 * int TCPWrite(TCPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout)
 *
 * Write data to the opened connection within the timeout period. It returns the actual number of
 * bytes written.
 *
 * input:
 *   1. handle: handle of the existing TCP connection
 *   2. Buffer: buffer to hold the outgoing data
 *   3. BufferSize: the size of data in bytes to be written
 *   4. timeout: time out period. The function will not return when all bytes are sent if it is
 *   	portMAX_DELAY
 * output:
 * 		the amount of data actually read if positive. Otherwise the error code.
 *
 ***************************************************************************************************/
int TCPWrite(TCPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout)
{
	char *ptr;
	TickType_t counter=0;
	int BytesSent;
	int TotalBytes = 0;

	ptr = (char *)Buffer;
	while ((BufferSize >0) && (counter < timeout))
	{
		// read data from socket until all byte are read or timed out
		BytesSent = sl_Send(handle,ptr,BufferSize,0);
		if ((BytesSent < 0) && (BytesSent != SL_EAGAIN))
			return WIFI_TCP_WRITE_ERROR;
		else
		{
			if (BytesSent >=0 )
			{
				BufferSize -= BytesSent;
				TotalBytes += BytesSent;
				ptr +=BytesSent;
			}
			if (BufferSize > 0)
			{
				// if there are still data to read, delay one tick
				vTaskDelay(1);
				if (timeout < portMAX_DELAY)
					counter++; 	//increment counter if the timeout is not portMAX_DELAY
			}
		}
	}
	return TotalBytes;
}

/****************************************************************************************************************
 *
 *	TCPHandle_t TCPStartListen(int short port)
 *
 *	Open a local port to listen for incoming TCP connection
 *
 *	input:
 *   1.	port: local port number waiting for connection
 *
 *  output:
 *  	It returns the handle to the connection if positive or error code if negative.
 *
 ****************************************************************************************************************/

TCPHandle_t TCPStartListen(unsigned int short port)
{
	SlSockAddrIn_t  sLocalAddr;
	_i16            iAddrSize;
	int 			iSocket;
	int 			iStatus;
	long            NonBlocking = 1;

	//filling the TCP server socket address. Use 0 for local.
	sLocalAddr.sin_family = SL_AF_INET;
	sLocalAddr.sin_port = sl_Htons((unsigned short)port);
	sLocalAddr.sin_addr.s_addr = 0;

	// creating a TCP socket
	iSocket = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
	if( iSocket < 0 )
	{
	    // error
		return WIFI_TCP_SOCKET_FAILURE;
	}
	iAddrSize = sizeof(SlSockAddrIn_t);

	// binding the TCP socket to the TCP server address
	iStatus = sl_Bind(iSocket, (SlSockAddr_t *)&sLocalAddr, iAddrSize);
	if( iStatus < 0 )
	{
	    // error
	    sl_Close(iSocket);
	    return WIFI_SERVER_BIND_ERROR;
	}
	// listen to the socket
	iStatus = sl_Listen(iSocket, 0);
	if( iStatus < 0 )
	{
	   sl_Close(iSocket);
	   return WIFI_LISTEN_ERROR;
	}
	// setting socket option to make the socket as non blocking
	iStatus = sl_SetSockOpt(iSocket, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &NonBlocking, sizeof(NonBlocking));
	if( iStatus < 0 )
	{
	   sl_Close(iSocket);
	   return WIFI_SOCKET_OPTION_ERROR;
	}
	return iSocket;
}

/***********************************************************************************************************
 * TCPHandle_t TCPListen(TCPHandle_t handle, TickType_t timeout)
 *
 * Listen for the incoming connection request to the designate socket
 *
 * input:
 *  1. handle: the handle created by TCPStartListen that identifies the incoming port
 *  2. timeout: The timeout period for listening to the incoming connection request
 *  3. SourceIP: receiving the IP address of the remote device initiating the connection
 *  4. port: receving the port of the remote device initiating the connection
 *
 * output:
 *  It returns the handle to the connection if positive or error code if negative.
 *
 ***********************************************************************************************************/

TCPHandle_t TCPListen(TCPHandle_t handle, TickType_t timeout)
{
	int iNewSocket;
	int timeoutCounter = 0;
	SlSockAddrIn_t  sAddr;
	int             iAddrSize;
	iNewSocket = SL_EAGAIN;
	int NonBlocking = 1;
	int iStatus;

	iNewSocket = SL_EAGAIN;
	// waiting for an incoming TCP connection
	while( iNewSocket < 0 )
	{
	    // accepts a connection form a TCP client, if there is any
	    // otherwise returns SL_EAGAIN
	    iNewSocket = sl_Accept(handle, ( struct SlSockAddr_t *)&sAddr, (SlSocklen_t*)&iAddrSize);
	    if( iNewSocket == SL_EAGAIN )
	    {
	        vTaskDelay(1);
	        if (timeout < portMAX_DELAY)
	        {
	        	timeoutCounter++;
	        	if (timeoutCounter>=timeout)
	        		return WIFI_LISTEN_TIMEOUT;
	        }

	    }
	    else if( iNewSocket < 0 )
	    {
	       // error
	       sl_Close(iNewSocket);
	       return WIFI_LISTEN_ERROR;
	    }
	}
	// setting socket option to make the socket as non blocking
	iStatus = sl_SetSockOpt(iNewSocket, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &NonBlocking, sizeof(NonBlocking));
	if( iStatus < 0 )
	{
	   sl_Close(iNewSocket);
	   return WIFI_SOCKET_OPTION_ERROR;
	}
	else
		return iNewSocket;
}

/*****************************************************************************************
 *
 * int TCPStopListen(TCPHandle_t handle)
 *
 * Close the local socket for the listening of incoming connection request
 *
 * input:
 *  1. handle: the handle for the listening of incoming connection request
 *
 ******************************************************************************************/
int TCPStopListen(TCPHandle_t handle)
{
	int iStatus;

	iStatus = sl_Close(handle);
	if (iStatus < 0)
		return WIFI_TCP_HANDLE_ERROR;
	else
		return 0;
}

/*****************************************************************************************
 *
 * int TCPCloseConnection(int socket)
 *
 * Close the TCP socket for connection
 *
 * input:
 *  1. handle: the handle for the listening of incoming connection request
 *
 ******************************************************************************************/
int TCPCloseConnection(TCPHandle_t handle)
{
	int iStatus;

	iStatus = sl_Close(handle);
	if (iStatus < 0)
		return WIFI_TCP_HANDLE_ERROR;
	else
		return 0;
}

/***********************************************************************************************************************
 *
 *  UDPHandle_t UDPOpen(void)
 *
 *  Set up UDP to connect remote server using IP address and port and return the handle
 *
 *  input:
 *  	None
 *
 *  output:
 *  	It returns the handle to the connection if positive or error code if negative.
 *
 ***********************************************************************************************************************/
UDPHandle_t UDPOpen(void)
{
	int iSocketID;
	int iStatus;
	int NonBlocking=1;

	// creating a UDP socket
	iSocketID = sl_Socket(SL_AF_INET,SL_SOCK_DGRAM, 0);
	if( iSocketID < 0 )
	{
	    //Cannot create socket. release UDP port memory
		return  WIFI_UDP_SOCKET_FAILURE;
	}
	// setting socket option to make the socket as non blocking
	iStatus = sl_SetSockOpt(iSocketID, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &NonBlocking, sizeof(NonBlocking));
	if( iStatus < 0 )
	{
		sl_Close(iSocketID);
		return WIFI_SOCKET_OPTION_ERROR;
	}
	//return handle
	return iSocketID;
}

/****************************************************************************************************************
 *
 *  int GetIPFromDN(char *DName, unsigned long *DestinationIP)
 *
 *  Get the IP address from FQDN
 *
 *  input:
 *  	1. DName: the Full QUalifie Domain Name (FQDN) of the remote device
 *  	2. DestinationIP: the returned IP address of the FQDN defined in DName
 *
 *  output:
 *  	It returns 0 if successful or error code if negative.
 *
 ***************************************************************************************************************/
int GetIPFromDN(char *DName, unsigned long *DestinationIP)
{
	int iStatus;

	// retrieve IP address from FQDN
	iStatus = sl_NetAppDnsGetHostByName((_i8 *)DName, strlen(DName), DestinationIP,SL_AF_INET);
	if (iStatus < 0)
	{
		// error -- cannot get the remote IP address
		return WIFI_REMOTE_ADDR_UNRESOLVED;
	}
	else
	{

		return 0;
	}
}

/*****************************************************************************************************************
 * void UDPSetAddressPort(unsigned long IPAddress, unsigned short int port)
 *
 * This function set the UDP remote address for UDPWrite function. If the address is changed for a new destination,
 * this function needs to be called to set the new IP address and port.
 *
 * input:
 * 	1. IPAddress: IP address for the remote destination
 * 	2. port: the UDP port number of the remote destination.
 *
 * output:
 * 	none
 *****************************************************************************************************************/
void UDPSetAddressPort(unsigned long IPAddress, unsigned short int port)
{
	UDPRemoteAddr.sin_family = SL_AF_INET;
	UDPRemoteAddr.sin_addr.s_addr = sl_Htonl(IPAddress);
	UDPRemoteAddr.sin_port = sl_Htons(port);
	return;
}

/****************************************************************************************************************
 *
 *	UDPHandle_t UDPListen(unsigned short port)
 *
 *	prepare a local UDP port ready for incoming UDP connection
 *
 *	input:
 *   1.	port: local port number waiting for connection
 *
 *  output:
 *  	It returns the handle to the connection if positive or error code if negative.
 *
 ****************************************************************************************************************/
UDPHandle_t UDPListen(unsigned int short port)
{
	UDPHandle_t handle;
	SlSockAddrIn_t	Addr;
	int iStatus;
	int NonBlocking=1;

	handle = sl_Socket(SL_AF_INET,SL_SOCK_DGRAM, 0);
	Addr.sin_family = SL_AF_INET;
	Addr.sin_port = sl_Htons(port);
	Addr.sin_addr.s_addr = 0;
	iStatus = sl_Bind(handle, (SlSockAddr_t *)&Addr, sizeof(SlSockAddr_t));
	if (iStatus < 0)
	{
		sl_Close(handle);
		return WIFI_UDP_BIND_ERROR;
	}
	// setting socket option to make the socket as non blocking
	iStatus = sl_SetSockOpt(handle, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &NonBlocking, sizeof(NonBlocking));
	if( iStatus < 0 )
	{
		sl_Close(handle);
		return WIFI_SOCKET_OPTION_ERROR;
	}
	return handle;
}

/***************************************************************************************************
 *
 * int UDPRead(UDPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout)
 *
 * Read data from the UDP port defined by handle within the timeout period. It returns the actual
 * number of bytes read.
 *
 * input:
 *   1. handle: Open UDP handle
 *   2. Buffer: buffer to hold the incoming data
 *   3. BufferSize: the size of data in bytes to be read
 *   4. timeout: time out period. The function will not return when all bytes are received if it is
 *   	portMAX_DELAY
 * output:
 * 		the amount of data actually read if positive. Otherwise the error code.
 *
 ***************************************************************************************************/

int UDPRead(UDPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout)
{
	char *ptr;
	TickType_t counter=0;
	SlSockAddr_t sAddr;
	SlSocklen_t sAddrSize;
	int BytesRead;
	int TotalBytes = 0;

	sAddrSize = sizeof(sAddr);
	ptr = (char *)Buffer;
	while ((BufferSize >0) && (counter < timeout))
	{
		// read data from socket until all byte are read or timed out
		BytesRead = sl_RecvFrom(handle,ptr,BufferSize,0, &sAddr, &sAddrSize);
		if ((BytesRead < 0) && (BytesRead != SL_EALREADY))
			return WIFI_UDP_READ_ERROR;
		// adjust the size of data to be read
		if (BytesRead >=0 )
		{
			BufferSize -= BytesRead;
			TotalBytes += BytesRead;
			ptr +=BytesRead;
		}
		if (BufferSize > 0)
		{
			// if there are still data to read, delay one tick
			vTaskDelay(1);
			if (timeout < portMAX_DELAY)
				counter++; 	//increment counter if the timeout is not portMAX_DELAY
		}
	}
	return TotalBytes;
}

/***************************************************************************************************
 *
 * int UDPWrite(UDPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout)
 *
 * Send data to the remote port defined in handle within the timeout period. It returns the actual
 * number of bytes written.
 *
 * input:
 *   1. handle: opened UDP handle
 *   2. Buffer: buffer to hold the outgoing data
 *   3. BufferSize: the size of data in bytes to be written
 *   4. timeout: time out period. The function will not return when all bytes are sent if it is
 *   	portMAX_DELAY
 * output:
 * 		the amount of data actually read if positive. Otherwise the error code.
 *
 ***************************************************************************************************/
int UDPWrite(UDPHandle_t handle, void *Buffer, int BufferSize, TickType_t timeout)
{
	char *ptr;
	TickType_t counter=0;
	int BytesSent;
	int TotalBytes = 0;

	ptr = (char *)Buffer;
	while ((BufferSize >0) && (counter < timeout))
	{
		// read data from socket until all byte are read or timed out
		BytesSent = sl_SendTo(handle,ptr,BufferSize,0,(SlSockAddr_t *)&UDPRemoteAddr, sizeof(UDPRemoteAddr));
		if ((BytesSent < 0) && (BytesSent != SL_EALREADY))
			return WIFI_UDP_WRITE_ERROR;
		if (BytesSent >=0 )
		{
			BufferSize -= BytesSent;
			TotalBytes += BytesSent;
			ptr +=BytesSent;
		}
		if (BufferSize > 0)
		{
			// if there are still data to read, delay one tick
			vTaskDelay(1);
			if (timeout < portMAX_DELAY)
				counter++; 	//increment counter if the timeout is not portMAX_DELAY
		}
	}
	return TotalBytes;
}


/*****************************************************************************************
 *
 * int UDPClose(UDPHandle_t handle)
 *
 * Close the UDP handle
 *
 * input:
 *  1. handle: opened UDP handle
 *
 ******************************************************************************************/

int UDPClose(UDPHandle_t handle)
{
	int iStatus;

	iStatus = sl_Close(handle);
	if (iStatus < 0)
		return WIFI_UDP_HANDLE_ERROR;
	else
		return 0;
}
