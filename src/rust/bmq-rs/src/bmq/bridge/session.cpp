#include "bmq-rs/src/bmq/bridge/session.h"

#include "bmq-rs/src/bridge.rs.h"
#include "bmq-rs/src/bmq/bridge/errors.h"
#include "rust/cxx.h"

#include <bmqa_abstractsession.h>
#include <bmqa_closequeuestatus.h>
#include <bmqa_event.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_messageproperties.h>
#include <bmqa_openqueuestatus.h>
#include <bmqa_queueid.h>
#include <bmqa_session.h>
#include <bmqa_sessionevent.h>

#include <bmqt_compressionalgorithmtype.h>
#include <bmqt_queueflags.h>
#include <bmqt_resultcode.h>
#include <bmqt_sessionoptions.h>
#include <bmqt_uri.h>

#include <bsl_string_view.h>
#include <bsls_types.h>

#include <exception>
#include <memory>
#include <stdexcept>

namespace BloombergLP::bmq::bridge {

namespace {

// Support for bsl bridging

struct RustStrUtil
{
  static bsl::string_view to_string_view(const ::rust::Str& str)
  {
    return bsl::string_view(str.data(), str.size());
  }

  static bmqt::Uri to_uri(const ::rust::Str& str)
  {
    return bmqt::Uri(RustStrUtil::to_string_view(str));
  }
};

std::uint64_t
to_queue_flags(QueueMode mode)
{
  bsls::Types::Uint64 flags = 0;
  switch (mode) {
    case QueueMode::Admin:
      bmqt::QueueFlagsUtil::setAdmin(&flags);
      break;
    case QueueMode::Read:
      bmqt::QueueFlagsUtil::setReader(&flags);
      break;
    case QueueMode::Write:
      bmqt::QueueFlagsUtil::setWriter(&flags);
      break;
    case QueueMode::ReadWrite:
      bmqt::QueueFlagsUtil::setReader(&flags);
      bmqt::QueueFlagsUtil::setWriter(&flags);
      break;
    default:
      break;
  }
  return flags;
}

bmqt::QueueOptions
to_queue_options(const QueueOptions& options)
{
  bmqt::QueueOptions queue_options;
  queue_options.setMaxUnconfirmedMessages(options.max_unconfirmed_messages)
    .setMaxUnconfirmedBytes(options.max_unconfirmed_bytes)
    .setConsumerPriority(options.consumer_priority)
    .setSuspendsOnBadHostHealth(options.suspends_on_bad_host_health);
  return queue_options;
}

bmqa::MessageProperties
to_message_properties(const MessageProperties& properties)
{
  bmqa::MessageProperties message_properties;
  return message_properties;
}

bmqt::CompressionAlgorithmType::Enum
to_compression_algorithm_type(CompressionType compression_type)
{
  switch (compression_type) {
    case CompressionType::None: {
      return bmqt::CompressionAlgorithmType::e_NONE;
    } break;
    case CompressionType::Zlib: {
      return bmqt::CompressionAlgorithmType::e_ZLIB;
    } break;
    default: {
      return bmqt::CompressionAlgorithmType::e_UNKNOWN;
    }
  }
}

class BridgeException : public std::runtime_error
{
public:
  BridgeException(const char* what)
    : std::runtime_error(what)
  {}
};

class BridgeEventHandler : public bmqa::SessionEventHandler
{
public:
  BridgeEventHandler(OnSessionEvent on_session_event,
                     OnMessageEvent on_message_event)
    : d_on_session_event(on_session_event)
    , d_on_message_event(on_message_event)
  {}

  ~BridgeEventHandler() = default;

  void onSessionEvent(const bmqa::SessionEvent& event) override
  {
    d_on_session_event(event);
  }

