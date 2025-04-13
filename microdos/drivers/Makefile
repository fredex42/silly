.PHONY: early_serial bios_disk

all: early_serial bios_disk

ata_pio:
	make -C ata_pio

bios_disk:
	make -C bios_disk

early_serial:
	make -C early_serial

cmos:
	make -C cmos

ps2_controller:
	make -C ps2_controller

kb:
	make -C kb

pci:
	make -C pci

clean:
	# make -C ata_pio clean
	make -C early_serial clean
	make -C bios_disk clean
	# make -C cmos clean
	# make -C pci clean
	# make -C ps2_controller clean
	# make -C kb clean
