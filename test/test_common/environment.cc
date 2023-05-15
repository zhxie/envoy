#include "test/test_common/environment.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "envoy/common/platform.h"

#include "source/common/common/assert.h"
#include "source/common/common/compiler_requirements.h"
#include "source/common/common/logger.h"
#include "source/common/common/macros.h"
#include "source/common/common/utility.h"
#include "source/common/filesystem/directory.h"
#include "source/common/io/io_uring_impl.h"

#include "absl/container/node_hash_map.h"

#ifdef ENVOY_HANDLE_SIGNALS
#include "source/common/signal/signal_action.h"
#endif

#include "source/server/options_impl.h"

#include "test/test_common/file_system_for_test.h"
#include "test/test_common/network_utility.h"

#include "absl/debugging/symbolize.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "gtest/gtest.h"
#include "spdlog/spdlog.h"

using bazel::tools::cpp::runfiles::Runfiles;
using Envoy::Filesystem::Directory;
using Envoy::Filesystem::DirectoryEntry;

namespace Envoy {
namespace {

std::string makeTempDir(std::string basename_template) {
#ifdef WIN32
  std::string name_template{};
  if (std::getenv("TMP")) {
    name_template = TestEnvironment::getCheckedEnvVar("TMP") + "\\" + basename_template;
  } else if (std::getenv("WINDIR")) {
    name_template = TestEnvironment::getCheckedEnvVar("WINDIR") + "\\TEMP\\" + basename_template;
  } else {
    name_template = basename_template;
  }
  char* dirname = ::_mktemp(&name_template[0]);
  RELEASE_ASSERT(dirname != nullptr, fmt::format("failed to create tempdir from template: {} {}",
                                                 name_template, errorDetails(errno)));
  TestEnvironment::createPath(dirname);
#else
  std::string name_template = "/tmp/" + basename_template;
  char* dirname = ::mkdtemp(&name_template[0]);
  RELEASE_ASSERT(dirname != nullptr, fmt::format("failed to create tempdir from template: {} {}",
                                                 name_template, errorDetails(errno)));
#endif
  return std::string(dirname);
}

std::string getOrCreateUnixDomainSocketDirectory() {
  const char* path = std::getenv("TEST_UDSDIR");
  if (path != nullptr) {
    return std::string(path);
  }
  // Generate temporary path for Unix Domain Sockets only. This is a workaround
  // for the sun_path limit on `sockaddr_un`, since TEST_TMPDIR as generated by
  // Bazel may be too long.
  return makeTempDir("envoy_test_uds.XXXXXX");
}

std::string getTemporaryDirectory() {
  std::string temp_dir;
  if (std::getenv("TEST_TMPDIR")) {
    temp_dir = TestEnvironment::getCheckedEnvVar("TEST_TMPDIR");
  } else if (std::getenv("TMPDIR")) {
    temp_dir = TestEnvironment::getCheckedEnvVar("TMPDIR");
  } else {
    return makeTempDir("envoy_test_tmp.XXXXXX");
  }
  TestEnvironment::createPath(temp_dir);
  return temp_dir;
}

// Allow initializeOptions() to remember CLI args for getOptions().
int argc_;
char** argv_;

} // namespace

void TestEnvironment::createPath(const std::string& path) {
  if (Filesystem::fileSystemForTest().directoryExists(path)) {
    return;
  }
  const Filesystem::PathSplitResult parent =
      Filesystem::fileSystemForTest().splitPathFromFilename(path);
  if (parent.file_.length() > 0) {
    TestEnvironment::createPath(std::string(parent.directory_));
  }
#ifndef WIN32
  RELEASE_ASSERT(::mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0,
                 absl::StrCat("failed to create path: ", path));
#else
  RELEASE_ASSERT(::CreateDirectory(path.c_str(), NULL),
                 absl::StrCat("failed to create path: ", path));
#endif
}

// On linux, attempt to unlink any file that exists at path,
// ignoring the result code, to avoid traversing a symlink,
// On windows, also attempt to remove the directory in case
// it is actually a symlink/junction, ignoring the result code.
// Proceed to iteratively recurse the directory if it still remains
void TestEnvironment::removePath(const std::string& path) {
  RELEASE_ASSERT(absl::StartsWith(path, TestEnvironment::temporaryDirectory()),
                 "cowardly refusing to remove test directory not in temp path");
#ifndef WIN32
  (void)::unlink(path.c_str());
#else
  (void)::DeleteFile(path.c_str());
  (void)::RemoveDirectory(path.c_str());
#endif
  if (!Filesystem::fileSystemForTest().directoryExists(path)) {
    return;
  }
  Directory directory(path);
  std::string entry_name;
  entry_name.reserve(path.size() + 256);
  entry_name.append(path);
  entry_name.append("/");
  size_t fileidx = entry_name.size();
  for (const DirectoryEntry& entry : directory) {
    entry_name.resize(fileidx);
    entry_name.append(entry.name_);
    if (entry.type_ == Envoy::Filesystem::FileType::Regular) {
#ifndef WIN32
      RELEASE_ASSERT(::unlink(entry_name.c_str()) == 0,
                     absl::StrCat("failed to remove file: ", entry_name));
#else
      RELEASE_ASSERT(::DeleteFile(entry_name.c_str()),
                     absl::StrCat("failed to remove file: ", entry_name));
#endif
    } else if (entry.type_ == Envoy::Filesystem::FileType::Directory) {
      if (entry.name_ != "." && entry.name_ != "..") {
        removePath(entry_name);
      }
    }
  }
#ifndef WIN32
  RELEASE_ASSERT(::rmdir(path.c_str()) == 0,
                 absl::StrCat("failed to remove path: ", path, " (rmdir failed)"));
#else
  RELEASE_ASSERT(::RemoveDirectory(path.c_str()), absl::StrCat("failed to remove path: ", path));
#endif
}

void TestEnvironment::renameFile(const std::string& old_name, const std::string& new_name) {
  Filesystem::fileSystemForTest().renameFile(old_name, new_name);
}

void TestEnvironment::createSymlink(const std::string& target, const std::string& link) {
#ifdef WIN32
  const DWORD attributes = ::GetFileAttributes(target.c_str());
  ASSERT_NE(attributes, INVALID_FILE_ATTRIBUTES);
  int flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
  if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
    flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
  }

