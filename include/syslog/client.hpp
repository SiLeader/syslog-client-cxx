//
// Created by cerussite on 4/2/20.
//

#pragma once

#ifdef __XTENSA__ // ESP32
#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#else // Linux
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <zconf.h>
#endif

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace slog {

enum class Facility {
  kKernel = 0,
  kUserLevel = 1,
  kMailSystem = 2,
  kSystemDaemons = 3,
  kSecurityAuthorization = 4,
  kMessagesGeneratedInternallyBuSyslog = 5,
  kLinePrinterSubsystem = 6,
  kNetworkNewsSubsystem = 7,
  kUucpSubsystem = 8,
  kClockDaemon = 9,
  kFtpDaemon = 11,
  kNtpSubsystem = 12,
  kLogAudit = 13,
  kLogAlert = 14,
  kLocalUse0 = 16,
  kLocalUse1 = 17,
  kLocalUse2 = 18,
  kLocalUse3 = 19,
  kLocalUse4 = 20,
  kLocalUse5 = 21,
  kLocalUse6 = 22,
  kLocalUse7 = 23,
};

enum class Severity {
  kEmergency = 0,
  kAlert = 1,
  kCritical = 2,
  kError = 3,
  kWarning = 4,
  kNotice = 5,
  kInformational = 6,
  kDebug = 7,
};

class SyslogClient {
 public:
  using clock_type = std::chrono::system_clock;

 private:
  int socket_ = -1;
  int process_id_;
  Facility facility_;
  std::string hostname_;
  std::string app_name_;

  sockaddr_in peer_;

 public:
  SyslogClient(Facility facility, std::string hostname,
               std::string app_name) noexcept
      : process_id_(getpid()),
        facility_(facility),
        hostname_(std::move(hostname)),
        app_name_(std::move(app_name)),
        peer_() {}

  ~SyslogClient() { close(); }

 public:
  void open(const std::string& peer_host, std::uint16_t port = 514) {
    close();

    peer_.sin_family = AF_INET;
    peer_.sin_port = htons(port);
    peer_.sin_addr.s_addr = inet_addr(peer_host.c_str());

    socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  }

  void close() {
    if (socket_ >= 0) {
      ::close(socket_);
      socket_ = -1;
    }
  }

 public:
  void write(Severity severity, clock_type::time_point timestamp,
             const std::string& message) {
    std::stringstream ss;
    createDatagram(ss, severity, timestamp, message);

    auto data_str = ss.str();
    ::sendto(socket_, data_str.c_str(), data_str.size(), 0,
             reinterpret_cast<sockaddr*>(&peer_), sizeof(peer_));
  }

  void write(Severity severity, const std::string& message) {
    write(severity, clock_type::now(), message);
  }

 private:
  void createDatagram(std::ostream& ss, Severity severity,
                      clock_type::time_point timestamp,
                      const std::string& message) {
    static constexpr char SP = ' ';
    static constexpr char NILVALUE = '-';

    // PRI
    {
      auto pri_value =
          static_cast<int>(facility_) * 8 + static_cast<int>(severity);
      ss << "<" << pri_value << ">";
    }

    // VERSION
    { ss << "1"; }

    ss << SP;

    // TIMESTAMP
    {
      auto tt = clock_type::to_time_t(timestamp);

      struct tm tmbuf;
      gmtime_r(&tt, &tmbuf);

      auto year = tmbuf.tm_year + 1900;
      auto month = tmbuf.tm_mon + 1;
      auto date = tmbuf.tm_mday;

      auto hour = tmbuf.tm_hour;
      auto minute = tmbuf.tm_min;
      auto second = tmbuf.tm_sec;

      ss << std::setw(4) << std::setfill('0') << year << '-' << std::setw(2)
         << std::setfill('0') << month << '-' << std::setw(2)
         << std::setfill('0') << date << 'T' << std::setw(2)
         << std::setfill('0') << hour << ':' << std::setw(2)
         << std::setfill('0') << minute << ':' << std::setw(2)
         << std::setfill('0') << second << "+00:00";
    }

    ss << SP;

    // HOSTNAME
    { ss << hostname_; }

    ss << SP;

    // APP-NAME
    { ss << app_name_; }

    ss << SP;

    // PROCID
    { ss << process_id_; }

    ss << SP;

    // MSGID
    { ss << NILVALUE; }

    ss << SP;

    // Structured-Data
    { ss << NILVALUE; }

    ss << SP;

    //   MSG
    { ss << message; }
  }
};

}  // namespace slog
