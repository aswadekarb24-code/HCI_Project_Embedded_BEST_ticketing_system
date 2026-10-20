#include "timetable.h"
#include "timetables_generated.h"

const RouteTimetable *timetable_for_route(const Route *route) {
    int i = catalog_route_index(route);
    if (i < 0 || (size_t)i >= sizeof TIMETABLES / sizeof *TIMETABLES) return NULL;
    return &TIMETABLES[i];
}
