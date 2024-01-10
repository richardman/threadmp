#include <chrono>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <thread>

#include "./symp.h"

using namespace std::chrono_literals;

void Writer( ) {
    std::cout << "writer" << std::endl;
    symp::RegisterThread( "writer" );

    std::this_thread::sleep_for( 2000ms );
    while( true ) {

        int sleep_factor = std::rand( ) & 0xF;
        std::this_thread::sleep_for( 10ms * sleep_factor );

        static int i = 0;
        std::string message = std::string( "Hello World!! " ) + std::to_string( i++ );

        size_t reply_length = 10;
        char reply_message[reply_length];

        char buf[message.length() + 1];
        strcpy( buf, message.c_str() );
        symp::Send( "reader", message.length() + 1, buf, reply_length, reply_message );

        std::cout << "Got reply, length: " << reply_length << " message '" << reply_message << "'" << std::endl;
    }

}


void Reader( ) {
    std::cout << "reader" << std::endl;
    std::this_thread::sleep_for( 2000ms );
    symp::RegisterThread( "reader" );

    while( true ) {

        int sleep_factor = std::rand( ) & 0xF;
        std::this_thread::sleep_for( 10ms * sleep_factor );

        const int buflen = 100;
        char buffer[buflen];

        std::string sender_name;
        size_t len = buflen;
        symp::Receive( len, buffer, sender_name );

        std::cout << "Got message, length " << len << " message: '" << buffer << "'" << std::endl;

        static int i = 0;
        std::string reply = std::string( "OK!" ) + std::to_string( i++ );

        symp::Reply( sender_name, reply.length() + 1, reply.c_str() );

    }
}

int main( int argc, char *argv[] ) {

    std::thread thread1( Reader );
    std::thread thread2( Writer );

    thread1.detach();
    thread2.detach();

    while( true );


}