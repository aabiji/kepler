#pragma once

#include <format>

#define THROW_ERROR(fmt, ...)                                                  \
  do {                                                                         \
    int _line = __LINE__;                                                      \
    const char *_file = __FILE__;                                              \
    throw std::runtime_error(                                                  \
        std::vformat(std::string("{}:{} | ") + fmt,                            \
                     std::make_format_args(_file, _line, ##__VA_ARGS__)));     \
  } while (0)

void gl_debug_callback(unsigned int src, unsigned int type, unsigned int id,
                       unsigned int severity, int _length, const char *message,
                       const void *_user_param);