#pragma once

#include <stdint.h>
#include <string>

namespace symp {

    int RegisterThread( const std::string &name );
    int deRegisterThread( void );

    int Send( const std::string &receiver_name, 
            size_t message_length, char message[], 
            size_t &reply_message_length, char reply_message[], const int32_t wait_ms = 0 );

    int Receive( size_t &message_length, char message[], std::string &sender_name, const int32_t wait_ms = 0 );

    int Reply( const std::string sender_name, size_t message_length, const char message[] );



}