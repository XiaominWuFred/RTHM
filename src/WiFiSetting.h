/*
 * WiFiHeader.h
 *
 *  Created on: Jan 2, 2018
 *      Author: xwu
 */

#ifndef SOURCE_WIFISETTING_H_
#define SOURCE_WIFISETTING_H_

#include <stdbool.h>
#include "hw_types.h"
#include "simplelink.h"
#include "common.h"

#define LISTENPORT				10					//Chip port number used for listening
#define USER_FILE_NAME          "WiFiprofile.txt"	//file name used for flash memory access
#define SSIDMAXLENGTH			16					//maximum SSID length
#define PASSCODEMAXLENGTH		16					//maximum Passcode length
#define SL_SCAN_ENABLE  		1					//enable Simple link scan
#define SL_SCAN_DISABLE  		0					//disable simple link scan
#define SCANPERIOD				5					//scan period in ms

#define SUCCESS					0					//SUCCESS
#define POLICY_SET_FIALED		-11					//policy set failed
#define POLICY_CLEAR_FIALED		-12					//policy clear failed
#define NO_USABLE_WIFI_INFO_CONTAINED	-22			//no right wifi info in the profile
#define PROFILE_ADD_ERROR		-17					//adding profile error
#define PROFILE_DELETE_ERROR	-19					//deleting profile error
#define ERROR					-1					//error
#define FILE_CLOSE_ERROR		-2					//file colse failed
#define FILE_OPEN_WRITE_FAILED	-3					//file write open failed
#define FILE_WRITE_FAILED		-4					//file write failed
#define FILE_OPEN_READ_FAILED	-5					//FILE_OPEN_READ_FAILED
#define FILE_READ_FAILED		-6					//FILE_READ_FAILED
#define	SL_START_ERROR			-7					//SL_START_ERROR
#define SET_AP_ERROR			-8					//SET_AP_ERROR
#define TCP_READ_ERROR			-9					//TCP_READ_ERROR
#define MATCH					0					//SSID match
#define NONMATCH				-1					//SSID not match
#define DEVICE_IP_NOT_AQUIRED	-104				//device IP not aquired
#define TCP_CONNECTION_FAIL		-101				//TCP connect fail
#define TCP_SEND_FAIL			-102				//TCP send fail
#define TCP_CLOSE_FAIL			-103				//TCP close fail


typedef struct{
	uint8_t SSIDPasscode[SSIDMAXLENGTH + PASSCODEMAXLENGTH];	//buffer used to read and write flash memory
	uint8_t* SSID;									//pointer to SSID
	int8_t* Passcode;								//pointer to Passcode
	uint8_t index;									//used to replace profile
}WiFi_INFO_t;

typedef WiFi_INFO_t* WiFi_INFO_HANDLE_t;


typedef struct{
	uint8_t index;
	int8_t security;
	uint8_t* SSID;
	uint16_t SSIDlength;
	SlSecParams_t secParams;
}WiFi_PROFILE_PARAM_t;

typedef WiFi_PROFILE_PARAM_t* WiFi_PROFILE_HANDLE_t;

int8_t AutoConnectFromProfile();	//get wifi profile from flash
int32_t Initial_SimpleLink();												  	//initial simple link
void GenerateSSID(uint8_t* Ssid);												//generate random SSID for device
int8_t Configure_AP(WiFi_INFO_HANDLE_t wifi_info_handle);			//configure AP and waiting input WiFi profile
int8_t Configure_STA(WiFi_INFO_HANDLE_t wifi_info_handle);						//configure STA and connect to router(AP)
WiFi_INFO_HANDLE_t CreateWifiInfo();											//create WiFi info data field
void CloseWifiInfo(WiFi_INFO_HANDLE_t wifi_info_handle);						//delete WiFi info data field
int8_t AddNewProfile(WiFi_INFO_HANDLE_t wifi_info_handle);				//update new WiFi profile into flash
WiFi_INFO_HANDLE_t getWifi_info_handle(); //getter

//send connected wlan SSID to connected device, use TCP
int8_t send_connected_SSID( unsigned char * WiFi_ConnectionSSID, unsigned long device_IP, uint16_t port, uint16_t timeout);

WiFi_PROFILE_HANDLE_t creatProfile();	//create a profile data feild
//int8_t setProfile(WiFi_PROFILE_HANDLE_t handle, uint8_t index);
int8_t send_profile(WiFi_PROFILE_HANDLE_t handle, unsigned long device_IP, uint16_t port, uint16_t timeout);	//send stored wifi profiles to remote terminal
int8_t closeProfile(WiFi_PROFILE_HANDLE_t handle); //free allocated space for profiles

void setDeviceOn();	//setter
void setDeviceOff(); //setter
bool IsDeviceOn();	//getter

#endif /* SOURCE_WIFISETTING_H_ */
