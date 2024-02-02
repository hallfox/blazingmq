// Dummy types
pub struct SessionEvent;
pub struct MessageEvent;
pub struct AckEvent;

pub trait EventHandler {
    fn on_session_event(event: &SessionEvent) {}
    fn on_message_event(event: &MessageEvent) {}
    fn on_ack_event(event: &MessageEvent) {}
}

/// Basic handler for session events that does nothing.
pub struct SimpleEventHandler;

impl EventHandler for SimpleEventHandler {}
