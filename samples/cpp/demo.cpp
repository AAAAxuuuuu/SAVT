#include <string>

namespace demo {

class Logger {
public:
    void log(const std::string& message);
};

class OrderService {
public:
    void submit();

private:
    Logger logger_;
};

void Logger::log(const std::string& message) {
    (void)message;
}

void OrderService::submit() {
    logger_.log("submit");
}

}  // namespace demo

