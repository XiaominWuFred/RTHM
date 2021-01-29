
//#include "compatible.h"
#include "RRSignalProc.h"
#include <stdlib.h>
#include <stdint.h>

// constants for pulse detector
#define PULSE_REFACTORY_PERIOD		70		// The minimum time lag between two legitmate pulses in number of data points
#define PULSE_MIN_WIDTH				4		// minimum pulse width
#define SEARCH_PULSE_STATE			0		// pulse search state
#define VALIDATE_PULSE_STATE		1		// pulse validate state
#define	SKIP_STATE					2		// Skip detection until after the pulse refractory period

// constants for respiratory pattern detection
#define INHALE_SEARCH_STATE	0x00
#define EXHALE_SEARCH_STATE	0x01

// constants for respiratory detection
#define HPMAFILTERSIZE	4					// define 16-point HP MA filter
#define	LPMAFILTERSIZE	4					// define 16-point LP MA filter
#define DETMAFILTERSIZE	3					// define 8-point LP MA filter

#define PULSE_THRESHOLD	30					// pulse threshold
#define NOISE_THRESHOLD 160					// noise threshold 
#define PUSLE_THRESHOLD_MULTIPLIER	6		// multiplier to determine the pulse detector threshold
#define MIN_INHALATION_PERIOD		35		// minimum period for inhalation
#define MAX_INHALATION_PERIOD		400		// maximum period for inhalation
#define MIN_EXHALATION_PERIOD		200 	// minimum period for exhalation
#define MAX_EXHALATION_PERIOD		2000	// maximum period for exhalation

// Moving average filter
ma_handle_t Initialize_MA_Filter(uint16_t order)
{
	ma_handle_t handle;
	if (order > 0)
	{
		// Allocate memory for MA_status
		handle = (ma_handle_t) malloc(sizeof(ma_status_t));
		if (handle == NULL)
			//return NULL if memory allocation failed
			return NULL; 
		//allocate memory for the data buffer
		handle->ma_buffer = (int16_t *) malloc((1<< order) * sizeof(int16_t));
		if (handle->ma_buffer == NULL)
		{
			// memory allocation failed. release the memory for MA handle
			free(handle);
			return NULL;
		}
		// initialize the data buffer to zeros
		memset(handle->ma_buffer, 0, (1 << order) * sizeof(int16_t));
		// initialize the rest of the handle data
		handle->ma_output = 0;
		handle->ma_order = order;
		handle->ma_mask = ~((int16_t)0xFFFF << order);
		handle->ma_index = 0;
		handle->ma_offset = 0;
		return handle;
	}
	else
		return NULL;
		
}

int16_t Execute_MA_Filter(ma_handle_t handle, int16_t data)
{
	int16_t temp;

	// set temporary value to zero
	temp = 0;
	if (handle != NULL)
	{
		// valid MA handle. Process the MA filter
		temp = data - handle->ma_buffer[handle->ma_index];
		handle->ma_offset += temp & handle->ma_mask;
		temp = handle->ma_output+(temp >> handle->ma_order);
		if (handle->ma_offset > handle->ma_mask)
		{
			temp++;
			handle->ma_offset &= handle->ma_mask;
		}
		handle->ma_buffer[handle->ma_index++] = data;
		handle->ma_output = temp;
		if (handle->ma_index >= (1 << handle->ma_order))
			handle->ma_index = 0;
	}
	
	return temp;
}

void Close_MA_Filter(ma_handle_t handle)
{
	if (handle != NULL)
	{
		// valid handle. release memory used by the MA filter
		if (handle->ma_buffer != NULL) 
			free(handle->ma_buffer);
		free(handle);
	}
}

// pulse detection
pulse_handle_t Initialize_Pulse_Detector()
{
	pulse_handle_t handle;

	//allocate memory for pulse detector
	handle = (pulse_handle_t) malloc(sizeof(pulse_detecor_status_t));
	if (handle != NULL)
	{
		handle->flag = 0x00;
		handle->state = SEARCH_PULSE_STATE;
		handle->skipdelay = 0x00;
		handle->pulsewidth = 0x00;
	}
	return handle;
}

