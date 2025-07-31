/** This driver implements minimal, hardware agnostic, asynchronous
 *	CAN2.0 device driver for Eltek Valere PSU
 *
 * 
 * Part name:  TBCM 360/3000 HE
 * Part No:    241121.000
 * Batch No:   794033

 * AC Input:   100-250V
 * Frequency:  45-66Hz
 * AC Current: 14Amax
 * AC Fuse:    25A F
 * DC Output:  250-420V/10Amax
 * 
 * Revision:   2.1
 * SW: 01.00/01.00
 * 
 * 
 * The driver can only communicate with one device at a time, but multiple
 * 	instances of the driver can be run to achieve multiple devices support.
 * 
 * WARNING: the communication protocol is only suitable for TBCM series
 * 	and not compliant with protocol described in Doc No. 2086930
 * 
 * WARNING: the driver is not tested on real hardware YET
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/******************************************************************************
 * CLASS
 *****************************************************************************/
enum tbcm_360_3000_he_dri_state {
	TBCM_360_3000_HE_DRI_STATE_INIT, /* Initial state */

	/* Listen for devices, log events about detected devices */
	TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES,

	/* Query selected device for info */
	TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE,

	/* Something gone wrong */
	TBCM_360_3000_HE_DRI_STATE_FAULT,

	/*
	 * OTHER STATES (communiacation with selected device)
	 * 
	 * ...
	 */

	TBCM_360_3000_HE_DRI_STATE_IDLE
};

enum tbcm_360_3000_he_dri_event {
	TBCM_360_3000_HE_DRI_EVENT_NONE,

	/* Some device reported it's serial number to the bus */
	TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO
};

struct tbcm_360_3000_he_dri_frame {
	uint32_t id;
	uint8_t  len;
	uint8_t  data[8U];
};

struct tbcm_360_3000_he_dri {
	uint8_t _state;

	struct tbcm_360_3000_he_dri_frame _frame;

	/* Serial No (as string) */
	char _serial_no[(6U * 2U) + 1U];
};

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
void _tbcm_360_3000_he_dri_stringify_serial_no(
					     struct tbcm_360_3000_he_dri *self)
{
	uint8_t i;
	uint8_t byte_value;

	/* Lookup table for hex digits */
	const char hex_chars[] = "0123456789ABCDEF"; 
 
	for (i = 0U; i < 6U; i++) {
		byte_value = self->_frame.data[i];

		self->_serial_no[i * 2U] = 
					 hex_chars[(byte_value >> 4U) & 0x0FU];

		self->_serial_no[(i * 2U) + 1U] = 
					 hex_chars[byte_value & 0x0FU];
	}

	/* Null-terminate the string */
	self->_serial_no[6U * 2U] = '\0';
}

uint8_t _tbcm_360_3000_he_dri_hex2int(const char c)
{
	uint8_t result = 0;

	if ((c >= '0') && (c <= '9')) {
		result = c - (uint8_t)'0';
	} else if ((c >= 'A') && (c <= 'F')) {
		result = c - (uint8_t)'A' + 10U;
	} else if ((c >= 'a') && (c <= 'f')) {
		result = c - (uint8_t)'a' + 10U;
	} else {}

	return result;
}

void _tbcm_360_3000_he_dri_binarize_serial_no(
					     struct tbcm_360_3000_he_dri *self,
					     uint8_t *buf)
{
	uint8_t i;

	for (i = 0U; i < 6U; i++) {
		const char c_upper = self->_serial_no[i * 2U];
		const char c_lower = self->_serial_no[(i * 2U) + 1U];

		buf[i] = (_tbcm_360_3000_he_dri_hex2int(c_upper) << 4U) |
			  _tbcm_360_3000_he_dri_hex2int(c_lower);
	}
}

bool _tbcm_360_3000_he_dri_validate_serial_no(const char *serial_no)
{
	bool is_valid = true;
	uint8_t i;

	/* Serial number SHALL only contain 12 decimal digits (not HEX) */
	for (i = 0U; i < (6U * 2U); i++) {
		const char c = serial_no[i];
		
		if ((c < '0') || (c > '9')) {
			is_valid = false;
		}
	}

	return is_valid;
}

/* void _tbcm_360_3000_he_dri_sender_update(struct tbcm_360_3000_he_dri *self)
{
	switch (self->_sender_state)
	{
		TODO
	}
} */

/******************************************************************************
 * PUBLIC
 *****************************************************************************/
void tbcm_360_3000_he_dri_init(struct tbcm_360_3000_he_dri *self)
{
	self->_state = TBCM_360_3000_HE_DRI_STATE_IDLE;

	self->_frame.len = 0U;
}

void tbcm_360_3000_he_dri_start_listen_for_devices(
					     struct tbcm_360_3000_he_dri *self)
{
	self->_state = TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES;

	self->_frame.len = 0U;
}

const char *tbcm_360_3000_he_dri_get_serial_no(
					     struct tbcm_360_3000_he_dri *self)
{
	return self->_serial_no;
}

void tbcm_360_3000_he_dri_select_device(struct tbcm_360_3000_he_dri *self,
					const char *serial_no)
{
	bool is_valid_serial_no = false;

	if (serial_no != NULL) {
		if (_tbcm_360_3000_he_dri_validate_serial_no(serial_no))
		{
			(void)memcpy(self->_serial_no, serial_no, 6);
			is_valid_serial_no = true;
		}
	} else {
		if (_tbcm_360_3000_he_dri_validate_serial_no(self->_serial_no))
		{
			self->_state = TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE;
			is_valid_serial_no = true;
		}
	}

	if (is_valid_serial_no)
	{
		self->_state = TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE;
	} else {
		self->_state = TBCM_360_3000_HE_DRI_STATE_FAULT;
	}
}

void tbcm_360_3000_he_dri_write_frame(struct tbcm_360_3000_he_dri *self,
				      struct tbcm_360_3000_he_dri_frame *frame)
{
	self->_frame = *frame;
}

enum tbcm_360_3000_he_dri_event tbcm_360_3000_he_dri_update(
					     struct tbcm_360_3000_he_dri *self,
					     uint32_t delta_time_ms)
{
	enum tbcm_360_3000_he_dri_event e = TBCM_360_3000_HE_DRI_EVENT_NONE;

	(void)delta_time_ms;

	switch (self->_state) {
	case TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES:
		if ((self->_frame.id == 0x350U) && (self->_frame.len == 6U)) {
			e = TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO;

			_tbcm_360_3000_he_dri_stringify_serial_no(self);
		}

		break;

	case TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE:
		/* Se */
		break;

	default:
		break;
	}

	return e;
}
