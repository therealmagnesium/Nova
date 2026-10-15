#pragma once
#include "Core/Base.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <deque>
#include <string>

namespace Nova
{
    enum class TextColor
    {
        Black = 0,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
        LightBlack,
        LightRed,
        LightGreen,
        LightYellow,
        LightBlue,
        LightMagenta,
        LightCyan,
        LightWhite,
        Count,
    };

    enum class LogLevel : u8
    {
        Info,
        Warn,
        Error,
        Fatal
    };

    struct LogEntry
    {
        LogLevel level;
        std::string text;
    };

    namespace Logs
    {
        void Push(LogLevel level, const char* text); // Log.cpp: ring buffer capped at 2048 entries
        const std::deque<LogEntry>& GetEntries();
        void Clear();
    }

    template <typename... Args>
    inline void Log(LogLevel level, const char* prefix, const char* message, TextColor color, Args... args)
    {
        const char* color_table[(u32)TextColor::Count] = {
            "\x1b[30m", // Black
            "\x1b[31m", // Red
            "\x1b[32m", // Green
            "\x1b[33m", // Yellow
            "\x1b[34m", // Blue
            "\x1b[35m", // Magenta
            "\x1b[36m", // Cyan
            "\x1b[37m", // White
            "\x1b[90m", // Light black
            "\x1b[91m", // Light red
            "\x1b[92m", // Light green
            "\x1b[93m", // Light yellow
            "\x1b[94m", // Light blue
            "\x1b[95m", // Light magenta
            "\x1b[96m", // Light cyan
            "\x1b[97m", // Light white
        };

        char text_buffer[8192] = {};
        snprintf(text_buffer, sizeof(text_buffer), message, args...);
        Logs::Push(level, text_buffer); // Plain text, no ANSI codes
        printf("%s%s %s \033[0m\n", color_table[(u32)color], prefix, text_buffer);
    }
}

#define INFO(message, ...) Nova::Log(Nova::LogLevel::Info, "Info:", message, Nova::TextColor::Green, ##__VA_ARGS__)
#define WARN(message, ...) Nova::Log(Nova::LogLevel::Warn, "Warn:", message, Nova::TextColor::Yellow, ##__VA_ARGS__)
#define ERROR(message, ...) Nova::Log(Nova::LogLevel::Error, "Error:", message, Nova::TextColor::LightRed, ##__VA_ARGS__)
#define FATAL(message, ...) Nova::Log(Nova::LogLevel::Fatal, "Fatal:", message, Nova::TextColor::Red, ##__VA_ARGS__)

#define ASSERT(expression, message, ...)   \
    {                                      \
        if (!(expression))                 \
        {                                  \
            FATAL(message, ##__VA_ARGS__); \
            exit(420);                     \
        }                                  \
    }
#define ASSERT_ERROR(expression, message, ...) \
    {                                          \
        if (!(expression))                     \
        {                                      \
            ERROR(message, ##__VA_ARGS__);     \
            return;                            \
        }                                      \
    }

#define ASSERT_RETURN(expression, returnVal, message, ...) \
    {                                                      \
        if (!(expression))                                 \
        {                                                  \
            ERROR(message, ##__VA_ARGS__);                 \
            return returnVal;                              \
        }                                                  \
    }
