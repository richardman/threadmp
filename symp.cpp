
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unordered_map>

#include "./status_codes.h"
#include "./symp.h"

namespace symp {

    enum thread_state_t {
        NORMAL, RECEIVE_WAIT, REPLY_WAIT, RECEIVE_PENDING
    };

    class thread_t {
      public:
        std::string name;
        std::thread::id thread_id;

        thread_state_t state = NORMAL;
        std::condition_variable cv_receive;
        std::mutex cv_receive_mutex;
        std::condition_variable cv_reply;
        std::mutex cv_reply_mutex;              

        std::string sender_name;

        size_t receive_message_length = 0;
        size_t reply_message_length = 0;

        char *receive_message = nullptr;
        char *reply_message = nullptr;      

        thread_t( const std::string &a_name, std::thread::id a_thread_id ) : name( a_name ), thread_id( a_thread_id ) {}
    };

    std::unordered_map<std::string, std::thread::id> name_to_id_map;
    std::unordered_map<std::thread::id, std::shared_ptr<thread_t>> id_to_thread_map;

    std::mutex kernel_mutex;

    int Send( const std::string &receiver_name, 
            size_t message_length, char message[], 
            size_t &reply_message_length, char reply_message[], const int32_t wait_ms ) {

        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );

        auto find_this_thread_iter = id_to_thread_map.find( std::this_thread::get_id() );
        if( find_this_thread_iter == id_to_thread_map.end() ) return ERROR_THREAD_NOT_REGISTERED;
        auto this_thread = find_this_thread_iter->second;

        auto find_id_iter = name_to_id_map.find( receiver_name );
        if( find_id_iter == name_to_id_map.end() ) return ERROR_BAD_RECEIVER;

        auto receiver_id = find_id_iter->second;

        // should not happen
        auto find_thread_iter = id_to_thread_map.find( receiver_id );
        if( find_thread_iter == id_to_thread_map.end() ) return ERROR_BAD_RECEIVER_INTERNAL;

        auto receiver = find_thread_iter->second;
        if( receiver->thread_id != receiver_id ) return ERROR_INTERNAL;

        receiver->sender_name = this_thread->name;
        this_thread->reply_message_length = reply_message_length;
        this_thread->reply_message = reply_message;

        if( receiver->state == RECEIVE_WAIT ) {
            receiver->state = NORMAL;

            std::cout << "<< SEND receiver " << receiver_name << " in RECEIVE_WAIT" << std::endl;

            if( message_length > receiver->receive_message_length ) message_length = receiver->receive_message_length;
            memcpy( receiver->receive_message, message, message_length );

            std::unique_lock<std::mutex> lk_receive( receiver->cv_receive_mutex );
            lk_receive.unlock();
            receiver->cv_receive.notify_one();           

        } else if( receiver->state == NORMAL ) {
            receiver->state = RECEIVE_PENDING;

            std::cout << "<< SEND receiver " << receiver_name << " in NORMAL state, wait for it to wake up" << std::endl;            

            receiver->receive_message_length = message_length;
            receiver->receive_message = message;

        } else {

            std::cout << "!!ERROR SEND " << receiver_name << " BAD STATE " << receiver->state << std::endl;

            return ERROR_SEND_FAIL;
        }

        this_thread->state = REPLY_WAIT;

        lock_kernel.unlock();

        std::unique_lock<std::mutex> lk_reply( this_thread->cv_reply_mutex );
        this_thread->cv_reply.wait( lk_reply ); 
        lk_reply.unlock();

