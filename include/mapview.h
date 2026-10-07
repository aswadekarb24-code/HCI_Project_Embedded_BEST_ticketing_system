#ifndef MAPVIEW_H
#define MAPVIEW_H

/* Pure camera/projection math for the satellite basemap. No SDL dependency,
 * so it is unit testable the same way catalog.c/booking.c are (see
 * ARCHITECTURE.md's input/drawing/business-rules separation rule). */

typedef struct {
    double lon_min, lon_max, lat_min, lat_max;
    int image_width, image_height;
} MapBounds;

/* Generated from config/map_bounds.json by tools/generate_catalog.py, must
 * match the bbox tools/fetch_basemap.py actually fetched into
 * assets/mumbai_satellite.png. */
extern const MapBounds MAP_BOUNDS;

typedef struct { int x, y, w, h; } MapRect;
typedef struct { int center_x, center_y, zoom_index; } Camera;

/* Linear EPSG:4326 projection of a geo coordinate into basemap pixel space.
 * Shared formula with tools/import_gtfs.py so stop dots and the basemap can
 * never drift apart. */
void mapview_geo_to_pixel(double lat, double lon, int *out_x, int *out_y);

int mapview_zoom_count(void);

void camera_init(Camera *camera);
void camera_zoom_in(Camera *camera);
void camera_zoom_out(Camera *camera);
/* Recenters the camera on an image-space point, clamped to basemap bounds. */
void camera_pan_to(Camera *camera, int image_x, int image_y);
/* Source rect (within the basemap image) the camera currently frames, sized
 * to match the destination viewport's aspect ratio and clamped so it never
 * extends past the basemap edges. */
MapRect camera_viewport(const Camera *camera, int viewport_w, int viewport_h);

/* Convert between on-screen map-box coordinates and basemap image pixel
 * coordinates, given the on-screen map box's top-left (viewport_x/y) and
 * size (viewport_w/h). Used for drawing stop dots and for tap hit-testing. */
void camera_image_to_screen(const Camera *camera, int viewport_x, int viewport_y,
                             int viewport_w, int viewport_h, int image_x, int image_y,
                             int *out_screen_x, int *out_screen_y);
void camera_screen_to_image(const Camera *camera, int viewport_x, int viewport_y,
                             int viewport_w, int viewport_h, int screen_x, int screen_y,
                             int *out_image_x, int *out_image_y);

#endif