int16_t Execute_Pulse_Detector(pulse_handle_t handle, int16_t data, int16_t threshold)
{
	uint16_t pulse_status;

	// set the detection state as false
	pulse_status = PULSE_NOT_DETECTED;
	if (handle != NULL)
	{
		switch (handle->state)
		{	
			case SEARCH_PULSE_STATE:
			{	// Search state
				if (data > threshold)
				{	// if the data is above threshold, reset the pulse width 
					// and set the next state as validate.
					handle->pulsewidth = 0;
					handle->state = VALIDATE_PULSE_STATE;
				}
				break;
			}
			
			case VALIDATE_PULSE_STATE:
			{	// validate pulse
				if (data > threshold)
				{
					// increment the pulse width if the data is above threshold
					handle->pulsewidth++;
					if (handle->pulsewidth > PULSE_MIN_WIDTH)
					{
						//if the pulse width is above minimum width, set next state as SKIP.
						handle->state = SKIP_STATE;
						// Set pulse detected
						pulse_status = PULSE_DETECTED;
						// Set the minimum period to skip any pulses
						handle->skipdelay = PULSE_REFACTORY_PERIOD;
					}
				}
				else
					// data is below threshold. False pulse and go back to search state
					handle->state = SEARCH_PULSE_STATE;
				break;
			}
				
			case SKIP_STATE:
			{	// decrement the skipdelay
				handle->skipdelay--;
				if (handle->skipdelay == 0)
					// go to pulse search state after PULSE_REFACTORY_PERIOD
					handle->state = SEARCH_PULSE_STATE;
				break;
			}
		}	
	}
	// return the pulse status
	return pulse_status;
}

void Close_Pulse_Detector(pulse_handle_t handle)
{
	// release the pulse detector data
	if (handle != NULL)
		free(handle);
	return;
}

// respiratory pulse identification
resp_handle_t Initialize_Resp_Detector(uint16_t min_inhale, uint16_t max_inhale, uint16_t min_exhale, uint16_t max_exhale)
{
	resp_handle_t handle;

	handle = (resp_handle_t)malloc(sizeof(resp_detector_status_t));
	if (handle != NULL)
	{
		handle->min_inhale = min_inhale;
		handle->max_inhale = max_inhale;
		handle->min_exhale = min_exhale;
		handle->max_exhale = max_exhale;
		handle->s_inhale = 0;
		handle->s_exhale = 0;
		handle->state = INHALE_SEARCH_STATE;
		handle->counter = 0;	//the counter starts from zero when a pulse is detected
	}
	return handle;
}

int16_t Execute_Resp_Detector(resp_handle_t handle, int16_t data)
{
	uint16_t pulse_status;

	pulse_status = 0;	//no pulse is detected
	if (data == PULSE_NOT_DETECTED)
	{
		if (handle->counter > handle->max_inhale)
			handle->state = INHALE_SEARCH_STATE;
	}
	else
	{
		if (handle->state == INHALE_SEARCH_STATE)
		{	// current state is exhalation. searching for inhalation pulse
			if (handle->counter > handle->min_exhale)
			{
				// current pulse is at least min_exhale away from the exhalation pulse.
				// This is considered as the inhalation pulse
				handle->counter = 0;
				handle->state = EXHALE_SEARCH_STATE;
				pulse_status = INHALE_PULSE | PULSE_DETECTED;
			}		
		}
		if (handle->state == EXHALE_SEARCH_STATE)
		{	// current state is inhalation. searching for exhalation pulse
			if (handle->counter > handle->min_inhale)
			{
				// current pulse is at least min_inhale away from the inhalation pulse.
				// This is considered as the inhalation pulse
				handle->counter = 0;
				handle->state = INHALE_SEARCH_STATE;
				pulse_status = EXHALE_PULSE | PULSE_DETECTED;
			}
		}
	}
	handle->counter++;
	return pulse_status;
}

int Set_Resp_Detector(resp_handle_t handle, uint16_t min_inhale, uint16_t max_inhale, uint16_t min_exhale, uint16_t max_exhale)
{
	if ((min_inhale < max_inhale) && (min_exhale < max_exhale))
	{
		handle->min_inhale = min_inhale;
		handle->max_inhale = max_inhale;
		handle->min_exhale = min_exhale;
		handle->max_exhale = max_exhale;
		return SUCCESS1;
	}
	else
		return FAILURE1;
}

