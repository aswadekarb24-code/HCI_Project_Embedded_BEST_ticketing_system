#include "booking.h"
#include "ticket_qr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    Route route = {.number = "TEST", .stop_count = 4};
    Booking booking;
    booking_init(&booking);
    booking.route = &route;

    /* A two-index-interval ride follows the prototype fare rule: Rs. 10 base
     * plus Rs. 5 per interval, for Rs. 20. */
    booking.start_stop = 0;
    booking.end_stop = 2;
    assert(booking_fare(&booking) == 20);
    booking.start_stop = 2;
    booking.end_stop = 0;
    assert(booking_fare(&booking) == 20);
    booking.end_stop = 4;
    assert(booking_validate(&booking) == BOOKING_STOP_OUT_OF_RANGE);
    assert(booking_fare(&booking) == 0);

    booking.start_stop = 0;
    booking.end_stop = 2;
    assert(booking_ticket_id(&booking) == 0);
    assert(booking_valid_until(&booking) == 0);
    assert(booking_complete(&booking) == BOOKING_OK);
    assert(booking.printer_paper == 2);
    assert(booking_ticket_id(&booking) == 1001);
    assert(booking.purchased_at > 0);
    assert(booking_valid_until(&booking) == booking.purchased_at + BOOKING_VALIDITY_SECONDS);

    char payload[32];
    assert(ticket_qr_payload(booking_ticket_id(&booking), payload, sizeof payload) > 0);
    assert(strcmp(payload, "BEST-TICKET-1001") == 0);

    /* Regression for the reported ₹185 fare: these are adjacent stops on
     * route 45AS, so their prototype fare is ₹15 (₹10 base + one ₹5 span). */
    const Route *route_45as = catalog_find_route("45AS");
    assert(route_45as != NULL);
    int tadwadi = -1;
    int maharana_pratap = -1;
    for (size_t i = 0; i < route_45as->stop_count; ++i) {
        if (strcmp(route_45as->stops[i].name_en, "Tadwadi (Mazgaon)") == 0)
            tadwadi = (int)i;
        if (strcmp(route_45as->stops[i].name_en, "Maharana Pratap Chowk (Mazgaon)") == 0)
            maharana_pratap = (int)i;
    }
    assert(tadwadi >= 0 && maharana_pratap >= 0);
    assert(abs(tadwadi - maharana_pratap) == 1);
    booking.route = route_45as;
    booking.start_stop = tadwadi;
    booking.end_stop = maharana_pratap;
    assert(booking_fare(&booking) == 15);
    puts("ticketing tests passed");
    return 0;
}
