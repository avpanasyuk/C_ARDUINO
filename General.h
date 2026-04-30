#pragma once

#include <stdarg.h>
#include <Arduino.h>
#include <Print.h>

// following define disables interrupts but enables them does not matter how function has returned
#define PAUSE_INTERRUPTS     \
  volatile struct _t {                \
    _t() { noInterrupts(); } \
    ~_t() { interrupts(); }  \
  } _;

namespace avp {
  String urldecode(String str);
  String urlencode(String str);
  String String_vprintf(const char *format, va_list ap);
  String String_printf(char const *format, ...);

  class Print : public ::Print {
  private:
    int (*putsFn)(const char *);

  public:
    explicit Print(int (*outputFunc)(const char *)) : putsFn(outputFunc) {}

    size_t write(uint8_t c) override {
      char buf[2] = {(char)c, '\0'};
      return putsFn(buf);
    }

    size_t write(const uint8_t *buffer, size_t size) override {
      for(size_t i = 0; i < size; i++) {
        write(buffer[i]);
      }
      return size;
    }
  }; // class Print;
} // namespace avp