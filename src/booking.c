#include "booking.h"
void booking_init(Booking *b) { *b=(Booking){.start_stop=-1,.end_stop=-1,.printer_paper=3,.ticket_number=1001}; }
int booking_fare(const Booking *b) { if (!b->route || b->start_stop < 0 || b->end_stop < 0) return 0; int hops=b->start_stop-b->end_stop; if(hops<0)hops=-hops; return 5+hops*5; }
BookingError booking_validate(const Booking *b) { if (!b->route) return BOOKING_NO_ROUTE; if(b->start_stop<0 || b->end_stop<0) return BOOKING_STOP_INCOMPLETE; if(b->start_stop==b->end_stop) return BOOKING_SAME_STOP; return BOOKING_OK; }
BookingError booking_complete(Booking *b) { BookingError e=booking_validate(b); if(e!=BOOKING_OK)return e; if(b->printer_paper<=0)return BOOKING_PRINTER_EMPTY; b->printer_paper--; b->ticket_number++; return BOOKING_OK; }
