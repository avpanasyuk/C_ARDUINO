#pragma once

#include <stdarg.h>
#include <Arduino.h>

// following define disables interrupts but enables them does not matter how function has returned
#define PAUSE_INTERRUPTS struct _t { _t() { noInterrupts(); } ~_t() { interrupts(); } } _

namespace avp {
  String urldecode(String str);
  String urlencode(String str);
  String String_vprintf(const char *format, va_list ap);
  String String_printf(char const *format, ...);
  void inline TogglePin(uint8_t pin) {
    digitalWrite(pin, !digitalRead(pin));
  } // TogglePin
} // namespace avp