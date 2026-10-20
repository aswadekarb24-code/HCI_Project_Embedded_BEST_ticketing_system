#include "timetable.h"
#include "catalog.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
/* Regression guard on tools/import_timetables.py / generate_catalog.py: the
 * generated TIMETABLES[] table must stay in lockstep (same length, same
 * order) with ROUTES[], and every departure must be a well-formed,
 * non-decreasing "HH:MM". */
int main(void) {
    size_t n;
    const Route *routes = catalog_routes(&n);
    assert(n > 0);
    for (size_t i = 0; i < n; i++) {
        const RouteTimetable *t = timetable_for_route(&routes[i]);
        assert(t != NULL);
        assert(t->departure_count <= MAX_DEPARTURES);
        for (size_t j = 0; j < t->departure_count; j++) {
            const char *hhmm = t->departures[j];
            assert(hhmm && strlen(hhmm) == 5 && hhmm[2] == ':');
            if (j > 0) assert(strcmp(t->departures[j-1], hhmm) <= 0);
        }
        assert(catalog_route_index(&routes[i]) == (int)i);
    }
    assert(timetable_for_route(NULL) == NULL);
    Route not_in_catalog = {0};
    assert(catalog_route_index(&not_in_catalog) == -1);
    assert(timetable_for_route(&not_in_catalog) == NULL);
    puts("timetable tests passed");
}
