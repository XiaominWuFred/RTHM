#pragma once
#ifndef RRSIGNALPROC
#define RRSIGNALPROC

//#include "compatible.h"
#include <stdint.h>

#define FAILURE1	0
#define SUCCESS1	1

// Moveing average filter data
typedef struct ma_status
{
	int16_t	*ma_buffer;	// buffer to hold the signal data points
	int16_t	ma_output;	// the moving average output from the MA filter
	int16_t	ma_offset;	// offset to avoid error accumulation
	uint16_t	ma_order;	// The order of the MA filter. It must be 2^ma_order
	uint16_t	ma_mask;	// mask for offset
	uint16_t	ma_index;	// the index of the next available space for the new data
} ma_status_t;

typedef ma_status_t *ma_handle_t;

// pulse detector data
typedef struct pulse_detector
{
	uint16_t state;	//state of pulse detector
	uint16_t skipdelay;	//number of data points must skip after the pulse is detected
	uint16_t pulsewidth;	//number of points above threshold consecutive
	uint16_t flag;	// flag indicating the detection status
} pulse_detecor_status_t;

typedef pulse_detecor_status_t *pulse_handle_t;

// respiratory pattern detector data
typedef struct resp_detector
{
	uint16_t state;		//state of respiratory pattern detector, inhalation or exhalation onset detection
	uint16_t s_inhale;	//time stamp of the latest detected inhalation onset
	uint16_t min_inhale;	//minimum period for inhalation period
	uint16_t max_inhale;	//maximum period for inhalation period
	uint16_t s_exhale;	//time stamp of the latest detected exhalation onset
	uint16_t min_exhale;	//minimum period for exhalation period
	uint16_t max_exhale;	//maximum period for inhalation period
	uint16_t counter;		//counter for timestamp
} resp_detector_status_t;

typedef resp_detector_status_t *resp_handle_t;



//respiratory detector data
typedef struct rr_detector
{
	ma_handle_t hpHandle;		// 16-point MA high pass filter
	ma_handle_t lpHandle;		// 16-point MA low pass filter
	ma_handle_t detHandle;		// 8-point MA low pass filter 
	pulse_handle_t pulseHandle;	// handle to the pulse detector
	resp_handle_t respHandle;	// handle to the respiratory cycle detector
	int16_t pulse_threshold;		// threshold to remove pulses
	int16_t noise_threshold;		// threshold to remove noises
} rr_detector_status_t;

typedef rr_detector_status_t *rr_handle_t;

// constants for respiratory pattern detection
#define PULSE_NOT_DETECTED			0x00	// flag indicating no pulse is detected
#define PULSE_DETECTED				0x01	// flag indicating the pulse is detected
#define INHALE_PULSE				0x02	// flag indicating the inhale pulse
#define EXHALE_PULSE				0x04	// flag indicating the exhale pulse

/***********************************************************************************
*	Initialize_MA_Filter(uint16_t order)
*	
*	This function initializes a MA filer. 
*
*	input:
*		1. order: It defines the order of the MA filter. The filter order is 2^order
*	output:
*		It returns the handle to the MA filter. If it returns NULL, the initialization
*	fails due to memory allocation.
*
************************************************************************************/
ma_handle_t Initialize_MA_Filter(uint16_t size);


/***********************************************************************************
*	Execute_MA_Filter(ma_handle_t *handle, int16_t data)
*
*   This function performs MA filtering point by point
*
*	input:
*		1. handle: This is the handle to the MA filter
*		2. data: the new data point
*	output:
*		It it is the MA filter output
*
*************************************************************************************/
int16_t Execute_MA_Filter(ma_handle_t handle, int16_t data);


/************************************************************************************
*	Close_MA_Filter(ma_handle_t handle)
*
*	This function stops the MA filter and releases the related memory
*
*	input:
*		1. handle: this is the handle to the MA filter to be destroyed
*	output:
*		None
*
*************************************************************************************/
void Close_MA_Filter(ma_handle_t handle);


/************************************************************************************
*	rr_handle_t Initialize_Pulse_Detector()
*
*	This function initialize the data for pulse detector
*
*	input:
*		None
*	output:
*		None
*		1.handle: the handle to pulse detector
*************************************************************************************/
pulse_handle_t Initialize_Pulse_Detector();


