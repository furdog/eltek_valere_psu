/** This driver implements minimal, hardware agnostic, asynchronous,
 *	CAN2.0, low-level device driver for Eltek Valere PSU
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
 * 	instances of the driver can be run to achieve multiple devices support
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
enum tbcm_360_3000_he_dri_writer_state {
	TBCM_360_3000_HE_DRI_WRITER_STATE_IDLE,
	TBCM_360_3000_HE_DRI_WRITER_STATE_QUERY, /* Query device */
	TBCM_360_3000_HE_DRI_WRITER_STATE_ACTIVE
};

enum tbcm_360_3000_he_dri_reader_state {
	TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO,
	TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID,
	TBCM_360_3000_HE_DRI_READER_STATE_DATA,

	TBCM_360_3000_HE_DRI_READER_STATE_DONE
};

enum tbcm_360_3000_he_dri_state {
	/* Listen for devices, log events about detected devices */
	TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES,

	/* Query selected device for info */
	TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE,

	/* Intermediade state, after device id was acknowledged */
	TBCM_360_3000_HE_DRI_STATE_ACK_ID,

	/* Communication with device is fully established */
	TBCM_360_3000_HE_DRI_STATE_ESTABLISHED,

	/* Something is gone wrong */
	TBCM_360_3000_HE_DRI_STATE_FAULT = (uint8_t)-1
};

enum tbcm_360_3000_he_dri_event {
	TBCM_360_3000_HE_DRI_EVENT_NONE,

	/* Some device reported it's serial number to the bus */
	TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO,

	/* Some device reported it's identifier to the bus */
	TBCM_360_3000_HE_DRI_EVENT_DEVICE_ID,

	/* Communication with device is fully established */
	TBCM_360_3000_HE_DRI_EVENT_ESTABLISHED,

	/* Something went wrong */
	TBCM_360_3000_HE_DRI_EVENT_FAULT
};

enum tbcm_360_3000_he_dri_flags {
	TBCM_360_3000_HE_DRI_FLAG_RX = 1,
	TBCM_360_3000_HE_DRI_FLAG_TX = 2
};

struct tbcm_360_3000_he_dri_frame {
	uint32_t id;
	uint8_t  len;
	uint8_t  data[8U];
};

struct tbcm_360_3000_he_dri_writer {
	uint8_t state;
	struct tbcm_360_3000_he_dri_frame frame;
};

struct tbcm_360_3000_he_dri_reader {
	uint8_t state;
	struct tbcm_360_3000_he_dri_frame frame;
};

struct tbcm_360_3000_he_dri {
	uint8_t _state;
	uint8_t _flags;

	struct tbcm_360_3000_he_dri_writer _writer;
	struct tbcm_360_3000_he_dri_reader _reader;

	/* Serial No (as string) */
	char _serial_no[(6U * 2U) + 1U];
	uint8_t _device_id;

	uint32_t _serial_no_query_timer;
};

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
void _tbcm_360_3000_he_dri_reader_init(struct tbcm_360_3000_he_dri *self)
{
	self->_reader.state = TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO;
}

void _tbcm_360_3000_he_dri_writer_init(struct tbcm_360_3000_he_dri *self)
{
	self->_writer.state = TBCM_360_3000_HE_DRI_WRITER_STATE_IDLE;
}

