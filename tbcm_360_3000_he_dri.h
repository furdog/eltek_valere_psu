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

#define TBCM_360_3000_HE_DRI_SERIAL_NO_QUERY_INTERVAL_MS 1000U
#define TBCM_360_3000_HE_DRI_SETTINGS_INTERVAL_MS        100U
#define TBCM_360_3000_HE_DRI_LINK_TIMEOUT_MS             5000U

/******************************************************************************
 * CLASS
 *****************************************************************************/

/* Driver has a writer/reader automatas which are responsible for I/O */
enum tbcm_360_3000_he_dri_reader_state {
	/* Read serial number */
	TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO,

	/* Read device id */
	TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID,

	/* Read device data (sensors) */
	TBCM_360_3000_HE_DRI_READER_STATE_DATA,

	/* Done reading */
	TBCM_360_3000_HE_DRI_READER_STATE_DONE,

	/* Data was not read for too long */
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

/* Automata that is responsible for writing frames onto stream */
struct tbcm_360_3000_he_dri_writer {
	bool send_settings; /* Send settings or not? */
	bool busy; /* No frames should be queued if we're busy */

	/* Current(temporary/buffer) frame we're working on */
	struct tbcm_360_3000_he_dri_frame frame;

	/* Settings frame */
	struct tbcm_360_3000_he_dri_frame x352;

	/* Timers */
	uint32_t serial_no_timer_ms; /* Timer for serial_no resend interval */
	uint32_t settings_timer_ms;  /* Timer for settings resend interval */
	/* TODO check if busy for too long */
};

/* Automata that is responsible for reading frames from input stream */
struct tbcm_360_3000_he_dri_reader {
	uint8_t state;
	bool    busy; /* No frames should be received if we're busy */

	/* Current(temporary/buffer) frame we're working on */
	struct tbcm_360_3000_he_dri_frame frame;

	/* Data frames */
	uint8_t rflags; /* Reception flags (which frames were received?) */
	struct tbcm_360_3000_he_dri_frame x353; /* rflags = 1U << 0U; */
	struct tbcm_360_3000_he_dri_frame x354; /* rflags = 1U << 1U; */
	struct tbcm_360_3000_he_dri_frame x355; /* rflags = 1U << 2U; */

	/* Timers */
	uint32_t link_timeout_ms; /* Link timeout (no data for too long) */
	uint32_t link_timer_ms;   /* Link timer */
	/* TODO check if busy for too long */
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
	int32_t  _fault_line;
	uint32_t _time_up_ms;
};

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
/* Debugging tools */
#ifndef TBCM_360_3000_HE_DRI_LOG
#define TBCM_360_3000_HE_DRI_LOG(v)
#endif

void _tbcm_360_3000_he_dri_dbg_event(struct tbcm_360_3000_he_dri *self,
				     enum tbcm_360_3000_he_dri_event event)
{
	const char *ev_names[] = {
		"TBCM_360_3000_HE_DRI_EVENT_NONE",
		"TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO",
		"TBCM_360_3000_HE_DRI_EVENT_DEVICE_ID",
		"TBCM_360_3000_HE_DRI_EVENT_ESTABLISHED",
		"TBCM_360_3000_HE_DRI_EVENT_FAULT"};

	(void)self;
	(void)event;

	TBCM_360_3000_HE_DRI_LOG(("t=%10u: %s", self->_time_up_ms,
				  ev_names[(uint8_t)event]));

	if (event == TBCM_360_3000_HE_DRI_EVENT_FAULT) {
		TBCM_360_3000_HE_DRI_LOG((" at line %i", self->_fault_line));
	}

	TBCM_360_3000_HE_DRI_LOG(("\n"));
}

void _tbcm_360_3000_he_dri_dbg_frame(struct tbcm_360_3000_he_dri *self,
				     struct tbcm_360_3000_he_dri_frame *frame,
				     bool is_rx)
{
	uint8_t i;

	(void)self;
	(void)frame;
	(void)is_rx;

	TBCM_360_3000_HE_DRI_LOG(("t=%10u: FRAME(%s), ID:%Xh, LEN:%u, DATA: ",
				  self->_time_up_ms, is_rx ? "RX" : "TX",
				  frame->id, frame->len));

	for (i = 0; i < frame->len; i++) {
		TBCM_360_3000_HE_DRI_LOG(("%02X ", frame->data[i]));
	}

	TBCM_360_3000_HE_DRI_LOG(("\n"));
}

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
	self->_writer.send_settings = false; /* Do not send settings */
	self->_writer.busy          = false;

