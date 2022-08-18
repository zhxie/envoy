#include "source/common/common/regex.h"

#include "envoy/common/exception.h"
#include "envoy/extensions/regex_engines/v3/google_re2.pb.h"
#include "envoy/extensions/regex_engines/v3/google_re2.pb.validate.h"
#include "envoy/type/matcher/v3/regex.pb.h"
#include "envoy/type/matcher/v3/regex.pb.validate.h"

#include "source/common/common/assert.h"
#include "source/common/common/fmt.h"
#include "source/common/runtime/runtime_features.h"

namespace Envoy {
namespace Regex {

CompiledGoogleReMatcher::CompiledGoogleReMatcher(const std::string& regex,
                                                 bool do_program_size_check)
    : regex_(regex, re2::RE2::Quiet) {
  if (!regex_.ok()) {
    throw EnvoyException(regex_.error());
  }

  if (do_program_size_check && Runtime::isRuntimeInitialized()) {
    const uint32_t regex_program_size = static_cast<uint32_t>(regex_.ProgramSize());
    const uint32_t max_program_size_error_level =
        Runtime::getInteger("re2.max_program_size.error_level", 100);
    if (regex_program_size > max_program_size_error_level) {
      throw EnvoyException(fmt::format("regex '{}' RE2 program size of {} > max program size of "
                                       "{} set for the error level threshold. Increase "
                                       "configured max program size if necessary.",
                                       regex, regex_program_size, max_program_size_error_level));
    }

    const uint32_t max_program_size_warn_level =
        Runtime::getInteger("re2.max_program_size.warn_level", UINT32_MAX);
    if (regex_program_size > max_program_size_warn_level) {
      ENVOY_LOG_MISC(warn,
                     "regex '{}' RE2 program size of {} > max program size of {} set for the warn "
                     "level threshold. Increase configured max program size if necessary.",
                     regex, regex_program_size, max_program_size_warn_level);
    }
  }
}

CompiledGoogleReMatcher::CompiledGoogleReMatcher(
    const envoy::type::matcher::v3::RegexMatcher& config)
    : CompiledGoogleReMatcher(config.regex(), !config.google_re2().has_max_program_size()) {
  const uint32_t regex_program_size = static_cast<uint32_t>(regex_.ProgramSize());

  // Check if the deprecated field max_program_size is set first, and follow the old logic if so.
  if (config.google_re2().has_max_program_size()) {
    const uint32_t max_program_size =
        PROTOBUF_GET_WRAPPED_OR_DEFAULT(config.google_re2(), max_program_size, 100);
    if (regex_program_size > max_program_size) {
      throw EnvoyException(fmt::format("regex '{}' RE2 program size of {} > max program size of "
                                       "{}. Increase configured max program size if necessary.",
                                       config.regex(), regex_program_size, max_program_size));
    }
  }
}

GoogleReEngine::GoogleReEngine() : GoogleReEngine(false) {}

GoogleReEngine::GoogleReEngine(bool partial_match) : partial_match_(partial_match) {}

CompiledMatcherPtr GoogleReEngine::matcher(const std::string& regex) const {
  if (partial_match_) {
    return std::make_unique<CompiledGoogleRePartialMatcher>(regex, true);
  }

  return std::make_unique<CompiledGoogleReMatcher>(regex, true);
}

EnginePtr GoogleReEngineFactory::createEngine(
    const Protobuf::Message& config,
    Server::Configuration::ServerFactoryContext& server_factory_context) {
  const auto google_re2 =
      MessageUtil::downcastAndValidate<const envoy::extensions::regex_engines::v3::GoogleRE2&>(
          config, server_factory_context.messageValidationVisitor());

  return std::make_shared<GoogleReEngine>(google_re2.partial_match());
}

ProtobufTypes::MessagePtr GoogleReEngineFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::extensions::regex_engines::v3::GoogleRE2>();
}

REGISTER_FACTORY(GoogleReEngineFactory, EngineFactory);

std::regex Utility::parseStdRegex(const std::string& regex, std::regex::flag_type flags) {
  // TODO(zuercher): In the future, PGV (https://github.com/envoyproxy/protoc-gen-validate)
  // annotations may allow us to remove this in favor of direct validation of regular
  // expressions.
  try {
    return std::regex(regex, flags);
  } catch (const std::regex_error& e) {
    throw EnvoyException(fmt::format("Invalid regex '{}': {}", regex, e.what()));
  }
}

} // namespace Regex
} // namespace Envoy
