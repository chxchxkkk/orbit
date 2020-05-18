// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_SSH_ERROR_H_
#define ORBIT_SSH_ERROR_H_

#include <libssh2_sftp.h>

#include <outcome.hpp>
#include <system_error>

namespace OrbitSsh {

enum class Error {
  kBannerRecv = LIBSSH2_ERROR_BANNER_RECV,
  kBannerSend = LIBSSH2_ERROR_BANNER_SEND,
  kInvalidMac = LIBSSH2_ERROR_INVALID_MAC,
  kKexFailure = LIBSSH2_ERROR_KEX_FAILURE,
  kAlloc = LIBSSH2_ERROR_ALLOC,
  kSocketSend = LIBSSH2_ERROR_SOCKET_SEND,
  kExchangeFailure = LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE,
  kTimeout = LIBSSH2_ERROR_TIMEOUT,
  kHostkeyInit = LIBSSH2_ERROR_HOSTKEY_INIT,
  kHostkeySign = LIBSSH2_ERROR_HOSTKEY_SIGN,
  kDecrypt = LIBSSH2_ERROR_DECRYPT,
  kSocketDisconnect = LIBSSH2_ERROR_SOCKET_DISCONNECT,
  kProto = LIBSSH2_ERROR_PROTO,
  kPasswordExpired = LIBSSH2_ERROR_PASSWORD_EXPIRED,
  kFile = LIBSSH2_ERROR_FILE,
  kMethodNone = LIBSSH2_ERROR_METHOD_NONE,
  kAuthenticationFailed = LIBSSH2_ERROR_AUTHENTICATION_FAILED,
  kPublickeyUnverified = LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
  kChannelOutOfOrder = LIBSSH2_ERROR_CHANNEL_OUTOFORDER,
  kChannelFailure = LIBSSH2_ERROR_CHANNEL_FAILURE,
  kChannelRequestDenied = LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED,
  kChannelUnknown = LIBSSH2_ERROR_CHANNEL_UNKNOWN,
  kChannelWindowExceeded = LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED,
  kChannelPacketExceeded = LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED,
  kChannelClosed = LIBSSH2_ERROR_CHANNEL_CLOSED,
  kChannelEofSent = LIBSSH2_ERROR_CHANNEL_EOF_SENT,
  kScpProtocol = LIBSSH2_ERROR_SCP_PROTOCOL,
  kZlib = LIBSSH2_ERROR_ZLIB,
  kSocketTimeout = LIBSSH2_ERROR_SOCKET_TIMEOUT,
  kSftpProtocol = LIBSSH2_ERROR_SFTP_PROTOCOL,
  kRequestDenied = LIBSSH2_ERROR_REQUEST_DENIED,
  kMethodNotSupported = LIBSSH2_ERROR_METHOD_NOT_SUPPORTED,
  kInvalid = LIBSSH2_ERROR_INVAL,
  kInvalidPollType = LIBSSH2_ERROR_INVALID_POLL_TYPE,
  kPublicKeyProtocol = LIBSSH2_ERROR_PUBLICKEY_PROTOCOL,
  kEagain = LIBSSH2_ERROR_EAGAIN,
  kBufferToSmall = LIBSSH2_ERROR_BUFFER_TOO_SMALL,
  kBadUse = LIBSSH2_ERROR_BAD_USE,
  kCompress = LIBSSH2_ERROR_COMPRESS,
  kOutOfBoundary = LIBSSH2_ERROR_OUT_OF_BOUNDARY,
  kAgentProtocol = LIBSSH2_ERROR_AGENT_PROTOCOL,
  kSocketRecv = LIBSSH2_ERROR_SOCKET_RECV,
  kEncrypt = LIBSSH2_ERROR_ENCRYPT,
  kBadSocket = LIBSSH2_ERROR_BAD_SOCKET,
  kKnownHosts = LIBSSH2_ERROR_KNOWN_HOSTS,
  kChannelWindowFull = LIBSSH2_ERROR_CHANNEL_WINDOW_FULL,
  kKeyfileAuthFailed = LIBSSH2_ERROR_KEYFILE_AUTH_FAILED,
  kSocketNone = LIBSSH2_ERROR_SOCKET_NONE,
  kInvalidIP,
  kUnknown,
  kFailedCreatingSession
};

struct ErrorCategory : std::error_category {
  using std::error_category::error_category;

  const char* name() const noexcept override { return "libssh2_sftp"; }
  std::string message(int condition) const override;
  std::error_condition default_error_condition(int c) const noexcept override {
    if (static_cast<Error>(c) == Error::kEagain) {
      return std::make_error_condition(
          std::errc::resource_unavailable_try_again);
    } else {
      return std::error_condition{c, *this};
    }
  }
};

inline const ErrorCategory& GetErrorCategory() {
  static ErrorCategory category{};
  return category;
}

inline std::error_code make_error_code(Error e) {
  return std::error_code{static_cast<int>(e), GetErrorCategory()};
}

inline bool shouldITryAgain(std::error_code e) {
  return e == std::errc::resource_unavailable_try_again;
}

template <typename T>
bool shouldITryAgain(const outcome::result<T>& result) {
  return result.has_error() &&
         result.error() == std::errc::resource_unavailable_try_again;
}

}  // namespace OrbitSsh

namespace std {
template <>
struct is_error_condition_enum<OrbitSsh::Error> : std::true_type {};
}  // namespace std
#endif  // ORBIT_SSH_ERROR_H_