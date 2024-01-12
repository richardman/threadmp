#pragma once

#include <stdint.h>
#include <string>

namespace threadmp {

    int RegisterThread( const std::string &name );
    int deRegisterThread( void );

    int Send( const std::string &receiver_name, const std::string &message, std::string &reply_message, const int32_t wait_ms = 0 );
    int Receive( std::string &message, std::string &sender_name, const int32_t wait_ms = 0 );
    int Reply( const std::string &sender_name, const std::string &message );

}