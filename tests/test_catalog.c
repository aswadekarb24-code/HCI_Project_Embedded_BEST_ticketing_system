#include "catalog.h"
#include "mapview.h"
#include <assert.h>
#include <stdio.h>
/* Regression guard on tools/generate_catalog.py: the generated table must
 * stay within the fixed-size arrays declared in catalog.h, and every route
 * needs real, non-empty names in all four languages. */
int main(void) {
    size_t n;
    const Route *routes = catalog_routes(&n);
    assert(n > 0);
    assert(n <= MAX_ROUTES);
    for (size_t i = 0; i < n; i++) {
        const Route *r = &routes[i];
        assert(r->stop_count >= 2);
        assert(r->stop_count <= MAX_STOPS);
        assert(r->number && r->number[0]);
        assert(r->name_en && r->name_en[0]);
        assert(r->name_mr && r->name_mr[0]);
        assert(r->name_hi && r->name_hi[0]);
        assert(r->name_gu && r->name_gu[0]);
        for (size_t j = 0; j < r->stop_count; j++) {
            const Stop *s = &r->stops[j];
            assert(s->name_en && s->name_en[0]);
            assert(s->name_mr && s->name_mr[0]);
            assert(s->name_hi && s->name_hi[0]);
            assert(s->name_gu && s->name_gu[0]);
            assert(s->x >= 0 && s->x <= MAP_BOUNDS.image_width);
            assert(s->y >= 0 && s->y <= MAP_BOUNDS.image_height);
        }
    }
    assert(catalog_find_route("__not_a_real_route__") == NULL);
    puts("catalog tests passed");
}
