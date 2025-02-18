
#ifndef GEOARROW_HPP_EXCEPTION_INCLUDED
#define GEOARROW_HPP_EXCEPTION_INCLUDED

#include <exception>
#include <string>

#include "geoarrow/geoarrow.h"

#if defined(GEOARROW_DEBUG)
#define _GEOARROW_THROW_NOT_OK_IMPL(NAME, EXPR, EXPR_STR, ERR)                      \
  do {                                                                              \
    const int NAME = (EXPR);                                                        \
    if (NAME) {                                                                     \
      throw ::geoarrow::ErrnoException(                                             \
          NAME,                                                                     \
          std::string(EXPR_STR) + std::string(" failed with errno ") +              \
              std::to_string(NAME) + std::string("\n * ") + std::string(__FILE__) + \
              std::string(":") + std::to_string(__LINE__),                          \
          ERR);                                                                     \
    }                                                                               \
  } while (0)
#else
#define _GEOARROW_THROW_NOT_OK_IMPL(NAME, EXPR, EXPR_STR, ERR)                  \
  do {                                                                          \
    const int NAME = (EXPR);                                                    \
    if (NAME) {                                                                 \
      throw ::geoarrow::ErrnoException(NAME,                                    \
                                       std::string(EXPR_STR) +                  \
                                           std::string(" failed with errno ") + \
                                           std::to_string(NAME),                \
                                       ERR);                                    \
    }                                                                           \
  } while (0)
#endif

#define GEOARROW_THROW_NOT_OK(ERR, EXPR)                                             \
  _GEOARROW_THROW_NOT_OK_IMPL(_GEOARROW_MAKE_NAME(errno_status_, __COUNTER__), EXPR, \
                              #EXPR, ERR)

namespace geoarrow {

class Exception : public std::exception {
 public:
  std::string message;

  Exception() = default;

  explicit Exception(const std::string& msg) : message(msg){};

  const char* what() const noexcept override { return message.c_str(); }
};

class ErrnoException : public Exception {
 public:
  GeoArrowErrorCode code{};

  ErrnoException(GeoArrowErrorCode code, const std::string& msg,
                 struct GeoArrowError* error)
      : code(code) {
    if (error != nullptr) {
      message = msg + ": \n" + error->message;
    } else {
      message = msg;
    }
  }
};

}  // namespace geoarrow

#endif
