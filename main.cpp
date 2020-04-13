#include <iostream>
#include <syslog/client.hpp>

int main() {
  std::cout << "Hello, World!" << std::endl;

  slog::SyslogClient client(slog::Facility::kSystemDaemons, "test.test.test",
                            "amc");
  client.open("127.0.0.1", 8514);

  client.write(slog::Severity::kInformational, "message");

  return 0;
}
