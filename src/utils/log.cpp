#include "log.hpp"
#define MAX_LOG_SIZE        1024

Log::Log()
{
    this->type = "UNKNOWN";
}

Log::Log(std::string type)
{
    this->type = type;
}

int Log::print(int log_type, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[MAX_LOG_SIZE];
    char * log_color = LOG_COLORS_T[log_type];
    vsnprintf(buffer, MAX_LOG_SIZE, format, args);
    printf("%s[%s] [%s] %s%s\n", log_color, this->get_time().c_str(), this->type.c_str(), buffer, KNRM);
    va_end(args);
    return 0;
}

std::string Log::get_time()
{
    const auto period = std::chrono::system_clock::now();
    return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(period.time_since_epoch()).count());
}

Log::~Log()
{
}