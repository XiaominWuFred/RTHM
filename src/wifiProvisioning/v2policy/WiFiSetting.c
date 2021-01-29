/*******************************************************************
 *******************************************************************
 * WiFiSetting.c
 *
 *  Created on: Jan 2, 2018
 *      Author: xwu
 *******************************************************************
 *******************************************************************/

// Standard includes.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "osi.h"
#include "fs.h"
#include "WiFiSetting.h"
#include "WiFiConnection.h"
#include "WiFiEvent.h"
#include "wlan.h"


bool Configuring;	//bits for blocking push button ISR miss entering
bool Device_on;		//bits to track simplelink device status

void setDeviceOn(){
	Device_on = true;
}

void setDeviceOff(){
	Device_on = false;
}

bool IsDeviceOn(){
	return Device_on;
}

/*******************************************************************
 *******************************************************************
 * creatProfile
 *
 * allocate memory for storing profile to be send
 *
 * return
 * wifi_profile_handle: an pointer to an allocated memory space holding profile got
 *******************************************************************
 *******************************************************************/
WiFi_PROFILE_HANDLE_t creatProfile(){
	WiFi_PROFILE_HANDLE_t wifi_profile_handle;
	wifi_profile_handle = (WiFi_PROFILE_HANDLE_t)malloc(sizeof(WiFi_PROFILE_PARAM_t));
	wifi_profile_handle->SSID = malloc(sizeof(SSIDMAXLENGTH));
	return wifi_profile_handle;
}

/*******************************************************************
 *******************************************************************
 * closeProfile
 *
 * free allocated memory for storing profile to be send
 *
 * function parameters:
 * handle: an pointer to an allocated memory space holding profile got
 *
 * return error checking value
 *
 *******************************************************************
 *******************************************************************/
int8_t closeProfile(WiFi_PROFILE_HANDLE_t handle){
	free(handle->SSID);
	free(handle);
	return SUCCESS;
}


/*******************************************************************
 *******************************************************************
 * send_profile
 *
 * send all saved WiFi profile with corresponding index to connected device.
 * profile sending structure:
 * one byte index for the stored profile is sent first
 * two bytes SSID length for the stored SSID is sent after
 * SSID is sent then.
 * profiles are sent one by one. for example: 0 SSID0
 * 											  1 SSID1
 * 											  2 SSID2
 * 											  3 SSID3
 *
 * function parameters:
 * handle: an pointer to an allocated memory space holding profile got
 * device_IP: a unsigned long data for device to send profiles to
 * port: 16 bits parameter, the port used by the device receiving profiles
 * timeout: 16 bits parameter, the time in ms the function will wait if no response
 *
 * return error checking value
 *
 * usually used inside
 * int8_t Configure_AP(WiFi_INFO_HANDLE_t wifi_info_handle)
 *
 *******************************************************************
 *******************************************************************/
int8_t send_profile(WiFi_PROFILE_HANDLE_t handle, unsigned long device_IP, uint16_t port, uint16_t timeout){
	TCPHandle_t TCPhandle;
	uint8_t i;
	int32_t ReValue;
	uint8_t index;

	//accessing 7 profile position with index i
	for(i = 0; i < 7; i++){
		//get profile
		handle->security = sl_WlanProfileGet(i,handle->SSID, &(handle->SSIDlength), NULL, &(handle->secParams), NULL, 0);
		//if profile at this index exists
		if(handle->security >= 0){
			//creat TCP under AP mode with device_IP
			TCPhandle = TCPOpenConnectionByIP(device_IP, port, timeout); //connect to another laptop by IP

			if(TCPhandle < 0){
				return TCP_CONNECTION_FAIL;
			}
			//send one byte index
			index = i + 48;
			ReValue = TCPWrite(TCPhandle, &index, 1, 2000);
			if(ReValue < 0){
				return TCP_SEND_FAIL;
			}
			//send length
			ReValue = TCPWrite(TCPhandle, &handle->SSIDlength, 2, 2000);
			if(ReValue < 0){
				return TCP_SEND_FAIL;
			}
			//send SSID
			ReValue = TCPWrite(TCPhandle, handle->SSID, (handle->SSIDlength), 2000);
			if(ReValue < 0){
				return TCP_SEND_FAIL;
			}
			//close TCP
			ReValue = TCPCloseConnection(TCPhandle);
			if(ReValue < 0){
				return TCP_CLOSE_FAIL;
			}
		}
	}

	return SUCCESS;


}

