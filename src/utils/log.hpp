#ifndef MEDIA_LOG
#define MEDIA_LOG

#include <string>
#include <chrono>
#include <cstdio>
#include <cstdarg>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

typedef enum {
    LOG_NORMAL,
    LOG_SUCCESS,
    LOG_WARNING,
    LOG_ERROR,
} LOG_T;

class Log {
public:
    Log();
    Log(std::string type);
    int print(int log_type, const char * format, ...);
    std::string get_time();
    ~Log();

private:
    std::string type;
    char LOG_COLORS_T[4][8] = {
        KNRM,
        KGRN,
        KYEL,
        KRED
    };
};

#endif //MEDIA_LOG