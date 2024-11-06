#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>

#include "dmx.h"

#define	PRU_BASE	0x4a300000U
#define	PRU_SHAREDMEM	(PRU_BASE + 0x1000U)

#define USEC	200UL
#define MSEC	200000UL

static void
print_sr(volatile struct shared_ram *sr)
{
	printf("sr_pru_flags = %x\n", sr->sr_pru_flags);
	printf("sr_arm_flags = %x\n", sr->sr_arm_flags);
	printf("sr_frame_intvl = %u\n", sr->sr_frame_intvl);
	printf("sr_frame_num = %u\n", sr->sr_frame_num);
	printf("sr_usel = %d\n", sr->sr_usel);
	printf("sr_uactive = %d\n", sr->sr_uactive);
}

int
main(int argc, char *argv[])
{
	int fd;
	uint i;
	int usel;
	volatile uint8_t *dp;
	volatile struct shared_ram *sr;
	volatile struct dmx_universe *u;
	int setup = 0;
	uint v;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0)
		err(1, "open(/dev/mem)");
	sr = mmap(NULL, 0x40000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
	    fd, PRU_SHAREDMEM);
	if (sr == MAP_FAILED)
		err(1, "mmap");

	//print_sr(sr);

	if (!(sr->sr_pru_flags & PRU_RUNNING))
		errx(1, "PRU not running?");

	if (!(sr->sr_arm_flags & DMX_ENA)) {
		setup = 1;
		fprintf(stderr, "doing initial setup\n");

		sr->sr_pru_flags = 0;
		__sync_synchronize();

		for (i = 0; i < 5; ++i) {
			usleep(100000);
			__sync_synchronize();
			if (sr->sr_pru_flags & PRU_RUNNING)
				break;
		}

		if (!(sr->sr_pru_flags & PRU_RUNNING))
			errx(1, "PRU not running?");
	}

	while (sr->sr_uactive != sr->sr_usel) {
		usleep(100000);
		__sync_synchronize();
	}

	if (sr->sr_usel == 0) {
		u = &sr->sr_u1;
		usel = 1;
	} else {
		u = &sr->sr_u0;
		usel = 0;
	}

	u->u_size = 48;
	u->u_start = 0;
	for (i = 0; i < 48; ++i)
		u->u_slot[i] = 0;
	__sync_synchronize();
	
	sr->sr_usel = usel;
	sr->sr_frame_intvl = 10*MSEC;	/* 10ms / 100fps */

	if (setup) {
		__sync_synchronize();
		sr->sr_arm_flags |= DMX_ENA;

		while (sr->sr_frame_num == 0)
			__sync_synchronize();
	}

	//print_sr(sr);

	while (1) {
		for (v = 0; v < 255; v += 1) {
			while (sr->sr_uactive != sr->sr_usel) {
				usleep(50);
				__sync_synchronize();
			}

			if (sr->sr_usel == 0) {
				u = &sr->sr_u1;
				usel = 1;
			} else {
				u = &sr->sr_u0;
				usel = 0;
			}

			u->u_size = 48;
			u->u_start = 0;
			for (i = 0; i < 48; ++i)
				u->u_slot[i] = (i & 1) ? v : 0;
			__sync_synchronize();

			sr->sr_usel = usel;
			__sync_synchronize();

			usleep(1000);
		}
		for (v = 255; v > 0; v -= 1) {
			while (sr->sr_uactive != sr->sr_usel) {
				usleep(50);
				__sync_synchronize();
			}

			if (sr->sr_usel == 0) {
				u = &sr->sr_u1;
				usel = 1;
			} else {
				u = &sr->sr_u0;
				usel = 0;
			}

			u->u_size = 48;
			u->u_start = 0;
			for (i = 0; i < 48; ++i)
				u->u_slot[i] = (i & 1) ? v : 0;
			__sync_synchronize();

			sr->sr_usel = usel;
			__sync_synchronize();

			usleep(1000);
		}
		for (v = 0; v < 255; v += 1) {
			while (sr->sr_uactive != sr->sr_usel) {
				usleep(50);
				__sync_synchronize();
			}

			if (sr->sr_usel == 0) {
				u = &sr->sr_u1;
				usel = 1;
			} else {
				u = &sr->sr_u0;
				usel = 0;
			}

			u->u_size = 48;
			u->u_start = 0;
			for (i = 1; i < 48; ++i)
				u->u_slot[i] = (i & 1) ? 0 : v;
			__sync_synchronize();

			sr->sr_usel = usel;
			__sync_synchronize();

			usleep(1000);
		}
		for (v = 255; v > 0; v -= 1) {
			while (sr->sr_uactive != sr->sr_usel) {
				usleep(50);
				__sync_synchronize();
			}

			if (sr->sr_usel == 0) {
				u = &sr->sr_u1;
				usel = 1;
			} else {
				u = &sr->sr_u0;
				usel = 0;
			}

			u->u_size = 48;
			u->u_start = 0;
			for (i = 1; i < 48; ++i)
				u->u_slot[i] = (i & 1) ? 0 : v;
			__sync_synchronize();

			sr->sr_usel = usel;
			__sync_synchronize();

			usleep(1000);
		}
	}

	return (0);
}