	self->_writer.x352.id  = 0x352U;
	self->_writer.x352.len = 8U;
	(void)memset(self->_writer.x352.data, 0U, 8U);

	/* Timers (must trigger immediately after start) */
	self->_writer.serial_no_timer_ms =
			      TBCM_360_3000_HE_DRI_SERIAL_NO_QUERY_INTERVAL_MS;
	self->_writer.settings_timer_ms  =
				     TBCM_360_3000_HE_DRI_SETTINGS_INTERVAL_MS;
}

void _tbcm_360_3000_he_dri_writer_send_query(struct tbcm_360_3000_he_dri *self)
{
	if (!self->_writer.busy) {
		self->_writer.busy      = true;
		self->_writer.frame.id  = 0x351U;
		self->_writer.frame.len = 6U;
		_tbcm_360_3000_he_dri_binarize_serial_no(self,
						     self->_writer.frame.data);
	}
}

void _tbcm_360_3000_he_dri_writer_send_settings(
					     struct tbcm_360_3000_he_dri *self)
{
	if (!self->_writer.busy) {
		self->_writer.busy  = true;
		self->_writer.frame = self->_writer.x352;
		self->_writer.frame.data[0] = self->_device_id;
	}
}

void _tbcm_360_3000_he_dri_writer_update(struct tbcm_360_3000_he_dri *self,
					 uint32_t delta_time_ms)
{
	/* Send serial number query every 1000ms
	 * (sent always during operation) */
	self->_writer.serial_no_timer_ms += delta_time_ms;

	if (self->_writer.serial_no_timer_ms >=
		            TBCM_360_3000_HE_DRI_SERIAL_NO_QUERY_INTERVAL_MS) {
		self->_writer.serial_no_timer_ms = 0U;
		_tbcm_360_3000_he_dri_writer_send_query(self);
	}

	/* Send charger settings (set voltage, current, etc) every 100ms */
	if (self->_writer.send_settings) {
		self->_writer.settings_timer_ms += delta_time_ms;

		if (self->_writer.settings_timer_ms >=
				   TBCM_360_3000_HE_DRI_SETTINGS_INTERVAL_MS) {
			self->_writer.settings_timer_ms = 0U;
			_tbcm_360_3000_he_dri_writer_send_settings(self);
		}
	}
}

/* Reader */

void _tbcm_360_3000_he_dri_reader_init(struct tbcm_360_3000_he_dri *self)
{
	self->_reader.state = TBCM_360_3000_HE_DRI_READER_STATE_SERIAL_NO;
	self->_reader.busy  = false;

	self->_reader.rflags = 0U;

	/* Timers */
	self->_reader.link_timeout_ms = TBCM_360_3000_HE_DRI_LINK_TIMEOUT_MS;
	self->_reader.link_timer_ms   = 0U;
}

