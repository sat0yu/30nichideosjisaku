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

int hex2ascii (char *str, int dec) {
    int i = 0, len; // length of the result string
    int buf[10]; // ((1 << 32) - 1).length = 10

    while(1) {
        buf[i++] = dec % 16;
        if(dec < 16) break;
        dec /= 16;
    }
    len = i;
    for(; i > 0; i--) {
        // 0x30 represents '0' and 0x60 does 'a'
        *(str++) = (buf[i-1] < 10) ? (buf[i-1] + 0x30) : (buf[i-1] - 9 + 0x60);
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
            switch(*fmt){
                case 'd':
                    len = decimal2ascii(str, va_arg (args, int));
                    break;
                case 'x':
                    len = hex2ascii(str, va_arg (args, int));
                    break;
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