/***********************************************************************************
*	int Execute_Puslse_Detector(pulse_handle_t *handle, int16_t data, int16_t threshold)
*
*   This function detect valid pulses in the data point by point
*
*	input:
*		1. handle: This is the handle to the RR detector
*		2. data: the new data point
		3. threshold: threshold for pulse detection
*	output:
*		handle to the RR detector data
*
*************************************************************************************/
int16_t Execute_Pulse_Detector(pulse_handle_t handle, int16_t data, int16_t threshold);


/************************************************************************************
*	Close_RR_detector(pulse_handle_t handle)
*
*	This function stops the pulse detection and releases the memory
*
*	input:
*		1. handle: this is the handle to the pulse detector to be destroyed
*	output:
*		None
*
*************************************************************************************/
void Close_Pulse_Detector(pulse_handle_t handle);


/************************************************************************************
*	rr_handle_t Initialize_Resp_Detector(uint16_t min_inhale, uint16_t max_inhale, uint16_t min_exhale, uint16_t max_exhale)
*
*	This function initialize the data for pulse detector
*
*	input:
*		1.min_inhale: minimum period for inhalation period
*		2.max_inhale: maximum period for inhalation period
*		3.min_exhale: minimum period for exhalation period
*		4.max_exhale: maximum period for exhalation period
*	output:
*		None
*		1.handle: the handle to Respipratory detector
*************************************************************************************/
resp_handle_t Initialize_Resp_Detector(uint16_t min_inhale, uint16_t max_inhale, uint16_t min_exhale, uint16_t max_exhale);


/***********************************************************************************
*	int Execute_Resp_Detector(pulse_handle_t *handle, int16_t data)
*
*   This function detect valid pulses in the data point by point
*
*	input:
*		1. data: zero - no pulse; none zero - pulse
*	output:
*		zero. no pulse
*		INHALATION_START. onset of inhalation detected
*
*************************************************************************************/
int16_t Execute_Resp_Detector(resp_handle_t handle, int16_t data);


/***********************************************************************************
*  int Set_Resp_Detector(resp_handle_t handle, uint16_t min_inhale, uint16_t max_inhale, uint16_t min_exhale, uint16_t max_exhale)
*
*  This function set the detection parameters for the respiratory period detection. 
*  A minimum sanity check is performed. i.e. The minmum value must be smaller than 
*  maximum value.
*  
*  input:
*      1. handle: the handle to pulse detector
*	   2. min_inhale: minimum inhalation period
*      3. max_inhale: maximum inhalation period
*      4. min_exhale: minimum exhalation period
*      5. max_exhale: maximum exhalation period
*  output:
*      FAILURE: The input values are not correct. parameters are not set
*      SUCCESS: Parameters are set correctly
***********************************************************************************/
int Set_Resp_Detector(resp_handle_t handle, uint16_t min_inhale, uint16_t max_inhale, uint16_t min_exhale, uint16_t max_exhale);


/************************************************************************************
*	Close_RR_detector(pulse_handle_t handle)
*
*	This function stops the respiratory pattern detection and releases the memory
*
*	input:
*		1. handle: this is the handle to the pulse detector to be destroyed
*	output:
*		None
*
*************************************************************************************/
void Close_Resp_Detector(resp_handle_t handle);


/************************************************************************************
*	rr_handle_t Initialize_RR_Detector()
*
*	This function initialize the data for respiratory detector
*
*	input:
*		None
*	output:
*		None
*		1.handle: the handle to pulse detector
*************************************************************************************/
rr_handle_t Initialize_RR_Detector();


/***********************************************************************************
*	int Execute_RR_Detector(rr_handle_t *handle, int16_t data)
*
*   This function detect valid respiratory pulses point by point
*
*	input:
*		1. handle: This is the handle to the RR detector
*		2. data: the new data point
*	output:
*		handle to the RR detector data
*
*************************************************************************************/
int16_t Execute_RR_Detector(rr_handle_t handle, int16_t data);


/************************************************************************************
*	Close_RR_detector(rr_handle_t handle)
*
*	This function stops the respiratory rate detection and releases the memory
*
*	input:
*		1. handle: this is the handle to the pulse detector to be destroyed
*	output:
*		None
*
*************************************************************************************/
void Close_RR_Detector(rr_handle_t handle);
#endif // !RRSIGNALPROC