  void onMessageEvent(const bmqa::MessageEvent& event) override
  {
    d_on_message_event(event);
  }

private:
  OnSessionEvent d_on_session_event;
  OnMessageEvent d_on_message_event;
};

} // namespace

Session::Session(bslma::ManagedPtr<SessionEventHandler> event_handler,
                 const bmqt::SessionOptions& options,
                 bmqt::CompressionAlgorithmType::Enum compression_type)
  : d_impl(std::make_unique<bmqa::Session>(event_handler, options))
  , d_message_compression_type(compression_type)
{}

GenericResultEnum
Session::start()
{
  return d_impl->start();
}

void
Session::stop()
{
  d_impl->stop();
}

std::int32_t
Session::open_queue_sync(rust::Str uri,
                         QueueMode mode,
                         QueueOptions options,
                         double timeout)
{
  bmqa::QueueId dummy;
  int flags = to_queue_flags(mode);
  bmqt::Uri queue_uri = RustStrUtil::to_uri(uri);
  bmqt::QueueOptions queue_options = to_queue_options(options);

  return d_impl
    ->openQueueSync(
      &dummy, queue_uri, flags, queue_options, bsls::TimeInterval(timeout))
    .result();
}

std::int32_t
Session::configure_queue_sync(rust::Str uri,
                              QueueOptions options,
                              double timeout)
{
  bmqa::QueueId queue_id;
  if (std::int32_t rc = d_impl->getQueueId(&queue_id, RustStrUtil::to_uri(uri));
      rc) {
    return rc;
  }

  bmqt::QueueOptions queue_options = to_queue_options(options);
  return d_impl
    ->configureQueueSync(&queue_id, queue_options, bsls::TimeInterval(timeout))
    .result();
}

std::int32_t
Session::close_queue_sync(rust::Str uri, double timeout)
{
  bmqa::QueueId queue_id;
  if (std::int32_t rc = d_impl->getQueueId(&queue_id, RustStrUtil::to_uri(uri));
      rc) {
    return rc;
  }
  return d_impl->closeQueueSync(&queue_id, bsls::TimeInterval(timeout))
    .result();
}

std::int32_t
Session::post(rust::Str uri,
              rust::Slice<const std::uint8_t> payload,
              MessageProperties properties,
              AckEventHandler on_ack)
{
  bmqa::QueueId queue_id;
  if (std::int32_t rc = d_impl->getQueueId(&queue_id, RustStrUtil::to_uri(uri));
      rc) {
    return rc;
  }

  auto message_properties = to_message_properties(properties);

  bmqa::MessageEventBuilder builder;
  d_impl->loadMessageEventBuilder(&builder);

  auto& message = builder.startMessage();
  message.setDataRef(reinterpret_cast<const char*>(payload.data()),
                     payload.size());
  message.setPropertiesRef(&message_properties);

  // TODO:
  // if (on_ack) {
  //   message.setCorrelationId(bmqt::CorrelationId(on_ack));
  // }

  message.setCompressionAlgorithmType(d_message_compression_type);

  if (std::int32_t rc = builder.packMessage(queue_id); rc) {
    return rc;
  }

  const auto& message_event = builder.messageEvent();
  return d_impl->post(message_event);
}

std::int32_t
Session::confirm(rust::Str broker_uri, Message message)
{
  return 0;
}

std::unique_ptr<Session>
make_session(OnSessionEvent on_session_event,
             OnMessageEvent on_message_event,
             rust::Str broker_uri,
             double timeout,
             CompressionType compression_type)
{
  if (to_compression_algorithm_type(compression_type) ==
      bmqt::CompressionAlgorithmType::e_UNKNOWN) {
    throw BridgeException("Unknown compression algorithm type");
  }

  auto event_handler_mp =
    bslma::ManagedPtrUtil::makeManaged<BridgeEventHandler>(on_session_event,
                                                           on_message_event);

  bsls::TimeInterval timeout_interval(timeout);
  bmqt::SessionOptions session_options;
  session_options.setBrokerUri(RustStrUtil::to_string_view(broker_uri))
    .setOpenQueueTimeout(timeout_interval)
    .setCloseQueueTimeout(timeout_interval)
    .setConfigureQueueTimeout(timeout_interval);
  bmqt::CompressionAlgorithmType::Enum compression_algorithm_type =
    to_compression_algorithm_type(compression_type);

  return std::make_unique<Session>(
    event_handler_mp, session_options, compression_algorithm_type);
}

} // namespace BloombergLP::bmq::bridge
