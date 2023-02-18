#include <types.h>

#ifndef __PS2_LOWLEVEL_H
#define __PS2_LOWLEVEL_H

//functions in lowlevel.asm
uint32_t ps2_lowlevel_init();
uint8_t ps2_send(uint8_t channel, uint8_t cmd); //send the given command and wait for a response. Returns the response byte.
uint8_t ps2_read();                             //read a data byte. No check is made if there is any new data available
uint8_t ps2_read_status();                      //read the status byte
uint8_t ps2_wait_for_data_or_timeout(uint32_t max_loop);  //wait for either data to be present or for a certain number of iterations to pass

//PS2 device commands
#define PS2_RESET               0xFF
#define PS2_DISABLE_SCANNING    0xF5
#define PS2_ENABLE_SCANNING     0xF4
#define PS2_IDENTIFY            0xF2
//PS2 device responses
#define PS2_RESPONSE_ACK        0xFA //Generic "success" response
#define PS2_RESPONSE_NACK       0xFC //Generic "failure" response

#define PS2_RESET_SUCCESS       0xFA
#define PS2_RESET_FAILURE       0xFC 
#define PS2_RESET_NOT_PRESENT   0x00 

//destructuring macros for ps2_lowlevel_init return value
#define PS2_INIT_CHANNEL_COUNT(rc) (uint8_t)(rc & 0xFF)
#define PS2_INIT_ERROR_CODE(rc)     (uint8_t)((rc >> 0x08) & 0xFF)
#define PS2_INIT_CH1_TEST(rc)       (uint8_t)((rc >> 0x10) & 0xFF)
#define PS2_INIT_CH2_TEST(rc)       (uint8_t)((rc >> 0x18) & 0xFF)

//command macros that pretend to be functions
#define ps2_reset(channel) ps2_send(channel, PS2_RESET);
#define ps2_disable_scanning(channel) ps2_send(channel, PS2_DISABLE_SCANNING);
#define ps2_enable_scanning(channel) ps2_send(channel, PS2_ENABLE_SCANNING);
#define ps2_identify(channel) ps2_send(channel, PS2_IDENTIFY);

#endif