  const BOOLEAN rc = ::CreateSymbolicLink(link.c_str(), target.c_str(), flags);
  ASSERT_NE(rc, 0);
#else
  const int rc = ::symlink(target.c_str(), link.c_str());
  ASSERT_EQ(rc, 0);
#endif
}

absl::optional<std::string> TestEnvironment::getOptionalEnvVar(const std::string& var) {
  const char* path = std::getenv(var.c_str());
  if (path == nullptr) {
    return {};
  }
  return std::string(path);
}

std::string TestEnvironment::getCheckedEnvVar(const std::string& var) {
  auto optional = getOptionalEnvVar(var);
  RELEASE_ASSERT(optional.has_value(), var);
  return optional.value();
}

void TestEnvironment::initializeTestMain(char* program_name) {
#ifdef WIN32
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

  _set_invalid_parameter_handler([](const wchar_t* expression, const wchar_t* function,
                                    const wchar_t* file, unsigned int line,
                                    uintptr_t pReserved) {});

  WSADATA wsa_data;
  const WORD version_requested = MAKEWORD(2, 2);
  RELEASE_ASSERT(WSAStartup(version_requested, &wsa_data) == 0, "");
#endif

  absl::InitializeSymbolizer(program_name);

#ifdef ENVOY_HANDLE_SIGNALS
  // Enabled by default. Control with "bazel --define=signal_trace=disabled"
  static Envoy::SignalAction handle_sigs;
#endif
}

void TestEnvironment::initializeOptions(int argc, char** argv) {
  argc_ = argc;
  argv_ = argv;
}

