CGT_PRU?=	/opt/ti-cgt-pru

PRU_CC=		$(CGT_PRU)/bin/clpru
PRU_CFLAGS=	-I$(CGT_PRU)/share/ti/cgt-pru/include
PRU_CFLAGS+=	--endian=little -v3 --hardware_mac=on -O0
PRU_LDIR=	$(CGT_PRU)/share/ti/cgt-pru/lib
PRU_LFLAGS=	--warn_sections --stack_size=0x100 --heap_size=0x100 --reread_libs
PRU_LIBS=	-i$(PRU_LDIR) --library=libc.a

CFLAGS+=	-D_GNU_SOURCE
LDFLAGS+=	-lm

.PHONY: all
all: dmx-pru0.fw dmxctl

dmxctl: dmxctl.c

dmx-pru0.fw: dmx.obj am335x_pru.cmd
	$(PRU_CC) $(PRU_CFLAGS) -z $(PRU_LFLAGS) -o $@ $< -mdmx.map am335x_pru.cmd $(PRU_LIBS)

%.obj: %.c
	$(PRU_CC) $(PRU_CFLAGS) --asm_listing -fe $@ $<

clean:
	rm -f dmx-pru0.fw dmxctl *.o *.obj *.lst *.map *.pp

.PHONY: install
install: all
	install -o root -m 0644 dmx-pru0.fw /lib/firmware
	install -o root -m 0755 dmxctl /usr/local/sbin

.PHONY: stop
stop:
	echo stop > /sys/class/remoteproc/remoteproc1/state

.PHONY: start
start:
	echo stop > /sys/class/remoteproc/remoteproc1/state || true
	echo -n dmx-pru0.fw > /sys/class/remoteproc/remoteproc1/firmware
	echo start > /sys/class/remoteproc/remoteproc1/state
