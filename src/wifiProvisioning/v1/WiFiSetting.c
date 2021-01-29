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

#include "osi.h"
#include "fs.h"
#include "WiFiSetting.h"
#include "WiFiConnection.h"
#include "WiFiEvent.h"


/*******************************************************************
 *******************************************************************
 * read flash memory
 * return SSID if flash memory stored it
 * return empty pointer if no ssid stored
 *******************************************************************
 *******************************************************************/
int8_t GetWiFiInfoFromFlash(WiFi_INFO_HANDLE_t wifi_info_handle){

	unsigned long ulToken = 0;
	long lFileHandle = 0;
	long lRetVal = -1;

	sl_Start(NULL,NULL,NULL);	//necessary for all sl functions

	//open need sl_start
	lRetVal = sl_FsOpen((uint8_t*)USER_FILE_NAME, FS_MODE_OPEN_READ, &ulToken, &lFileHandle);

	if(lRetVal < 0){
		lRetVal = sl_FsClose(&lFileHandle, 0, 0, 0);
		return FILE_OPEN_READ_FAILED;
	}
	//SSIDPasscode need malloc before use. return value is read size
	lRetVal = sl_FsRead(lFileHandle, 0, (unsigned char *)(wifi_info_handle->SSIDPasscode), SSIDMAXLENGTH + PASSCODEMAXLENGTH);

	if (lRetVal < 0){
		lRetVal = sl_FsClose(&lFileHandle, 0, 0, 0);
		return FILE_READ_FAILED;
	}

	lRetVal = sl_FsClose(&lFileHandle,0,0,0);
	if (SL_RET_CODE_OK != lRetVal)
	{
		return FILE_CLOSE_ERROR;
	}

	//success
	sl_Stop(0xFF);
	return SUCCESS; //return structure of wifi_info

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
 * List out all wifi around
 * compare input ssid with all listed ssids
 * return ssid with passcode if input ssid matches any other
 * return -1 if input ssid matches no other
 *******************************************************************
 *******************************************************************/
int8_t SearchWiFiList(WiFi_INFO_HANDLE_t wifi_info_handle){
	ConfigureSimpleLinkToDefaultState(); //necessary for WIFI SCAN
	sl_Start(NULL,NULL,NULL);	//necessary for sl functions
	uint32_t intervalInSeconds = SCANPERIOD; //SCAN each 5 ms

	//start SCAN
	sl_WlanPolicySet(SL_POLICY_SCAN,SL_SCAN_ENABLE, (uint8_t *)&intervalInSeconds,sizeof(intervalInSeconds));
	osi_Sleep(100); //let the SCAN finish before get SCAN result
	Sl_WlanNetworkEntry_t netEntries[10]; //get 10 result
	int16_t resultsCount = 0;

	//polling until found SCAN result

	while(resultsCount == 0){
		resultsCount = sl_WlanGetNetworkList(0,10,&netEntries[0]); //need simplelink to be configured as default state
	}

	if(strlen(wifi_info_handle->SSID) == 0 || strlen(wifi_info_handle->SSID) > SSIDMAXLENGTH){
		sl_Stop(0xFF);
		return ERROR; //error, caused by other function called before
	}else{

		int i;
		int8_t result; // 0 equal, -1 unequal
		//comparing each result till match or no match
		for(i=0; i< resultsCount; i++)
		{

			result = strcmp(netEntries[i].ssid, wifi_info_handle->SSID); //compare two Strings
			  if(result == MATCH){
				  //match
				  sl_WlanPolicySet(SL_POLICY_SCAN,SL_SCAN_DISABLE,0,0);
				  sl_Stop(0xFF);
				  return true;
			  }
		}
		//stop SCAN
		sl_WlanPolicySet(SL_POLICY_SCAN,SL_SCAN_DISABLE,0,0);
		sl_Stop(0xFF);	//stop simplelink
		return false; // no match
	}

}

/*******************************************************************
 *******************************************************************
 * configure device to SA mode
 * wait for wifi profiles inputs
 * user must input wifi profile as ssid and passcode during the operation of this function
 * otherwise, the function will suspend forever
 * return SSID and passcode
 *******************************************************************
 *******************************************************************/
int8_t Configure_AP(WiFi_INFO_HANDLE_t wifi_info_handle){

	// Configure the networking mode and ssid name(for AP mode)
	int32_t lMode;
	unsigned long status;
	long   lRetVal = -1;
	lRetVal = ConfigureSimpleLinkToDefaultState(); //set simpleLink to default state
	lRetVal = sl_Start(NULL,NULL,NULL);
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


	//listen to port 10 for input router infomation
	TCPHandle_t listenHandle;
	TCPHandle_t listenPortHandle;

	listenPortHandle = TCPStartListen(LISTENPORT);
	listenHandle = TCPListen(listenPortHandle, portMAX_DELAY);

	//get and decode SSID
	int32_t error;
	uint8_t SSIDPasscode_Length[4];

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
    uint16_t PasscodeLength = SSIDPasscode_Length[0]*1000 + SSIDPasscode_Length[1]*100 + SSIDPasscode_Length[2]*10 + SSIDPasscode_Length[3];

    error = TCPRead(listenHandle, wifi_info_handle->Passcode, PasscodeLength,  10000); //read passcode

	if(error < 0){
		return TCP_READ_ERROR; // error for debug
	}

	//add string end
	*(wifi_info_handle->Passcode + PasscodeLength) = '\0';

	TCPStopListen(listenPortHandle); //stop TCP listen
	return SUCCESS;
}

/*******************************************************************
 *******************************************************************
 * change device to STA mode and try connection with input ssid and passcode
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

	lRetVal = ConfigureSimpleLinkToDefaultState(); //configure simpleLink device to default state

	// Switch to STA role and restart
	lMode = Start_WiFi_Device(WIFI_STATION); //to STA
	if(lMode == WIFI_STATION){
		Set_Tx_Power(WIFI_STATION,10); //power configuration
	}


	//STA connect to router
	lRetVal =  STA_Connect(wifi_info_handle->SSID, wifi_info_handle->Passcode, WIFI_SEC_TYPE_WPA, 10000); //connect to router
	if(lRetVal >=0 ){
		return true;//connected
	}else{
		sl_Stop(0xFF);//if error, need stop simplelink
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
 *******************************************************************
 *******************************************************************/
void CloseWifiInfo(WiFi_INFO_HANDLE_t wifi_info_handle){
	//free allocated memory
	free(wifi_info_handle);
}

/*******************************************************************
 *******************************************************************
 * save the updated WiFi profile into flash memory
 * return 0 on success
 * return -1 on failure
 *******************************************************************
 *******************************************************************/
int8_t UpdateWiFiInfoToFlash(WiFi_INFO_HANDLE_t wifi_info_handle){
	unsigned long ulToken = 0;
	long lFileHandle = 0;
	long lRetVal = -1;

    lRetVal = sl_FsOpen(USER_FILE_NAME,
                FS_MODE_OPEN_CREATE(128, _FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE),
                        &ulToken,
						&lFileHandle);
    //used after sl_start.
    if(lRetVal < 0){
        // File may already be created
        lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
    }
    else
    {
        // close the user file
        lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
        if (SL_RET_CODE_OK != lRetVal){
           //FILE_CLOSE_ERROR
        	return FILE_CLOSE_ERROR;
        }
    }

    //  open a user file for writing
    lRetVal = sl_FsOpen((unsigned char *)USER_FILE_NAME,
                        FS_MODE_OPEN_WRITE,
                        &ulToken,
						&lFileHandle);
    if(lRetVal < 0){
        lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
        //FILE_OPEN_WRITE_FAILED
        return FILE_OPEN_WRITE_FAILED;
    }

    //write
    lRetVal = sl_FsWrite(lFileHandle, 0, (unsigned char *)(&wifi_info_handle->SSIDPasscode), SSIDMAXLENGTH + PASSCODEMAXLENGTH);
		if (lRetVal < 0){
			lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
			//FILE_WRITE_FAILED
			return FILE_WRITE_FAILED;
		}

	// close the user file
    lRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
	 if (SL_RET_CODE_OK != lRetVal){
	  //FILE_CLOSE_ERROR
		 return FILE_CLOSE_ERROR;
	 }

	 return true; // success
}

