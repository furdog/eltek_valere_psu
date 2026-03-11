/**
 * @file tbcm_360_3000_he_dri.h
 * @brief Eltek Valere PSU driver (Hardware-Agnostic)
 *
 * This file contains the software implementation of the CAN2.0,
 * low-level device driver for Eltek Valere PSU.
 * The design is hardware-agnostic, requiring an external adaptation layer
 * for hardware interaction.
 *
 * ```DETAILS
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
 * ```
 *
 * The driver can only communicate with one device at a time, but multiple
 * 	instances of the driver can be run to achieve multiple devices support
 *
 * WARNING: the communication protocol is only suitable for TBCM series
 * 	and not compliant with protocol described in Doc No. 2086930
 *
 * WARNING: the driver is not properly tested on real hardware!
 *
 *
 * **Conventions:**
 * C89, Linux kernel style, MISRA, rule of 10, No hardware specific code,
 * only generic C and some binding layer. Be extra specific about types.
 *
 * Scientific units where posible at end of the names, for example:
 * - timer_10s (timer_10s has a resolution of 10s per bit)
 * - power_150w (power 150W per bit or 0.15kw per bit)
 *
 * Keep variables without units if they're unknown or not specified or hard
 * to define with short notation.
 *
 * ```LICENSE
 * Copyright (c) 2025 furdog <https://github.com/furdog>
 *
 * SPDX-License-Identifier: 0BSD
 * ```
 *
 * Be free, be wise and take care of yourself!
 * With best wishes and respect, furdog
 */

#ifndef TBCM_360_3000_HE_DRI_HEADER_GUARD
#define TBCM_360_3000_HE_DRI_HEADER_GUARD

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/** Link timeout (missing any essential RX frame due certain period) */
#define TBCM_360_3000_HE_DRI_LINK_TIMEOUT_MS 5000u

/******************************************************************************
 * GENERIC CAN FRAME
 *****************************************************************************/
/** General purpose frame flags */
enum tbcm_360_3000_he_dri_frame_flags {
	/** Indicates frame has been changed and requires attention */
	TBCM_360_3000_HE_DRI_FRAME_FLAGS_DIRTY = 1u,

	/** Indicates that frame has been properly received at least once */
	TBCM_360_3000_HE_DRI_FRAME_FLAGS_RECV_AT_LEAST_ONCE = 2u
};

/** Simplified CAN2.0 frame representation */
struct tbcm_360_3000_he_dri_frame {
	/** General purpose timer (May be used for timeout checks) */
	uint32_t timer_ms;

	uint32_t id;	   /**< CAN2.0 Frame ID */
	uint8_t	 len;	   /**< CAN2.0 Frame DLC */
	uint8_t	 data[8u]; /**< CAN2.0 Frame DATA */

	/** General purpose flags set by user */
	uint8_t flags;
};

/*--------------------------------------------------------------- PUBLIC API */
/** Initializes frame. Default zero state */
void tbcm_360_3000_he_dri_frame_init(struct tbcm_360_3000_he_dri_frame *self,
				     const uint32_t id, const uint8_t len);

/** Check if frame is marked as dirty */
bool tbcm_360_3000_he_dri_frame_is_dirty(
    const struct tbcm_360_3000_he_dri_frame *self);

/** Set dirty flag. Returns true if changed state */
bool tbcm_360_3000_he_dri_frame_set_dirty(
    struct tbcm_360_3000_he_dri_frame *self);

/** Reset dirty flag. Returns true if changed state */
bool tbcm_360_3000_he_dri_frame_reset_dirty(
    struct tbcm_360_3000_he_dri_frame *self);

/** Returns true if has received frame at least once */
bool tbcm_360_3000_he_dri_frame_has_recv_once(
    const struct tbcm_360_3000_he_dri_frame *self);

/** Set recv at least once flag */
bool tbcm_360_3000_he_dri_frame_set_recv_once(
    struct tbcm_360_3000_he_dri_frame *self);
/*---------------------------------------------------------------------------*/

#ifdef TBCM_360_3000_HE_DRI_IMPLEMENTATION
void tbcm_360_3000_he_dri_frame_init(struct tbcm_360_3000_he_dri_frame *self,
				     const uint32_t id, const uint8_t len)
{
	assert(self && id && (len <= 8u));

	self->timer_ms = 0u;

	self->id  = id;
	self->len = len;
	(void)memset(self->data, 0u, 8u);

	self->flags = 0u;
}