bool TestEnvironment::shouldRunTestForIpVersion(Network::Address::IpVersion type) {
  const char* value = std::getenv("ENVOY_IP_TEST_VERSIONS");
  std::string option(value ? value : "");
  if (option.empty()) {
    return true;
  }
  if ((type == Network::Address::IpVersion::v4 && option == "v6only") ||
      (type == Network::Address::IpVersion::v6 && option == "v4only")) {
    return false;
  }
  return true;
}

std::vector<Network::Address::IpVersion> TestEnvironment::getIpVersionsForTest() {
  std::vector<Network::Address::IpVersion> parameters;
  for (auto version : {Network::Address::IpVersion::v4, Network::Address::IpVersion::v6}) {
    if (TestEnvironment::shouldRunTestForIpVersion(version)) {
      parameters.push_back(version);
      if (!Network::Test::supportsIpVersion(version)) {
        const auto version_string = Network::Test::addressVersionAsString(version);
        ENVOY_LOG_TO_LOGGER(
            Logger::Registry::getLog(Logger::Id::testing), warn,
            "Testing with IP{} addresses may not be supported on this machine. If "
            "testing fails, set the environment variable ENVOY_IP_TEST_VERSIONS to 'v{}only'.",
            version_string, version_string);
      }
    }
  }
  return parameters;
}

std::vector<Network::DefaultSocketInterface> TestEnvironment::getSocketInterfacesForTest() {
  std::vector<Network::DefaultSocketInterface> parameters{Network::DefaultSocketInterface::Default};
  if (Envoy::Io::isIoUringSupported()) {
    parameters.push_back(Network::DefaultSocketInterface::IoUring);
  } else {
    ENVOY_LOG_TO_LOGGER(Logger::Registry::getLog(Logger::Id::testing), warn,
                        "io_uring may not be supported on this machine.");
  }
  return parameters;
}

std::vector<spdlog::logger*> TestEnvironment::getSpdLoggersForTest() {
  std::vector<spdlog::logger*> logger_parameters;
  logger_parameters.push_back(&Logger::Registry::loggers()[0].getLogger());
  std::atomic<spdlog::logger*> flogger{nullptr};
  getFineGrainLogContext().initFineGrainLogger(__FILE__, flogger);
  logger_parameters.push_back(flogger.load(std::memory_order_relaxed));
  return logger_parameters;
}

Server::Options& TestEnvironment::getOptions() {
  static OptionsImpl* options = new OptionsImpl(
      argc_, argv_, [](bool) { return "1"; }, spdlog::level::err);
  return *options;
}

std::vector<Grpc::ClientType> TestEnvironment::getsGrpcVersionsForTest() {
#ifdef ENVOY_GOOGLE_GRPC
  return {Grpc::ClientType::EnvoyGrpc, Grpc::ClientType::GoogleGrpc};
#else
  return {Grpc::ClientType::EnvoyGrpc};
#endif
}

const std::string& TestEnvironment::temporaryDirectory() {
  CONSTRUCT_ON_FIRST_USE(std::string, getTemporaryDirectory());
}

std::string TestEnvironment::runfilesDirectory(const std::string& workspace) {
  RELEASE_ASSERT(runfiles_ != nullptr, "");
  auto path = runfiles_->Rlocation(workspace);
#ifdef WIN32
  path = std::regex_replace(path, std::regex("\\\\"), "/");
#endif
  return path;
}

std::string TestEnvironment::runfilesPath(const std::string& path, const std::string& workspace) {
  RELEASE_ASSERT(runfiles_ != nullptr, "");
  return runfiles_->Rlocation(absl::StrCat(workspace, "/", path));
}

const std::string TestEnvironment::unixDomainSocketDirectory() {
  CONSTRUCT_ON_FIRST_USE(std::string, getOrCreateUnixDomainSocketDirectory());
}

