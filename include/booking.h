#ifndef BOOKING_H
#define BOOKING_H

#include "catalog.h"

typedef enum { SCREEN_WELCOME, SCREEN_ROUTE, SCREEN_STOPS, SCREEN_PAYMENT, SCREEN_TICKET, SCREEN_HELP } Screen;
typedef enum { PAY_CASH, PAY_CARD, PAY_UPI } PaymentMethod;
typedef enum { BOOKING_OK, BOOKING_NO_ROUTE, BOOKING_STOP_INCOMPLETE, BOOKING_SAME_STOP, BOOKING_PRINTER_EMPTY } BookingError;
typedef struct { const Route *route; int start_stop; int end_stop; PaymentMethod payment; int printer_paper; unsigned ticket_number; } Booking;

void booking_init(Booking *booking);
int booking_fare(const Booking *booking);
BookingError booking_validate(const Booking *booking);
BookingError booking_complete(Booking *booking);
#endif
