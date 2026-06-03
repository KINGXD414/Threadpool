

#ifndef LOG_COMMON_HPP
#define LOG_COMMON_HPP

#define ANSI_FG_BLACK     "\033[30m"
#define ANSI_FG_RED       "\033[31m"
#define ANSI_FG_GREEN     "\033[32m"
#define ANSI_FG_YELLOW    "\033[33m"
#define ANSI_FG_BLUE      "\033[34m"
#define ANSI_FG_MAGENTA   "\033[35m"
#define ANSI_FG_CYAN      "\033[36m"
#define ANSI_FG_WHITE     "\033[37m"

#define ANSI_BG_BLACK     "\033[40m"
#define ANSI_BG_RED       "\033[41m"
#define ANSI_BG_GREEN     "\033[42m"
#define ANSI_BG_YELLOW    "\033[43m"
#define ANSI_BG_BLUE      "\033[44m"
#define ANSI_BG_MAGENTA   "\033[45m"
#define ANSI_BG_CYAN      "\033[46m"
#define ANSI_BG_WHITE     "\033[47m"

#define ANSI_RESET       "\033[0m"  // 重置颜色

namespace pool
{
    enum class LOG_LEVEL
    {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVEL
    };

    static const char *LLtoStr[] =
    {
        {"TRACE"}, // 0
        {"DEBUG"}, // 1
        {"INFO"},
        {"WARN"},
        {"ERROR"},
        {"FATAL"},
        {"NUM_LOG_LEVEL"}
    };


    static const char *LLFgColor[] =
    {
        ANSI_FG_CYAN,      // TRACE：青色
        ANSI_FG_BLUE,      // DEBUG：蓝色
        ANSI_FG_GREEN,     // INFO：绿色
        ANSI_FG_YELLOW,    // WARN：黄色
        ANSI_FG_RED,       // ERROR：红色
        ANSI_FG_MAGENTA,   // FATAL：洋红
        ANSI_RESET
        };


    static const char *LLBgColor[] =
    {
        ANSI_BG_BLACK,     // TRACE：黑底
        ANSI_BG_BLACK,     // DEBUG：黑底
        ANSI_BG_BLACK,     // INFO：黑底
        ANSI_BG_YELLOW,    // WARN：黄底
        ANSI_BG_RED,       // ERROR：红底
        ANSI_BG_MAGENTA,   // FATAL：洋红底
        ANSI_RESET
    };

    static const int SMALL_BUFF_SIZE = 128;
    static const int MEDIAN_BUFF_SIZE = 512;
    static const int LARGE_BUFF_SIZE = 1024;

} // namespace pool

#endif
