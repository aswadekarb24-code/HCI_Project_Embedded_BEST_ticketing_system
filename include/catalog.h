#ifndef CATALOG_H
#define CATALOG_H

#include <stddef.h>
#include <stdint.h>

/* MAX_STOPS covers the real BEST GTFS feed's longest route (118 stops,
 * confirmed by tools/import_gtfs.py) with margin. MAX_ROUTES covers all 495
 * BEST routes. Keep in sync with tools/generate_catalog.py. */
#define MAX_STOPS 128
#define MAX_ROUTES 512
typedef struct { const char *name_en; const char *name_mr; const char *name_hi; const char *name_gu; int x; int y; } Stop;
typedef struct { const char *number; const char *name_en; const char *name_mr; const char *name_hi; const char *name_gu; uint8_t region; uint32_t color; size_t stop_count; Stop stops[MAX_STOPS]; } Route;

const Route *catalog_routes(size_t *count);
const Route *catalog_find_route(const char *number);
int catalog_stop_index(const Route *route, int x, int y, int radius);
#endif
