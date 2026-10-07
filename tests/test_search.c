#include "search.h"
#include "catalog.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

static void test_ci_contains(void) {
    assert(ci_contains("100RING", "ring"));
    assert(ci_contains("100RING", "RING"));
    assert(ci_contains("100RING", "100"));
    assert(ci_contains("Churchgate Station", "station"));
    assert(!ci_contains("100RING", "xyz"));
    assert(ci_contains("anything", ""));
    assert(!ci_contains("", "a"));
}

static void test_filter_routes(void) {
    size_t n;
    const Route *routes = catalog_routes(&n);
    size_t idx[MAX_ROUTES];

    /* Empty query matches every route. */
    int all = filter_routes("", routes, n, idx, MAX_ROUTES);
    assert((size_t)all == n);

    /* A query matching nothing returns zero results, not a crash. */
    int none = filter_routes("__no_such_route__", routes, n, idx, MAX_ROUTES);
    assert(none == 0);

    /* Every returned index actually matches, and the result is a genuine
     * subset (regression guard: the filter must not invent or drop routes
     * beyond what the query decides). */
    int some = filter_routes(routes[0].number, routes, n, idx, MAX_ROUTES);
    assert(some >= 1);
    bool found_self = false;
    for (int i = 0; i < some; i++) {
        assert(ci_contains(routes[idx[i]].number, routes[0].number));
        if (idx[i] == 0) found_self = true;
    }
    assert(found_self);
}

static void test_filter_stops(void) {
    size_t n;
    const Route *routes = catalog_routes(&n);
    const Route *route = &routes[0];
    size_t idx[MAX_STOPS];

    int all = filter_stops("", route, idx, MAX_STOPS);
    assert((size_t)all == route->stop_count);

    int none = filter_stops("__no_such_stop__", route, idx, MAX_STOPS);
    assert(none == 0);

    int some = filter_stops(route->stops[0].name_en, route, idx, MAX_STOPS);
    assert(some >= 1);
    for (int i = 0; i < some; i++)
        assert(ci_contains(route->stops[idx[i]].name_en, route->stops[0].name_en));
}

int main(void) {
    test_ci_contains();
    test_filter_routes();
    test_filter_stops();
    puts("search tests passed");
}
