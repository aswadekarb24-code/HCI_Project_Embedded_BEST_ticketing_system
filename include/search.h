#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>
#include <stddef.h>
#include "catalog.h"

/* ASCII case-insensitive substring match. Search queries are always typed on
 * the Latin-only on-screen keypad (route codes and the GTFS source stop
 * names are both plain ASCII), so no locale-aware/Unicode folding is
 * needed. */
bool ci_contains(const char *haystack, const char *needle);

/* Writes the indices (into `routes`) of every route whose number matches
 * `query` into `out` (capacity `cap`), returns how many were written. An
 * empty query matches everything. */
int filter_routes(const char *query, const Route *routes, size_t count, size_t *out, int cap);

/* Same idea over a single route's stops, matched against each stop's
 * English name regardless of the active UI language -- see ui_sdl.c's
 * filter_stops() call sites for why. */
int filter_stops(const char *query, const Route *route, size_t *out, int cap);

#endif
