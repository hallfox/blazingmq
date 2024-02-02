# BlazingMQ bindings for Rust

This is an experimental crate that uses `cxx` to generate bindings to libbmq. It
is incomplete and highly unstable, **do not use this.**

## TODO

1. MessageProperties
2. Handle message events session events
    a. Consumer functions
3. Error handling
    a. cxx automatically converts exceptions to `Result<T>`, I think I'll be okay with that rather
       than a bespoke Result <-> error enum translation layer
4. Configure the correlation ID so that the on_ack_handler can be called by the client
5. Find a way to generally pass callbacks to the C++ side
