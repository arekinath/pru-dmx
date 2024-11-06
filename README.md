# what

this is for bitbanging DMX512 out of the Beaglebone (AM335x) PRU.

it works via a simple direct access to PRU memory by the main CPU, and
outputs DMX on a GPIO (P9.13 so that it works with the RS485 driver
on the Comms Cape).

# why

the FTDI USB RS-485 things for DMX512 suck and can't manage stable
timing. this thing has very stable timing. some lights are unfortunately
very sensitive about that.
