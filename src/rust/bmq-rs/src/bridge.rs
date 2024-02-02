use cxx::UniquePtr;
use std::time::Duration;
use crate::event_handler::{EventHandler, SessionEvent, AckEvent, MessageEvent};

#[cxx::bridge(namespace = "BloombergLP::bmq::bridge")]
pub mod ffi {
    #[derive(Debug, Clone, Copy)]
    enum QueueMode {
        Admin,
        Read,
        Write,
        ReadWrite,
    }

    #[derive(Debug, Hash, Clone, Copy)]
    enum CompressionType {
        None,
        Zlib,
    }

    #[derive(Debug)]
    struct QueueOptions {
        max_unconfirmed_messages: i32,
        max_unconfirmed_bytes: i32,
        consumer_priority: i32,
        suspends_on_bad_host_health: bool,
    }

    //TODO: This object needs to imitate a generic JSON-y object. We could just have this be a HashMap<String, Val>
    // (where Val = bool|i8|i16|i32|i64|String|Vec<u8>). Not necessary to start with but this will probably require
    // a fair amount of thought on the Rust side and then we can simply translate it through extern "Rust" functions.
    #[derive(Debug)]
    struct MessageProperties {
        _dummy: u8,
    }

    #[derive(Debug)]
    struct Message {
        _dummy: u8,
    }

    #[repr(i32)]
    enum OpenQueueResultEnum {
        #[cxx_name = "e_SUCCESS"]
        Success = 0,
        #[cxx_name = "e_UNKNOWN"]
        Unknown = -1,
        #[cxx_name = "e_TIMEOUT"]
        Timeout = -2,
        #[cxx_name = "e_NOT_CONNECTED"]
        NotConnected = -3,
        #[cxx_name = "e_CANCELED"]
        Canceled = -4,
        #[cxx_name = "e_NOT_SUPPORTED"]
        NotSupported = -5,
        #[cxx_name = "e_REFUSED"]
        Refused = -6,
        #[cxx_name = "e_INVALID_ARGUMENT"]
        InvalidArgument = -7,
        #[cxx_name = "e_NOT_READY"]
        NotReady = -8,
        #[cxx_name = "e_ALREADY_OPENED"]
        AlreadyOpened = 100,
        #[cxx_name = "e_ALREADY_IN_PROGRESS"]
        AlreadyInProgress = 101,
        #[cxx_name = "e_INVALID_URI"]
        InvalidUri = -100,
        #[cxx_name = "e_INVALID_FLAGS"]
        InvalidFlags = -101,
        #[cxx_name = "e_CORRELATIONID_NOT_UNIQUE"]
        CorrelationIdNotUnique = -102,
    }

    #[repr(i32)]
    enum GenericResultEnum {
        #[cxx_name = "e_SUCCESS"]
        Success = 0,
        #[cxx_name = "e_UNKNOWN"]
        Unknown = -1,
        #[cxx_name = "e_TIMEOUT"]
        Timeout = -2,
        #[cxx_name = "e_NOT_CONNECTED"]
        NotConnected = -3,
        #[cxx_name = "e_CANCELLED"]
        Cancelled = -4,
        #[cxx_name = "e_NOT_SUPPORTED"]
        NotSupported = -5,
        #[cxx_name = "e_REFUSED"]
        Refused = -6,
        #[cxx_name = "e_INVALID_ARGUMENT"]
        InvalidArgument = -7,
        #[cxx_name = "e_NOT_READY"]
        NotReady = -8,
        #[cxx_name = "e_LAST"]
        Last = -8,
    }

    extern "C++" {
        include!("bmq-rs/src/bmq/bridge/errors.h");

        type OpenQueueResultEnum;
        type GenericResultEnum;
    }

    extern "Rust" {
        type AckEventContext;
    }

    extern "Rust" {
        type BridgeContext;
    }

