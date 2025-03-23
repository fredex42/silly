.PHONY: ata_pio early_serial cmos pci ps2_controller kb

all: ata_pio early_serial cmos pci ps2_controller kb


ata_pio:
	make -C ata_pio

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
	make -C ata_pio clean
	make -C early_serial clean
	make -C cmos clean
	make -C pci clean
	make -C ps2_controller clean
	make -C kb clean
