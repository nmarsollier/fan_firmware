# fan_firmware — top-level convenience Makefile.
#
# Forwards the flash target to test/Makefile so it can be invoked from the
# project root without `cd test`:
#
#   make flash         flash the firmware
#
# The firmware itself is built with idf.py (CMake), not make.

MAKEFLAGS += --no-print-directory

.PHONY: flash

flash:
	@$(MAKE) -C test $@
