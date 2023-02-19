#include <types.h>
#include <sys/mmgr.h>
#include "lowlevel.h"
#include <acpi/fadt.h>
#include <panic.h>
#include <memops.h>
#include "controller.h"

struct PS2DriverState *driver_state;

/**
 * reads from the ps2 input port into the given buffer until either there are no bytes left or the buffer runs out of space.
 * note that the buffer is not null-terminated.
 * returns the number of bytes read.
*/
size_t read_multiple_bytes(char *buf, size_t len)
{
    size_t i = 0;

    while(ps2_wait_for_data_or_timeout(500)==0) {
        buf[i] = ps2_read();
        i++;
        if(i>=len) break;
    }
    return i;
}

/**
 * initialise and identify the device on the given PS2 channel. This assumes that the PS2 controller AND the driver state
 * have both been initialised.
*/
void init_device(uint8_t ch)
{
    uint8_t rv = 0, id_buffer[8];
    size_t count = 0;

    ps2_disable_scanning(ch);
    rv = ps2_reset(ch);
    if(rv != PS2_RESPONSE_ACK) kprintf("WARNING PS2 channel %d failed to reset\r\n", ch);
    if(rv==0) {
        kprintf("INFO PS2 channel %d is not connected\r\n", ch);
        return;
    }

    //if the reset command was ACK'd then there is usually another byte coming, a teest result which should be 0xAA
    count = read_multiple_bytes(id_buffer, 8);
    kprintf("DEBUG PS2 %d extra bytes after reset on channel %d: 0x%x\r\n", count, (uint32_t)ch, id_buffer[0]);

    rv = ps2_identify(ch);
    if(rv != PS2_RESPONSE_ACK) {
        kprintf("ERROR PS2 channel %d device failed to identify\r\n", ch);
    }

    //response codes are usually two-byte. So read in all bytes from the controller
    count = read_multiple_bytes(id_buffer, 8);
    kprintf("DEBUG PS2 identify returned %d bytes on channel %d\r\n", count, (uint32_t)ch);

    switch(id_buffer[0]) {
        case 0x00:
            kprintf("INFO PS2 channel %d has a standard 2-button mouse\r\n", (uint32_t)ch);
            driver_state->mouse_channel = ch;
            break;
        case 0x03:
            kprintf("INFO PS2 channel %d has a scroll-wheel mouse\r\n", (uint32_t)ch);
            driver_state->mouse_channel = ch;
            break;
        case 0x04:
            kprintf("INFO PS2 channel %d has a 5-button mouse\r\n", (uint32_t)ch);
            driver_state->mouse_channel = ch;
            break;
        case 0xAB:
            kprintf("INFO PS2 channel %d has a keyboard, subtype 0x%x\r\n", (uint32_t)ch, (uint32_t)id_buffer[1]);
            break;
        default:
            kprintf("WARNING PS2 channel %d unidentified. Controller returned 0x%x, 0x%x\r\n", (uint32_t)ch, (uint32_t)id_buffer[0], (uint32_t)id_buffer[1]);
            break;
    }
    rv = ps2_enable_scanning(ch);
    if(rv != PS2_RESPONSE_ACK) kprintf("WARNING PS2 channel %d failed to re-enable\r\n", ch);
}

/**
 * This function is called from kickoff to initialise the controller and devices.
*/
void ps2_initialise()
{
    driver_state = NULL;
    uint8_t ch1_avail = 0;
    uint8_t ch2_avail = 0;
    uint8_t channel_count = 0;

    cli();
    //Step one - check if we even have a PS2 controller. First stop is ACPI
    struct FADT *fadt = acpi_get_fadt();
    kprintf("INFO PS2 Fixed ACPI Description Table revision is 0x%x\r\n", fadt->h.Revision);
    if(fadt->h.Revision>=2) {
        kprintf("INFO ACPI FADT should have the boot descriptor flags\r\n");
        if(!(fadt->iaPCBootArch & BA_8042_PRESENT)) {
            kputs("ERROR ACPI indicates that PS2 controller is not present, will not initialise.\r\n");
            return;
        } else {
            kputs("INFO PS2 controller should be present\r\n");
        }
    } else {
        kputs("INFO ACPI revision is too old to tell us, assuming PS2 controller exists.\r\n");
    }

    //Step two - initialise the controller
    uint32_t init_code = ps2_lowlevel_init();
    if(PS2_INIT_ERROR_CODE(init_code) != 0) {
        kprintf("ERROR PS2 Unable to initialise controller hardware, code %d\r\n", (uint32_t)PS2_INIT_ERROR_CODE(init_code));
        return;
    }
    channel_count = PS2_INIT_CHANNEL_COUNT(init_code);
    if(channel_count==0) {
        kputs("ERROR PS2 controller did not have any channels\r\n");
        return;
    }
    if(PS2_INIT_CH1_TEST(init_code)!=0) {
        kprintf("WARNING PS2 channel 1 failed self-test: 0x%x\r\n", (uint32_t)PS2_INIT_CH1_TEST(init_code));
    } else {
        ch1_avail = 1;
    }
    if(channel_count>1) {
        if(PS2_INIT_CH2_TEST(init_code)!=0) {
            kprintf("WARNING PS2 channel 2 failed self-test: 0x%x\r\n", (uint32_t)PS2_INIT_CH2_TEST(init_code));
        } else {
            ch2_avail = 1;
        }
    }
    if(!ch1_avail && !ch2_avail) {
        kputs("ERROR PS2 all channels failed, not initialising.\r\n");
        return;
    }

    kprintf("INFO PS2 %d channel controller initialised\r\n", PS2_INIT_CHANNEL_COUNT(init_code));

    //Step three - allocate memory
    driver_state = (struct PS2DriverState *)vm_alloc_lowmem(NULL, 1, MP_READWRITE);
    if(!driver_state) {
        k_panic("ERROR Unable to initialise memory for keyboard buffer!\r\n");
    } else {
        kprintf("DEBUG PS2 state buffer is at 0x%x\r\n", driver_state);
    }
    memset((void *)driver_state, 0, PAGE_SIZE);
    driver_state->buffer.buffer_size = 256;
    //keyboard buffer content follows the driver state struct itself
    driver_state->buffer.content = (char *)(driver_state + sizeof(struct PS2DriverState));
    kprintf("DEBUG PS2 keyboard buffer content at 0x%x\r\n", driver_state->buffer.content);
    
    ps2_disable_interrupts();

    //Step four - configure devices
    if(ch1_avail) {
        init_device(1);
    }
    if(ch2_avail) {
        init_device(2);
    }

    ps2_enable_interrupts();
    sti();
}