bool tbcm_360_3000_he_dri_frame_is_dirty(
    const struct tbcm_360_3000_he_dri_frame *self)
{
	assert(self);

	return (self->flags &
		(uint8_t)TBCM_360_3000_HE_DRI_FRAME_FLAGS_DIRTY) > 0u;
}

bool tbcm_360_3000_he_dri_frame_set_dirty(
    struct tbcm_360_3000_he_dri_frame *self)
{
	bool changed = tbcm_360_3000_he_dri_frame_is_dirty(self);

	assert(self);

	self->flags |= TBCM_360_3000_HE_DRI_FRAME_FLAGS_DIRTY;

	return !changed;
}

bool tbcm_360_3000_he_dri_frame_reset_dirty(
    struct tbcm_360_3000_he_dri_frame *self)
{
	bool changed = tbcm_360_3000_he_dri_frame_is_dirty(self);

	assert(self);

	self->flags &= ~TBCM_360_3000_HE_DRI_FRAME_FLAGS_DIRTY;

	return changed;
}

bool tbcm_360_3000_he_dri_frame_has_recv_once(
    const struct tbcm_360_3000_he_dri_frame *self)
{
	assert(self);

	return (self->flags &
		(uint8_t)TBCM_360_3000_HE_DRI_FRAME_FLAGS_RECV_AT_LEAST_ONCE) >
	       0u;
}

bool tbcm_360_3000_he_dri_frame_set_recv_once(
    struct tbcm_360_3000_he_dri_frame *self)
{
	bool changed = tbcm_360_3000_he_dri_frame_has_recv_once(self);

	assert(self);

	self->flags |= TBCM_360_3000_HE_DRI_FRAME_FLAGS_RECV_AT_LEAST_ONCE;

	return !changed;
}
#endif /* TBCM_360_3000_HE_DRI_IMPLEMENTATION */

/******************************************************************************
 * DEVICE
 *****************************************************************************/
/** tbcm_360_3000_he device must be somehow identified in the system.
 *  This descriptor contains all information that is responsible for device
 *  identification. */
struct tbcm_360_3000_he_dri_device_descriptor {
	uint8_t serial_no[6]; /**< Device serial number */
	uint8_t serial_len;   /**< Device serial number length */
	uint8_t id;	      /**< Device session identifier */
};

/*--------------------------------------------------------------- PUBLIC API */
/** Initialize device descriptor */
void tbcm_360_3000_he_dri_device_descriptor_init(
    struct tbcm_360_3000_he_dri_device_descriptor *self);
/*---------------------------------------------------------------------------*/

#ifdef TBCM_360_3000_HE_DRI_IMPLEMENTATION
void tbcm_360_3000_he_dri_device_descriptor_init(
    struct tbcm_360_3000_he_dri_device_descriptor *self)
{
	assert(self);

	(void)memset(self->serial_no, 0u, 6u);

	self->serial_len = 0u;
	self->id	 = 0u;
}
#endif /* TBCM_360_3000_HE_DRI_IMPLEMENTATION */

/******************************************************************************
 * WRITER
 *****************************************************************************/
/** Heartbeat (or serial no ack) message period */
#define TBCM_360_3000_HE_DRI_X351_INTERVAL_MS 1000u

/** Send settings message period */
#define TBCM_360_3000_HE_DRI_X352_INTERVAL_MS 100u

/** Events generated by Writer main fsm(step) function */
enum tbcm_360_3000_he_dri_writer_event {
	TBCM_360_3000_HE_DRI_WRITER_EVENT_NONE,
	TBCM_360_3000_HE_DRI_WRITER_EVENT_READY_TO_SEND
};

/** Writer state. Either sends Serial No frames, or all frames */
enum _tbcm_360_3000_he_dri_writer_state {
	_TBCM_360_3000_HE_DRI_WRITER_STATE_WAIT_DESCRIPTOR,
	_TBCM_360_3000_HE_DRI_WRITER_STATE_SEND_SERIAL_NO,
	_TBCM_360_3000_HE_DRI_WRITER_STATE_SEND_ALL
};

/** Writer TX message queue flags */
enum _tbcm_360_3000_he_dri_writer_flags {
	_TBCM_360_3000_HE_DRI_WRITER_FLAGS_ALLOW_X351 = (1u << 0u),
	_TBCM_360_3000_HE_DRI_WRITER_FLAGS_ALLOW_X352 = (1u << 1u)
};

/** Writer main instance */
struct tbcm_360_3000_he_dri_writer {
	uint8_t _flags;

