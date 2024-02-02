use std::time::Duration;

pub use crate::bridge::ffi::{CompressionType, QueueMode};
use crate::bridge::{self, ffi};
use crate::defaults;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum BlazingMqError {
    #[error("Session creation failure")]
    SessionCreate,
    #[error("C++ exception")]
    CxxException(#[from] cxx::Exception),
    #[error("Session error")]
    GenericSessionError(#[from] ffi::GenericResultEnum),
}

pub type Result<T> = std::result::Result<T, BlazingMqError>;

/// Builder for a [Session] object.
///
/// Default parameters
/// 
/// | Field              | Default                                     |
/// |--------------------|---------------------------------------------|
/// | `broker_uri`       | [defaults::BROKER_DEFAULT_URI]              |
/// | `timeout`          | [defaults::QUEUE_OPERATION_DEFAULT_TIMEOUT] |
/// | `compression_type` | [CompressionType::None]                     |
#[derive(PartialEq, Eq, Hash)]
pub struct SessionBuilder<'a, H: EventHandler> {
    broker_uri: Option<&'a str>,
    timeout: Option<Duration>,
    compression_type: CompressionType,
    event_handler: Option<H>,
}

impl<'a, H: EventHandler> SessionBuilder<'a, H> {
    pub fn new() -> Self {
        Self {
            broker_uri: None,
            timeout: None,
            compression_type: CompressionType::None,
            event_handler: None,
        }
    }

    pub fn broker_uri(&mut self, broker_uri: &'a str) -> &mut Self {
        self.broker_uri = Some(broker_uri);
        self
    }

    pub fn timeout(&mut self, timeout: Duration) -> &mut Self {
        self.timeout = Some(timeout);
        self
    }

    pub fn compression_type(&mut self, compression_type: CompressionType) -> &mut Self {
        self.compression_type = compression_type;
        self
    }

    pub fn event_handler<H: EventHandler>(handler: H) -> &mut Self {
        self.event_handler = Some(handler);
        self
    }

    pub fn on_event(event_cb: fn(Event)) {
        self.on_event = event_cb;
    }

    pub fn build(&self) -> Result<Session> {
        let broker_uri = self.broker_uri.unwrap_or(defaults::BROKER_DEFAULT_URI);
        let timeout = self
            .timeout
            .as_ref()
            .unwrap_or(&defaults::QUEUE_OPERATION_DEFAULT_TIMEOUT);
        let inner = ffi::make_session(
            |context| (),
            |context| (),
            broker_uri,
            timeout.as_secs_f64(),
            self.compression_type,
        )?;

        if inner.is_null() {
            return Err(BlazingMqError::SessionCreate);
        }

        let event_handler = Box::new(self.event_handler.unwrap_or(SimpleEventHandler));

        // Safety: Session was just created.
        let rc = unsafe { inner.pin().start() };

        if rc != ffi::GenericResultEnum::Success {
            return Err(BlazingMqError::SessionCreate(()));
        }

        let session = Session {
            inner,
            event_handler,
        };

        Ok(session)
    }
}

pub struct Session {
    inner: cxx::UniquePtr<ffi::Session>,
    event_handler: Box<EventHandler>,
}

impl Session {
    pub fn open_queue(&mut self, uri: &str, mode: QueueMode) -> QueueBuilder {
        QueueBuilder {
            session: self,
            uri: uri.into(),
            mode,
            max_unconfirmed_messages: None,
            max_unconfirmed_bytes: None,
            consumer_priority: None,
            suspends_on_bad_host_health: None,
            timeout: None,
        }
    }

    pub fn post(&self, message: !) {}
}

impl Drop for Session {
    fn drop(&mut self) {
        // Safety: `stop` is never called while the session is alive, and `start` is called before it is constructed
        unsafe {
            self.inner.pin_mut().stop();
        }
    }
}

pub struct QueueBuilder<'a> {
    session: &'a mut Session,
    uri: String,
    mode: QueueMode,
    max_unconfirmed_messages: Option<u32>,
    max_unconfirmed_bytes: Option<u32>,
    consumer_priority: Option<u32>,
    suspends_on_bad_host_health: Option<bool>,
    timeout: Option<time::Duration>,
}

impl QueueBuilder {
    pub fn max_unconfirmed_messages(&mut self, unconfirmed_messages: u32) -> &mut Self {
        self.max_unconfirmed_messages = Some(unconfirmed_messages);
        self
    }

    pub fn max_unconfirmed_bytes(&mut self, unconfirmed_bytes: u32) -> &mut Self {
        self.max_unconfirmed_bytes = Some(unconfirmed_bytes);
        self
    }

    pub fn consumer_priority(&mut self, priority: u32) -> &mut Self {
        self.consumer_priority = Some(priority);
        self
    }

    pub fn suspends_on_bad_host_health(&mut self, is_suspending: bool) -> &mut Self {
        self.suspends_on_bad_host_health = Some(is_suspending);
        self
    }

    pub fn timeout(&mut self, timeout: time::Duration) -> &mut self {
        self.timeout = Some(timeout);
        self
    }

    pub fn connect(self) -> Result<Queue> {
        // Safety: Live `Session` objects should always be started
        let rc = unsafe {
            // TODO: Retrieve QueueId for its correlation ID
            self.session.inner.pin_mut().open_queue_sync(
                &self.uri,
                self.mode,
                self.options,
                self.timeout,
            )
        };

        if rc != OpenQueueResultEnum::Success {
            return Err(rc);
        }

        Ok(Queue {
            uri: self.uri,
            mode: self.mode,
        })
    }
}

#[derive(Default)]
pub struct Queue {
    uri: String,
    //TODO id: u64,
    mode: QueueMode,
}

impl Queue {
    fn new() -> Self {
        Default::default()
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn default_session() {
        let session = SessionBuilder::new().build();
        assert!(session.is_some());
    }

    #[test]
    fn can_produce() {
        let mut session = SessionBuilder::new()
            .broker_uri("tcp://localhost:30114")
            .build()
            .expect("Build session");

        session.open_queue(
                "bmq://bmq.tutorial.hello/test-rust", QueueMode::Write)
                .timeout(time::Duration(300))
                .connect()?;
        )
    }
}
