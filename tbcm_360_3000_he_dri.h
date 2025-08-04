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

/* Driver has a writer/reader automatas which are responsible for I/O
 * These states are used by the automatas: */
enum tbcm_360_3000_he_dri_writer_state {
	/* TODO Remove unused states */
	TBCM_360_3000_HE_DRI_WRITER_STATE_IDLE,
	TBCM_360_3000_HE_DRI_WRITER_STATE_QUERY, /* Query device */
	TBCM_360_3000_HE_DRI_WRITER_STATE_ACTIVE
};

enum tbcm_360_3000_he_dri_reader_state {
	TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO,
	TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID,
	TBCM_360_3000_HE_DRI_READER_STATE_DATA,

	TBCM_360_3000_HE_DRI_READER_STATE_DONE,
	TBCM_360_3000_HE_DRI_READER_STATE_TIMEOUT
};

/* The driver has it's own state too */
enum tbcm_360_3000_he_dri_state {
	/* Listen for devices, log events about detected devices */
	TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES,

	/* Query selected device for info */
	TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE,

	/* Intermediade state, after device id was acknowledged */
	TBCM_360_3000_HE_DRI_STATE_ACK_ID,

	/* Communication with device is fully established */
	TBCM_360_3000_HE_DRI_STATE_ESTABLISHED,

	/* Something has gone wrong */
	TBCM_360_3000_HE_DRI_STATE_FAULT = (uint8_t)-1
};

/* Driver's main loop might return some event codes: */
enum tbcm_360_3000_he_dri_event {
	TBCM_360_3000_HE_DRI_EVENT_NONE, /* No events */

	/* Some device reported it's serial number to the bus */
	TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO,

	/* Some device reported it's identifier to the bus */
	TBCM_360_3000_HE_DRI_EVENT_DEVICE_ID,

	/* Communication with device is fully established */
	TBCM_360_3000_HE_DRI_EVENT_ESTABLISHED,

	/* Something went wrong */
	TBCM_360_3000_HE_DRI_EVENT_FAULT
};

/* Simplified CAN2.0 frame representation */
struct tbcm_360_3000_he_dri_frame {
	uint32_t id;
	uint8_t  len;
	uint8_t  data[8U];
};

/* Data frames we will actively work with.
 * It contains various runtime PSU parameters (voltage, current, temp, etc) */
struct tbcm_360_3000_he_dri_data_frames {
	uint8_t rflags; /* Reception flags (which frames were received?) */
	
	struct tbcm_360_3000_he_dri_frame x353; /* rflags = 1U << 0U; */
	struct tbcm_360_3000_he_dri_frame x354; /* rflags = 1U << 1U; */
	struct tbcm_360_3000_he_dri_frame x355; /* rflags = 1U << 2U; */
};

/* Automata that is responsible for writing frames onto stream */
struct tbcm_360_3000_he_dri_writer {
	uint8_t state;
	bool    busy; /* No frames should be queued if we're busy */

	/* Current frame we're working on */
	struct tbcm_360_3000_he_dri_frame frame;

	/* Timers */
	uint32_t serial_no_timer_ms; /* Timer for serial_no resend interval */
	uint32_t settings_timer_ms;  /* Timer for settings resend interval */
};

/* Automata that is responsible for reading frames from input stream */
struct tbcm_360_3000_he_dri_reader {
	uint8_t state;
	bool    busy; /* No frames should be received if we're busy */

	/* Current frame we're working on */
	struct tbcm_360_3000_he_dri_frame frame;

	/* Data frames */
	struct tbcm_360_3000_he_dri_data_frames data;

	/* Timers */
	uint32_t link_timeout_ms; /* Link timeout (no data for too long) */
	uint32_t link_timer_ms;   /* Link timer */
};

/* Main driver class */
struct tbcm_360_3000_he_dri {
	uint8_t _state;

	struct tbcm_360_3000_he_dri_writer _writer;
	struct tbcm_360_3000_he_dri_reader _reader;

	/* Serial No (as string) */
	char _serial_no[(6U * 2U) + 1U];
	uint8_t _device_id;

	/* DEBUG */
	int32_t _fault_line;
};

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
/* Serial number related methods */

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

/* Writer */

