.PHONY: ata_pio early_serial

all: ata_pio early_serial

ata_pio:
	make -C ata_pio

early_serial:
	make -C early_serial

clean:
	make -C ata_pio clean
	make -C early_serial clean
