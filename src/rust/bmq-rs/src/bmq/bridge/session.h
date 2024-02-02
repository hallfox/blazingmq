#ifndef BMQ_BRIDGE_SESSION_H
#define BMQ_BRIDGE_SESSION_H

#include <bmqa_abstractsession.h>
#include <bmqt_compressionalgorithmtype.h>

#include <memory>

namespace BloombergLP::bmq::bridge {
class Session;
}

#include "bmq-rs/src/bridge.rs.h"
#include "rust/cxx.h"

namespace BloombergLP {

namespace bmqt {
class SessionOptions;
} // namespace bmqt

namespace bmq {
namespace bridge {

class Session
{
public:
  using AckEventHandler = rust::Fn<void(rust::Box<AckEventContext>)>;

  /// Create a bridge to a bmqa::Session.
  Session(bslma::ManagedPtr<bmqa::SessionEventHandler> event_handler,
          const bmqt::SessionOptions& options,
          bmqt::CompressionAlgorithmType::Enum compression_type);

  /// Connect to the BMQ broker and start the message processing for this
  /// Session. This method blocks until either the Session is connected to the
  /// broker, fails to connect, or the operation times out. If the optionally
  /// specified timeout is not populated, use the one defined in the session
  /// options. Return 0 on success, or a non-zero value corresponding to the
  /// bmqt::GenericResult::Enum enum values otherwise. The behavior is undefined
  /// if this method is called on an already started Session.
  GenericResultEnum start();

  /// Gracefully disconnect from the BMQ broker and stop the operation of this
  /// Session. This method blocks waiting for all already invoked event handlers
  /// to exit and all session-related operations to be finished. No other method
  /// but start() may be used after this method returns. This method must NOT be
  /// called if the session is in synchronous mode (i.e., not using the
  /// EventHandler), stopAsync() should be called in this case.
  void stop();

  /// Open the queue having the specified uri with the specified flags (a
  /// combination of the values defined in bmqt::QueueFlags::Enum), using the
  /// specified queueId to correlate events related to that queue. Return a
  /// result providing the status and context of the operation. Use the
  /// optionally specified options to configure some advanced settings. Note
  /// that this operation fails if queueId is non-unique. If the optionally
  /// specified timeout is not populated, use the one defined in the session
  /// options. This operation will block until either success, failure, or
  /// timing out happens.
  ///
  /// THREAD: Note that calling this method from the event
  /// processing thread(s) (i.e., from the EventHandler callback, if provided)
  /// WILL lead to a DEADLOCK.
  std::int32_t open_queue_sync(rust::Str uri,
                               QueueMode flags,
                               QueueOptions options,
                               double timeout);

  /// Configure the queue identified by the specified queueId using the
  /// specified options and return a result providing the status and context of
  /// the operation. Fields from options that have not been explicitly set will
  /// not be modified. If the optionally specified timeout is not populated, use
  /// the one defined in the session options. This operation returns error if
  /// there is a pending configure for the same queue. This operation will block
  /// until either success, failure, or timing out happens.
  ///
  /// THREAD: Note that calling this method from the event processing thread(s)
  /// (i.e., from the EventHandler callback, if provided) WILL lead to a
  /// DEADLOCK.
  std::int32_t configure_queue_sync(rust::Str uri,
                                    QueueOptions options,
                                    double timeout);

  /// Close the queue identified by the specified queueId and return a result
  /// providing the status and context of the operation. If the optionally
  /// specified timeout is not populated, use the one defined in the session
  /// options. Any outstanding configureQueue request for this queueId will be
  /// canceled. This operation will block until either success, failure, or
  /// timing out happens. Once this method returns, there is guarantee that no
  /// more messages and events for this queueId will be received. Note that
  /// successful processing of this request in the broker closes this session's
  /// view of the queue; the underlying queue may not be deleted in the broker.
  /// When this method returns, the correlationId associated to the queue is
  /// cleared.
  ///
  /// THREAD: Note that calling this method from the event processing thread(s)
  /// (i.e., from the EventHandler callback, if provided) WILL lead to a
  /// DEADLOCK.
  std::int32_t close_queue_sync(rust::Str uri, double timeout);

  /// Asynchronously post the specified event that must contain one or more
  /// Messages. The return value is one of the values defined in the
  /// bmqt::PostResult::Enum enum. Return zero on success and a non-zero value
  /// otherwise. Note that success implies that SDK has accepted the event and
  /// will eventually deliver it to the broker. The behavior is undefined unless
  /// the session was started.
  std::int32_t post(rust::Str uri,
                    rust::Slice<const std::uint8_t> payload,
                    MessageProperties properties,
                    AckEventHandler on_ack);

  /// Asynchronously confirm the receipt of the message with which the specified
  /// cookie is associated. This indicates that the application is done
  /// processing the message and that the broker can safely discard it from the
  /// queue according to the retention policy set up for that queue. Return 0 on
  /// success, and a non-zero value otherwise. Note that success implies that
  /// SDK has accepted the message and will eventually send confirmation
  /// notification to the broker.
  std::int32_t confirm(rust::Str uri, Message message);

  // std::int32_t queue_options(QueueId& queue_id, rust::Str uri);

private:
  std::unique_ptr<bmqa::AbstractSession> d_impl;
  bmqt::CompressionAlgorithmType::Enum d_message_compression_type;
};

using OnSessionEvent = rust::Fn<void(rust::Box<BridgeContext>)>;
using OnMessageEvent = rust::Fn<void(rust::Box<BridgeContext>)>;

/// @brief Create a handle to a BlazingMQ session.
///
/// @param broker_uri The URI for the BlazingMQ broker being connected to.
/// @param timeout The timeout for establishing connection to the broker.
/// @param compression_type The default compression algorithm to use when
/// sending messages.
/// @returns A pointer to the session handle.
/// @throws BridgeException if `compression_type` is invalid.
std::unique_ptr<Session>
make_session(OnSessionEvent on_session_event,
             OnMessageEvent on_message_event,
             rust::Str broker_uri,
             double timeout,
             CompressionType compression_type);
             
} // namespace bridge
} // namespace bmq
} // namespace BloombergLP

#endif
