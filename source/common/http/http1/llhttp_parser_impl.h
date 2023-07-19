#pragma once

#include <memory>

#include "source/common/http/http1/parser.h"

#include "llhttp.h"

namespace Envoy {
namespace Http {
namespace Http1 {

class LlhttpHttpParserImpl : public Parser {
public:
  LlhttpHttpParserImpl(MessageType type, ParserCallbacks* data);
  ~LlhttpHttpParserImpl() override = default;

  // Http1::Parser
  size_t execute(const char* slice, int len) override;
  void resume() override;
  CallbackResult pause() override;
  ParserStatus getStatus() const override;
  Http::Code statusCode() const override;
  bool isHttp11() const override;
  absl::optional<uint64_t> contentLength() const override;
  bool isChunked() const override;
  absl::string_view methodName() const override;
  absl::string_view errorMessage() const override;
  int hasTransferEncoding() const override;

private:
  llhttp_t parser_;
  llhttp_settings_t settings_;
};

} // namespace Http1
} // namespace Http
} // namespace Envoy
