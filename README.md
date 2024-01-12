# ThreadMP, a synchronous message passing API for std::thread
ThreadMP provides synchronous message passing functions for C++ std::thread. The API functions are simple to use:

```
namespace threadmp {

    int RegisterThread( const std::string &name );
    int deRegisterThread( void );

    int Send( const std::string &receiver_name, const std::string &message, std::string &reply_message, const int32_t wait_ms = 0 );
    int Receive( std::string &message, std::string &sender_name, const int32_t wait_ms = 0 );
    int Reply( const std::string &sender_name, const std::string &message );

}
```

The API is especially useful for implementing protocols for a client-server system, where a server listens to incoming requests using the `threadmp::Receive` function, and clients send requests using the `threadmp::Send` function. Once a server thread receives and processes a request, it sends a reply message to the client using the `threadmp::Reply` function and the client may continue its processing. The calling threads for these API functions block until the function returns.

Multiple senders may send messages to the receiver and the system handles all synchronization and queuing, e.g. a sender may send a request anytime regardless whether the server is listening for a request, processing a request, or  currently doing something else. 

## (JSON) Messages
Messages are `std::string`. It is up to the communicating threads to interpret the content of the strings. It is recommended that JSON be used to implement the protocol, and serialization and deserialization routines be used to convert between the JSON objects and strings. While there are a number of JSON C++ libraries available, the author recommends nlohmann's header only JSON library. It is also recommended that JSON be used only for communications and any real processing should be done by extracting data from the JSON into native C++ data variables.

# Advantages
- Synchronous message passing automatically provides synchronization between the threads
- Implemented using standard C++ and all std::thread features can be used

# How To Use
A thread that uses this API must first register its name to the system. The name must be unique is the mean for the threads to identify message senders and receivers.

`extern int threadmp::RegisterThread( const std::string &name );`
registers the current thread into the system.
-`name` : a string used to identify the thread. It must be unique within the system.

`extern int threadmp::Send( const std::string &receiver_name, const std::string &message, std::string &reply_message, const int32_t wait_ms );`
sends a message to a receiver and waits for the reply.
- `receiver_name` : name of the receiver thread
- `message` : message to send to the receiver
- `reply_message` : reply message from the receiver
- `wait_ms` : (not yet implemented) if the receiver does not receive the message by `wait_ms`, then the function returns with an error code. If `wait_ms` is zero, then the function does not time out.

`extern int threadmp::Receive( std::string &message, std::string &sender_name, const int32_t wait_ms = 0 );`
receives a message.
-`message` : the message received
-`sender_name` : the name of the sender. Needed by the Reply function.
- `wait_ms` : (not yet implemented) if the receiver does not receive the message by `wait_ms`, then the function returns with an error code. If `wait_ms` is zero, then the function does not time out.

`extern int threadmp::Reply( const std::string &sender_name, const std::string &message );`
- `sender_name` : name of the sender
- `message` : the reply message to the sender

# Limitations
- Once a thread receives a message, it must reply to the sender, otherwise, the sender will remain blocked
- Between the `Receive` and `Reply` calls, the thread should not any other ThreadMP API functions. e.g. it should not call `Send` or `Receive` again until it replies to the first sender
- Messages are copied between the internal buffers, and may present a performance issue if the message size is very large

# License
**(TBD)**
Copyright (C) 2024 by Richard Man. All rights reserved. The code will probably be released under an Open Source License later.