void _tbcm_360_3000_he_dri_reader_parse_data(struct tbcm_360_3000_he_dri *self)
{
	switch (self->_reader.frame.id) {
	case 0x353U:
		/* Validate reader ID */
		if (self->_reader.frame.data[0] != self->_device_id) {
			break;
		}

		self->_reader.x353 = self->_reader.frame;

		/* TODO PARSE FRAME */

		self->_reader.rflags |= 1U << 0U;

		break;

	case 0x354U:
		if (self->_reader.frame.data[0] != self->_device_id) {
			break;
		}

		self->_reader.x354 = self->_reader.frame;

		/* TODO PARSE FRAME */

		self->_reader.rflags |= 1U << 1U;

		break;

	case 0x355U:
		if (self->_reader.frame.data[0] != self->_device_id) {
			break;
		}

		self->_reader.x355 = self->_reader.frame;

		/* TODO PARSE FRAME */

		self->_reader.rflags |= 1U << 2U;

		break;

	default:
		break;
	}

	/* Reset timeout timer if all frames were got */
	if (self->_reader.rflags == 7U) {
		self->_reader.link_timer_ms = 0U;
		self->_reader.rflags        = 0U;

		/* We can send settings at this point */
		self->_writer.send_settings = true;
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
	self->_time_up_ms =  0U;
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
		_tbcm_360_3000_he_dri_writer_init(self);

		self->_state = TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE;
		self->_reader.state =
				   TBCM_360_3000_HE_DRI_READER_STATE_DEVICE_ID;
		self->_reader.busy  = false;
	} else { /* serial is not valid */
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
		self->_reader.rflags   = 0U;
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
	bool accept_frame = !self->_reader.busy;

	if (accept_frame) {
		self->_reader.frame = *frame;
		self->_reader.busy  = true;

		_tbcm_360_3000_he_dri_dbg_frame(self, &self->_reader.frame,
						true);
	}

	return accept_frame;
}

bool tbcm_360_3000_he_dri_read_frame(struct tbcm_360_3000_he_dri *self,
				     struct tbcm_360_3000_he_dri_frame *frame)
{
	bool has_frame = self->_writer.busy;

	if (has_frame) {
		*frame             = self->_writer.frame;
		self->_writer.busy = false;

		_tbcm_360_3000_he_dri_dbg_frame(self, &self->_writer.frame,
						false);
	}

	return has_frame;
}

/* Setters */

void tbcm_360_3000_he_dri_set_charging_mode(struct tbcm_360_3000_he_dri *self,
					    uint8_t val)
{
	/* 0 - charging disabled, 1 - charging enabled
	 * Probably does select constant voltage or constant current mode? */
	self->_writer.x352.data[1] = val;
}

void tbcm_360_3000_he_dri_set_power_W(struct tbcm_360_3000_he_dri *self,
				      float val)
{
	(void)self;
	(void)val;
}

void tbcm_360_3000_he_dri_set_voltage_V(struct tbcm_360_3000_he_dri *self,
					float val)
{
	float clamped = val;
	uint16_t raw;

	if (val < 0.0f) {
		clamped = 0.0f;
	}

	/* voltage 0V - ?V scaled by 10x */
	raw = (uint16_t)(clamped * 10.0);

	self->_writer.x352.data[4] = (raw >> 8) & 0x0FU;
	self->_writer.x352.data[5] = (raw >> 0) & 0x0FU;
}

void tbcm_360_3000_he_dri_set_current_A(struct tbcm_360_3000_he_dri *self,
					float val)
{
	float clamped = val;
	uint16_t raw  = 0U;

	if (val > 10.0f) {
		clamped = 10.0f;
	}

	if (val < 0.0f) {
		clamped = 0.0f;
	}

	/* percents 0 - 100% scaled by 10x */
	raw = (uint16_t)(clamped * 100.0f);

	/* We're setting power ratio here from 0 to 100%
	 * have no idea what does it means, but actual current value field
	 * has no effect on current output. Maybe its because constant voltage
	 * mode is set? TODO specify behaviour */
	self->_writer.x352.data[2] = (raw >> 8) & 0x0FU;
	self->_writer.x352.data[3] = (raw >> 0) & 0x0FU;

	/* Actual current field */
	self->_writer.x352.data[6] = 0U;
	self->_writer.x352.data[7] = 0U;
}

void tbcm_360_3000_he_dri_set_defaults(struct tbcm_360_3000_he_dri *self)
{
	tbcm_360_3000_he_dri_set_voltage_V(self, 250.0f);
	tbcm_360_3000_he_dri_set_current_A(self, 0.0f);
	tbcm_360_3000_he_dri_set_power_W(self, 0.0f);
	tbcm_360_3000_he_dri_set_charging_mode(self, 0U);
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

	self->_time_up_ms += delta_time_ms;
	if (e != TBCM_360_3000_HE_DRI_EVENT_NONE) {
		_tbcm_360_3000_he_dri_dbg_event(self, e);
	}

	return e;
}
