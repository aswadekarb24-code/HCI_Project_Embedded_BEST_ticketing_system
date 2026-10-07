#include "catalog.h"
#include <string.h>

#include "routes_generated.h"
const Route *catalog_routes(size_t *count) { *count = sizeof ROUTES / sizeof *ROUTES; return ROUTES; }
const Route *catalog_find_route(const char *number) { size_t n; const Route *r = catalog_routes(&n); for (size_t i=0;i<n;i++) if (!strcmp(r[i].number, number)) return &r[i]; return NULL; }
int catalog_stop_index(const Route *route, int x, int y, int radius) { if (!route) return -1; for (size_t i=0;i<route->stop_count;i++) { int dx=route->stops[i].x-x, dy=route->stops[i].y-y; if (dx*dx+dy*dy <= radius*radius) return (int)i; } return -1; }
