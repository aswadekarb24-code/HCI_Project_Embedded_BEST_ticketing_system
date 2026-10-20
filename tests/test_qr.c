#include "ticket_qr.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void assert_finder(unsigned char modules[TICKET_QR_SIZE][TICKET_QR_SIZE], int left, int top) {
    for (int y = 0; y < 7; y++) for (int x = 0; x < 7; x++) {
        int expected = x == 0 || x == 6 || y == 0 || y == 6 ||
                       (x >= 2 && x <= 4 && y >= 2 && y <= 4);
        assert(modules[top + y][left + x] == expected);
    }
}

int main(void) {
    unsigned char first[TICKET_QR_SIZE][TICKET_QR_SIZE];
    unsigned char repeat[TICKET_QR_SIZE][TICKET_QR_SIZE];
    unsigned char next[TICKET_QR_SIZE][TICKET_QR_SIZE];
    char payload[32];

    assert(ticket_qr_payload(1001, payload, sizeof payload) > 0);
    assert(strcmp(payload, "BEST-TICKET-1001") == 0);
    assert(ticket_qr_payload(1001, payload, 4) == 0);
    assert(ticket_qr_payload(1001, NULL, sizeof payload) == 0);

    ticket_qr_encode(1001, first);
    ticket_qr_encode(1001, repeat);
    ticket_qr_encode(1002, next);
    assert(memcmp(first, repeat, sizeof first) == 0);
    assert(memcmp(first, next, sizeof first) != 0);
    assert_finder(first, 0, 0);
    assert_finder(first, TICKET_QR_SIZE - 7, 0);
    assert_finder(first, 0, TICKET_QR_SIZE - 7);
    assert(first[0][7] == 0 && first[7][0] == 0); /* finder separators */
    puts("qr tests passed");
    return 0;
}