        reply_message_length = this_thread->reply_message_length;
        return SUCCESS;
    }

    int Receive( size_t &message_length, char message[], std::string &sender_name, const int32_t wait_ms ) {

        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );

        auto find_this_thread_iter = id_to_thread_map.find( std::this_thread::get_id() );
        if( find_this_thread_iter == id_to_thread_map.end() ) return ERROR_THREAD_NOT_REGISTERED;
        auto this_thread = find_this_thread_iter->second;

        if( this_thread->state == RECEIVE_PENDING ) {
            if( message_length > this_thread->receive_message_length )
                message_length = this_thread->receive_message_length;
                
            memcpy( message, this_thread->receive_message, message_length );
            this_thread->state = NORMAL;
            
            lock_kernel.unlock();            
            
        } else if( this_thread->state == NORMAL ) {
            this_thread->state = RECEIVE_WAIT;
            this_thread->receive_message_length = message_length;
            this_thread->receive_message = message;

            lock_kernel.unlock();
            
            std::cout << "<< RECEIVE " << this_thread->name << " waiting for new message" << std::endl;

            std::unique_lock<std::mutex> lk_receive( this_thread->cv_receive_mutex );
            this_thread->cv_receive.wait( lk_receive );        
            lk_receive.unlock();

        } else {
            std::cout << "!!ERROR RECEIVE " << this_thread->name << " BAD STATE " << this_thread->state << std::endl;
            
            return ERROR_RECEIVE_FAIL;
        }

        sender_name = this_thread->sender_name;
        return SUCCESS;
    }
    
    
    int Reply( const std::string sender_name, size_t message_length, const char message[] ) {

        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );

        auto find_sender_id_iter = name_to_id_map.find( sender_name );
        if( find_sender_id_iter == name_to_id_map.end() ) return ERROR_BAD_REPLY;

        auto sender_id = find_sender_id_iter->second;

        auto find_thread_iter = id_to_thread_map.find( sender_id );
        if( find_thread_iter == id_to_thread_map.end() ) return ERROR_BAD_RECEIVER_INTERNAL;

        auto sender_thread = find_thread_iter->second;

        lock_kernel.unlock();

        if( sender_thread->state == REPLY_WAIT ) {
            sender_thread->state = NORMAL;

            if( sender_thread->reply_message_length > message_length )
                sender_thread->reply_message_length = message_length;
            
            memcpy( sender_thread->reply_message, message, sender_thread->reply_message_length );

        } else {

            std::cout << "!!ERROR REPLY " << sender_name << " BAD STATE " << sender_thread->state << std::endl;
            return ERROR_REPLY_FAIL;
        }

        std::unique_lock<std::mutex> lk_reply( sender_thread->cv_reply_mutex );
        lk_reply.unlock();

        sender_thread->cv_reply.notify_one(); 

        return SUCCESS;
    }


    int RegisterThread( const std::string &name ) {
        std::thread::id thread_id = std::this_thread::get_id();

        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );

        auto find_thread_iter = id_to_thread_map.find( thread_id );
        if( find_thread_iter != id_to_thread_map.end() ) {
            if( find_thread_iter->second->name == name ) return SUCCESS;
            return ERROR_THREAD_ALREADY_REGISTERED;
        }

        auto find_id_iter = name_to_id_map.find( name );
        if( find_id_iter != name_to_id_map.end() ) {
            if( find_id_iter->second == thread_id ) return SUCCESS;
            return ERROR_NAME_ALREADY_REGISTERED;
        }

        name_to_id_map.emplace( name, thread_id );
        id_to_thread_map.emplace( thread_id, new thread_t( name, thread_id ) );

        return SUCCESS;
    }


    int deRegisterThread( void ) {
        std::thread::id thread_id = std::this_thread::get_id();

        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );

        auto find_thread_iter = id_to_thread_map.find( thread_id );
        if( find_thread_iter == id_to_thread_map.end() ) return ERROR_THREAD_NOT_REGISTERED;
      
        auto find_id_iter = name_to_id_map.find( find_thread_iter->second->name );
        if( find_id_iter == name_to_id_map.end() ) return ERROR_INTERNAL;

        id_to_thread_map.erase( find_thread_iter );
        name_to_id_map.erase( find_id_iter );

        return SUCCESS;
    }


}