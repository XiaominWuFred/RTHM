/*
 * WiFiEvent.c
 *
 *  Created on: Oct 9, 2015
 *      Author: wei
 */
#include "SimpleLink.h"
#include "WiFiEvent.h"
#include "nonos.h"

//Network connection data
unsigned long  WiFi_Status = 0;		// SimpleLink Status

//unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
unsigned long  WiFi_GatewayIP = 0; 	//Network Gateway IP address
unsigned long  WiFi_IP = 0;			//device IP address
unsigned long  ConnectedDevice_IP = 0;
unsigned long  WiFi_DNS = 0;		//DNS server IP
unsigned char  WiFi_ConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  WiFi_ConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID

unsigned long getWiFi_Status(){
	return WiFi_Status;
}

void setWiFi_Status(unsigned long a){
	WiFi_Status = a;
}

unsigned long getWiFi_IP(){
	return WiFi_IP;
}

unsigned long getConnectedDevice_IP(){
	return ConnectedDevice_IP;
}
//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
   switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SetNetworkStatus(WIFI_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'-Applications
            // can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(WiFi_ConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(WiFi_ConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

#ifdef SIMPLELINK_EVENT_MESSAGE
        /*    UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s ,"
                        "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      WiFi_ConnectionSSID,WiFi_ConnectionBSSID[0],
                      WiFi_ConnectionBSSID[1],WiFi_ConnectionBSSID[2],
                      WiFi_ConnectionBSSID[3],WiFi_ConnectionBSSID[4],
                      WiFi_ConnectionBSSID[5]);*/
#endif
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            ClearNetworkStatus(WIFI_CONNECTION | WIFI_IP_AQUIRED );

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION
#ifdef SIMPLELINK_EVENT_MESSAGE
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
            	pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;
            	/*UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           WiFi_ConnectionSSID,WiFi_ConnectionBSSID[0],
                           WiFi_ConnectionBSSID[1],WiFi_ConnectionBSSID[2],
                           WiFi_ConnectionBSSID[3],WiFi_ConnectionBSSID[4],
                           WiFi_ConnectionBSSID[5]);*/
            }
            else
            {
               /* UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           WiFi_ConnectionSSID,WiFi_ConnectionBSSID[0],
                           WiFi_ConnectionBSSID[1],WiFi_ConnectionBSSID[2],
                           WiFi_ConnectionBSSID[3],WiFi_ConnectionBSSID[4],
                           WiFi_ConnectionBSSID[5]);*/
            }
