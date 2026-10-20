#ifndef TICKET_PDF_H
#define TICKET_PDF_H

#include <stddef.h>
#include <SDL.h>

int ticket_pdf_write(SDL_Surface *surface, unsigned ticket_number, char *path, size_t path_size);

#endif
