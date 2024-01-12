#include <chrono>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <thread>

#include "./threadmp.h"

using namespace std::chrono_literals;

void Writer( ) {
    std::cout << "writer" << std::endl;
    threadmp::RegisterThread( "writer" );

    std::this_thread::sleep_for( 2000ms );
    while( true ) {

        int sleep_factor = std::rand( ) & 0xF;
        std::this_thread::sleep_for( 10ms * sleep_factor );

        static int i = 0;
        std::string message = std::string( "Hello World!! " ) + std::to_string( i++ );

        size_t reply_length = 4;
        char reply_message[reply_length+1];
        reply_message[reply_length] = 0;

        char buf[message.length() + 1];
        strcpy( buf, message.c_str() );
        threadmp::Send( "reader", message.length() + 1, buf, reply_length, reply_message );

        std::cout << "writer Got reply, length: " << reply_length << " message '" << reply_message << "'" << std::endl;
    }

}


void Writer2( ) {
    std::cout << "writer2" << std::endl;
    threadmp::RegisterThread( "writer2" );

    std::this_thread::sleep_for( 2000ms );
    while( true ) {

        int sleep_factor = std::rand( ) & 0xF;
        std::this_thread::sleep_for( 18ms * sleep_factor );

        static int i = 0;
        std::string message = std::string( "I am Alive!!!! " ) + std::to_string( i++ );

        size_t reply_length = 10;
        char reply_message[reply_length+1];
        reply_message[reply_length] = 0;

        char buf[message.length() + 1];
        strcpy( buf, message.c_str() );
        threadmp::Send( "reader", message.length() + 1, buf, reply_length, reply_message );

        std::cout << "writer2 Got reply, length: " << reply_length << " message '" << reply_message << "'" << std::endl;

    }

}


void Writer3( ) {
    std::cout << "writer3" << std::endl;
    threadmp::RegisterThread( "writer3" );

    while( true ) {

        int sleep_factor = std::rand( ) & 0xF;
        std::this_thread::sleep_for( 13ms * sleep_factor );

        static int i = 0;
        std::string message = std::string( "Hello Darkness My Old Friends... " ) + std::to_string( i++ );

        size_t reply_length = 20;
        char reply_message[reply_length+1];
        reply_message[reply_length] = 0;

        char buf[message.length() + 1];
        strcpy( buf, message.c_str() );
        threadmp::Send( "reader", message.length() + 1, buf, reply_length, reply_message );

        std::cout << "writer3 Got reply, length: " << reply_length << " message '" << reply_message << "'" << std::endl;

    }

}


void Writer4( ) {
    std::cout << "writer4" << std::endl;
    threadmp::RegisterThread( "writer4" );

    while( true ) {

        int sleep_factor = std::rand( ) & 0x3;
        std::this_thread::sleep_for( 1s * sleep_factor );

        static int i = 0;
        std::string message = std::string( "The philosopher is starving and it's all your fault " ) + std::to_string( i++ );

        size_t reply_length = 13;
        char reply_message[reply_length+1];
        reply_message[reply_length] = 0;

        char buf[message.length() + 1];
        strcpy( buf, message.c_str() );
        threadmp::Send( "reader", message.length() + 1, buf, reply_length, reply_message );

        std::cout << "writer4 Got reply, length: " << reply_length << " message '" << reply_message << "'" << std::endl;

    }

}


void Reader( ) {
    std::cout << "reader" << std::endl;
    threadmp::RegisterThread( "reader" );

    while( true ) {

        int sleep_factor = std::rand( ) & 0xF;
        std::this_thread::sleep_for( 10ms * sleep_factor );

        const int buflen = 100;
        char buffer[buflen];

        std::string sender_name;
        size_t len = buflen;
        threadmp::Receive( len, buffer, sender_name );

        std::cout << "Got message, length " << len << " message: '" << buffer << "'" << std::endl;

        static int i = 0;
        std::string reply = std::string( "OK!" ) + std::to_string( i++ );

        threadmp::Reply( sender_name, reply.length() + 1, reply.c_str() );

    }
}



int main( int argc, char *argv[] ) {

    std::thread thread0( Reader );
    std::thread thread1( Writer );
    std::thread thread2( Writer2 );
    std::thread thread3( Writer3 );
    std::thread thread4( Writer4 );

    thread0.detach();
    thread1.detach();
    thread2.detach();
    thread3.detach();
    thread4.detach();

    while( true ) std::this_thread::sleep_for( 10s );


}