#include "ticket_pdf.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static int write_pdf(const char *path, const unsigned char *pixels, int width, int height) {
    uLong source_size = (uLong)width * (uLong)height * 3U;
    uLongf compressed_size = compressBound(source_size);
    unsigned char *compressed = malloc((size_t)compressed_size);
    if (!compressed) return 0;
    if (compress2(compressed, &compressed_size, pixels, source_size, Z_BEST_COMPRESSION) != Z_OK) {
        free(compressed);
        return 0;
    }

    FILE *file = fopen(path, "wb");
    if (!file) { free(compressed); return 0; }
    long offsets[6] = {0};
    fputs("%PDF-1.4\n%\xE2\xE3\xCF\xD3\n", file);
    offsets[1] = ftell(file);
    fputs("1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n", file);
    offsets[2] = ftell(file);
    fputs("2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n", file);
    offsets[3] = ftell(file);
    /* 5 x 3 inch landscape ticket page, rendered at 300 dpi from SDL. */
    fputs("3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 360 216] "
          "/Resources << /XObject << /Im0 4 0 R >> >> /Contents 5 0 R >>\nendobj\n", file);
    offsets[4] = ftell(file);
    fprintf(file, "4 0 obj\n<< /Type /XObject /Subtype /Image /Width %d /Height %d "
                  "/ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /FlateDecode /Length %lu >>\nstream\n",
            width, height, (unsigned long)compressed_size);
    fwrite(compressed, 1, (size_t)compressed_size, file);
    fputs("\nendstream\nendobj\n", file);
    offsets[5] = ftell(file);
    static const char content[] = "q 360 0 0 216 0 0 cm /Im0 Do Q\n";
    fprintf(file, "5 0 obj\n<< /Length %u >>\nstream\n", (unsigned)(sizeof content - 1));
    fwrite(content, 1, sizeof content - 1, file);
    fputs("endstream\nendobj\n", file);
    long xref = ftell(file);
    fputs("xref\n0 6\n0000000000 65535 f \n", file);
    for (int i = 1; i <= 5; i++) fprintf(file, "%010ld 00000 n \n", offsets[i]);
    fprintf(file, "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n", xref);
    int ok = !ferror(file);
    if (fclose(file) != 0) ok = 0;
    free(compressed);
    return ok;
}

int ticket_pdf_write(SDL_Surface *surface, unsigned ticket_number, char *path, size_t path_size) {
    if (!surface || !path || path_size == 0) return 0;
    char *directory = SDL_GetPrefPath("BEST", "TicketKiosk");
    if (!directory) return 0;
    int written = SDL_snprintf(path, path_size, "%sticket-%u.pdf", directory, ticket_number);
    SDL_free(directory);
    if (written < 0 || (size_t)written >= path_size) return 0;

    SDL_Surface *rgb = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB24, 0);
    if (!rgb) return 0;
    size_t pixel_size = (size_t)rgb->w * (size_t)rgb->h * 3U;
    unsigned char *pixels = malloc(pixel_size);
    if (!pixels) { SDL_FreeSurface(rgb); return 0; }
    if (SDL_LockSurface(rgb) != 0) { free(pixels); SDL_FreeSurface(rgb); return 0; }
    for (int y = 0; y < rgb->h; y++)
        memcpy(pixels + (size_t)y * (size_t)rgb->w * 3U,
               (const unsigned char *)rgb->pixels + (size_t)y * (size_t)rgb->pitch,
               (size_t)rgb->w * 3U);
    SDL_UnlockSurface(rgb);
    int result = write_pdf(path, pixels, rgb->w, rgb->h);
    free(pixels);
    SDL_FreeSurface(rgb);
    return result;
}
