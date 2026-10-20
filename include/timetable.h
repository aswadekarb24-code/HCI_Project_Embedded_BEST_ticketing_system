#ifndef TIMETABLE_H
#define TIMETABLE_H
#include "catalog.h"

/* Real BEST routes run up to ~900 trips/day; tools/import_timetables.py
 * evenly subsamples each route's departures down to this cap (always
 * keeping the first and last of the day) before baking them into
 * generated/timetables_generated.h, so this is an approximate, not
 * exhaustive, schedule. Keep in sync with tools/import_timetables.py. */
#define MAX_DEPARTURES 48

typedef struct { size_t departure_count; const char *departures[MAX_DEPARTURES]; } RouteTimetable;

/* Looks up the timetable generated in lockstep with catalog_routes() (same
 * index, see catalog_route_index()) -- returns NULL if route is NULL or not
 * a pointer into the catalog. departure_count may legitimately be 0 if the
 * source GTFS feed had no usable trips for that specific route_id. */
const RouteTimetable *timetable_for_route(const Route *route);
#endif
