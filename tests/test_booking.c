#include "booking.h"
#include "catalog.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    size_t n;
    const Route *routes = catalog_routes(&n);
    assert(n > 0);
    assert(catalog_find_route(routes[0].number) == &routes[0]);
    assert(routes[0].stop_count >= 2);

    Booking b;
    booking_init(&b);
    assert(booking_validate(&b) == BOOKING_NO_ROUTE);
    b.route = &routes[0];
    assert(booking_validate(&b) == BOOKING_STOP_INCOMPLETE);
    b.start_stop = 0;
    b.end_stop = 0;
    assert(booking_validate(&b) == BOOKING_SAME_STOP);
    b.end_stop = (int)routes[0].stop_count - 1;
    int expected_fare = 5 + b.end_stop * 5;
    assert(booking_fare(&b) == expected_fare);
    assert(booking_complete(&b) == BOOKING_OK);
    assert(b.printer_paper == 2);
    b.printer_paper = 0;
    assert(booking_complete(&b) == BOOKING_PRINTER_EMPTY);
    puts("booking tests passed");
}