	/* Telemetry frames */
	struct tbcm_360_3000_he_dri_frame _x351; /* Serial no frame */
	struct tbcm_360_3000_he_dri_frame _x352; /* Settings frame */
};

/*--------------------------------------------------------------- PUBLIC API */
/** Initializes Writer FSM in default, unconfigured, clean state.
 *  Managed internally within main driver.
 *  User should not call this manually */
void tbcm_360_3000_he_dri_writer_init(
    struct tbcm_360_3000_he_dri_writer *self);

/** Sets device serial no for the writer, so it knows who it talks with */
bool tbcm_360_3000_he_dri_writer_set_serial_no(
    struct tbcm_360_3000_he_dri_writer *self, const uint8_t *serial_no,
    const uint8_t serial_no_len);

/** Sets device id for the writer, so it knows who it talks with */
bool tbcm_360_3000_he_dri_writer_set_id(
    struct tbcm_360_3000_he_dri_writer *self, const uint8_t id);

/** POP queued TX frame from Writer. The user is responsible
 *  to send this frame to lower hardware layer.
 *  Returns true if there is a frame queued.
 *  Returns false if there's no frames queued */
bool tbcm_360_3000_he_dri_writer_pop_tx_frame(
    struct tbcm_360_3000_he_dri_writer *self,
    struct tbcm_360_3000_he_dri_frame  *frame);

/** Main writer FSM. Makes a single step, returns simple event */
enum tbcm_360_3000_he_dri_writer_event
tbcm_360_3000_he_dri_writer_step(struct tbcm_360_3000_he_dri_writer *self,
				 const uint32_t delta_time_ms);
/*---------------------------------------------------------------------------*/

#ifdef TBCM_360_3000_HE_DRI_IMPLEMENTATION
void tbcm_360_3000_he_dri_writer_init(struct tbcm_360_3000_he_dri_writer *self)
{
	assert(self);

	self->_flags = 0u;

	tbcm_360_3000_he_dri_frame_init(&self->_x351, 0x351u, 6u);
	tbcm_360_3000_he_dri_frame_init(&self->_x352, 0x352u, 8u);
}

bool tbcm_360_3000_he_dri_writer_set_serial_no(
    struct tbcm_360_3000_he_dri_writer *self, const uint8_t *serial_no,
    const uint8_t serial_no_len)
{
	bool valid = false;

	assert(self && serial_no);

	/* Validate state and descriptor serial no len */
	if ((self->_flags == 0u) && (serial_no_len == 6u)) {
		valid = true;

		(void)memcpy(self->_x351.data, serial_no, serial_no_len);
		/*self->_x352.data[0] = desc->id;*/

		/* Set to exceed send interval, so it fires immediately */
		self->_x351.timer_ms = TBCM_360_3000_HE_DRI_X351_INTERVAL_MS;

		/* Set new state */
		self->_flags |= _TBCM_360_3000_HE_DRI_WRITER_FLAGS_ALLOW_X351;
	}

	return valid;
}

bool tbcm_360_3000_he_dri_writer_set_id(
    struct tbcm_360_3000_he_dri_writer *self, const uint8_t id)
{
	bool valid = false;

	assert(self);

	/* Validate state */
	if (self->_flags ==
	    (uint8_t)_TBCM_360_3000_HE_DRI_WRITER_FLAGS_ALLOW_X351) {
		valid = true;

		self->_x352.data[0] = id;

		/* Set to exceed send interval, so it fires immediately */
		self->_x352.timer_ms = TBCM_360_3000_HE_DRI_X352_INTERVAL_MS;

		self->_flags |= _TBCM_360_3000_HE_DRI_WRITER_FLAGS_ALLOW_X352;
	}

	return valid;
}

bool tbcm_360_3000_he_dri_writer_pop_tx_frame(
    struct tbcm_360_3000_he_dri_writer *self,
    struct tbcm_360_3000_he_dri_frame  *frame)
{
	bool has_frame = false;

	assert(self && frame);

	if (tbcm_360_3000_he_dri_frame_reset_dirty(&self->_x351)) {
		*frame	  = self->_x351;
		has_frame = true;
	} else if (tbcm_360_3000_he_dri_frame_reset_dirty(&self->_x352)) {
		*frame	  = self->_x352;
		has_frame = true;
	} else {
	}

	return has_frame;
}