/***************************************************************************
 * *************************************************************************
 * send_connected_SSID
 * parameters: WiFi_ConnectionSSID : pointer to unsigned char data, of SSID of
 * 			   connected wlan. i.e. the SSID of remote AP device.
 * 			   device_IP : the unsigned long type IP address of connected Device
 * 			   port : the 16 bits value of port the connected device used to listen
 * 			   timeout : 16 bits value of timeout
 *
 * precondition: the device is at STA mode and connected with wlan, also connected with
 * 				 a third terminal: a laptop for example
 * 				 the third device and this device should connect to same AP device.
 *
 * return: error code
 * *************************************************************************
 ***************************************************************************/
int8_t send_connected_SSID( unsigned char * WiFi_ConnectionSSID, unsigned long device_IP, uint16_t port, uint16_t timeout){

	TCPHandle_t handle;
	int8_t ReValue;
	uint16_t length = strlen(WiFi_ConnectionSSID);

	//starting a TCP connection

	//IP,PORT NUMBER, TIME OUT
	handle = TCPOpenConnectionByIP(device_IP, port, timeout); //connect to another laptop by IP

	if(handle < 0){
		return TCP_CONNECTION_FAIL;
	}


	ReValue = TCPWrite(handle, 0x20, 1, 2000);

	if(ReValue < 0){
		return TCP_SEND_FAIL;
	}

	ReValue = TCPWrite(handle, &length, 2, 2000);

	if(ReValue < 0){
		return TCP_SEND_FAIL;
	}

	ReValue = TCPWrite(handle, WiFi_ConnectionSSID, length, 2000);

	if(ReValue < 0){
		return TCP_SEND_FAIL;
	}

	ReValue = TCPCloseConnection(handle);
	if(ReValue < 0){
		return TCP_CLOSE_FAIL;
	}

	return SUCCESS;
}


/*******************************************************************
 *******************************************************************
 * AutoConnectFromProfile
 * set auto connection policy
 * if one of saved profile connected successfully
 * connected signal return
 * if no saved profile or saved profiles all cannot connected
 * no connection signal return
 *
 *******************************************************************
 *******************************************************************/
int8_t AutoConnectFromProfile(){
	//unsigned char policyVal;
	unsigned long ulToken = 0;
	unsigned long status;
	long lFileHandle = 0;
	long lRetVal = -1;
	//long lMode;

	if(IsDeviceOn()){
		sl_Stop(0xFF);
	}
	lRetVal = ConfigureSimpleLinkToDefaultState(); //set simpleLink to default state

	sl_Start(NULL,NULL,NULL);	//necessary for all sl functions
	setDeviceOn();

    //set AUTO policy
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1,0,0,0,0), NULL, 0);
    if(lRetVal == -1){
    	return POLICY_SET_FIALED;
    }

    //wait until IP is acquired
    TickType_t TimeCounter = 0;
	while (!CheckAllNetworkStatus(WIFI_CONNECTION | WIFI_IP_AQUIRED) && (TimeCounter<2000))
	{
		vTaskDelay(500);
		TimeCounter += 500;
   	}

	osi_Sleep(2000); //for status check correctness

	if (CheckAllNetworkStatus(WIFI_CONNECTION | WIFI_IP_AQUIRED))
	{
		ClearNetworkStatus(WIFI_CONNECTION | WIFI_IP_AQUIRED);

		lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0,0,0,0,0), 0, 0);
		if(lRetVal == -1){
			sl_Stop(0xFF);
			setDeviceOff();
		    return POLICY_CLEAR_FIALED;
		}

		return SUCCESS;
	}
	else
	{
		sl_Stop(0xFF);
		setDeviceOff();
		return NO_USABLE_WIFI_INFO_CONTAINED;
	}

}

/*******************************************************************
 *******************************************************************
 * initial simplelink
 * should be called at first.
 *******************************************************************
 *******************************************************************/
int32_t Initial_SimpleLink(){
	int32_t lRetVal; //return variable

	// Start the SimpleLink Host
	lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
	if(lRetVal < 0){
		ERR_PRINT(lRetVal); //error
		return lRetVal; //negative number
	}

	return lRetVal; //success,positive return
}

/*******************************************************************
 *******************************************************************
 * used to generate random wifi ssid for device to be found
 *
 * parameter:
 *  Ssid: pointer to space allocated for holding generated SSID
 *******************************************************************
 *******************************************************************/
void GenerateSSID(uint8_t* Ssid){
	uint8_t name[] = "BMEdevice  ";
	memcpy(Ssid, name,sizeof(name));
	uint8_t randomeNum[2];
	srand(time(NULL));//set random seed
	randomeNum[0] = 48+ (rand() % 10);	//generate random number first digit
	randomeNum[1] = 48+ (rand() % 10);	//generate random number second digit
	memcpy(Ssid + sizeof(name) - 3, randomeNum, sizeof(randomeNum)); //put random number into ssid
}