std::string TestEnvironment::substitute(const std::string& str,
                                        Network::Address::IpVersion version) {
  const absl::node_hash_map<std::string, std::string> path_map = {
      {"test_tmpdir", TestEnvironment::temporaryDirectory()},
      {"test_udsdir", TestEnvironment::unixDomainSocketDirectory()},
      {"test_rundir", runfiles_ != nullptr ? TestEnvironment::runfilesDirectory() : "invalid"},
  };

  std::string out_json_string = str;
  for (const auto& it : path_map) {
    const std::regex port_regex("\\{\\{ " + it.first + " \\}\\}");
    out_json_string = std::regex_replace(out_json_string, port_regex, it.second);
  }

  // Substitute platform specific null device.
  const std::regex null_device_regex(R"(\{\{ null_device_path \}\})");
  out_json_string = std::regex_replace(out_json_string, null_device_regex,
                                       std::string(Platform::null_device_path).c_str());

  // Substitute IP loopback addresses.
  const std::regex loopback_address_regex(R"(\{\{ ip_loopback_address \}\})");
  out_json_string = std::regex_replace(out_json_string, loopback_address_regex,
                                       Network::Test::getLoopbackAddressString(version));
  const std::regex ntop_loopback_address_regex(R"(\{\{ ntop_ip_loopback_address \}\})");
  out_json_string = std::regex_replace(out_json_string, ntop_loopback_address_regex,
                                       Network::Test::getLoopbackAddressString(version));

  // Substitute IP any addresses.
  const std::regex any_address_regex(R"(\{\{ ip_any_address \}\})");
  out_json_string = std::regex_replace(out_json_string, any_address_regex,
                                       Network::Test::getAnyAddressString(version));

  // Substitute dns lookup family.
  const std::regex lookup_family_regex(R"(\{\{ dns_lookup_family \}\})");
  switch (version) {
  case Network::Address::IpVersion::v4:
    out_json_string = std::regex_replace(out_json_string, lookup_family_regex, "v4_only");
    break;
  case Network::Address::IpVersion::v6:
    out_json_string = std::regex_replace(out_json_string, lookup_family_regex, "v6_only");
    break;
  }

  // Substitute socket options arguments.
  const std::regex sol_socket_regex(R"(\{\{ sol_socket \}\})");
  out_json_string =
      std::regex_replace(out_json_string, sol_socket_regex, std::to_string(SOL_SOCKET));
  const std::regex so_reuseport_regex(R"(\{\{ so_reuseport \}\})");
  out_json_string =
      std::regex_replace(out_json_string, so_reuseport_regex, std::to_string(SO_REUSEPORT));

  return out_json_string;
}

std::string TestEnvironment::temporaryFileSubstitute(const std::string& path,
                                                     const PortMap& port_map,
                                                     Network::Address::IpVersion version) {
  return temporaryFileSubstitute(path, ParamMap(), port_map, version);
}

std::string TestEnvironment::readFileToStringForTest(const std::string& filename) {
  return Filesystem::fileSystemForTest().fileReadToEnd(filename);
}

std::string TestEnvironment::temporaryFileSubstitute(const std::string& path,
                                                     const ParamMap& param_map,
                                                     const PortMap& port_map,
                                                     Network::Address::IpVersion version) {
  RELEASE_ASSERT(!path.empty(), "requested path to substitute in is empty");
  // Load the entire file as a string, regex replace one at a time and write it back out. Proper
  // templating might be better one day, but this works for now.
  const std::string json_path = TestEnvironment::runfilesPath(path);
  std::string out_json_string = readFileToStringForTest(json_path);

  // Substitute params.
  for (const auto& it : param_map) {
    const std::regex param_regex("\\{\\{ " + it.first + " \\}\\}");
    out_json_string = std::regex_replace(out_json_string, param_regex, it.second);
  }

  // Substitute ports.
  for (const auto& it : port_map) {
    const std::regex port_regex("\\{\\{ " + it.first + " \\}\\}");
    out_json_string = std::regex_replace(out_json_string, port_regex, std::to_string(it.second));
  }

  // Substitute paths and other common things.
  out_json_string = substitute(out_json_string, version);

  auto name = Filesystem::fileSystemForTest().splitPathFromFilename(path).file_;
  const std::string extension = std::string(name.substr(name.rfind('.')));
  const std::string out_json_path =
      TestEnvironment::temporaryPath(name) + ".with.ports" + extension;
  {
    std::ofstream out_json_file(out_json_path, std::ios::binary);
    out_json_file << out_json_string;
  }
  return out_json_path;
}

