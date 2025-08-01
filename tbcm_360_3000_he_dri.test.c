#include "tbcm_360_3000_he_dri.h"
#include <assert.h>
#include <stdio.h>

struct tbcm_360_3000_he_dri dri;

/* Snapshot to reset dri to previous state */
struct tbcm_360_3000_he_dri dri_snapshot;

int main()
{
	struct tbcm_360_3000_he_dri_frame frame = {
		0x350U, 6U,
		{ 0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU }
	};

	tbcm_360_3000_he_dri_init(&dri);

	/* No events at init should occur (maybe?) */
	assert(tbcm_360_3000_he_dri_update(&dri, 0U) ==
					TBCM_360_3000_HE_DRI_EVENT_NONE);

	/* Accept serial number */
	tbcm_360_3000_he_dri_write_frame(&dri, &frame);
	assert(tbcm_360_3000_he_dri_update(&dri, 0U) ==
					TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO);

	printf("Discovered device serial number: %s\n",
		tbcm_360_3000_he_dri_get_serial_no(&dri));

	/* Compare serial numbers */
	assert(strcmp(tbcm_360_3000_he_dri_get_serial_no(&dri),
		      "0123456789AB") == 0U);

	/* Ack serial number */
	tbcm_360_3000_he_dri_ack_serial_no(&dri);

	/* Should fault, since serial_no contains 0xAB */
	assert(tbcm_360_3000_he_dri_update(&dri, 0U) == 
					   TBCM_360_3000_HE_DRI_EVENT_FAULT);

	/* No frames should be sent to device YET */
	assert(tbcm_360_3000_he_dri_read_frame(&dri, &frame) == false);

	/* Should succed */
	frame.data[5] = 0x00U;
	tbcm_360_3000_he_dri_write_frame(&dri, &frame);
	assert(tbcm_360_3000_he_dri_update(&dri, 0U) ==
					TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO);
	printf("Discovered device serial number: %s\n",
		tbcm_360_3000_he_dri_get_serial_no(&dri));
	tbcm_360_3000_he_dri_ack_serial_no(&dri);
	assert(tbcm_360_3000_he_dri_update(&dri, 0U) == 
					      TBCM_360_3000_HE_DRI_EVENT_NONE);
	assert(dri._state == TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE);

	/* Only one frame should be received */
	assert(tbcm_360_3000_he_dri_read_frame(&dri, &frame) == true);
	assert(tbcm_360_3000_he_dri_read_frame(&dri, &frame) == false);

	assert(frame.id  == 0x351U);
	assert(frame.len == 6U);
	assert(frame.data[0] == 0x01U);
	assert(frame.data[1] == 0x23U);
	assert(frame.data[2] == 0x45U);
	assert(frame.data[3] == 0x67U);
	assert(frame.data[4] == 0x89U);
	assert(frame.data[5] == 0x00U);

	/* Must repeat after one scond */
	tbcm_360_3000_he_dri_update(&dri, 999);
	assert(tbcm_360_3000_he_dri_read_frame(&dri, &frame) == false);
	tbcm_360_3000_he_dri_update(&dri, 1);
	assert(tbcm_360_3000_he_dri_read_frame(&dri, &frame) == true);

	/* Check device ID reception */
	assert(dri._device_id == 0U);
	frame.id  = 0x352U; /* Should discard this ID */
	frame.len = 8U;
	tbcm_360_3000_he_dri_write_frame(&dri, &frame);
	assert(tbcm_360_3000_he_dri_update(&dri, 0U) == 
					 TBCM_360_3000_HE_DRI_EVENT_NONE);
	assert(dri._device_id == 0U);
	frame.id  = 0x353U; /* Should accept this ID */
	tbcm_360_3000_he_dri_write_frame(&dri, &frame);
	assert(tbcm_360_3000_he_dri_update(&dri, 0U) == 
					 TBCM_360_3000_HE_DRI_EVENT_DEVICE_ID);
	assert(dri._device_id == 1U);
	printf("Discovered device id: %u\n",
		tbcm_360_3000_he_dri_get_device_id(&dri));

	tbcm_360_3000_he_dri_ack_device_id(&dri);
	assert(tbcm_360_3000_he_dri_update(&dri, 0U) == 
				       TBCM_360_3000_HE_DRI_EVENT_ESTABLISHED);

	return 0;
}
