#include <types.h>
#include <sys/mmgr.h>
#include "lowlevel.h"
#include <acpi/fadt.h>
#include <panic.h>
#include <memops.h>
#include "controller.h"

struct PS2DriverState *driver_state;

/**
 * initialise and identify the device on the given PS2 channel. This assumes that the PS2 controller AND the driver state
 * have both been initialised.
*/
void init_device(uint8_t ch)
{
    uint8_t rv = 0, id_one = 0, id_two = 0;
    ps2_disable_scanning(ch);
    rv = ps2_reset(ch);
    if(rv != PS2_RESPONSE_ACK) kprintf("WARNING PS2 channel %d failed to reset\r\n", ch);
    if(rv==0) {
        kprintf("INFO PS2 channel %d is not connected\r\n", ch);
        return;
    }
    rv = ps2_identify(ch);
    if(rv != PS2_RESPONSE_ACK) {
        kprintf("ERROR PS2 channel %d device failed to identify\r\n", ch);
    }
    //response codes are usually two-byte. So read in a second byte from the controller
    if(ps2_wait_for_data_or_timeout(500)==0){   //0 => we got data
        id_two = ps2_read();
    } else {                                    //1 => timed out
        id_two = 0;
    }

    switch(rv) {
        case 0x00:
            kprintf("INFO PS2 channel %d has a standard 2-button mouse\r\n", ch);
            driver_state->mouse_channel = ch;
            break;
        case 0x03:
            kprintf("INFO PS2 channel %d has a scroll-wheel mouse\r\n", ch);
            driver_state->mouse_channel = ch;
            break;
        case 0x04:
            kprintf("INFO PS2 channel %d has a 5-button mouse\r\n", ch);
            driver_state->mouse_channel = ch;
            break;
        case 0xAB:
            kprintf("INFO PS2 channel %d has a keyboard\r\n", ch);

    }
}

/**
 * This function is called from kickoff to initialise the controller and devices.
*/
void ps2_initialise()
{
    driver_state = NULL;
    uint8_t ch1_avail = 0;
    uint8_t ch2_avail = 0;

    cli();
    //Step one - check if we even have a PS2 controller. First stop is ACPI
    struct FADT *fadt = acpi_get_fadt();
    kprintf("INFO PS2 Fixed ACPI Description Table revision is %d\r\n");
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
        kprintf("ERROR PS2 Unable to initialise controller hardware, code %d\r\n", PS2_INIT_ERROR_CODE(init_code));
        return;
    }
    if(PS2_INIT_CHANNEL_COUNT(init_code)==0) {
        kputs("ERROR PS2 controller did not have any channels\r\n");
        return;
    }
    if(PS2_INIT_CH1_TEST(init_code)!=0) {
        kprintf("WARNING PS2 channel 1 failed self-test: %d\r\n", PS2_INIT_CH1_TEST(init_code));
    } else {
        ch1_avail = 1;
    }
    if(PS2_INIT_CH2_TEST(init_code)!=0) {
        kprintf("WARNING PS2 channel 2 failed self-test: %d\r\n", PS2_INIT_CH2_TEST(init_code));
    } else {
        ch2_avail = 1;
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
    }
    memset((void *)driver_state, 0, PAGE_SIZE);
    driver_state->buffer.buffer_size = 256;
    //keyboard buffer content follows the driver state struct itself
    driver_state->buffer.content = (char *)(driver_state + sizeof(struct PS2DriverState));

    //Step four - configure devices
    if(ch1_avail) {
        init_device(1);
    }
    if(ch2_avail) {
        init_device(2);
    }

    sti();
}