Json::ObjectSharedPtr TestEnvironment::jsonLoadFromString(const std::string& json,
                                                          Network::Address::IpVersion version) {
  return Json::Factory::loadFromString(substitute(json, version));
}

// This function is not used for Envoy Mobile tests, and ::system() is not supported on iOS.
#ifndef TARGET_OS_IOS
void TestEnvironment::exec(const std::vector<std::string>& args) {
  std::stringstream cmd;
  // Symlinked args[0] can confuse Python when importing module relative, so we let Python know
  // where it can find its module relative files.
  cmd << "bash -c \"PYTHONPATH=$(dirname " << args[0] << ") ";
  for (auto& arg : args) {
    cmd << arg << " ";
  }
  cmd << "\"";
  if (::system(cmd.str().c_str()) != 0) {
    std::cerr << "Failed " << cmd.str() << "\n";
    RELEASE_ASSERT(false, "");
  }
}
#endif

std::string TestEnvironment::writeStringToFileForTest(const std::string& filename,
                                                      const std::string& contents,
                                                      bool fully_qualified_path, bool do_unlink) {
  const std::string out_path =
      fully_qualified_path ? filename : TestEnvironment::temporaryPath(filename);
  if (do_unlink) {
    unlink(out_path.c_str());
  }

  Filesystem::FilePathAndType out_file_info{Filesystem::DestinationType::File, out_path};
  Filesystem::FilePtr file = Filesystem::fileSystemForTest().createFile(out_file_info);
  const Filesystem::FlagSet flags{1 << Filesystem::File::Operation::Write |
                                  1 << Filesystem::File::Operation::Create};
  const Api::IoCallBoolResult open_result = file->open(flags);
  EXPECT_TRUE(open_result.return_value_) << open_result.err_->getErrorDetails();
  const Api::IoCallSizeResult result = file->write(contents);
  EXPECT_EQ(contents.length(), result.return_value_);
  return out_path;
}

void TestEnvironment::setEnvVar(const std::string& name, const std::string& value, int overwrite) {
#ifdef WIN32
  if (!overwrite) {
    size_t requiredSize;
    const int rc = ::getenv_s(&requiredSize, nullptr, 0, name.c_str());
    ASSERT_EQ(0, rc);
    if (requiredSize != 0) {
      return;
    }
  }
  const int rc = ::_putenv_s(name.c_str(), value.c_str());
  ASSERT_EQ(0, rc);
#else
  const int rc = ::setenv(name.c_str(), value.c_str(), overwrite);
  ASSERT_EQ(0, rc);
#endif
}

void TestEnvironment::unsetEnvVar(const std::string& name) {
#ifdef WIN32
  const int rc = ::_putenv_s(name.c_str(), "");
  ASSERT_EQ(0, rc);
#else
  const int rc = ::unsetenv(name.c_str());
  ASSERT_EQ(0, rc);
#endif
}

void TestEnvironment::setRunfiles(Runfiles* runfiles) { runfiles_ = runfiles; }

Runfiles* TestEnvironment::runfiles_{};

AtomicFileUpdater::AtomicFileUpdater(const std::string& filename)
    : link_(filename), new_link_(absl::StrCat(filename, ".new")),
      target1_(absl::StrCat(filename, ".target1")), target2_(absl::StrCat(filename, ".target2")),
      use_target1_(true) {
  unlink(link_.c_str());
  unlink(new_link_.c_str());
  unlink(target1_.c_str());
  unlink(target2_.c_str());
}

void AtomicFileUpdater::update(const std::string& contents) {
  const std::string target = use_target1_ ? target1_ : target2_;
  use_target1_ = !use_target1_;
  {
    std::ofstream file(target, std::ios_base::binary);
    file << contents;
  }
  TestEnvironment::createSymlink(target, new_link_);
  TestEnvironment::renameFile(new_link_, link_);
}

} // namespace Envoy
