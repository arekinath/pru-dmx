#include <stdint.h>
#include <stddef.h>

#include <rsc_types.h>
#include <am335x/pru_cfg.h>
#include <am335x/pru_ctrl.h>
#include <am335x/pru_iep.h>
#include <am335x/pru_intc.h>

#include "prugpio.h"
#include "dmx.h"

#define	P9_11	(1UL<<30)
#define	P9_13	(1UL<<31)

#define	DMX_TX	P9_13
#define	DMX_RX	P9_11
#define	PRU_IEP_EVT	0x7

volatile register uint32_t __R30;
volatile register uint32_t __R31;

//#define	USEC	200
//#define	MSEC	200000
#define USEC	200U
#define	MSEC	200000U

#pragma DATA_SECTION(srdata, ".shared")
volatile struct shared_ram srdata;
#pragma DATA_SECTION(testbuf, ".shared")
volatile uint8_t testbuf[16];

static void
setup_iep_timer(void)
{
	/* Disable the counter */
	CT_IEP.TMR_GLB_CFG_bit.CNT_EN = 0;

	/* Reset state */
	CT_IEP.TMR_CNT = 0xFFFFFFFF;
	CT_IEP.TMR_COMPEN_bit.COMPEN_CNT = 0;
	CT_IEP.TMR_CMP_STS_bit.CMP_HIT = 0xFF;

	/* Count cycles at 200MHz (OCP clock) */
	CT_IEP.TMR_GLB_CFG_bit.DEFAULT_INC = 1;

	/* Enable comparator overflow, and counter reset on overflow */
	CT_IEP.TMR_GLB_STS_bit.CNT_OVF = 0x1;
	CT_IEP.TMR_CMP_CFG_bit.CMP0_RST_CNT_EN = 1;
	CT_IEP.TMR_CMP_CFG_bit.CMP_EN = 1;

	/* Now set up the INTC */
	CT_INTC.SIPR0 = 0xFFFFFFFF;
	CT_INTC.CMR1 = 0x00000000;
	CT_INTC.CMR1_bit.CH_MAP_7 = 0;
	CT_INTC.HMR0_bit.HINT_MAP_0 = 0;
	CT_INTC.ESR0 = 0x80;
	CT_INTC.HIER = 1;
	CT_INTC.GER = 1;
}

static void
await_iep_timer(void)
{
	/* Poll until timer expires. */
	do {
		while ((__R31 & 0x40000000) == 0) {
			;
		}
	} while (CT_INTC.HIPIR0 != PRU_IEP_EVT);
	CT_INTC.SECR0 = (1 << PRU_IEP_EVT);
	CT_IEP.TMR_CMP_STS_bit.CMP_HIT = 1;
}

static void
write_byte(uint8_t v)
{
	volatile uint32_t *gpio0 = (void *)GPIO0;

	/* Start bit */
	gpio0[GPIO_CLEARDATAOUT] = DMX_TX;
	__delay_cycles(4*USEC);

	/* Data bits */
#define	DATABIT(MASK)	do { \
	if ((v & (MASK)) != 0) {	\
		gpio0[GPIO_SETDATAOUT] = DMX_TX;	\
	} else {	\
		gpio0[GPIO_CLEARDATAOUT] = DMX_TX;	\
	} \
	__delay_cycles(4*USEC); \
	} while (0)
	DATABIT(0x01);
	DATABIT(0x02);
	DATABIT(0x04);
	DATABIT(0x08);
	DATABIT(0x10);
	DATABIT(0x20);
	DATABIT(0x40);
	DATABIT(0x80);

	/* Stop bits */
	gpio0[GPIO_SETDATAOUT] = DMX_TX;
	__delay_cycles(8*USEC);
}

void
main(void)
{
	volatile uint32_t *gpio0 = (void *)GPIO0;
	volatile struct shared_ram *sr = &srdata;
	uint8_t usel;
	uint32_t i;
	volatile struct dmx_universe *u;

	/* Enable OCP master port. */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

	/* Enable cycle counter */
	PRU0_CTRL.CTRL_bit.CTR_EN = 1;

	sr->sr_pru_flags = 0;
	sr->sr_arm_flags = 0;
	sr->sr_frame_intvl = 0;
	sr->sr_frame_num = 0;
	sr->sr_usel = 0;
	sr->sr_uactive = 0;

	gpio0[GPIO_OE] &= ~DMX_TX;
	gpio0[GPIO_SETDATAOUT] = DMX_TX;
	__delay_cycles(10);

	setup_iep_timer();

	while (1) {
		while (!(sr->sr_arm_flags & DMX_ENA)) {
			sr->sr_pru_flags |= PRU_RUNNING;
		}

		CT_IEP.TMR_CMP0 = sr->sr_frame_intvl;
		CT_IEP.TMR_GLB_CFG_bit.CNT_EN = 1;

		usel = sr->sr_usel;
		if (usel == 0)
			u = &sr->sr_u0;
		else if (usel == 1)
			u = &sr->sr_u1;
		else
			continue;

		sr->sr_uactive = usel;
		++sr->sr_frame_num;

		gpio0[GPIO_CLEARDATAOUT] = DMX_TX;
		__delay_cycles(100*USEC);

		gpio0[GPIO_SETDATAOUT] = DMX_TX;
		__delay_cycles(12*USEC);

		write_byte(u->u_start);

		for (i = 0; i < u->u_size; ++i)
			write_byte(u->u_slot[i]);

		gpio0[GPIO_SETDATAOUT] = DMX_TX;
		__delay_cycles(20*USEC);

		await_iep_timer();
	}

	__halt();
}

/*struct my_resource_table {
	struct resource_table base;
	uint32_t offset[1];
};

#pragma DATA_SECTION(pru_remoteproc_ResourceTable, ".resource_table")
#pragma RETAIN(pru_remoteproc_ResourceTable)
struct my_resource_table pru_remoteproc_ResourceTable = {
	.base = {
		.ver = 1,
		.num = 0
	},
	.offset = {
		0,
	},
};*/
