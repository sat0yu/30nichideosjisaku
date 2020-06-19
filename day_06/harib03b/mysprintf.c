#include <stdarg.h>

int decimal2ascii (char *str, int dec) {
    int i = 0, len; // length of the result string
    int buf[10]; // ((1 << 32) - 1).length = 10

    while(1) {
        buf[i++] = dec % 10;
        if(dec < 10) break;
        dec /= 10;
    }
    len = i;
    for(; i > 0; i--) {
        *(str++) = buf[i-1] + 0x30; // The ascii code for '0'
    }
    return len;
}

void sprintf (char *str, char *fmt, ...) {
    int len;

    va_list args;
    va_start (args, fmt);

    while (*fmt) {
        if(*fmt=='%') {
            fmt++;
            if(*fmt == 'd'){
                len = decimal2ascii(str, va_arg (args, int));
            }
            str += len;
            fmt++;
        } else {
            *(str++) = *(fmt++);
        }
    }
    *str = 0x00;
    va_end (args);
}