    extern "C++" {
        include!("bmq-rs/src/bmq/bridge/session.h");

        // Most methods on Session are unsafe to call due to start/stop
        type Session;

        unsafe fn start(self: Pin<&mut Session>) -> GenericResultEnum;
        unsafe fn stop(self: Pin<&mut Session>);
        unsafe fn open_queue_sync(
            self: Pin<&mut Session>,
            uri: &str,
            mode: QueueMode,
            options: QueueOptions,
            timeout: f64,
        ) -> OpenQueueResultEnum;
        unsafe fn configure_queue_sync(
            self: Pin<&mut Session>,
            uri: &str,
            options: QueueOptions,
            timeout: f64,
        ) -> i32;
        unsafe fn close_queue_sync(self: Pin<&mut Session>, uri: &str, timeout: f64) -> i32;
        unsafe fn post(
            self: Pin<&mut Session>,
            uri: &str,
            payload: &[u8],
            properties: MessageProperties,
            on_ack: fn(Box<AckEventContext>),
        ) -> i32;
    }

    unsafe extern "C++" {
        include!("bmq-rs/src/bmq/bridge/session.h");

        fn make_session(
            on_session_event: fn(Box<BridgeContext>),
            on_message_event: fn(Box<BridgeContext>),
            broker_uri: &str,
            timeout: f64,
            compression_type: CompressionType,
        ) -> Result<UniquePtr<Session>>;
    }
}

enum PropertyValue {
    Bool(bool),
    Char(i8),
    Short(i16),
    Int32(i32),
    Int64(i64),
    String(String),
    Binary(Vec<u8>),
}

type MessageProperties = HashMap<String, PropertyValue>;

impl Default for ffi::QueueOptions {
    fn default() -> Self {
        Self {
            max_unconfirmed_messages: 1000,
            max_unconfirmed_bytes: 33554432,
            consumer_priority: 0,
            suspends_on_bad_host_health: false,
        }
    }
}

//TODO: We can pass function pointers across the FFI boundary, but those functions cannot capture environments.
// Since we likely want to support this use case:
//
//```rs
// session.post(message, |ack| {
//   ...
// });
//
// We'll have to figure out how to manage the lifetime of that closure and then pass it around as an extern "Rust"
// type.

/// A wrapper type for EventHandlers. Since cxx can only work with types over
/// the FFI bound and doesn't understand dyn Traits, we wrap the dyn Trait in a
/// real type that cxx can work with.
struct EventHandlerContext<'a> {
    event_handler: &dyn EventHandler,
}

impl<'a> EventHandlerContext {
    fn on_session_event(&self, event: &SessionEvent) {
        self.event_handler(event);
    }

    fn on_message_event(&self, event: &MessageEvent) {
        self.on_message_event(event);
    }

    fn on_ack_event(&self, event: &MessageEvent) {
        self.on_ack_event(event);
    }
}

#[cfg(test)]
mod test {
    use super::ffi;
    use super::ffi::CompressionType;
    use crate::defaults::{BROKER_DEFAULT_URI, QUEUE_OPERATION_DEFAULT_TIMEOUT};

    const DEFAULT_GATEWAY: &str = "tcp://localhost:30114";

    #[test]
    fn session_construct_is_not_null() {
        let session = ffi::make_session(
            BROKER_DEFAULT_URI,
            QUEUE_OPERATION_DEFAULT_TIMEOUT.as_secs_f64(),
            CompressionType::None,
        )
        .expect("Failed to construct session");

        assert(!session.is_null());
    }

    #[test]
    fn open_queue() {
        let mut session = ffi::make_session(
            DEFAULT_GATEWAY,
            QUEUE_OPERATION_DEFAULT_TIMEOUT.as_secs_f64(),
            CompressionType::None,
        )
        .expect("Failed to construct session");

        assert_eq!(0, session.pin_mut().start());

        assert_eq!(
            0,
            session.pin_mut().open_queue_sync(
                "bmq://bmq.tutorial.hello/test-rust",
                ffi::QueueMode::Write,
                ffi::QueueOptions::default(),
                300.0
            )
        );

        session.pin_mut().stop();
    }

    #[test]
    fn unknown_compression() {
        let mut session = ffi::make_session(
            DEFAULT_GATEWAY,
            QUEUE_OPERATION_DEFAULT_TIMEOUT.as_secs_f64(),
            CompressionType::None,
        );
    }
}
