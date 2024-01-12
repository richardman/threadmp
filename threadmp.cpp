
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unordered_map>

#include "./status_codes.h"
#include "./threadmp.h"

namespace threadmp {

    bool debug = false;

    enum thread_state_t {
        NORMAL, RECEIVE_WAIT, REPLY_WAIT, RECEIVE_PENDING, RECEIVE_PROCESSING, SEND_BLOCKED
    };

    class thread_t;

    class waiting_thread_t {
      public:
        std::condition_variable wait_cv;
        std::mutex wait_mutex;
        std::shared_ptr<thread_t> sender;

        waiting_thread_t( std::shared_ptr<thread_t> a_sender ) : sender( a_sender ) {}
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

        std::queue<std::shared_ptr<waiting_thread_t>> waiting_senders;
        thread_t( const std::string &a_name, std::thread::id a_thread_id ) : name( a_name ), thread_id( a_thread_id ) {}
    };

    std::unordered_map<std::string, std::thread::id> name_to_id_map;
    std::unordered_map<std::thread::id, std::shared_ptr<thread_t>> id_to_thread_map;

    std::mutex kernel_mutex;

    static inline std::shared_ptr<thread_t> FindThreadById( std::thread::id id ) {
        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );
        auto find_this_thread_iter = id_to_thread_map.find( id );
        if( find_this_thread_iter == id_to_thread_map.end() ) throw;