void _tbcm_360_3000_he_dri_writer_init(struct tbcm_360_3000_he_dri *self)
{
	self->_writer.state = TBCM_360_3000_HE_DRI_WRITER_STATE_IDLE;
	self->_writer.busy  = false;

	/* Timers */
	self->_writer.serial_no_timer_ms = 0U;
	self->_writer.settings_timer_ms  = 0U;
}


void _tbcm_360_3000_he_dri_writer_send_query(struct tbcm_360_3000_he_dri *self)
{
	self->_writer.busy      = true;
	self->_writer.frame.id  = 0x351U;
	self->_writer.frame.len = 6U;
	_tbcm_360_3000_he_dri_binarize_serial_no(self,
						 self->_writer.frame.data);
}

void _tbcm_360_3000_he_dri_writer_start(struct tbcm_360_3000_he_dri *self)
{
	self->_writer.state = TBCM_360_3000_HE_DRI_WRITER_STATE_QUERY;

	self->_writer.serial_no_timer_ms = 0U;
	self->_writer.settings_timer_ms  = 0U;

	_tbcm_360_3000_he_dri_writer_send_query(self);
}

void _tbcm_360_3000_he_dri_writer_update(struct tbcm_360_3000_he_dri *self,
					 uint32_t delta_time_ms)
{
	switch (self->_writer.state) {
	case TBCM_360_3000_HE_DRI_WRITER_STATE_QUERY:
		self->_writer.serial_no_timer_ms += delta_time_ms;

		if (self->_writer.serial_no_timer_ms >= 1000U) {
			self->_writer.serial_no_timer_ms = 0U;
			_tbcm_360_3000_he_dri_writer_send_query(self);
		}

		break;

	case TBCM_360_3000_HE_DRI_WRITER_STATE_ACTIVE:
		break;

	default:
		break;
	}
}

/* Reader */

void _tbcm_360_3000_he_dri_reader_init(struct tbcm_360_3000_he_dri *self)
{
	self->_reader.state = TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO;
	self->_reader.busy  = false;

	self->_reader.data.rflags = 0U;

	/* Timers */
	self->_reader.link_timeout_ms = 1000U;
	self->_reader.link_timer_ms   = 0U;
}

void _tbcm_360_3000_he_dri_reader_parse_data(struct tbcm_360_3000_he_dri *self)
{
	switch (self->_reader.frame.id) {
	case 0x353U:
		/* TODO PARSE FRAME */

		self->_reader.data.rflags |= 1U << 0U;

		break;

	case 0x354U:
		/* TODO PARSE FRAME */

		self->_reader.data.rflags |= 1U << 1U;

		break;

	case 0x355U:
		/* TODO PARSE FRAME */

		self->_reader.data.rflags |= 1U << 2U;

		break;

	default:
		break;
	}

	/* Reset timeout timer if all frames were got */
	if (self->_reader.data.rflags == 7U) {
		self->_reader.link_timer_ms = 0U;
		self->_reader.data.rflags   = 0U;
	}
}

void _tbcm_360_3000_he_dri_reader_update(struct tbcm_360_3000_he_dri *self,
					   uint32_t delta_time_ms)
{
	(void)delta_time_ms;

	switch (self->_reader.state) {
	case TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO:
		if (self->_reader.busy &&
		    (self->_reader.frame.id == 0x350U) &&
		    (self->_reader.frame.len == 6U)) {
			_tbcm_360_3000_he_dri_stringify_serial_no(self,
						   self->_reader.frame.data);

			self->_reader.state =
				TBCM_360_3000_HE_DRI_READER_STATE_DONE;
		} else {
			self->_reader.busy = false;
		}

		break;

	case TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID:
		if (self->_reader.busy &&
		    ((self->_reader.frame.id == 0x353U) ||
		     (self->_reader.frame.id == 0x354U) ||
		     (self->_reader.frame.id == 0x355U)) &&
		    (self->_reader.frame.len == 8U)) {
			self->_reader.state =
				      TBCM_360_3000_HE_DRI_READER_STATE_DONE;

			self->_device_id = self->_reader.frame.data[0];
		} else {
			self->_reader.busy = false;
		}

		break;

	case TBCM_360_3000_HE_DRI_READER_STATE_DATA:
		self->_reader.link_timer_ms += delta_time_ms;

		if (self->_reader.busy) {
			_tbcm_360_3000_he_dri_reader_parse_data(self);
		}

		/* Check for timeout */
		if (self->_reader.link_timer_ms >=
		    self->_reader.link_timeout_ms) {
			self->_reader.state =
				     TBCM_360_3000_HE_DRI_READER_STATE_TIMEOUT;
		}

		self->_reader.busy = false;

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
	self->_state = TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES;

	_tbcm_360_3000_he_dri_reader_init(self);
	_tbcm_360_3000_he_dri_writer_init(self);

	self->_serial_no[0U] = '\0';
	self->_device_id     = 0U;

	/* DEBUG */
	self->_fault_line = -1;
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
		self->_fault_line = __LINE__;
	} else if (!accept) { /* If serial is rejected */
		self->_reader.state =
				   TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO;
		self->_reader.busy  = false;
	} else if (_tbcm_360_3000_he_dri_validate_serial_no(self->_serial_no))
	{ /* If serial is valid */
		_tbcm_360_3000_he_dri_writer_start(self);

		self->_state = TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE;
		self->_reader.state =
				   TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID;
		self->_reader.busy  = false;
	} else {
		self->_state = TBCM_360_3000_HE_DRI_STATE_FAULT;
		self->_fault_line = __LINE__;
	}
}

