#include "ticket_qr.h"

#include <stdio.h>
#include <string.h>

enum { DATA_CODEWORDS = 19, ECC_CODEWORDS = 7, TOTAL_CODEWORDS = 26 };

int ticket_qr_payload(unsigned ticket_number, char *out, size_t out_size) {
    if (!out || out_size == 0) return 0;
    int written = snprintf(out, out_size, "BEST-TICKET-%u", ticket_number);
    return written >= 0 && (size_t)written < out_size ? written : 0;
}

static unsigned char gf_mul(unsigned char x, unsigned char y) {
    unsigned char z = 0;
    for (int i = 7; i >= 0; i--) {
        z = (unsigned char)((z << 1) ^ ((z >> 7) * 0x1D));
        z ^= (unsigned char)(((y >> i) & 1U) * x);
    }
    return z;
}

static void append_bits(unsigned char *bytes, int *bit_count, unsigned value, int count) {
    for (int i = count - 1; i >= 0; i--) {
        if ((value >> i) & 1U)
            bytes[*bit_count >> 3] |= (unsigned char)(1U << (7 - (*bit_count & 7)));
        (*bit_count)++;
    }
}

static int alpha_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    switch (c) {
        case ' ': return 36;
        case '$': return 37;
        case '%': return 38;
        case '*': return 39;
        case '+': return 40;
        case '-': return 41;
        case '.': return 42;
        case '/': return 43;
        case ':': return 44;
        default: return -1;
    }
}

static void make_data(unsigned ticket_number, unsigned char data[DATA_CODEWORDS]) {
    char payload[26];
    ticket_qr_payload(ticket_number, payload, sizeof payload);
    int length = (int)strlen(payload);
    int bits = 0;
    memset(data, 0, DATA_CODEWORDS);
    append_bits(data, &bits, 0x2, 4); /* QR alphanumeric mode */
    append_bits(data, &bits, (unsigned)length, 9);
    for (int i = 0; i + 1 < length; i += 2) {
        int a = alpha_value(payload[i]);
        int b = alpha_value(payload[i + 1]);
        append_bits(data, &bits, (unsigned)(a * 45 + b), 11);
    }
    if (length & 1) append_bits(data, &bits, (unsigned)alpha_value(payload[length - 1]), 6);
    int capacity = DATA_CODEWORDS * 8;
    int terminator = capacity - bits < 4 ? capacity - bits : 4;
    append_bits(data, &bits, 0, terminator);
    while (bits & 7) append_bits(data, &bits, 0, 1);
    for (unsigned char pad = 0xEC; bits < capacity; pad ^= 0xFD)
        append_bits(data, &bits, pad, 8);
}

static unsigned char *make_codewords(unsigned ticket_number, unsigned char out[TOTAL_CODEWORDS]) {
    static const unsigned char generator[ECC_CODEWORDS] = {87, 229, 146, 149, 238, 102, 21};
    unsigned char data[DATA_CODEWORDS];
    unsigned char ecc[ECC_CODEWORDS] = {0};
    make_data(ticket_number, data);
    for (int i = 0; i < DATA_CODEWORDS; i++) {
        unsigned char factor = data[i] ^ ecc[0];
        memmove(ecc, ecc + 1, ECC_CODEWORDS - 1);
        ecc[ECC_CODEWORDS - 1] = 0;
        for (int j = 0; j < ECC_CODEWORDS; j++) ecc[j] ^= gf_mul(generator[j], factor);
    }
    memcpy(out, data, DATA_CODEWORDS);
    memcpy(out + DATA_CODEWORDS, ecc, ECC_CODEWORDS);
    return out;
}

static void set_function(unsigned char modules[TICKET_QR_SIZE][TICKET_QR_SIZE],
                         unsigned char function[TICKET_QR_SIZE][TICKET_QR_SIZE], int x, int y, int dark) {
    if (x < 0 || x >= TICKET_QR_SIZE || y < 0 || y >= TICKET_QR_SIZE) return;
    modules[y][x] = (unsigned char)(dark != 0);
    function[y][x] = 1;
}

static void finder(unsigned char modules[TICKET_QR_SIZE][TICKET_QR_SIZE],
                   unsigned char function[TICKET_QR_SIZE][TICKET_QR_SIZE], int cx, int cy) {
    for (int dy = -1; dy <= 7; dy++) for (int dx = -1; dx <= 7; dx++) {
        int x = cx + dx, y = cy + dy;
        int inside = dx >= 0 && dx <= 6 && dy >= 0 && dy <= 6;
        int dark = inside && (dx == 0 || dx == 6 || dy == 0 || dy == 6 ||
                              (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4));
        set_function(modules, function, x, y, dark);
    }
}

static void format_bits(unsigned char modules[TICKET_QR_SIZE][TICKET_QR_SIZE],
                        unsigned char function[TICKET_QR_SIZE][TICKET_QR_SIZE]) {
    int value = 8; /* Error correction level L and mask pattern 0. */
    int rem = value;
    for (int i = 0; i < 10; i++) rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    unsigned bits = (unsigned)(((value << 10) | rem) ^ 0x5412);
    for (int i = 0; i <= 5; i++) set_function(modules, function, 8, i, (int)(bits >> i) & 1);
    set_function(modules, function, 8, 7, (int)(bits >> 6) & 1);
    set_function(modules, function, 8, 8, (int)(bits >> 7) & 1);
    set_function(modules, function, 7, 8, (int)(bits >> 8) & 1);
    for (int i = 9; i < 15; i++) set_function(modules, function, 14 - i, 8, (int)(bits >> i) & 1);
    for (int i = 0; i < 8; i++) set_function(modules, function, TICKET_QR_SIZE - 1 - i, 8, (int)(bits >> i) & 1);
    for (int i = 8; i < 15; i++) set_function(modules, function, 8, TICKET_QR_SIZE - 15 + i, (int)(bits >> i) & 1);
    set_function(modules, function, 8, TICKET_QR_SIZE - 8, 1);
}

void ticket_qr_encode(unsigned ticket_number, unsigned char modules[TICKET_QR_SIZE][TICKET_QR_SIZE]) {
    unsigned char function[TICKET_QR_SIZE][TICKET_QR_SIZE] = {{0}};
    unsigned char codewords[TOTAL_CODEWORDS];
    make_codewords(ticket_number, codewords);
    memset(modules, 0, TICKET_QR_SIZE * TICKET_QR_SIZE);
    finder(modules, function, 0, 0);
    finder(modules, function, TICKET_QR_SIZE - 7, 0);
    finder(modules, function, 0, TICKET_QR_SIZE - 7);
    for (int i = 8; i < TICKET_QR_SIZE - 8; i++) {
        set_function(modules, function, i, 6, (i & 1) == 0);
        set_function(modules, function, 6, i, (i & 1) == 0);
    }
    format_bits(modules, function);

    int bit = 0;
    for (int right = TICKET_QR_SIZE - 1; right >= 1; right -= 2) {
        if (right == 6) right = 5;
        for (int vert = 0; vert < TICKET_QR_SIZE; vert++) {
            int y = ((right + 1) & 2) == 0 ? TICKET_QR_SIZE - 1 - vert : vert;
            for (int j = 0; j < 2; j++) {
                int x = right - j;
                if (function[y][x]) continue;
                int dark = 0;
                if (bit < TOTAL_CODEWORDS * 8)
                    dark = (codewords[bit >> 3] >> (7 - (bit & 7))) & 1;
                bit++;
                modules[y][x] = (unsigned char)(dark ^ (((x + y) & 1) == 0));
            }
        }
    }
}