#endif
            memset(WiFi_ConnectionSSID,0,sizeof(WiFi_ConnectionSSID));
            memset(WiFi_ConnectionBSSID,0,sizeof(WiFi_ConnectionBSSID));
        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
		{
        	//connected as an AP/P2P
			// when device is in AP mode and any client connects to device cc3xxx
			SetNetworkStatus(WIFI_CONNECTION);

			//
			// Information about the connected client (like SSID, MAC etc) will be
			// available in 'slPeerInfoAsyncResponse_t' - Applications
			// can use it if required
			//
			// slPeerInfoAsyncResponse_t *pEventData = NULL;
			// pEventData = &pSlWlanEvent->EventData.APModeStaConnected;
			//
			 break;
		}

        case SL_WLAN_STA_DISCONNECTED_EVENT:
		{
        	//disconnected as an AP/P2P
			slWlanConnectAsyncResponse_t*  pEventData = NULL;

			ClearNetworkStatus(WIFI_CONNECTION | WIFI_IP_AQUIRED );
			pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

			// If the user has initiated 'Disconnect' request,
			//'reason_code' is SL_USER_INITIATED_DISCONNECTION
#ifdef SIMPLELINK_EVENT_MESSAGE
			if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
			{
			//    UART_PRINT("Device disconnected from the AP on application's request \n\r");
			}
			    else
			{
			  //  UART_PRINT("Device disconnected from the AP on an ERROR..!! \n\r");
			}
#endif
			break;
		}

        case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
		{

			//smart configuration start
			SetNetworkStatus(WIFI_SMARTCONFIG_START);
			break;
		}

        case SL_WLAN_SMART_CONFIG_STOP_EVENT:
		{
        	//smart configure stop
			ClearNetworkStatus(WIFI_SMARTCONFIG_START);
			break;
		}

        case SL_WLAN_P2P_DEV_FOUND_EVENT:
        {

			//found P2P device
			SetNetworkStatus(WIFI_P2P_DEV_FOUND);
			break;
        }
        case SL_WLAN_P2P_NEG_REQ_RECEIVED_EVENT:
        {
        	// negotiate request received
        	SetNetworkStatus(WIFI_P2P_REQ_RECEIVED);
        	break;
        }
        case SL_WLAN_CONNECTION_FAILED_EVENT:
        {
        	// Connection failure
        	SetNetworkStatus(WIFI_CONNECTION_FAILED);
        	break;
        }
        default:
        {
#ifdef SIMPLELINK_EVENT_MESSAGE
         /*   UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);*/
#endif
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
   switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SetNetworkStatus(WIFI_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            //Gateway IP address, Device IP address and DNS server address
            WiFi_GatewayIP = pEventData->gateway;
            WiFi_IP = pEventData->ip;
            WiFi_DNS = pEventData->dns;
#ifdef SIMPLELINK_EVENT_MESSAGE
        /*    UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
            "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));*/
#endif
            break;
        }
        case SL_NETAPP_IP_LEASED_EVENT:
        {
        	// Network IP leased in AP or P2P
        	SetNetworkStatus(WIFI_IP_LEASED);

        	ConnectedDevice_IP = (pNetAppEvent)->EventData.ipLeased.ip_address;

        	break;
        }

        case SL_NETAPP_IP_RELEASED_EVENT:
        {
          	// Network IP released in AP or P2P
           	break;
        }
        default:
        {
#ifdef SIMPLELINK_EVENT_MESSAGE
      /*      UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);*/
#endif
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    // Unused in this application
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
#ifdef SIMPLELINK_EVENT_MESSAGE
 /*  UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);*/
#endif
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    //
    // This application doesn't work w/ socket - Events are not expected
    //
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status)
            {
                case SL_ECLOSE:
#ifdef SIMPLELINK_EVENT_MESSAGE
            /*        UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n",
                                    pSock->socketAsyncEvent.SockTxFailData.sd);*/
#endif
                    break;
                default:
#ifdef SIMPLELINK_EVENT_MESSAGE
           /*         UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);*/
#endif
                  break;
            }
            break;

        default:
#ifdef SIMPLELINK_EVENT_MESSAGE
        	//UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
#endif
          break;
    }

}

//This function initializes
//the internal variables related to the network connection
void InitializeWiFiVariables()
{
    WiFi_Status = 0;
    WiFi_GatewayIP = 0;
    memset(WiFi_ConnectionSSID,0,sizeof(WiFi_ConnectionSSID));
    memset(WiFi_ConnectionBSSID,0,sizeof(WiFi_ConnectionBSSID));
}

//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************

long ConfigureSimpleLinkToDefaultState()
{
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode
    if (ROLE_STA != lMode)
    {
        if (ROLE_AP == lMode)
        {
            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while(!CheckAnyNetworkStatus(WIFI_IP_AQUIRED))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
            }
        }

        // Switch to STA role and restart
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again
        if (ROLE_STA != lRetVal)
        {
            // We don't want to proceed if the device is not coming up in STA-mode
            ASSERT_ON_ERROR( - 1);
        }
    }

    // Set connection policy to Auto + SmartConfig
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
//    lRetVal = sl_WlanProfileDel(0xFF);
//    ASSERT_ON_ERROR(lRetVal);



    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait
        while(CheckAnyNetworkStatus(WIFI_CONNECTION))
        {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask, sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(200);
    ASSERT_ON_ERROR(lRetVal);

    InitializeWiFiVariables();

    return lRetVal; // Success
}

//This function sets the flag in the WiFi_Status defined in status
void SetNetworkStatus(long status)
{
	WiFi_Status |= status;
}

//This function clears the flag in the WiFi_Status defined in status
void ClearNetworkStatus(long status)
{
	WiFi_Status &= ~status;
}

//This function checks if any of the flag in the WiFi_Status defined by status
long CheckAnyNetworkStatus(long status)
{
	return (WiFi_Status & status) != 0;
}

//This function checks if all of the flags in the WiFi_Status defined by status
long CheckAllNetworkStatus(long status)
{
	return (WiFi_Status & status) == status;
}

unsigned long GetGatewayIP()
{
	return WiFi_GatewayIP;
}
