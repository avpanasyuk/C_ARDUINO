#include "../C_ARDUINO/General.h"
namespace avp {

  static unsigned char h2int(char c) {
    if(c >= '0' && c <= '9') {
      return((unsigned char)c - '0');
    }
    if(c >= 'a' && c <= 'f') {
      return((unsigned char)c - 'a' + 10);
    }
    if(c >= 'A' && c <= 'F') {
      return((unsigned char)c - 'A' + 10);
    }
    return(0);
  }

  String urldecode(String str) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    for(int i = 0; i < str.length(); i++) {
      c = str.charAt(i);
      if(c == '+') {
        encodedString += ' ';
      } else if(c == '%') {
        i++;
        code0 = str.charAt(i);
        i++;
        code1 = str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString += c;
      } else {
        encodedString += c;
      }
      yield();
    }
    return encodedString;
  }

  String urlencode(String str) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    char code2;
    for(int i = 0; i < str.length(); i++) {
      c = str.charAt(i);
      if(c == ' ') {
        encodedString += '+';
      } else if(isalnum(c)) {
        encodedString += c;
      } else {
        code1 = (c & 0xf) + '0';
        if((c & 0xf) > 9) {
          code1 = (c & 0xf) - 10 + 'A';
        }
        c = (c >> 4) & 0xf;
        code0 = c + '0';
        if(c > 9) {
          code0 = c - 10 + 'A';
        }
        code2 = '\0';
        encodedString += '%';
        encodedString += code0;
        encodedString += code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
  }

  String String_vprintf(const char *format, va_list ap) {
    va_list ap_;
    va_copy(ap_, ap); // turns out vsnprintf is changing ap, so we have to make a reserve copy
    int Size = vsnprintf(nullptr, 0, format, ap_);
    if(Size < 0) return "string_vprintf: format is wrong!";
    char Buffer[Size + 1]; // +1 to include ending zero byte
    vsprintf(Buffer, format, ap);
    return String(Buffer); // we do not write ending 0 byte
  } // string_vprintf

  String String_printf(char const *format, ...) {
    va_list ap;
    va_start(ap, format);
    String Out = String_vprintf(format, ap);
    va_end(ap);
    return Out;
  } // string_printf

} // namespace avp


