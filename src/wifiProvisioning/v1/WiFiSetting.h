/*
 * WiFiHeader.h
 *
 *  Created on: Jan 2, 2018
 *      Author: xwu
 */

#ifndef SOURCE_WIFISETTING_H_
#define SOURCE_WIFISETTING_H_

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

typedef struct{
	uint8_t SSIDPasscode[SSIDMAXLENGTH + PASSCODEMAXLENGTH];	//buffer used to read and write flash memory
	uint8_t* SSID;									//pointer to SSID
	uint8_t* Passcode;								//pointer to Passcode
}WiFi_INFO_t;

typedef WiFi_INFO_t* WiFi_INFO_HANDLE_t;

int8_t GetWiFiInfoFromFlash(WiFi_INFO_HANDLE_t wifi_info_handle);	//get wifi profile from flash
int32_t Initial_SimpleLink();												  	//initial simple link
void GenerateSSID(uint8_t* Ssid);												//generate random SSID for device
int8_t SearchWiFiList(WiFi_INFO_HANDLE_t wifi_info_handle);						//list out WiFi around and match
int8_t Configure_AP(WiFi_INFO_HANDLE_t wifi_info_handle);			//configure AP and waiting input WiFi profile
int8_t Configure_STA(WiFi_INFO_HANDLE_t wifi_info_handle);						//configure STA and connect to router(AP)
WiFi_INFO_HANDLE_t CreateWifiInfo();											//create WiFi info data field
void CloseWifiInfo(WiFi_INFO_HANDLE_t wifi_info_handle);						//delete WiFi info data field
int8_t UpdateWiFiInfoToFlash(WiFi_INFO_HANDLE_t wifi_info_handle);				//update new WiFi profile into flash

#endif /* SOURCE_WIFISETTING_H_ */