enum tbcm_360_3000_he_dri_writer_event
tbcm_360_3000_he_dri_writer_step(struct tbcm_360_3000_he_dri_writer *self,
				 const uint32_t delta_time_ms)
{
	enum tbcm_360_3000_he_dri_writer_event event =
	    TBCM_360_3000_HE_DRI_WRITER_EVENT_NONE;

	assert(self);

	if ((self->_flags &
	     (uint8_t)_TBCM_360_3000_HE_DRI_WRITER_FLAGS_ALLOW_X351) > 0u) {
		self->_x351.timer_ms += delta_time_ms;

		if (self->_x351.timer_ms >=
		    TBCM_360_3000_HE_DRI_X351_INTERVAL_MS) {
			/* For periodic timers we substract period
			 * from a timer variable. For interval timers, we
			 * set timer variable to zero. */
			self->_x351.timer_ms = 0u;

			(void)tbcm_360_3000_he_dri_frame_set_dirty(
			    &self->_x351);

			event =
			    TBCM_360_3000_HE_DRI_WRITER_EVENT_READY_TO_SEND;
		}
	}

	if ((self->_flags &
	     (uint8_t)_TBCM_360_3000_HE_DRI_WRITER_FLAGS_ALLOW_X352) > 0u) {
		self->_x352.timer_ms += delta_time_ms;

		if (self->_x352.timer_ms >=
		    TBCM_360_3000_HE_DRI_X352_INTERVAL_MS) {
			self->_x352.timer_ms = 0u;
			(void)tbcm_360_3000_he_dri_frame_set_dirty(
			    &self->_x352);

			event =
			    TBCM_360_3000_HE_DRI_WRITER_EVENT_READY_TO_SEND;
		}
	}

	return event;
}
#endif /* TBCM_360_3000_HE_DRI_IMPLEMENTATION */

/******************************************************************************
 * READER
 *****************************************************************************/
/** Events generated by Reader FSM */
enum tbcm_360_3000_he_dri_reader_event {
	/** No event */
	TBCM_360_3000_HE_DRI_READER_EVENT_NONE,

	/** Discovered new device */
	TBCM_360_3000_HE_DRI_READER_EVENT_DISCOVERY,

	/** RX timeout */
	TBCM_360_3000_HE_DRI_READER_EVENT_TIMEOUT
};

/** Reader state. */
enum _tbcm_360_3000_he_dri_reader_state {
	/** Device discovery state */
	_TBCM_360_3000_HE_DRI_READER_STATE_DISCOVERY,

	/** Main state, where all checks performed */
	_TBCM_360_3000_HE_DRI_READER_STATE_SERVING,

	/** Timeout */
	_TBCM_360_3000_HE_DRI_READER_STATE_TIMEOUT
};

/** RX flags that indicate frame presence in RX queue */
enum _tbcm_360_3000_he_dri_reader_flags {
	_TBCM_360_3000_HE_DRI_READER_FLAGS_GOT_X350 = (1u << 1u),
	_TBCM_360_3000_HE_DRI_READER_FLAGS_GOT_X353 = (1u << 2u),
	_TBCM_360_3000_HE_DRI_READER_FLAGS_GOT_X354 = (1u << 3u),
	_TBCM_360_3000_HE_DRI_READER_FLAGS_GOT_X355 = (1u << 4u),
	_TBCM_360_3000_HE_DRI_READER_FLAGS_GOT_ALL  = (1u << 5u)
};

/** Main reader instance */
struct tbcm_360_3000_he_dri_reader {
	uint8_t _state;

	uint8_t _flags;

	struct tbcm_360_3000_he_dri_frame _x350; /**< Serial No */
	struct tbcm_360_3000_he_dri_frame _x353; /**< Data frame */
	struct tbcm_360_3000_he_dri_frame _x354; /**< Data frame */
	struct tbcm_360_3000_he_dri_frame _x355; /**< Data frame */
};

/*--------------------------------------------------------------- PUBLIC API */
/** Initializes Reader */
void tbcm_360_3000_he_dri_reader_init(
    struct tbcm_360_3000_he_dri_reader *self);

/** PUSH RX frame to reader. The user is responsible
 *  to pop frame from lower hardware layer.
 *  Returns true if sucessfully reads the frame.
 *  Returns false if busy. May reset frame dirty flag (if it was consumed)
 */
bool tbcm_360_3000_he_dri_reader_push_rx_frame(
    struct tbcm_360_3000_he_dri_reader	    *self,
    struct tbcm_360_3000_he_dri_frame *frame);

