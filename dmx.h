#if !defined(_DMX_H)
#define	_DMX_H

#include <stdint.h>

enum dmx_pru_flags {
	PRU_RUNNING	= (1<<0),
};

enum dmx_arm_flags {
	DMX_ENA		= (1<<0),
};

#pragma pack(1)

struct dmx_universe {
	uint16_t		u_size;
	uint16_t		u_start;
	uint8_t			u_slot[512];
};

struct shared_ram {
	uint32_t		sr_arm_flags;
	uint32_t		sr_pru_flags;

	uint32_t		sr_frame_intvl;
	uint32_t		sr_frame_num;

	uint32_t		sr_usel;
	uint32_t		sr_uactive;
	struct dmx_universe	sr_u0;
	struct dmx_universe	sr_u1;
};


#endif
