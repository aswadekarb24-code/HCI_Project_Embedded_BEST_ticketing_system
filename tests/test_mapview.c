#include "mapview.h"
#include <assert.h>
#include <stdio.h>

static int near(int a, int b, int tolerance) { int d = a - b; if (d < 0) d = -d; return d <= tolerance; }

static void test_projection(void) {
    int x, y;
    mapview_geo_to_pixel(MAP_BOUNDS.lat_max, MAP_BOUNDS.lon_min, &x, &y);
    assert(x == 0 && y == 0);
    mapview_geo_to_pixel(MAP_BOUNDS.lat_min, MAP_BOUNDS.lon_max, &x, &y);
    assert(x == MAP_BOUNDS.image_width && y == MAP_BOUNDS.image_height);
    double mid_lat = (MAP_BOUNDS.lat_min + MAP_BOUNDS.lat_max) / 2;
    double mid_lon = (MAP_BOUNDS.lon_min + MAP_BOUNDS.lon_max) / 2;
    mapview_geo_to_pixel(mid_lat, mid_lon, &x, &y);
    assert(near(x, MAP_BOUNDS.image_width / 2, 1));
    assert(near(y, MAP_BOUNDS.image_height / 2, 1));
    /* Coordinates outside the basemap bbox still clamp into image bounds
     * instead of producing negative/out-of-range pixels. */
    mapview_geo_to_pixel(MAP_BOUNDS.lat_max + 5, MAP_BOUNDS.lon_min - 5, &x, &y);
    assert(x >= 0 && x <= MAP_BOUNDS.image_width);
    assert(y >= 0 && y <= MAP_BOUNDS.image_height);
}

static void test_zoom_clamping(void) {
    Camera camera;
    camera_init(&camera);
    assert(camera.zoom_index == 0);
    camera_zoom_out(&camera);
    assert(camera.zoom_index == 0);
    int count = mapview_zoom_count();
    assert(count >= 2);
    for (int i = 0; i < count + 5; i++) camera_zoom_in(&camera);
    assert(camera.zoom_index == count - 1);
    for (int i = 0; i < count + 5; i++) camera_zoom_out(&camera);
    assert(camera.zoom_index == 0);
}

static void test_viewport_clamping(void) {
    Camera camera;
    camera_init(&camera);
    camera_zoom_in(&camera);
    camera_zoom_in(&camera);
    /* Pan past every corner; the viewport must always stay inside the
     * basemap image regardless of where the camera center lands. */
    int corners[4][2] = {
        {0, 0},
        {MAP_BOUNDS.image_width, 0},
        {0, MAP_BOUNDS.image_height},
        {MAP_BOUNDS.image_width, MAP_BOUNDS.image_height},
    };
    for (int i = 0; i < 4; i++) {
        camera_pan_to(&camera, corners[i][0], corners[i][1]);
        MapRect vp = camera_viewport(&camera, 754, 650);
        assert(vp.x >= 0 && vp.y >= 0);
        assert(vp.x + vp.w <= MAP_BOUNDS.image_width);
        assert(vp.y + vp.h <= MAP_BOUNDS.image_height);
        assert(vp.w > 0 && vp.h > 0);
    }
}

static void test_screen_image_roundtrip(void) {
    Camera camera;
    camera_init(&camera);
    camera_zoom_in(&camera);
    int vx = 270, vy = 70, vw = 754, vh = 650;
    int image_x = MAP_BOUNDS.image_width / 2, image_y = MAP_BOUNDS.image_height / 2;
    int sx, sy;
    camera_image_to_screen(&camera, vx, vy, vw, vh, image_x, image_y, &sx, &sy);
    assert(sx >= vx && sx <= vx + vw);
    assert(sy >= vy && sy <= vy + vh);
    int rx, ry;
    camera_screen_to_image(&camera, vx, vy, vw, vh, sx, sy, &rx, &ry);
    assert(near(rx, image_x, 2));
    assert(near(ry, image_y, 2));
}

int main(void) {
    test_projection();
    test_zoom_clamping();
    test_viewport_clamping();
    test_screen_image_roundtrip();
    puts("mapview tests passed");
}