/** Returns length and pointer to device serial no, if available */
uint8_t tbcm_360_3000_he_dri_reader_get_serial_no(
    struct tbcm_360_3000_he_dri_reader *self, const uint8_t **serial_no);

/** Accept discovered device. Returns false if in invalid state */
bool tbcm_360_3000_he_dri_reader_accept_serial_no(
    struct tbcm_360_3000_he_dri_reader *self);

/** Reject discovered device. Returns false if in invalid state */
bool tbcm_360_3000_he_dri_reader_reject_serial_no(
    struct tbcm_360_3000_he_dri_reader *self);

/** Main Reader FSM function. Does single step. Returns simple event. */
enum tbcm_360_3000_he_dri_reader_event
tbcm_360_3000_he_dri_reader_step(struct tbcm_360_3000_he_dri_reader *self,
				 uint32_t delta_time_ms);
/*---------------------------------------------------------------------------*/

#ifdef TBCM_360_3000_HE_DRI_IMPLEMENTATION
void tbcm_360_3000_he_dri_reader_init(struct tbcm_360_3000_he_dri_reader *self)
{
	assert(self);

	self->_state = _TBCM_360_3000_HE_DRI_READER_STATE_DISCOVERY;

	self->_flags = 0u;

	tbcm_360_3000_he_dri_frame_init(&self->_x350, 0x350u, 6u);
	tbcm_360_3000_he_dri_frame_init(&self->_x353, 0x353u, 8u);
	tbcm_360_3000_he_dri_frame_init(&self->_x354, 0x354u, 8u);
	tbcm_360_3000_he_dri_frame_init(&self->_x355, 0x355u, 8u);

	/* TODO check if busy for too long */
}

/** Validate message ID. Will return true only if discovered a device and if
 * message ID matches this device */
bool _tbcm_360_3000_he_dri_reader_validate_id(
    struct tbcm_360_3000_he_dri_reader	    *self,
    const struct tbcm_360_3000_he_dri_frame *frame)
{
	bool valid = false;

	assert(self && frame);

	if ((self->_state ==
	     (uint8_t)_TBCM_360_3000_HE_DRI_READER_STATE_SERVING) &&
	    (frame->data[0] == self->_x350.data[0])) {
		valid = true;
	}

	return valid;
}

/** Parse incoming "serial no" message */
bool _tbcm_360_3000_he_dri_reader_parse_serial_no(
    struct tbcm_360_3000_he_dri_reader	    *self,
    struct tbcm_360_3000_he_dri_frame *frame)
{
	bool valid = false;
	bool busy  = false;

	assert(self && frame);

	/* Skip frame with invalid length */
	if (frame->len > 6U) {
		valid = false;
	}
	/* The system has not processed previous frame */
	else if (tbcm_360_3000_he_dri_frame_is_dirty(&self->_x350)) {
		busy = true;
	}
	/* If "serial no" has been marked as "received at least once",
	 * which basically means it was previously accepted by the user */
	else if (tbcm_360_3000_he_dri_frame_has_recv_once(&self->_x350)) {
		/* Check "serial no" */
		if ((frame->len == self->_x350.len) &&
		    (memcmp(frame->data, self->_x350.data, frame->len) == 0)) {
			valid = true;

			/* Reset reception timer to prevent timeout */
			self->_x350.timer_ms = 0u;
		}
	} else {
		valid = true;

		/* Register "serial no" in the system */
		self->_x350 = *frame;

		/* Flag this frame as dirty, so the system will be forced
		 * to tell user either accept or reject "serial no" */
		(void)tbcm_360_3000_he_dri_frame_set_dirty(&self->_x350);
	}

	/* Reset original frame dirty flag,
	 * to signalize it has been successfully consumed and no further
	 * processing is needed. */
	if (valid) {
		(void)tbcm_360_3000_he_dri_frame_reset_dirty(frame);
	}

	return busy;
}

bool tbcm_360_3000_he_dri_reader_push_rx_frame(
    struct tbcm_360_3000_he_dri_reader	    *self,
    struct tbcm_360_3000_he_dri_frame *frame)
{
	bool busy = false;

	assert(self && frame);

