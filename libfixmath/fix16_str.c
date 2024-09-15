#include "fix16.h"
#include <stdbool.h>
#ifndef FIXMATH_NO_CTYPE
#include <ctype.h>
#else
static inline int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

static inline int isspace(int c)
{
    return c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\v' || c == '\f';
}
#endif

static const uint32_t scales[8] = {
    /* 5 decimals is enough for full fix16_t precision */
    1, 10, 100, 1000, 10000, 100000, 100000, 100000
};

static char *itoa_loop(char *buf, uint32_t scale, uint32_t value, bool skip)
{
    while (scale)
    {
        unsigned digit = (value / scale);
    
        if (!skip || digit || scale == 1)
        {
            skip = false;
            *buf++ = '0' + digit;
            value %= scale;
        }
        
        scale /= 10;
    }
    return buf;
}

int fix16_to_str(fix16_t value, char *buf, int decimals)
{
    char *const beg = buf;

    uint32_t uvalue = (value >= 0) ? value : -value;
    if (value < 0)
        *buf++ = '-';

    /* Separate the integer and decimal parts of the value */
    unsigned intpart = uvalue >> 16;
    uint32_t fracpart = uvalue & 0xFFFF;
    uint32_t scale = scales[decimals & 7];
    fracpart = fix16_mul(fracpart, scale);
    
    if (fracpart >= scale)
    {
        /* Handle carry from decimal part */
        intpart++;
        fracpart -= scale;    
    }
    
    /* Format integer part */
    buf = itoa_loop(buf, 10000, intpart, true);
    
    /* Format decimal part (if any) */
    if (scale != 1)
    {
        *buf++ = '.';
        buf = itoa_loop(buf, scale / 10, fracpart, false);
    }
    
    *buf = '\0';

    /* yocto-8 HACK: walk back n 0 digits */
    if (scale != 1)
    {
        while (*(buf - 1) == '0')
        {
            --buf;
            *buf = '\0';
        }

        if (*(buf - 1) == '.')
        {
            --buf;
            *buf = '\0';
        }
    }

    return buf - beg;
}

fix16_t strtofix16(const char *buf, char **end)
{
    while (isspace(*buf))
        buf++;
    
    /* Decode the sign */
    bool negative = (*buf == '-');
    if (*buf == '+' || *buf == '-')
        buf++;

    /* Decode the integer part */
    uint32_t intpart = 0;
    int count = 0;
    while (isdigit(*buf))
    {
        intpart *= 10;
        intpart += *buf++ - '0';
        count++;
    }

    if (end != nullptr) {
        *end = const_cast<char*>(buf);
    }
    
    // if (count == 0 || count > 5
    //     || intpart > 32768 || (!negative && intpart > 32767))
    //     return fix16_overflow;
    
    fix16_t value = intpart << 16;
    
    /* Decode the decimal part */
    if (*buf == '.' || *buf == ',')
    {
        buf++;
        
        uint32_t fracpart = 0;
        uint32_t scale = 1;
        while (isdigit(*buf) && scale < 100000) {
            scale *= 10;
            fracpart *= 10;
            fracpart += *buf++ - '0';
        }
        
        value += fix16_div(fracpart, scale);
    }
    
    while (*buf != '\0' && isdigit(*buf)) {
        buf++;
    }

    if (end != nullptr) {
        *end = const_cast<char*>(buf);
    }
    
    return negative ? -value : value;
}

fix16_t fix16_to_str(const char* buf)
{
    return strtofix16(buf, nullptr);
}
