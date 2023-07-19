#include "source/common/http/http1/llhttp_parser_impl.h"

namespace Envoy {
namespace Http {
namespace Http1 {
namespace {

ParserStatus intToStatus(int rc) {
  switch (rc) {
  case HPE_OK:
    return ParserStatus::Ok;
  case HPE_PAUSED:
    return ParserStatus::Paused;
  default:
    return ParserStatus::Error;
  }
}

} // namespace

LlhttpHttpParserImpl::LlhttpHttpParserImpl(MessageType type, ParserCallbacks* data) {
  llhttp_type parser_type;
  switch (type) {
  case MessageType::Request:
    parser_type = HTTP_REQUEST;
    break;
  case MessageType::Response:
    parser_type = HTTP_RESPONSE;
    break;
  }

  llhttp_settings_init(&settings_);
  llhttp_init(&parser_, parser_type, &settings_);
  llhttp_set_lenient_chunked_length(&parser_, true);
  llhttp_set_lenient_keep_alive(&parser_, true);
  llhttp_set_lenient_transfer_encoding(&parser_, true);
  parser_.data = data;
  settings_.on_message_begin = [](llhttp_t* parser) -> int {
    auto* conn_impl = static_cast<ParserCallbacks*>(parser->data);
    return static_cast<int>(conn_impl->onMessageBegin());
  };
  settings_.on_url = [](llhttp_t* parser, const char* at, size_t length) -> int {
    auto* conn_impl = static_cast<ParserCallbacks*>(parser->data);
    return static_cast<int>(conn_impl->onUrl(at, length));
  };
  settings_.on_status = [](llhttp_t* parser, const char* at, size_t length) -> int {
    auto* conn_impl = static_cast<ParserCallbacks*>(parser->data);
    return static_cast<int>(conn_impl->onStatus(at, length));
  };
  settings_.on_header_field = [](llhttp_t* parser, const char* at, size_t length) -> int {
    auto* conn_impl = static_cast<ParserCallbacks*>(parser->data);
    return static_cast<int>(conn_impl->onHeaderField(at, length));
  };
  settings_.on_header_value = [](llhttp_t* parser, const char* at, size_t length) -> int {
    auto* conn_impl = static_cast<ParserCallbacks*>(parser->data);
    return static_cast<int>(conn_impl->onHeaderValue(at, length));
  };
  settings_.on_headers_complete = [](llhttp_t* parser) -> int {
    auto* conn_impl = static_cast<ParserCallbacks*>(parser->data);
    return static_cast<int>(conn_impl->onHeadersComplete());
  };
  settings_.on_body = [](llhttp_t* parser, const char* at, size_t length) -> int {
    static_cast<ParserCallbacks*>(parser->data)->bufferBody(at, length);
    return 0;
  };
  settings_.on_message_complete = [](llhttp_t* parser) -> int {
    auto* conn_impl = static_cast<ParserCallbacks*>(parser->data);
    return static_cast<int>(conn_impl->onMessageComplete());
  };
  settings_.on_chunk_header = [](llhttp_t* parser) -> int {
    // A 0-byte chunk header is used to signal the end of the chunked body.
    // When this function is called, http-parser holds the size of the chunk in
    // parser->content_length. See
    // https://github.com/nodejs/llhttp/blob/main/src/native/api.h#L59
    const bool is_final_chunk = (parser->content_length == 0);
    static_cast<ParserCallbacks*>(parser->data)->onChunkHeader(is_final_chunk);
    return 0;
  };
}

size_t LlhttpHttpParserImpl::execute(const char* slice, int len) {
  llhttp_errno_t err = llhttp_execute(&parser_, slice, len);
  if (err == HPE_OK) {
    return len;
  } else {
    return parser_.error_pos - slice;
  }
}

void LlhttpHttpParserImpl::resume() { llhttp_resume(&parser_); }

CallbackResult LlhttpHttpParserImpl::pause() {
  llhttp_pause(&parser_);
  return CallbackResult::Success;
}

ParserStatus LlhttpHttpParserImpl::getStatus() const { return intToStatus(parser_.error); }

Http::Code LlhttpHttpParserImpl::statusCode() const {
  return static_cast<Http::Code>(parser_.status_code);
}

bool LlhttpHttpParserImpl::isHttp11() const {
  return parser_.http_major == 1 && parser_.http_minor == 1;
}

absl::optional<uint64_t> LlhttpHttpParserImpl::contentLength() const {
  // An unset content length will be have all bits set.
  // See
  // https://github.com/nodejs/http-parser/blob/ec8b5ee63f0e51191ea43bb0c6eac7bfbff3141d/http_parser.h#L311
  if (parser_.content_length == ULLONG_MAX) {
    return absl::nullopt;
  }
  return parser_.content_length;
}

bool LlhttpHttpParserImpl::isChunked() const { return parser_.flags & F_CHUNKED; }

absl::string_view LlhttpHttpParserImpl::methodName() const {
  return llhttp_method_name(static_cast<llhttp_method_t>(parser_.method));
}

absl::string_view LlhttpHttpParserImpl::errorMessage() const {
  return llhttp_errno_name(static_cast<llhttp_errno>(parser_.error));
}

int LlhttpHttpParserImpl::hasTransferEncoding() const {
  return parser_.flags & F_TRANSFER_ENCODING;
}

} // namespace Http1
} // namespace Http
} // namespace Envoy