	switch (frame->id) {
	/* Got device serial no */
	case 0x350u:
		busy =
		    _tbcm_360_3000_he_dri_reader_parse_serial_no(self, frame);
		break;

	case 0x353u:
		if (tbcm_360_3000_he_dri_frame_is_dirty(&self->_x353)) {
			break;
		}

		/* Validate reader ID */
		if (!_tbcm_360_3000_he_dri_reader_validate_id(self, frame)) {
			break;
		}

		self->_x353 = *frame;

		/* 0 1 2 3 4 5 6 7, MSB first */
		/* byte 6, 7 - output voltage * 10 (assumed) */
		/* byte 4, 5 - output current * 10 (assumed) */

		(void)tbcm_360_3000_he_dri_frame_set_recv_once(&self->_x353);

		break;

	case 0x354u:
		if (tbcm_360_3000_he_dri_frame_is_dirty(&self->_x354)) {
			break;
		}

		/* Validate reader ID */
		if (!_tbcm_360_3000_he_dri_reader_validate_id(self, frame)) {
			break;
		}

		self->_x354 = *frame;

		/* 0 1 2 3 4 5 6 7 MSB first */
		/* byte 1 temp1 celsius * 1 (assumed) */
		/* byte 2 temp2 celsius * 1 (assumed) */
		/* byte 3?, 4 input voltage * 1 (assumed) */
		/* byte 7 has to do something with input voltage too */

		(void)tbcm_360_3000_he_dri_frame_set_recv_once(&self->_x354);

		break;

	case 0x355u:
		if ((self->_flags &
		     (uint8_t)_TBCM_360_3000_HE_DRI_READER_FLAGS_GOT_X355) >
		    0u) {
			break;
		}

		/* Validate reader ID */
		if (!_tbcm_360_3000_he_dri_reader_validate_id(self, frame)) {
			break;
		}

		self->_x355 = *frame;

		/* Unknown, all zeros, maybe error flags? */

		(void)tbcm_360_3000_he_dri_frame_set_recv_once(&self->_x355);

		break;

	default:
		break;
	}

	return !busy; /* Return true if not busy */
}

uint8_t tbcm_360_3000_he_dri_reader_get_serial_no(
    struct tbcm_360_3000_he_dri_reader *self, const uint8_t **serial_no)
{
	assert(self && serial_no);

	*serial_no = self->_x350.data;

	return self->_x350.len;
}

bool tbcm_360_3000_he_dri_reader_accept_serial_no(
    struct tbcm_360_3000_he_dri_reader *self)
{
	bool valid = false;

	assert(self);

	if ((self->_state ==
	     (uint8_t)_TBCM_360_3000_HE_DRI_READER_STATE_DISCOVERY) &&
	    tbcm_360_3000_he_dri_frame_is_dirty(&self->_x350)) {
		/* Validate frame (set as recv at least once) */
		(void)tbcm_360_3000_he_dri_frame_set_recv_once(&self->_x350);

		/* Mark it processed */
		(void)tbcm_360_3000_he_dri_frame_reset_dirty(&self->_x350);

		valid = true;
	}

	return valid;
}

bool tbcm_360_3000_he_dri_reader_reject_serial_no(struct tbcm_360_3000_he_dri_reader *self)
{
}

enum tbcm_360_3000_he_dri_reader_event
tbcm_360_3000_he_dri_reader_step(struct tbcm_360_3000_he_dri_reader *self,
				 uint32_t delta_time_ms)
{
	enum tbcm_360_3000_he_dri_reader_event ev =
	    TBCM_360_3000_HE_DRI_READER_EVENT_NONE;

	(void)delta_time_ms;

	assert(self);

	switch (self->_state) {
	/* Initial state. Seek for any new devices on the bus */
	case _TBCM_360_3000_HE_DRI_READER_STATE_DISCOVERY:
		/* Check if we've received any serial no frame */
		if (tbcm_360_3000_he_dri_frame_is_dirty(&self->_x350)) {
			ev = TBCM_360_3000_HE_DRI_READER_EVENT_DISCOVERY;

			/* We're waiting for ACCEPT or REJECT from the user
			 * at this point */
		}

		break;

	case _TBCM_360_3000_HE_DRI_READER_STATE_SERVING:
		/*if (self->_link_timer_ms >=
		    TBCM_360_3000_HE_DRI_LINK_TIMEOUT_MS) {
			self->_state =
			    _TBCM_360_3000_HE_DRI_READER_STATE_TIMEOUT;

			ev = TBCM_360_3000_HE_DRI_READER_EVENT_NONE;
		}*/

		break;

	case _TBCM_360_3000_HE_DRI_READER_STATE_TIMEOUT:
		break;

	default:
		break;
	}

	return ev;
}
#endif /* TBCM_360_3000_HE_DRI_IMPLEMENTATION */

#endif /* TBCM_360_3000_HE_DRI_HEADER_GUARD */
