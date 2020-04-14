#include "log/colour_formatter.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "log/log_level.hpp"

namespace eng
{

std::string colour_formatter::format(
    const log_level level,
    const std::string &tag,
    const std::string &message,
    const std::string &filename,
    const int line)
{
    std::stringstream strm{ };

    // apply an ANSI escape sequence to start colour output
    switch(level)
    {
        case log_level::DEBUG:
            strm << "\x1b[35m";
            break;
        case log_level::INFO:
            strm << "\x1b[34m";
            break;
        case log_level::WARN:
            strm << "\x1b[33m";
            break;
        case log_level::ERROR:
            strm << "\x1b[31m";
            break;
        default: break;
    }

    // write message and reset ANSI escape code
    strm <<
        formatter_.format(level, tag, message, filename, line)
        << "\x1b[0m";

    return strm.str();
}

}