/*******************************************************************
 *******************************************************************
 * configure device to SA mode
 * wait for wifi profiles inputs
 * user must input wifi profile as ssid and passcode during the operation of this function
 * otherwise, the function will suspend forever
 *
 * function parameters:
 * wifi_info_handle: an pointer to an allocated memory space holding wifi infomation
 *
 * return SSID and passcode
 *******************************************************************
 *******************************************************************/
int8_t Configure_AP(WiFi_INFO_HANDLE_t wifi_info_handle){

	// Configure the networking mode and ssid name(for AP mode)
	int32_t lMode;
	unsigned long status;
	long   lRetVal = -1;

	//check simple link device on or off
	if(IsDeviceOn()){
		sl_Stop(0xFF);
		setDeviceOff();
	}else{

	}

	lRetVal = ConfigureSimpleLinkToDefaultState(); //set simpleLink to default state
	lRetVal = sl_Start(NULL,NULL,NULL); //start sl device
	setDeviceOn();	//tracking the sl device on or off status
	if (lRetVal < 0){
		//"Failed to start the device \n\r"
		return SL_START_ERROR;
	}

	if(lRetVal != ROLE_AP){
		uint8_t Ssid[11]; //used for this device's ssid
		GenerateSSID(Ssid);
		lRetVal = sl_WlanSetMode(ROLE_AP); //to AP mode
		lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, strlen(Ssid),(unsigned char*)Ssid);
		//Device is configured in AP mode

		//Restart Network processor
		lRetVal = sl_Stop(SL_STOP_TIMEOUT);
		// reset status bits
		setWiFi_Status(0);
		lMode = sl_Start(NULL,NULL,NULL);
		if( lMode != ROLE_AP){
			//Unable to set AP mode, exiting Application...
			sl_Stop(SL_STOP_TIMEOUT);
			setDeviceOff();//tracking the sl device on or off status
			return SET_AP_ERROR;
		}
	}

	status = 0;
	status = getWiFi_Status();

	while((status && WIFI_IP_AQUIRED) == 0){
	  //looping till ip is acquired
		status = getWiFi_Status();
		_SlNonOsMainLoopTask();
	}

	//"Connect a client to Device\n\r"
	status = 0;
	status = getWiFi_Status();
	unsigned long re = status & WIFI_IP_LEASED;
	while(re == 0){
		//wating for the client to connect
		status = getWiFi_Status();
		re = status & WIFI_IP_LEASED;
	}//"Client is connected to Device\n\r"

	WiFi_PROFILE_HANDLE_t profileHandle = creatProfile();
	int8_t RValue;
	unsigned long connectedIP = 0;

	connectedIP = getConnectedDevice_IP();

	if(connectedIP == 0){
		return DEVICE_IP_NOT_AQUIRED;
	}

	//send profiles to connected device
	RValue = send_profile(profileHandle, connectedIP, 5000, 10000);
	//close profile for next time allocation
	closeProfile(profileHandle);
	//listen to port 10 for input router infomation
	TCPHandle_t listenHandle;
	TCPHandle_t listenPortHandle;
	listenPortHandle = TCPStartListen(LISTENPORT);
	listenHandle = TCPListen(listenPortHandle, portMAX_DELAY);

	//get and decode SSID
	int32_t error;
	uint8_t SSIDPasscode_Length[4];
	uint8_t index;

	error = TCPRead(listenHandle, &index, 1,  10000);//read SSID length
	if(error < 0){
		return TCP_READ_ERROR;
	}

	wifi_info_handle->index = index;

	error = TCPRead(listenHandle, SSIDPasscode_Length, sizeof(SSIDPasscode_Length),  10000);//read SSID length
	if(error < 0){
		return TCP_READ_ERROR;
	}
    uint16_t SSIDLength = SSIDPasscode_Length[0]*1000 + SSIDPasscode_Length[1]*100 + SSIDPasscode_Length[2]*10 + SSIDPasscode_Length[3];

    error = TCPRead(listenHandle, wifi_info_handle->SSID, SSIDLength,  10000); //read SSID
	if(error < 0){
		return TCP_READ_ERROR;
	}
	//add string end
	*(wifi_info_handle->SSID + SSIDLength) = '\0';

	//get and decode Passcode
	error = TCPRead(listenHandle, SSIDPasscode_Length, sizeof(SSIDPasscode_Length),  10000); //read passcode length
	if(error < 0){
		return TCP_READ_ERROR;
	}
    int32_t PasscodeLength = SSIDPasscode_Length[0]*1000 + SSIDPasscode_Length[1]*100 + SSIDPasscode_Length[2]*10 + SSIDPasscode_Length[3];

    error = TCPRead(listenHandle, wifi_info_handle->Passcode, PasscodeLength,  10000); //read passcode

	if(error < 0){
		return TCP_READ_ERROR; // error for debug
	}

	//add string end
	*(wifi_info_handle->Passcode + PasscodeLength) = '\0';

	TCPStopListen(listenPortHandle); //stop TCP listen
	TCPStopListen(listenHandle); //stop TCP listen

	return SUCCESS;
}

