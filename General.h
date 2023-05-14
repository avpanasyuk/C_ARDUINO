#pragma once

#include <Arduino.h>

// following define disables interrupts but enables them does not matter how function has returned
#define PAUSE_INTERRUPTS struct _t { _t() { noInterrupts(); } ~_t() { interrupts(); } } _

String urldecode(String str);
String urlencode(String str);