        return find_this_thread_iter->second;
    }


    static inline std::thread::id FindIdByName( const std::string &name ) {
        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );        
        auto find_id_iter = name_to_id_map.find( name );
        if( find_id_iter == name_to_id_map.end() ) throw;

        return find_id_iter->second;
    }


    int Send( const std::string &receiver_name, 
            size_t message_length, char message[], 
            const int32_t wait_ms ) {

        size_t reply_length = 0;
        char reply_buf[0];

        return Send( receiver_name, message_length, message, reply_length, reply_buf, wait_ms );

    }


    int Send( const std::string &receiver_name, 
            size_t message_length, char message[], 
            size_t &reply_message_length, char reply_message[], const int32_t wait_ms ) {

        auto this_thread = FindThreadById( std::this_thread::get_id() );
        auto receiver_id = FindIdByName( receiver_name );

        auto receiver = FindThreadById( receiver_id );
        if( receiver->thread_id != receiver_id ) throw;

        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );

        if( debug ) std::cout << "fcnSEND from: " << this_thread->name << " to: " << receiver_name << std::endl;

        if( this_thread->state != NORMAL ) return ERROR_SEND_FAIL;

        if( receiver->state == RECEIVE_WAIT ) {

            receiver->state = RECEIVE_PROCESSING;

            if( debug ) std::cout << "  in SEND(" << this_thread->name << ") receiver " << receiver_name << " is in RECEIVE_WAIT. Will wake it up" << std::endl;

            receiver->sender_name = this_thread->name;
            this_thread->reply_message_length = reply_message_length;
            this_thread->reply_message = reply_message;

            if( message_length > receiver->receive_message_length ) message_length = receiver->receive_message_length;
            memcpy( receiver->receive_message, message, message_length );

            std::unique_lock<std::mutex> lk_receive( receiver->cv_receive_mutex );
            lk_receive.unlock();
            receiver->cv_receive.notify_one();           

        } else if( receiver->state == NORMAL ) {
            receiver->state = RECEIVE_PENDING;

            if( debug ) std::cout << "  in SEND(" << this_thread->name << ") receiver " << receiver_name << " in NORMAL state, wait for it to wake up" << std::endl;            

            receiver->sender_name = this_thread->name;
            this_thread->reply_message_length = reply_message_length;
            this_thread->reply_message = reply_message;

            receiver->receive_message_length = message_length;
            receiver->receive_message = message;

        } else if( receiver->state == RECEIVE_PROCESSING || receiver->state == RECEIVE_PENDING ) {

            if( debug )  std::cout << "  in SEND(" << this_thread->name << ") receiver " << receiver_name << " in " << ( receiver->state == RECEIVE_PROCESSING ? "PROCESSING" : "PENDING" ) << " state " << std::endl << "  Will set self to SEND_BLOCKED" << std::endl;  
            
            this_thread->state = SEND_BLOCKED;
            auto waitee = std::make_shared<waiting_thread_t>( this_thread );

            receiver->waiting_senders.push( waitee );

            lock_kernel.unlock();
            std::unique_lock<std::mutex> lock_wait( waitee->wait_mutex );
            waitee->wait_cv.wait( lock_wait );

            lock_kernel.lock();
            if( this_thread->state != NORMAL ) throw;
            lock_kernel.unlock();

            if( debug ) std::cout << "  in SEND(" << this_thread->name << ") was SEND_BLOCKED, now unblocked. Resending..." << std::endl;
            // retry
            return Send( receiver_name, message_length, message, reply_message_length, reply_message, wait_ms );

        } else {

            std::cout << "!!ERROR SEND " << receiver_name << " BAD STATE " << receiver->state << std::endl;
            throw;
            return ERROR_SEND_FAIL;
        }

        this_thread->state = REPLY_WAIT;

        std::unique_lock<std::mutex> lk_reply( this_thread->cv_reply_mutex );
        lock_kernel.unlock();

        if( debug ) std::cout << "  in SEND(" << this_thread->name << ") waiting for reply" << std::endl;
        this_thread->cv_reply.wait( lk_reply ); 
        lk_reply.unlock();

        lock_kernel.lock();

        if( debug ) std::cout << "  in SEND(" << this_thread->name << ") returing after REPLIED" << std::endl;
        reply_message_length = this_thread->reply_message_length;

        return SUCCESS;
    }

    int Receive( size_t &message_length, char message[], std::string &sender_name, const int32_t wait_ms ) {

        auto this_thread = FindThreadById( std::this_thread::get_id() ); 

        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );
        if( debug ) std::cout << "fcnRECEIVE in: " << this_thread->name << std::endl;

        if( this_thread->state == RECEIVE_PENDING ) {
            if( message_length > this_thread->receive_message_length )
                message_length = this_thread->receive_message_length;
                
            memcpy( message, this_thread->receive_message, message_length );
            this_thread->state = RECEIVE_PROCESSING;
            
            if( debug ) std::cout << "  in RECEIVE(" << this_thread->name << ") got pending message from: " << this_thread->sender_name << std::endl;
            sender_name = this_thread->sender_name;        
            
        } else if( this_thread->state == NORMAL ) {
            this_thread->state = RECEIVE_WAIT;
            this_thread->receive_message_length = message_length;
            this_thread->receive_message = message;
           
            if( debug ) std::cout << "  in RECEIVE(" << this_thread->name << ") will wait for new message" << std::endl;

            std::unique_lock<std::mutex> lk_receive( this_thread->cv_receive_mutex );
            lock_kernel.unlock();

            this_thread->cv_receive.wait( lk_receive );        
            lk_receive.unlock();

            lock_kernel.lock();
            sender_name = this_thread->sender_name;

        } else {
            std::cout << "!!ERROR RECEIVE " << this_thread->name << " BAD STATE " << this_thread->state << std::endl;
            throw;

            return ERROR_RECEIVE_FAIL;
        }

        if( debug ) std::cout << "  in RECEIVE(" << this_thread->name << ") returning after copied message from: " << sender_name << std::endl;

        return SUCCESS;
    }
    
    
    int Reply( const std::string sender_name, size_t message_length, const char message[] ) {

        auto sender_id = FindIdByName( sender_name);
        auto sender_thread = FindThreadById( sender_id ); 
        auto this_thread = FindThreadById( std::this_thread::get_id() );

        std::unique_lock<std::mutex> lock_kernel( kernel_mutex );
        if( debug ) std::cout << "fcnREPLY from: " << this_thread->name << " replying to: " << sender_name << std::endl;

        if( this_thread->state == RECEIVE_PROCESSING && sender_thread->state == REPLY_WAIT ) {
            sender_thread->state = NORMAL;

            if( sender_thread->reply_message_length > message_length )
                sender_thread->reply_message_length = message_length;
            
            memcpy( sender_thread->reply_message, message, sender_thread->reply_message_length );

            if( debug ) std::cout << "  in REPLY(" << this_thread->name << ") sender was in REPLY_WAIT. Normal processing" << std::endl;

        } else {

            if( debug ) std::cout << "!!ERROR REPLY " << sender_name << " BAD STATE this thread state " << this_thread->state << " sender state " << sender_thread->state << std::endl;

            throw;
            return ERROR_REPLY_FAIL;
        }

        this_thread->state = NORMAL;

        std::unique_lock<std::mutex> lk_reply( sender_thread->cv_reply_mutex );
        lk_reply.unlock();

        sender_thread->cv_reply.notify_one(); 

        // wake up any pending sender
        if( this_thread->waiting_senders.size() > 0 ) {
            auto waitee = this_thread->waiting_senders.front();
            this_thread->waiting_senders.pop();

            if( waitee->sender->state != SEND_BLOCKED ) throw;

            if( debug ) std::cout << "  in REPLY(" << this_thread->name << ") waking up SEND_BLOCKED sender: " << waitee->sender->name << std::endl;
            waitee->sender->state = NORMAL;
            std::unique_lock<std::mutex> lock_wait( waitee->wait_mutex );
            lock_wait.unlock();

            waitee->wait_cv.notify_one();
        }

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