# fan_firmware — top-level convenience Makefile.
#
#   make flash          flash the firmware (button-less: enters download mode first)
#   make flash-download flash a board already in ROM download mode (first flash)
#   make enter-flash    only put the module into ROM download mode
#
# `flash` sends the protocol's `action/device/bootloader` command over the USB
# CDC port, waits for the USB-Serial-JTAG (download) port to enumerate, then
# runs idf.py against it — no BOOT button needed. `enter-flash` only performs
# the first step. `flash-download` flashes a board that is already in download
# mode: a blank board has no app to answer the bootloader command. The firmware
# itself is built with idf.py (CMake), not make.

MAKEFLAGS += --no-print-directory

.PHONY: flash flash-download enter-flash

# Button-less flash: enter download mode, then flash the re-enumerated port.
# The ESP-IDF environment is sourced so idf.py resolves inside scripts/flash.py.
flash:
	@bash -c 'source "$$HOME/.espressif/v6.0.1/esp-idf/export.sh" && python3 scripts/flash.py $(PORT)'

# First flash of a blank board: it is already in ROM download mode (no app is
# running to answer `capabilities`), so flash the download port directly.
flash-download:
	@bash -c 'source "$$HOME/.espressif/v6.0.1/esp-idf/export.sh" && python3 scripts/flash.py --download $(PORT)'

# PORT is optional: pass it (PORT=/dev/cu.usbmodem...), or the script
# requires a single /dev/cu.usbmodem* device.
enter-flash:
	@bash -c 'source "$$HOME/.espressif/v6.0.1/esp-idf/export.sh" && python3 scripts/enter_download.py $(PORT)'
