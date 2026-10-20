#include "search.h"
#include <string.h>

bool ci_contains(const char *haystack, const char *needle) {
    if (!*needle) return true;
    size_t hn = strlen(haystack), nn = strlen(needle);
    if (nn > hn) return false;
    for (size_t i = 0; i + nn <= hn; i++) {
        size_t j = 0;
        for (; j < nn; j++) {
            char x = haystack[i + j], y = needle[j];
            if (x >= 'A' && x <= 'Z') x += 32;
            if (y >= 'A' && y <= 'Z') y += 32;
            if (x != y) break;
        }
        if (j == nn) return true;
    }
    return false;
}

int filter_routes(const char *query, const Route *routes, size_t count, size_t *out, int cap) {
    int n = 0;
    for (size_t i = 0; i < count && n < cap; i++)
        if (ci_contains(routes[i].number, query)) out[n++] = i;
    return n;
}

int filter_stops(const char *query, const Route *route, size_t *out, int cap) {
    int n = 0;
    for (size_t i = 0; i < route->stop_count && n < cap; i++)
        if (ci_contains(route->stops[i].name_en, query)) out[n++] = i;
    return n;
}