void tbcm_360_3000_he_dri_accept_serial_no(struct tbcm_360_3000_he_dri *self)
{
	tbcm_360_3000_he_dri_ack_serial_no(self, true);
}

void tbcm_360_3000_he_dri_reject_serial_no(struct tbcm_360_3000_he_dri *self)
{
	tbcm_360_3000_he_dri_ack_serial_no(self, false);
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
		self->_fault_line = __LINE__;
	} else if (!accept) { /* If id is rejected */
		self->_reader.state =
				   TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID;
		self->_reader.busy  = false;
	} else {
		self->_state = TBCM_360_3000_HE_DRI_STATE_ACK_ID;
		
		self->_reader.state = TBCM_360_3000_HE_DRI_READER_STATE_DATA;
		self->_reader.data.rflags   = 0U;
		self->_reader.link_timer_ms = 0U;
		
		/* keep busy true, so DATA state will consume this frame too */
		/* self->_reader.busy  = false; */
	}
}

void tbcm_360_3000_he_dri_accept_device_id(struct tbcm_360_3000_he_dri *self)
{
	tbcm_360_3000_he_dri_ack_device_id(self, true);
}

void tbcm_360_3000_he_dri_reject_device_id(struct tbcm_360_3000_he_dri *self)
{
	tbcm_360_3000_he_dri_ack_device_id(self, false);
}

/* Driver I/O */

bool tbcm_360_3000_he_dri_write_frame(struct tbcm_360_3000_he_dri *self,
				      struct tbcm_360_3000_he_dri_frame *frame)
{
	bool is_busy = self->_reader.busy;

	if (!is_busy) {
		self->_reader.frame = *frame;
		self->_reader.busy  = true;
	}

	return is_busy;
}

bool tbcm_360_3000_he_dri_read_frame(struct tbcm_360_3000_he_dri *self,
				     struct tbcm_360_3000_he_dri_frame *frame)
{
	bool has_frame = self->_writer.busy;

	if (has_frame) {
		*frame             = self->_writer.frame;
		self->_writer.busy = false;
	}

	return has_frame;
}

/* Update */

void tbcm_360_3000_he_dri_recover_from_fault(
					     struct tbcm_360_3000_he_dri *self)
{
	self->_state = TBCM_360_3000_HE_DRI_STATE_LISTEN_DEVICES;
	self->_reader.state =
			   TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO;
	self->_reader.busy  = false;
}

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
		/* FALLTHROUGH */

	case TBCM_360_3000_HE_DRI_STATE_ESTABLISHED:
		_tbcm_360_3000_he_dri_writer_update(self, delta_time_ms);
		_tbcm_360_3000_he_dri_reader_update(self, delta_time_ms);

		if (self->_reader.state ==
		    (uint8_t)TBCM_360_3000_HE_DRI_READER_STATE_TIMEOUT) {
			e = TBCM_360_3000_HE_DRI_EVENT_FAULT;

			tbcm_360_3000_he_dri_recover_from_fault(self);
		}

		break;

	case TBCM_360_3000_HE_DRI_STATE_FAULT:
		e = TBCM_360_3000_HE_DRI_EVENT_FAULT;

		tbcm_360_3000_he_dri_recover_from_fault(self);

		break;

	default:
		break;
	}

	return e;
}
