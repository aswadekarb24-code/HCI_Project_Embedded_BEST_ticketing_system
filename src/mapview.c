#include "mapview.h"
#include "map_bounds_generated.h"

/* Discrete zoom steps (source-rect shrink factor). Deliberately a short,
 * button-driven list rather than smooth/continuous zoom -- a kiosk has no
 * pinch gesture, only click-as-touch (see SETUP.md). */
static const double ZOOM_LEVELS[] = {1.0, 2.0, 4.0};
enum { ZOOM_COUNT = (int)(sizeof(ZOOM_LEVELS) / sizeof(ZOOM_LEVELS[0])) };

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void mapview_geo_to_pixel(double lat, double lon, int *out_x, int *out_y) {
    double fx = (lon - MAP_BOUNDS.lon_min) / (MAP_BOUNDS.lon_max - MAP_BOUNDS.lon_min);
    double fy = (MAP_BOUNDS.lat_max - lat) / (MAP_BOUNDS.lat_max - MAP_BOUNDS.lat_min);
    *out_x = clampi((int)(fx * MAP_BOUNDS.image_width), 0, MAP_BOUNDS.image_width);
    *out_y = clampi((int)(fy * MAP_BOUNDS.image_height), 0, MAP_BOUNDS.image_height);
}

int mapview_zoom_count(void) { return ZOOM_COUNT; }

void camera_init(Camera *camera) {
    camera->center_x = MAP_BOUNDS.image_width / 2;
    camera->center_y = MAP_BOUNDS.image_height / 2;
    camera->zoom_index = 0;
}

void camera_zoom_in(Camera *camera) {
    if (camera->zoom_index < ZOOM_COUNT - 1) camera->zoom_index++;
}

void camera_zoom_out(Camera *camera) {
    if (camera->zoom_index > 0) camera->zoom_index--;
}

void camera_pan_to(Camera *camera, int image_x, int image_y) {
    camera->center_x = clampi(image_x, 0, MAP_BOUNDS.image_width);
    camera->center_y = clampi(image_y, 0, MAP_BOUNDS.image_height);
}

MapRect camera_viewport(const Camera *camera, int viewport_w, int viewport_h) {
    double scale = ZOOM_LEVELS[clampi(camera->zoom_index, 0, ZOOM_COUNT - 1)];
    int src_w = (int)(MAP_BOUNDS.image_width / scale);
    int src_h = (int)(MAP_BOUNDS.image_height / scale);
    if (src_w < 1) src_w = 1;
    if (src_h < 1) src_h = 1;
    /* Match the destination viewport's aspect ratio so the image never looks
     * stretched, by shrinking whichever axis is oversized. */
    if ((long)src_w * viewport_h > (long)src_h * viewport_w)
        src_w = (int)((long)src_h * viewport_w / viewport_h);
    else
        src_h = (int)((long)src_w * viewport_h / viewport_w);
    if (src_w > MAP_BOUNDS.image_width) src_w = MAP_BOUNDS.image_width;
    if (src_h > MAP_BOUNDS.image_height) src_h = MAP_BOUNDS.image_height;
    int x = clampi(camera->center_x - src_w / 2, 0, MAP_BOUNDS.image_width - src_w);
    int y = clampi(camera->center_y - src_h / 2, 0, MAP_BOUNDS.image_height - src_h);
    return (MapRect){x, y, src_w, src_h};
}

void camera_image_to_screen(const Camera *camera, int viewport_x, int viewport_y,
                             int viewport_w, int viewport_h, int image_x, int image_y,
                             int *out_screen_x, int *out_screen_y) {
    MapRect vp = camera_viewport(camera, viewport_w, viewport_h);
    *out_screen_x = viewport_x + (int)((long)(image_x - vp.x) * viewport_w / vp.w);
    *out_screen_y = viewport_y + (int)((long)(image_y - vp.y) * viewport_h / vp.h);
}

void camera_screen_to_image(const Camera *camera, int viewport_x, int viewport_y,
                             int viewport_w, int viewport_h, int screen_x, int screen_y,
                             int *out_image_x, int *out_image_y) {
    MapRect vp = camera_viewport(camera, viewport_w, viewport_h);
    *out_image_x = vp.x + (int)((long)(screen_x - viewport_x) * vp.w / viewport_w);
    *out_image_y = vp.y + (int)((long)(screen_y - viewport_y) * vp.h / viewport_h);
}
