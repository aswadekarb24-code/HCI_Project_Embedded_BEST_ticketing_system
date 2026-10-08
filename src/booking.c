#include "booking.h"

#include <time.h>

void booking_init(Booking *b) {
    *b = (Booking){.start_stop = -1, .end_stop = -1, .printer_paper = 3, .ticket_number = 1001};
}

unsigned booking_ticket_id(const Booking *b) {
    return b && b->purchased_at > 0 && b->ticket_number > 0 ? b->ticket_number - 1 : 0;
}

time_t booking_valid_until(const Booking *b) {
    return b && b->purchased_at > 0 ? b->purchased_at + BOOKING_VALIDITY_SECONDS : 0;
}

BookingError booking_validate(const Booking *b) {
    if (!b || !b->route) return BOOKING_NO_ROUTE;
    if (b->start_stop < 0 || b->end_stop < 0) return BOOKING_STOP_INCOMPLETE;
    if ((size_t)b->start_stop >= b->route->stop_count ||
        (size_t)b->end_stop >= b->route->stop_count) return BOOKING_STOP_OUT_OF_RANGE;
    if (b->start_stop == b->end_stop) return BOOKING_SAME_STOP;
    return BOOKING_OK;
}

int booking_fare(const Booking *b) {
    if (booking_validate(b) != BOOKING_OK) return 0;
    int stop_index_span = b->start_stop - b->end_stop;
    if (stop_index_span < 0) stop_index_span = -stop_index_span;
    /* Prototype fare: Rs. 10 base plus Rs. 5 per stop-index interval. */
    return 10 + stop_index_span * 5;
}

BookingError booking_complete(Booking *b) {
    BookingError error = booking_validate(b);
    if (error != BOOKING_OK) return error;
    if (b->printer_paper <= 0) return BOOKING_PRINTER_EMPTY;
    b->printer_paper--;
    b->ticket_number++;
    b->purchased_at = time(NULL);
    return BOOKING_OK;
}