void Close_Resp_Detector(resp_handle_t handle)
{
	if (handle != NULL)
	{
		free(handle);
	}
	return;
}

//Repiratory rate detection
rr_handle_t Initialize_RR_Detector()
{
	rr_handle_t handle;

	// allocate memnory for the pulse detector configurations
	handle = (rr_handle_t)malloc(sizeof(rr_detector_status_t));
	if (handle == NULL)
		return NULL;	//memory allocation error
	// Set the handles to the MA filters NULL
	handle->detHandle = NULL;
	handle->hpHandle = NULL;
	handle->lpHandle = NULL;
	handle->pulseHandle = NULL;
	handle->respHandle = NULL;
	
	// Create the MA filter for high pass filter
	handle->hpHandle = Initialize_MA_Filter(HPMAFILTERSIZE);
	if (handle->hpHandle == NULL)
	{
		// memeory allocation error. release all allocate memory
		Close_RR_Detector(handle);
		return NULL;
	}
	// Create the MA filter for low pass filter
	handle->lpHandle = Initialize_MA_Filter(LPMAFILTERSIZE);
	if (handle->lpHandle == NULL)
	{
		// memeory allocation error. release all allocate memory
		Close_RR_Detector(handle);
		return NULL;
	}
	// Create filter for threshold detection
	handle->detHandle = Initialize_MA_Filter(DETMAFILTERSIZE);
	if (handle->detHandle == NULL)
	{
		// memeory allocation error. release all allocate memory
		Close_RR_Detector(handle);
		return NULL;
	}
	// return the handle to the pulse detector configuration
	
	handle->pulseHandle = Initialize_Pulse_Detector();
	if (handle->pulseHandle == NULL)
	{
		Close_RR_Detector(handle);
		return NULL;
	}

	//return the handle to respiratory pattern recognition
	handle->respHandle = Initialize_Resp_Detector(MIN_INHALATION_PERIOD, MAX_INHALATION_PERIOD, MIN_EXHALATION_PERIOD, MAX_EXHALATION_PERIOD);
	if (handle->respHandle == NULL)
	{
		Close_Resp_Detector(handle->respHandle);
		return NULL;
	}
	// Set detector parameters
	handle->noise_threshold = NOISE_THRESHOLD;
	handle->pulse_threshold = PULSE_THRESHOLD;
	return handle;
}

int16_t Execute_RR_Detector(rr_handle_t handle, int16_t data)
{
	int16_t temp_data;
	int16_t pulse_threshold_data;

	// High pass filter the data
	
	temp_data = data - Execute_MA_Filter(handle->hpHandle, data);
	// take absolute value
	pulse_threshold_data = (temp_data > 0? temp_data : -temp_data);
	// remove pulses
	
	pulse_threshold_data = (pulse_threshold_data > handle->pulse_threshold ? 0 : pulse_threshold_data);
	// get the moving average of the baseline noise
	pulse_threshold_data = Execute_MA_Filter(handle->lpHandle, pulse_threshold_data);
	
	// adjust the threshold
	pulse_threshold_data = pulse_threshold_data*PUSLE_THRESHOLD_MULTIPLIER;
	// set the data below the threshold to zero
	temp_data = (temp_data > pulse_threshold_data ? temp_data : 0);
	// take the absolute value
	// temp_data = (temp_data > 0 ? temp_data : -temp_data);
	
	// get the moving average
	temp_data = Execute_MA_Filter(handle->detHandle, temp_data);
	temp_data = Execute_Pulse_Detector(handle->pulseHandle, temp_data,handle->noise_threshold);
	
	temp_data = Execute_Resp_Detector(handle->respHandle, temp_data);
	return temp_data;
}

void Close_RR_Detector(rr_handle_t handle)
{
	if (handle != NULL)
	{
		if (handle->respHandle != NULL)
			Close_Resp_Detector(handle->respHandle); 
		if (handle->pulseHandle != NULL)
			Close_Pulse_Detector(handle->pulseHandle);
		if (handle->detHandle != NULL)
			Close_MA_Filter(handle->detHandle);
		if (handle->lpHandle != NULL)
			Close_MA_Filter(handle->lpHandle);
		if (handle->hpHandle != NULL)
			Close_MA_Filter(handle->hpHandle);
		free(handle);
	}
	return;
}
