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
		{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF }
	};

	tbcm_360_3000_he_dri_init(&dri);

	/* Frame should be discarded before listening */
	tbcm_360_3000_he_dri_write_frame(&dri, &frame);
	tbcm_360_3000_he_dri_start_listen_for_devices(&dri);
	assert(tbcm_360_3000_he_dri_update(&dri, 0) == 
					      TBCM_360_3000_HE_DRI_EVENT_NONE);

	/* Frame should be accepted after listening */
	tbcm_360_3000_he_dri_write_frame(&dri, &frame);
	assert(tbcm_360_3000_he_dri_update(&dri, 0) ==
					TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO);

	printf("Discovered device serial number: %s\n",
		tbcm_360_3000_he_dri_get_serial_no(&dri));

	/* Compare serial numbers */
	assert(strcmp(tbcm_360_3000_he_dri_get_serial_no(&dri),
		      "0123456789AB") == 0);

	/* Select device (NULL might be passed for current serial No) */
	dri_snapshot = dri;
	tbcm_360_3000_he_dri_select_device(&dri, "0123456789AB");
	dri = dri_snapshot;
	tbcm_360_3000_he_dri_select_device(&dri, NULL);

	/* Should fault, since serial_no contains 0xAB */
	assert(tbcm_360_3000_he_dri_update(&dri, 0) == 
					      TBCM_360_3000_HE_DRI_EVENT_NONE);
	assert(dri._state == TBCM_360_3000_HE_DRI_STATE_FAULT);

	/* No frames should be sent to device YET */
	assert(tbcm_360_3000_he_dri_read_frame(&dri, &frame) == false);

	/* Should succed */
	tbcm_360_3000_he_dri_select_device(&dri, "012345678900");
	assert(tbcm_360_3000_he_dri_update(&dri, 0) == 
					      TBCM_360_3000_HE_DRI_EVENT_NONE);					
	assert(dri._state == TBCM_360_3000_HE_DRI_STATE_QUERY_DEVICE);

	/* Only one frame should be received */
	assert(tbcm_360_3000_he_dri_read_frame(&dri, &frame) == true);
	assert(tbcm_360_3000_he_dri_read_frame(&dri, &frame) == false);

	assert(frame.id  == 0x351U);
	assert(frame.len == 6U);
	assert(frame.data[0] == 0x01);
	assert(frame.data[1] == 0x23);
	assert(frame.data[2] == 0x45);
	assert(frame.data[3] == 0x67);
	assert(frame.data[4] == 0x89);
	assert(frame.data[5] == 0x00);

	return 0;
}
