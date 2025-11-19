#pragma once

#include <stdint.h>
#include <stddef.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static inline void kmemset(void *dst, int value, size_t count) {
    uint8_t *d = (uint8_t *)dst;
    while (count--) {
        *d++ = (uint8_t)value;
    }
}

static inline void kmemcpy(void *dst, const void *src, size_t count) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (count--) {
        *d++ = *s++;
    }
}

static inline size_t kstrlen(const char *s) {
    size_t len = 0;
    if (!s) {
        return 0;
    }
    while (s[len]) {
        len++;
    }
    return len;
}

static inline int kstrcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (int)((unsigned char)*a) - (int)((unsigned char)*b);
}

static inline int kstrncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *b && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return (int)((unsigned char)*a) - (int)((unsigned char)*b);
}

static inline char *kstrcpy(char *dst, const char *src) {
    char *out = dst;
    while ((*dst++ = *src++)) {
        ;
    }
    return out;
}

static inline char *kstrncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; ++i) {
        dst[i] = src[i];
    }
    for (; i < n; ++i) {
        dst[i] = '\0';
    }
    return dst;
}

static inline void kstrcat(char *dst, const char *src, size_t max_len) {
    size_t len = kstrlen(dst);
    size_t idx = 0;
    while (src[idx] && (len + idx + 1) < max_len) {
        dst[len + idx] = src[idx];
        idx++;
    }
    dst[len + idx] = '\0';
}

static inline void kitoa(int value, char *buf, size_t buf_len) {
    if (buf_len == 0) {
        return;
    }
    char tmp[32];
    int neg = value < 0;
    size_t i = 0;
    if (value == 0) {
        tmp[i++] = '0';
    } else {
        uint32_t v = neg ? (uint32_t)(-value) : (uint32_t)value;
        while (v && i < ARRAY_SIZE(tmp)) {
            tmp[i++] = '0' + (v % 10);
            v /= 10;
        }
        if (neg) {
            tmp[i++] = '-';
        }
    }
    size_t out = 0;
    while (i && out + 1 < buf_len) {
        buf[out++] = tmp[--i];
    }
    buf[out] = '\0';
}

static inline char kupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

static inline int kisdigit(char c) {
    return c >= '0' && c <= '9';
}

