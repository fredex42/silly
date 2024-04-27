.PHONY: ata_pio early_serial cmos pci

all: ata_pio early_serial cmos pci

ata_pio:
	make -C ata_pio

early_serial:
	make -C early_serial

cmos:
	make -C cmos

pci:
	make -C pci

clean:
	make -C ata_pio clean
	make -C early_serial clean
	make -C cmos clean
	make -C pci clean