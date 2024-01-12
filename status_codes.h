#pragma once


#define SUCCESS                             200
#define ERROR_INTERNAL                      301
#define FAIL                                900


#define ERROR_THREAD_NOT_REGISTERED         1001
#define ERROR_NAME_ALREADY_REGISTERED       1002
#define ERROR_THREAD_ALREADY_REGISTERED     1003

#define ERROR_SEND_FAIL                     2001
#define ERROR_SENDER_BAD_STATE              2002
#define ERROR_BAD_SENDER                    2003
#define ERROR_BAD_SENDER_INTERNAL           2004

#define ERROR_RECEIVE_FAIL                  3001
#define ERROR_RECEIVER_BAD_STATE            3002
#define ERROR_BAD_RECEIVER                  3003
#define ERROR_BAD_RECEIVER_INTERNAL         3004

#define ERROR_REPLY_FAIL                    4001
#define ERROR_REPLY_BAD_STATE               4002
#define ERROR_BAD_REPLY                     4003
#define ERROR_BAD_REPLY_INTERNAL            4004
