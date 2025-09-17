#ifndef __ATA_READWRITE_H
#define __ATA_READWRITE_H

/* defined in readwrite.c */
void ata_complete_read_lowerhalf(SchedulerTask *t);
void ata_complete_write_lowerhalf(SchedulerTask *t);
void ata_continue_write(ATAPendingOperation *op);
void ata_continue_read_chunk(SchedulerTask *t);

/* defined in interrupt.c */
void ata_dump_errors(uint8_t err);
#endif
