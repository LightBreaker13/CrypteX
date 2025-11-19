#include "input.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "dN"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "dN"(port));
}

static const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};

static int shift;

void input_init(void) {
    shift = 0;
    outb(0x64, 0xAE);
}

int kbd_read_char(void) {
    if (!(inb(0x64) & 1)) {
        return -1;
    }
    uint8_t sc = inb(0x60);
    if (sc == 0x2A || sc == 0x36) {
        shift = 1;
        return -1;
    }
    if (sc == 0xAA || sc == 0xB6) {
        shift = 0;
        return -1;
    }
    if (sc & 0x80) {
        return -1;
    }
    char ch = 0;
    if (sc < sizeof(keymap)) {
        ch = keymap[sc];
    }
    if (!ch) {
        return -1;
    }
    if (shift && ch >= 'a' && ch <= 'z') {
        ch -= 32;
    }
    return (int)ch;
}

int mouse_poll(mouse_state_t *state) {
    (void)state;
    return 0;
}

