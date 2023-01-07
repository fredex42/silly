;This file defines the constants used for native API functions
%define API_NONE            0x0

%define API_EXIT            0x00000001
%define API_CREATE_PROCESS  0x00000002
%define API_SLEEP           0x00000003

%define API_CLOSE           0x00000008
%define API_OPEN            0x00000009
%define API_READ            0x0000000A
%define API_WRITE           0x0000000B
%define API_DUP             0x0000000C
%define API_IOCTL           0x0000000D

%define API_ERR_NOTFOUND    0x80000001    ;No such api code found

extern api_terminate_current_process
extern api_sleep_current_process
extern api_create_process
