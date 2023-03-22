#pragma once

// following define disables interrupts but enables them does not matter how function has returned
#define PAUSE_INTERRUPTS struct _t { _t() { noInterrupts(); } ~_t() { interrupts(); } } _