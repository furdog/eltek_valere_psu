#define TBCM_360_3000_HE_DRI_IMPLEMENTATION
#include "tbcm_360_3000_he_dri.h"

#include <limits.h>

void tbcm_360_3000_he_dri_writer_test(struct tbcm_360_3000_he_dri_writer *self)
{
	struct tbcm_360_3000_he_dri_frame frame;
	const uint8_t			  serial_no[6] = {0x12u, 0x34u, 0x56u,
							  0x78u, 0x9Au, 0xBCu};

	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) ==
	       false);

	assert(tbcm_360_3000_he_dri_writer_step(self, 0u) ==
	       TBCM_360_3000_HE_DRI_WRITER_EVENT_NONE);

	/* Nothing should fire in any amount of time,
	 * until device descriptor is passed */
	assert(tbcm_360_3000_he_dri_writer_step(self, UINT_MAX) ==
	       TBCM_360_3000_HE_DRI_WRITER_EVENT_NONE);

	/* test fail if no serial len has been set */
	assert(tbcm_360_3000_he_dri_writer_set_serial_no(self, serial_no,
							 0u) == false);

	/* Do not allow send settings before descriptor is set */
	assert(tbcm_360_3000_he_dri_writer_set_id(self, 0u) == false);

	/* OK */
	assert(tbcm_360_3000_he_dri_writer_set_serial_no(self, serial_no,
							 6u) == true);

	/* Test fail on invalid state (already has been set) */
	assert(tbcm_360_3000_he_dri_writer_set_serial_no(self, serial_no,
							 6u) == false);

	/* Three stage timed test */
	assert(tbcm_360_3000_he_dri_writer_step(self, 0u) ==
	       TBCM_360_3000_HE_DRI_WRITER_EVENT_READY_TO_SEND);
	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) == true);
	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) ==
	       false);

	assert(tbcm_360_3000_he_dri_writer_step(
		   self, TBCM_360_3000_HE_DRI_X351_INTERVAL_MS - 1u) ==
	       TBCM_360_3000_HE_DRI_WRITER_EVENT_NONE);

	assert(tbcm_360_3000_he_dri_writer_step(self, 1u) ==
	       TBCM_360_3000_HE_DRI_WRITER_EVENT_READY_TO_SEND);

	/* Test output */
	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) == true);
	assert((frame.id == 0x351u) && (frame.len == 6u));
	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) ==
	       false);

	/* Allow send of settings + state validation check fail */
	assert(tbcm_360_3000_he_dri_writer_set_id(self, 0u) == true);
	assert(tbcm_360_3000_he_dri_writer_set_id(self, 0u) == false);

	/* Three stage timed test */
	assert(tbcm_360_3000_he_dri_writer_step(self, 0u) ==
	       TBCM_360_3000_HE_DRI_WRITER_EVENT_READY_TO_SEND);
	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) == true);
	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) ==
	       false);

	assert(tbcm_360_3000_he_dri_writer_step(
		   self, TBCM_360_3000_HE_DRI_X352_INTERVAL_MS - 1u) ==
	       TBCM_360_3000_HE_DRI_WRITER_EVENT_NONE);

	assert(tbcm_360_3000_he_dri_writer_step(self, 1u) ==
	       TBCM_360_3000_HE_DRI_WRITER_EVENT_READY_TO_SEND);

	/* Test output */
	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) == true);
	/* printf("%x, %u\n", frame.id, frame.len); */
	assert((frame.id == 0x352u) && (frame.len == 8u));
	assert(tbcm_360_3000_he_dri_writer_pop_tx_frame(self, &frame) ==
	       false);
}

void tbcm_360_3000_he_dri_reader_test(struct tbcm_360_3000_he_dri_reader *self)
{
	struct tbcm_360_3000_he_dri_frame	      frame;
	struct tbcm_360_3000_he_dri_device_descriptor desc;

	const uint8_t serial_no[6] = {0x12u, 0x34u, 0x56u,
				      0x78u, 0x9Au, 0xBCu};

	tbcm_360_3000_he_dri_device_descriptor_init(&desc);

	/* Init "serial no" message */
	tbcm_360_3000_he_dri_frame_init(&frame, 0x350, 6u);
	memcpy(frame.data, serial_no, 6u);

	assert(tbcm_360_3000_he_dri_reader_step(self, 0u) ==
	       TBCM_360_3000_HE_DRI_READER_EVENT_NONE);

	/* Can't accept anything that was not even received */
	assert(tbcm_360_3000_he_dri_reader_get_serial_no(self, &desc) ==
	       false);
	assert(tbcm_360_3000_he_dri_reader_accept_serial_no(self) == false);

	/* Receive "serial no" */
	assert(tbcm_360_3000_he_dri_reader_push_rx_frame(self, &frame) ==
	       true);

	assert(tbcm_360_3000_he_dri_reader_step(self, 0u) ==
	       TBCM_360_3000_HE_DRI_READER_EVENT_DISCOVERY);
	assert(tbcm_360_3000_he_dri_reader_step(self, 0u) ==
	       TBCM_360_3000_HE_DRI_READER_EVENT_DISCOVERY);

	assert(desc.serial_no == NULL);
	assert(tbcm_360_3000_he_dri_reader_get_serial_no(self, &desc) == true);
	assert(desc.serial_no != NULL);

	assert(memcmp(desc.serial_no, serial_no, 6u) == 0);
	assert(tbcm_360_3000_he_dri_reader_accept_serial_no(self) == true);

	assert(tbcm_360_3000_he_dri_reader_step(self, 0u) ==
	       TBCM_360_3000_HE_DRI_READER_EVENT_NONE);

	/* Receive "serial no again" */
	assert(tbcm_360_3000_he_dri_reader_push_rx_frame(self, &frame) ==
	       true);
}

int main()
{
	struct tbcm_360_3000_he_dri_writer wrt;
	struct tbcm_360_3000_he_dri_reader rdr;

	tbcm_360_3000_he_dri_writer_init(&wrt);
	tbcm_360_3000_he_dri_writer_test(&wrt);

	tbcm_360_3000_he_dri_reader_init(&rdr);
	tbcm_360_3000_he_dri_reader_test(&rdr);

	return 0;
}