void _tbcm_360_3000_he_dri_stringify_serial_no(
					     struct tbcm_360_3000_he_dri *self,
					     uint8_t *serial_no)
{
	uint8_t i;
	uint8_t byte_value;

	/* Lookup table for hex digits */
	const char hex_chars[] = "0123456789ABCDEF";

	for (i = 0U; i < 6U; i++) {
		byte_value = serial_no[i];

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

void _tbcm_360_3000_he_dri_writer_send_query(struct tbcm_360_3000_he_dri *self)
{
	self->_flags |= TBCM_360_3000_HE_DRI_FLAG_TX;
	self->_writer.frame.id  = 0x351U;
	self->_writer.frame.len = 6U;
	_tbcm_360_3000_he_dri_binarize_serial_no(self,
						 self->_writer.frame.data);
}

void _tbcm_360_3000_he_dri_writer_start(struct tbcm_360_3000_he_dri *self)
{
	self->_serial_no_query_timer = 0U;
	self->_writer.state = TBCM_360_3000_HE_DRI_WRITER_STATE_QUERY;

	_tbcm_360_3000_he_dri_writer_send_query(self);
}

void _tbcm_360_3000_he_dri_writer_update(struct tbcm_360_3000_he_dri *self,
					 uint32_t delta_time_ms)
{
	switch (self->_writer.state) {
	case TBCM_360_3000_HE_DRI_WRITER_STATE_QUERY:
		self->_serial_no_query_timer += delta_time_ms;

		if (self->_serial_no_query_timer >= 1000U) {
			self->_serial_no_query_timer = 0U;
			_tbcm_360_3000_he_dri_writer_send_query(self);
		}

		break;

	case TBCM_360_3000_HE_DRI_WRITER_STATE_ACTIVE:
		break;

	default:
		break;
	}
}

void _tbcm_360_3000_he_dri_reader_update(struct tbcm_360_3000_he_dri *self,
					   uint32_t delta_time_ms)
{
	(void)delta_time_ms;

	switch (self->_reader.state) {
	case TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO:
		if ((self->_flags & (uint8_t)TBCM_360_3000_HE_DRI_FLAG_RX) &&
		    (self->_reader.frame.id == 0x350U) &&
		    (self->_reader.frame.len == 6U)) {
			_tbcm_360_3000_he_dri_stringify_serial_no(self,
						   self->_reader.frame.data);

			self->_reader.state =
				TBCM_360_3000_HE_DRI_READER_STATE_DONE;
		}

		self->_flags &= ~TBCM_360_3000_HE_DRI_FLAG_RX;

		break;

	case TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID:
		if ((self->_flags & (uint8_t)TBCM_360_3000_HE_DRI_FLAG_RX) &&
		    ((self->_reader.frame.id == 0x353U) ||
		     (self->_reader.frame.id == 0x354U) ||
		     (self->_reader.frame.id == 0x355U)) &&
		    (self->_reader.frame.len == 8U)) {
			self->_reader.state =
				      TBCM_360_3000_HE_DRI_READER_STATE_DONE;

			self->_device_id = self->_reader.frame.data[0];
		}

		self->_flags &= ~TBCM_360_3000_HE_DRI_FLAG_RX;

		break;

	case TBCM_360_3000_HE_DRI_READER_STATE_DATA:

		break;

	default:
		break;
	}
}

/******************************************************************************
 * PUBLIC
 *****************************************************************************/
void tbcm_360_3000_he_dri_init(struct tbcm_360_3000_he_dri *self)
{
	self->_state        = TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES;

	self->_flags = 0U;

	_tbcm_360_3000_he_dri_reader_init(self);
	_tbcm_360_3000_he_dri_writer_init(self);

	self->_serial_no[0U] = '\0';
	self->_device_id     = 0U;

	self->_serial_no_query_timer = 0U;
}

/* Serial number */

const char *tbcm_360_3000_he_dri_get_serial_no(
					     struct tbcm_360_3000_he_dri *self)
{
	return self->_serial_no;
}

void tbcm_360_3000_he_dri_ack_serial_no(struct tbcm_360_3000_he_dri *self,
					bool accept)
{
	/* We should be in valid state */
	if ((self->_state !=
	      (uint8_t)TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES) ||
	    (self->_reader.state !=
	      (uint8_t)TBCM_360_3000_HE_DRI_READER_STATE_DONE)) {
		self->_state = TBCM_360_3000_HE_DRI_STATE_FAULT;
	} else if (!accept) { /* If serial is rejected */
		self->_reader.state =
			TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO;
	} else if (_tbcm_360_3000_he_dri_validate_serial_no(self->_serial_no))
	{ /* If serial is valid */
		_tbcm_360_3000_he_dri_writer_start(self);

		self->_state = TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE;
		self->_reader.state
			= TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID;
	} else {
		self->_state = TBCM_360_3000_HE_DRI_STATE_FAULT;
	}
}

/* Device id (runtime) */

uint8_t tbcm_360_3000_he_dri_get_device_id(struct tbcm_360_3000_he_dri *self)
{
	return self->_device_id;
}

void tbcm_360_3000_he_dri_ack_device_id(struct tbcm_360_3000_he_dri *self,
					bool accept)
{
	/* We should be in valid state */
	if ((self->_state !=
	      (uint8_t)TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE) ||
	    (self->_reader.state !=
	      (uint8_t)TBCM_360_3000_HE_DRI_READER_STATE_DONE)) {
		self->_state = TBCM_360_3000_HE_DRI_STATE_FAULT;
	} else if (!accept) { /* If id is rejected */
		self->_reader.state =
			TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID;
	} else {
		self->_state = TBCM_360_3000_HE_DRI_STATE_ACK_ID;
		self->_reader.state
			= TBCM_360_3000_HE_DRI_READER_STATE_DATA;
	}
}

/* Driver I/O */

bool tbcm_360_3000_he_dri_write_frame(struct tbcm_360_3000_he_dri *self,
				      struct tbcm_360_3000_he_dri_frame *frame)
{
	bool is_busy = self->_flags & (uint8_t)TBCM_360_3000_HE_DRI_FLAG_RX;

	if (!is_busy) {
		self->_reader.frame = *frame;
		self->_flags   |= TBCM_360_3000_HE_DRI_FLAG_RX;
	}

	return is_busy;
}

bool tbcm_360_3000_he_dri_read_frame(struct tbcm_360_3000_he_dri *self,
				     struct tbcm_360_3000_he_dri_frame *frame)
{
	bool has_frame = self->_flags & (uint8_t)TBCM_360_3000_HE_DRI_FLAG_TX;

	if (has_frame) {
		*frame = self->_writer.frame;

		self->_flags &= ~TBCM_360_3000_HE_DRI_FLAG_TX;
	}

	return has_frame;
}

/* Update */

enum tbcm_360_3000_he_dri_event tbcm_360_3000_he_dri_update(
					     struct tbcm_360_3000_he_dri *self,
					     uint32_t delta_time_ms)
{
	enum tbcm_360_3000_he_dri_event e = TBCM_360_3000_HE_DRI_EVENT_NONE;

	(void)delta_time_ms;

	switch (self->_state) {
	case TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES:
		_tbcm_360_3000_he_dri_reader_update(self, delta_time_ms);

		if (self->_reader.state ==
		    (uint8_t)TBCM_360_3000_HE_DRI_READER_STATE_DONE) {
			e = TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO;
		}

		break;

	case TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE:
		_tbcm_360_3000_he_dri_writer_update(self, delta_time_ms);
		_tbcm_360_3000_he_dri_reader_update(self, delta_time_ms);

		if (self->_reader.state ==
		    (uint8_t)TBCM_360_3000_HE_DRI_READER_STATE_DONE) {
			e = TBCM_360_3000_HE_DRI_EVENT_DEVICE_ID;
		}

		break;

	case TBCM_360_3000_HE_DRI_STATE_ACK_ID:
		e = TBCM_360_3000_HE_DRI_EVENT_ESTABLISHED;

		self->_state = TBCM_360_3000_HE_DRI_STATE_ESTABLISHED;
		break;

	case TBCM_360_3000_HE_DRI_STATE_ESTABLISHED:
		_tbcm_360_3000_he_dri_writer_update(self, delta_time_ms);
		_tbcm_360_3000_he_dri_reader_update(self, delta_time_ms);
		break;

	case TBCM_360_3000_HE_DRI_STATE_FAULT:
		e = TBCM_360_3000_HE_DRI_EVENT_FAULT;

		self->_state = TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES;
		self->_reader.state =
				 TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO;
		break;

	default:
		break;
	}

	return e;
}