/*******************************************************************
 *******************************************************************
 * change device to STA mode and try connection with input ssid and passcode
 *
 * function parameters:
 * wifi_info_handle: an pointer to an allocated memory space holding wifi infomation
 *
 * return 0 on success
 * return -1 or other negative number on failure
 *******************************************************************
 *******************************************************************/
int8_t Configure_STA(WiFi_INFO_HANDLE_t wifi_info_handle){
	//disconnect previous network
	int32_t lRetVal, lMode;
	lRetVal = sl_WlanDisconnect();
	if(0 == lRetVal){
		// Wait
		while(CheckAnyNetworkStatus(WIFI_CONNECTION)){ // wait for all network disconnect
			#ifndef SL_PLATFORM_MULTI_THREADED
				_SlNonOsMainLoopTask();
			#endif
		}
	}

	lRetVal = sl_Stop(0xFF); // stop the simpleLink device for follwing configuration
	if(lRetVal < 0){
		return false; //error
	}

	setDeviceOff();

	lRetVal = ConfigureSimpleLinkToDefaultState(); //configure simpleLink device to default state

	// Switch to STA role and restart
	lMode = Start_WiFi_Device(WIFI_STATION); //to STA
	if(lMode == WIFI_STATION){
		Set_Tx_Power(WIFI_STATION,10); //power configuration
	}

	setDeviceOn();

	//STA connect to router
	lRetVal =  STA_Connect(wifi_info_handle->SSID, wifi_info_handle->Passcode, WIFI_SEC_TYPE_WPA, 10000); //connect to router
	if(lRetVal >=0 ){
		return true;//connected
	}else{
		sl_Stop(0xFF);//if error, need stop simplelink
		setDeviceOff();
		return false;//error
	}
}

/*******************************************************************
 *******************************************************************
 * create space for wifi_info
 * should be called before all other method using wifi_info_handle
 *******************************************************************
 *******************************************************************/
WiFi_INFO_HANDLE_t CreateWifiInfo(){
	WiFi_INFO_HANDLE_t wifi_info_handle;
	wifi_info_handle = malloc(sizeof(WiFi_INFO_t));//allocate space for WiFi_INFO
	wifi_info_handle->SSID = wifi_info_handle->SSIDPasscode;
	wifi_info_handle->Passcode = wifi_info_handle->SSIDPasscode + SSIDMAXLENGTH;
	return wifi_info_handle;
}

/*******************************************************************
 *******************************************************************
 * to free allocated memory for wifi_info
 * used when all configuration finished, or system close
 *
 * function parameters:
 * wifi_info_handle: an pointer to an allocated memory space holding wifi infomation
 *******************************************************************
 *******************************************************************/
void CloseWifiInfo(WiFi_INFO_HANDLE_t wifi_info_handle){
	//free allocated memory
	free(wifi_info_handle);
}

/*******************************************************************
 *******************************************************************
 * save the updated WiFi profile into flash memory
 *
 * function parameters:
 * wifi_info_handle: an pointer to an allocated memory space holding wifi infomation
 *
 * return 0 on success
 * return -1 on failure
 *******************************************************************
 *******************************************************************/
int8_t AddNewProfile(WiFi_INFO_HANDLE_t wifi_info_handle){
	int16_t ReValue;

    //Add Profile
    SlSecParams_t secParams;
    secParams.Key = wifi_info_handle->Passcode;
    secParams.KeyLen = strlen(secParams.Key);
    secParams.Type = SL_SEC_TYPE_WPA_WPA2;

	if(IsDeviceOn()){

	}else{
		sl_Start(NULL,NULL,NULL);
		setDeviceOn();
	}

	ReValue = sl_WlanProfileAdd(wifi_info_handle->SSID, strlen(wifi_info_handle->SSID), NULL, &secParams, NULL, 1, NULL);

	if(ReValue == SL_ERROR_PREFERRED_NETWORK_LIST_FULL){
		//profile is full
		ReValue = sl_WlanProfileDel(wifi_info_handle->index); //delete stored profiles

		if(ReValue < 0){
			//delete error
			return PROFILE_DELETE_ERROR;
		}

		ReValue = sl_WlanProfileAdd(wifi_info_handle->SSID, strlen(wifi_info_handle->SSID), NULL, &secParams, NULL, 1, NULL);

		if(ReValue < 0){
			//add error
			return PROFILE_ADD_ERROR;
		}
	}else if(ReValue < 0){
		//add error
		return PROFILE_ADD_ERROR;
	}

	return true; // success
}

