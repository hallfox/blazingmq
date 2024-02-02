use std::time::Duration;

pub const BROKER_DEFAULT_URI: &str = "tcp://localhost:30114";
pub const QUEUE_OPERATION_DEFAULT_TIMEOUT: Duration = Duration::from_secs(60 * 5);
