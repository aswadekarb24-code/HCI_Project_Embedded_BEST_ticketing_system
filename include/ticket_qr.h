#ifndef TICKET_QR_H
#define TICKET_QR_H

#include <stddef.h>

enum { TICKET_QR_SIZE = 21 };
int ticket_qr_payload(unsigned ticket_number, char *out, size_t out_size);
void ticket_qr_encode(unsigned ticket_number, unsigned char modules[TICKET_QR_SIZE][TICKET_QR_SIZE]);

#endif
