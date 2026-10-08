#include <SDL.h>
#include <SDL_ttf.h>
#include <png.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#endif
#include "booking.h"
#include "i18n.h"
#include "catalog.h"
#include "mapview.h"
#include "search.h"
#include "ticket_pdf.h"
#include "ticket_qr.h"

enum { WIDTH=1024, HEIGHT=720, SIDEBAR=270 };
enum { ROUTE_ROWS=8, STOP_ROWS=8 };
enum { QUERY_CAP=24 };
typedef struct { SDL_Window *window; SDL_Renderer *renderer; SDL_Texture *map_texture; TTF_Font *fonts[LANG_COUNT]; Booking booking; Screen screen; Language language; Camera camera; int route_scroll; int stop_scroll; const char *notice; bool searching; char route_query[QUERY_CAP]; char stop_query[QUERY_CAP]; } App;
typedef struct { int x,y,w,h; } Box;

static bool inside(Box b,int x,int y){return x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h;}
static void color(SDL_Renderer*r,uint32_t hex){SDL_SetRenderDrawColor(r,(hex>>16)&255,(hex>>8)&255,hex&255,255);}
static void fill(App*a,Box b,uint32_t c){color(a->renderer,c);SDL_Rect r={b.x,b.y,b.w,b.h};SDL_RenderFillRect(a->renderer,&r);}
/* SDL2 has no filled-circle primitive; scanline-fill via horizontal chords. */
static void fill_circle(App*a,int cx,int cy,int radius,uint32_t c){color(a->renderer,c);for(int dy=-radius;dy<=radius;dy++){int dx=(int)lround(sqrt((double)radius*radius-(double)dy*dy));SDL_RenderDrawLine(a->renderer,cx-dx,cy+dy,cx+dx,cy+dy);}}
/* A 1px SDL_RenderDrawLine reads as too thin to clearly "highlight" the
 * selected route against a busy satellite image, so the connecting line is
 * drawn a few times with small offsets to approximate a thicker stroke. */
static void thick_line(App*a,int x1,int y1,int x2,int y2){SDL_RenderDrawLine(a->renderer,x1,y1,x2,y2);SDL_RenderDrawLine(a->renderer,x1+1,y1,x2+1,y2);SDL_RenderDrawLine(a->renderer,x1,y1+1,x2,y2+1);SDL_RenderDrawLine(a->renderer,x1-1,y1,x2-1,y2);SDL_RenderDrawLine(a->renderer,x1,y1-1,x2,y2-1);}
/* *_f variants take an explicit font; the plain names default to the
 * current UI language's font. Needed because the language-switch button's
 * label is written in the *next* language's script, which the current
 * language's font may not have glyphs for (e.g. English's Latin-only font
 * can't render "मराठी"). */
static void text_f(App*a,TTF_Font*font,const char*s,int x,int y,uint32_t c){SDL_Color co={(Uint8)(c>>16),(Uint8)(c>>8),(Uint8)c,255};SDL_Surface*sf=TTF_RenderUTF8_Blended(font,s,co);if(!sf)return;SDL_Texture*t=SDL_CreateTextureFromSurface(a->renderer,sf);SDL_Rect d={x,y,sf->w,sf->h};SDL_RenderCopy(a->renderer,t,NULL,&d);SDL_DestroyTexture(t);SDL_FreeSurface(sf);}
static void text(App*a,const char*s,int x,int y,uint32_t c){text_f(a,a->fonts[a->language],s,x,y,c);}
static void centered_f(App*a,TTF_Font*font,const char*s,Box b,uint32_t c){int w;TTF_SizeUTF8(font,s,&w,NULL);text_f(a,font,s,b.x+(b.w-w)/2,b.y+(b.h-22)/2,c);}
static void centered(App*a,const char*s,Box b,uint32_t c){centered_f(a,a->fonts[a->language],s,b,c);}

/* Real GTFS names are much longer than the old 4-route mock data, so sidebar
 * list labels need truncation to avoid overflowing their 240px buttons.
 * Trims whole UTF-8 codepoints (never splits a multi-byte Devanagari/
 * Gujarati character). Deliberately no "..." suffix: the restricted
 * per-script Noto fonts (Devanagari/Gujarati) don't reliably include ASCII
 * punctuation glyphs -- an appended "..." showed as tofu boxes in testing. */
static size_t utf8_prev_len(const char *s, size_t len) {
    if (len == 0) return 0;
    size_t i = len - 1;
    while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80) i--;
    return i;
}
static void fit_text_f(TTF_Font *font, const char *s, int max_w, char *out, size_t out_size) {
    size_t len = strnlen(s, out_size - 1);
    memcpy(out, s, len);
    out[len] = 0;
    int w;
    TTF_SizeUTF8(font, out, &w, NULL);
    while (w > max_w && len > 0) {
        len = utf8_prev_len(out, len);
        out[len] = 0;
        TTF_SizeUTF8(font, out, &w, NULL);
    }
}
static void fit_text(App *a, const char *s, int max_w, char *out, size_t out_size) { fit_text_f(a->fonts[a->language], s, max_w, out, out_size); }
static void fit_button_f(App*a,TTF_Font*font,Box b,const char*s,uint32_t c,int pad){fill(a,b,c);char buf[192];fit_text_f(font,s,b.w-pad,buf,sizeof buf);centered_f(a,font,buf,b,0xFFFFFF);}
static void fit_button(App*a,Box b,const char*s,uint32_t c,int pad){fit_button_f(a,a->fonts[a->language],b,s,c,pad);}

static void button_f(App*a,TTF_Font*font,Box b,const char*s,uint32_t c){fill(a,b,c);centered_f(a,font,s,b,0xFFFFFF);}
static void button(App*a,Box b,const char*s,uint32_t c){button_f(a,a->fonts[a->language],b,s,c);}
/* Route/stop codes, numerals, currency amounts and citation text are never
 * translated or transliterated -- they're drawn with the English/Latin font
 * regardless of the active UI language, since Noto Sans Devanagari/Gujarati
 * only cover their own script (confirmed by probing: route codes like
 * "100RING" and the basemap attribution line rendered as tofu boxes in
 * Marathi/Hindi/Gujarati mode before this fix). */
static TTF_Font *latin_font(App*a){return a->fonts[LANG_EN];}
/* ci_contains()/filter_routes()/filter_stops() live in search.c/search.h --
 * SDL-free so they're unit tested the same way catalog.c/booking.c are
 * (see tests/test_search.c). Stop-name search always matches against the
 * English name, regardless of the active UI language: the on-screen keypad
 * only has Latin keys, and the Devanagari/Gujarati names are
 * transliterations of it anyway (see tools/import_gtfs.py), so searching
 * the English source is the one search that behaves consistently across
 * all four languages. */
static const char *stop_name(App*a,const Stop*s){switch(a->language){case LANG_MR:return s->name_mr;case LANG_HI:return s->name_hi;case LANG_GU:return s->name_gu;default:return s->name_en;}}
static const char *route_name(App*a,const Route*r){switch(a->language){case LANG_MR:return r->name_mr;case LANG_HI:return r->name_hi;case LANG_GU:return r->name_gu;default:return r->name_en;}}
static const char *booking_stop_name(App*a,int i){return stop_name(a,&a->booking.route->stops[i]);}
static void header(App*a,const char*title){fill(a,(Box){0,0,WIDTH,70},0x073B4C);text(a,title,22,23,0xFFFFFF);Language next=(Language)((a->language+1)%LANG_COUNT);button_f(a,a->fonts[next],(Box){845,15,155,40},language_name(next),0x118AB2);}

static Box map_box(void){return (Box){SIDEBAR,70,WIDTH-SIDEBAR,HEIGHT-70};}
static Box zoom_in_box(void){return (Box){WIDTH-65,80,50,40};}
static Box zoom_out_box(void){return (Box){WIDTH-65,128,50,40};}

static int stop_screen_hit(App*a,const Route*r,int sx,int sy,int radius){
    if(!r)return -1;
    Box b=map_box();
    for(size_t i=0;i<r->stop_count;i++){
        int px,py;
        camera_image_to_screen(&a->camera,b.x,b.y,b.w,b.h,r->stops[i].x,r->stops[i].y,&px,&py);
        int dx=px-sx,dy=py-sy;
        if(dx*dx+dy*dy<=radius*radius)return (int)i;
    }
    return -1;
}

static SDL_Texture *load_map_texture(SDL_Renderer *renderer){png_image image={.version=PNG_IMAGE_VERSION};if(!png_image_begin_read_from_file(&image,"assets/mumbai_satellite.png"))return NULL;image.format=PNG_FORMAT_RGBA;png_bytep pixels=malloc(PNG_IMAGE_SIZE(image));if(!pixels||!png_image_finish_read(&image,NULL,pixels,0,NULL)){free(pixels);png_image_free(&image);return NULL;}SDL_Surface*surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels,(int)image.width,(int)image.height,32,(int)image.width*4,SDL_PIXELFORMAT_RGBA32);SDL_Texture*texture=surface?SDL_CreateTextureFromSurface(renderer,surface):NULL;if(surface)SDL_FreeSurface(surface);free(pixels);png_image_free(&image);return texture;}

static void draw_map(App*a){
    Box b=map_box();
    fill(a,b,0xE9F4F6);
    if(a->map_texture){
        MapRect vp=camera_viewport(&a->camera,b.w,b.h);
        SDL_Rect source={vp.x,vp.y,vp.w,vp.h};
        SDL_Rect destination={b.x,b.y,b.w,b.h};
        SDL_RenderCopy(a->renderer,a->map_texture,&source,&destination);
    }
    /* Every real BEST stop, as small dots -- always visible so the map
     * reads as "the real network", but never as 473 overlapping colored
     * route lines at once (see ARCHITECTURE.md / the plan's clean-look
     * requirement). */
    size_t n;const Route*r=catalog_routes(&n);
    for(size_t k=0;k<n;k++)for(size_t i=0;i<r[k].stop_count;i++){
        int px,py;camera_image_to_screen(&a->camera,b.x,b.y,b.w,b.h,r[k].stops[i].x,r[k].stops[i].y,&px,&py);
        if(px<b.x||px>=b.x+b.w||py<b.y||py>=b.y+b.h)continue;
        fill_circle(a,px,py,2,0xF4F9FA);
    }
    /* The selected route (SCREEN_STOPS), highlighted on top: stops as
     * circles connected by a thickened line in the route's own color, with
     * the chosen start/end picked out in green/orange. */
    const Route*selected=a->booking.route;
    if(selected){
        color(a->renderer,selected->color);
        for(size_t i=1;i<selected->stop_count;i++){
            int x1,y1,x2,y2;
            camera_image_to_screen(&a->camera,b.x,b.y,b.w,b.h,selected->stops[i-1].x,selected->stops[i-1].y,&x1,&y1);
            camera_image_to_screen(&a->camera,b.x,b.y,b.w,b.h,selected->stops[i].x,selected->stops[i].y,&x2,&y2);
            thick_line(a,x1,y1,x2,y2);
        }
        for(size_t i=0;i<selected->stop_count;i++){
            int px,py;camera_image_to_screen(&a->camera,b.x,b.y,b.w,b.h,selected->stops[i].x,selected->stops[i].y,&px,&py);
            uint32_t c=(int)i==a->booking.start_stop?0x27AE60:(int)i==a->booking.end_stop?0xE67E22:selected->color;
            fill_circle(a,px,py,7,c);
            fill_circle(a,px,py,2,0xFFFFFF);
        }
    }
    button_f(a,latin_font(a),zoom_in_box(),"+",0x118AB2);
    button_f(a,latin_font(a),zoom_out_box(),"-",0x118AB2);
    text_f(a,latin_font(a),"Sentinel-2 cloudless / EOX, CC BY-NC-SA 4.0",b.x+12,b.y+b.h-28,0xFFFFFF);
}

static Box route_search_box(void){return (Box){15,118,240,26};}
static Box route_scroll_up_box(void){return (Box){15,150,115,26};}
static Box route_scroll_down_box(void){return (Box){140,150,115,26};}
static Box route_row_box(int row){return (Box){15,182+row*44,240,40};}
static Box route_help_box(void){return (Box){15,570,240,50};}

static void draw_sidebar(App*a){
    fill(a,(Box){0,70,SIDEBAR,HEIGHT-70},0xFFFFFF);
    text(a,tr(a->language,T_SELECT_ROUTE),18,93,0x073B4C);
    size_t n;const Route*r=catalog_routes(&n);
    size_t idx[MAX_ROUTES];
    int matches=filter_routes(a->route_query,r,n,idx,MAX_ROUTES);
    int max_scroll=matches>ROUTE_ROWS?matches-ROUTE_ROWS:0;
    if(a->route_scroll>max_scroll)a->route_scroll=max_scroll;
    fit_button_f(a,latin_font(a),route_search_box(),a->route_query[0]?a->route_query:"Search",0x073B4C,10);
    button_f(a,latin_font(a),route_scroll_up_box(),"^",a->route_scroll>0?0x118AB2:0xB0BEC5);
    button_f(a,latin_font(a),route_scroll_down_box(),"v",a->route_scroll<max_scroll?0x118AB2:0xB0BEC5);
    for(int row=0;row<ROUTE_ROWS;row++){
        int i=a->route_scroll+row;
        if(i>=matches)break;
        fit_button_f(a,latin_font(a),route_row_box(row),r[idx[i]].number,r[idx[i]].color,10);
    }
    button(a,route_help_box(),tr(a->language,T_HELP),0x118AB2);
    text(a,tr(a->language,T_TOUCH_HINT),20,640,0x555555);
}
static void draw_route(App*a){header(a,tr(a->language,T_SELECT_ROUTE));draw_map(a);draw_sidebar(a);text(a,tr(a->language,T_TAP_ROUTE),SIDEBAR+25,115,0x073B4C);}

static Box stop_search_box(void){return (Box){14,164,242,26};}
static Box stop_scroll_up_box(void){return (Box){14,196,118,26};}
static Box stop_scroll_down_box(void){return (Box){138,196,118,26};}
static Box stop_row_box(int row){return (Box){14,228+row*38,242,34};}
static Box stop_back_box(void){return (Box){14,545,115,48};}
static Box stop_continue_box(void){return (Box){140,545,116,48};}

static void draw_stops(App*a){
    header(a,tr(a->language,T_ROUTE));
    draw_map(a);
    fill(a,(Box){0,70,SIDEBAR,HEIGHT-70},0xFFFFFF);
    int lx=18;
    char label[32];snprintf(label,sizeof label,"%s ",tr(a->language,T_ROUTE));
    text(a,label,lx,95,0x073B4C);{int w;TTF_SizeUTF8(a->fonts[a->language],label,&w,NULL);lx+=w;}
    char number[32];snprintf(number,sizeof number,"%s - ",a->booking.route->number);
    text_f(a,latin_font(a),number,lx,95,0x073B4C);{int w;TTF_SizeUTF8(latin_font(a),number,&w,NULL);lx+=w;}
    char rname[160];fit_text(a,route_name(a,a->booking.route),235-lx,rname,sizeof rname);
    text(a,rname,lx,95,0x073B4C);
    text(a,tr(a->language,T_TAP_STOPS),18,130,0x333333);
    size_t idx[MAX_STOPS];
    int matches=filter_stops(a->stop_query,a->booking.route,idx,MAX_STOPS);
    int max_scroll=matches>STOP_ROWS?matches-STOP_ROWS:0;
    if(a->stop_scroll>max_scroll)a->stop_scroll=max_scroll;
    fit_button_f(a,latin_font(a),stop_search_box(),a->stop_query[0]?a->stop_query:"Search",0x073B4C,10);
    button_f(a,latin_font(a),stop_scroll_up_box(),"^",a->stop_scroll>0?0x118AB2:0xB0BEC5);
    button_f(a,latin_font(a),stop_scroll_down_box(),"v",a->stop_scroll<max_scroll?0x118AB2:0xB0BEC5);
    for(int row=0;row<STOP_ROWS;row++){
        int i=a->stop_scroll+row;
        if(i>=matches)break;
        size_t si=idx[i];
        uint32_t c=(int)si==a->booking.start_stop?0x27AE60:(int)si==a->booking.end_stop?0xE67E22:0x6C7A89;
        fit_button(a,stop_row_box(row),booking_stop_name(a,(int)si),c,10);
    }
    button(a,stop_back_box(),tr(a->language,T_BACK),0x6C7A89);
    button(a,stop_continue_box(),tr(a->language,T_CONTINUE),0x118AB2);
}

static void draw_payment(App*a){
    header(a,tr(a->language,T_PAY));
    fill(a,(Box){0,70,WIDTH,HEIGHT-70},0xE9F4F6);
    char s[256];
    char start[100],end[100];
    fit_text(a,booking_stop_name(a,a->booking.start_stop),280,start,sizeof start);
    fit_text(a,booking_stop_name(a,a->booking.end_stop),280,end,sizeof end);
    snprintf(s,sizeof s,"%s: ",a->booking.route->number);
    int w1,w2;TTF_SizeUTF8(latin_font(a),s,&w1,NULL);
    char rest[224];snprintf(rest,sizeof rest,"%s  ->  %s",start,end);
    TTF_SizeUTF8(a->fonts[a->language],rest,&w2,NULL);
    int lx=(WIDTH-(w1+w2))/2;
    text_f(a,latin_font(a),s,lx,140,0x073B4C);
    text(a,rest,lx+w1,140,0x073B4C);
    snprintf(s,sizeof s,"%s: ",tr(a->language,T_FARE));
    char amount[32];snprintf(amount,sizeof amount,"Rs. %d",booking_fare(&a->booking));
    TTF_SizeUTF8(a->fonts[a->language],s,&w1,NULL);TTF_SizeUTF8(latin_font(a),amount,&w2,NULL);
    lx=(WIDTH-(w1+w2))/2;
    text(a,s,lx,195,0x073B4C);
    text_f(a,latin_font(a),amount,lx+w1,195,0x073B4C);
    button(a,(Box){190,280,190,80},tr(a->language,T_CASH),a->booking.payment==PAY_CASH?0x27AE60:0x6C7A89);
    button(a,(Box){417,280,190,80},tr(a->language,T_CARD),a->booking.payment==PAY_CARD?0x27AE60:0x6C7A89);
    button(a,(Box){644,280,190,80},tr(a->language,T_UPI),a->booking.payment==PAY_UPI?0x27AE60:0x6C7A89);
    button(a,(Box){365,440,294,65},tr(a->language,T_PRINT),0x118AB2);
    button(a,(Box){20,640,120,45},tr(a->language,T_BACK),0x6C7A89);
    if(a->notice)centered(a,a->notice,(Box){0,535,WIDTH,35},0xD35400);
}
static Box ticket_save_box(void){return (Box){110,620,250,58};}
static Box ticket_print_box(void){return (Box){387,620,250,58};}
static Box ticket_new_box(void){return (Box){664,620,250,58};}

static void draw_qr(App*a,int x,int y,int size,unsigned ticket_number){
    enum { QUIET=4, GRID=TICKET_QR_SIZE+QUIET*2 };
    unsigned char modules[TICKET_QR_SIZE][TICKET_QR_SIZE];
    ticket_qr_encode(ticket_number,modules);
    fill(a,(Box){x,y,size,size},0xFFFFFF);
    int module=size/GRID;
    int used=module*GRID;
    int left=x+(size-used)/2+QUIET*module;
    int top=y+(size-used)/2+QUIET*module;
    for(int row=0;row<TICKET_QR_SIZE;row++)for(int col=0;col<TICKET_QR_SIZE;col++)
        if(modules[row][col])fill(a,(Box){left+col*module,top+row*module,module,module},0x101820);
}

static void ticket_time_text(time_t when,char*out,size_t size){
    struct tm local;
    if(!when) { snprintf(out,size,"--"); return; }
#if defined(_WIN32)
    if(localtime_s(&local,&when)!=0) { snprintf(out,size,"--"); return; }
#else
    if(!localtime_r(&when,&local)) { snprintf(out,size,"--"); return; }
#endif
    if(!strftime(out,size,"%d %b %Y  %H:%M",&local))snprintf(out,size,"--");
}

static void draw_ticket_layout(App*a,Box b){
    unsigned ticket_number=booking_ticket_id(&a->booking);
    fill(a,b,0xFFFFFF);
    fill(a,(Box){b.x,b.y,b.w,(int)(b.h*0.16)},0x073B4C);
    text_f(a,latin_font(a),"BEST MUMBAI",b.x+(int)(b.w*0.045),b.y+(int)(b.h*0.035),0xFFFFFF);
    char value[128];
    snprintf(value,sizeof value,"TICKET  #%u",ticket_number);
    int tw=0;TTF_SizeUTF8(latin_font(a),value,&tw,NULL);
    text_f(a,latin_font(a),value,b.x+b.w-(int)(b.w*0.045)-tw,b.y+(int)(b.h*0.035),0xFFFFFF);

    int left=b.x+(int)(b.w*0.055), top=b.y+(int)(b.h*0.205);
    int left_width=(int)(b.w*0.54);
    text(a,tr(a->language,T_ROUTE),left,top,0x58717C);
    snprintf(value,sizeof value,"%s",a->booking.route->number);
    text_f(a,latin_font(a),value,left,top+(int)(b.h*0.055),0x073B4C);

    int stop_y=top+(int)(b.h*0.155);
    text(a,tr(a->language,T_BOARDING),left,stop_y,0x58717C);
    char stop[192];
    fit_text(a,booking_stop_name(a,a->booking.start_stop),left_width,stop,sizeof stop);
    text(a,stop,left,stop_y+(int)(b.h*0.052),0x172A35);
    stop_y+=(int)(b.h*0.15);
    text(a,tr(a->language,T_DESTINATION),left,stop_y,0x58717C);
    fit_text(a,booking_stop_name(a,a->booking.end_stop),left_width,stop,sizeof stop);
    text(a,stop,left,stop_y+(int)(b.h*0.052),0x172A35);

    int qr_size=(int)(b.h*0.39);
    int qr_x=b.x+b.w-(int)(b.w*0.055)-qr_size;
    int qr_y=b.y+(int)(b.h*0.245);
    draw_qr(a,qr_x,qr_y,qr_size,ticket_number);
    ticket_qr_payload(ticket_number,value,sizeof value);
    centered_f(a,latin_font(a),value,(Box){qr_x-(int)(b.w*.04),qr_y+qr_size+2,qr_size+(int)(b.w*.08),(int)(b.h*.05)},0x465C66);

    int fare_y=b.y+(int)(b.h*.72);
    fill(a,(Box){b.x+(int)(b.w*.045),fare_y,b.w-(int)(b.w*.09),(int)(b.h*.12)},0xEAF5F5);
    snprintf(value,sizeof value,"Rs. %d",booking_fare(&a->booking));
    text_f(a,latin_font(a),value,left,fare_y+(int)(b.h*.025),0x073B4C);
    char until[80];
    ticket_time_text(booking_valid_until(&a->booking),until,sizeof until);
    int valid_x=b.x+(int)(b.w*.50);
    text(a,tr(a->language,T_VALID_UNTIL),valid_x,fare_y+(int)(b.h*.015),0x58717C);
    int vw=0;TTF_SizeUTF8(latin_font(a),until,&vw,NULL);
    text_f(a,latin_font(a),until,b.x+b.w-(int)(b.w*.055)-vw,fare_y+(int)(b.h*.06),0x073B4C);

    centered(a,tr(a->language,T_VALID_DURATION),(Box){b.x,b.y+(int)(b.h*.88),b.w,(int)(b.h*.06)},0x58717C);
}

static int draw_ticket_pdf_surface(App*a,SDL_Surface**surface_out){
    enum { PAGE_W=1500, PAGE_H=900 };
    SDL_Surface*surface=SDL_CreateRGBSurfaceWithFormat(0,PAGE_W,PAGE_H,32,SDL_PIXELFORMAT_RGBA32);
    if(!surface)return 0;
    SDL_Renderer*renderer=SDL_CreateSoftwareRenderer(surface);
    if(!renderer){SDL_FreeSurface(surface);return 0;}
    App page=*a;
    page.renderer=renderer;
    for(int i=0;i<LANG_COUNT;i++){
        page.fonts[i]=TTF_OpenFont(i==LANG_MR||i==LANG_HI?"assets/fonts/NotoSansDevanagari-Regular.ttf":
            i==LANG_GU?"assets/fonts/NotoSansGujarati-Regular.ttf":"assets/fonts/NotoSans-Regular.ttf",38);
        if(!page.fonts[i]){
            for(int j=0;j<i;j++)TTF_CloseFont(page.fonts[j]);
            SDL_DestroyRenderer(renderer);SDL_FreeSurface(surface);return 0;
        }
    }
    color(renderer,0xE9F4F6);SDL_RenderClear(renderer);
    draw_ticket_layout(&page,(Box){54,54,PAGE_W-108,PAGE_H-108});
    SDL_RenderPresent(renderer);
    for(int i=0;i<LANG_COUNT;i++)TTF_CloseFont(page.fonts[i]);
    SDL_DestroyRenderer(renderer);
    *surface_out=surface;
    return 1;
}

static int export_ticket_pdf(App*a,int print){
    SDL_Surface*surface=NULL;
    if(!draw_ticket_pdf_surface(a,&surface)){a->notice=tr(a->language,T_PDF_ERROR);return 0;}
    char path[1024];
    int ok=ticket_pdf_write(surface,booking_ticket_id(&a->booking),path,sizeof path);
    SDL_FreeSurface(surface);
    if(!ok){a->notice=tr(a->language,T_PDF_ERROR);return 0;}
#if defined(_WIN32)
    if(print){
        HINSTANCE result=ShellExecuteA(NULL,"print",path,NULL,NULL,SW_SHOWNORMAL);
        if((INT_PTR)result<=32){a->notice=tr(a->language,T_PDF_SAVED);return 1;}
    } else {
        char url[1200];
        size_t at=0;
        at+=(size_t)snprintf(url+at,sizeof url-at,"file:///");
        for(const char*p=path;*p&&at+4<sizeof url;p++){
            char c=*p=='\\'?'/':*p;
            if(c==' ')at+=(size_t)snprintf(url+at,sizeof url-at,"%%20");
            else url[at++]=c;
            url[at]=0;
        }
        SDL_OpenURL(url);
    }
#else
    char url[1200];
    size_t at=(size_t)snprintf(url,sizeof url,"file://");
    for(const char*p=path;*p&&at+4<sizeof url;p++){
        if(*p==' ')at+=(size_t)snprintf(url+at,sizeof url-at,"%%20");
        else url[at++]=*p;
        url[at]=0;
    }
    SDL_OpenURL(url);
#endif
    a->notice=tr(a->language,T_PDF_SAVED);
    return 1;
}

static void draw_ticket(App*a){
    header(a,tr(a->language,T_TICKET_READY));
    draw_ticket_layout(a,(Box){130,95,764,500});
    button(a,ticket_save_box(),tr(a->language,T_SAVE_PDF),0x118AB2);
    button(a,ticket_print_box(),tr(a->language,T_PRINT_PDF),0x27AE60);
    button(a,ticket_new_box(),tr(a->language,T_NEW_TICKET),0x6C7A89);
    if(a->notice)centered(a,a->notice,(Box){200,585,624,28},0xD35400);
}
static void draw_help(App*a){
    header(a,tr(a->language,T_HELP));
    fill(a,(Box){0,70,WIDTH,HEIGHT-70},0xE9F4F6);
    centered(a,tr(a->language,T_HELPLINE),(Box){0,230,WIDTH,45},0x073B4C);
    centered_f(a,latin_font(a),"Emergency: 112",(Box){0,285,WIDTH,45},0x073B4C);
    centered_f(a,latin_font(a),"Satellite imagery: Sentinel-2 cloudless (s2maps.eu) by EOX IT Services GmbH, CC BY-NC-SA 4.0",(Box){0,345,WIDTH,35},0x555555);
    centered_f(a,latin_font(a),"Bus route data: croyla/mumbai-gtfs (MIT-0)",(Box){0,380,WIDTH,35},0x555555);
    button(a,(Box){420,470,180,55},tr(a->language,T_BACK),0x118AB2);
}
/* On-screen keypad for the route-number / stop-name search (Latin only --
 * route codes and the GTFS source stop names are always plain ASCII, see
 * ci_contains()/filter_routes()/filter_stops()). draw_keyboard() and
 * click_keyboard() share the *_box() geometry helpers below so a key drawn
 * in one place is always the key that reacts to a tap there. */
static const char *const KB_ROWS[4] = {"1234567890","QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};
enum { KB_KEY_W=90, KB_KEY_H=64, KB_GAP=6, KB_TOP=160, KB_ROW_H=72 };
static Box kb_key_box(int row,int col){
    int len=(int)strlen(KB_ROWS[row]);
    int total_w=len*(KB_KEY_W+KB_GAP)-KB_GAP;
    int left=(WIDTH-total_w)/2;
    return (Box){left+col*(KB_KEY_W+KB_GAP),KB_TOP+row*KB_ROW_H,KB_KEY_W,KB_KEY_H};
}
static Box kb_backspace_box(void){Box last=kb_key_box(3,(int)strlen(KB_ROWS[3])-1);return (Box){last.x+last.w+KB_GAP+20,last.y,130,KB_KEY_H};}
static Box kb_clear_box(void){return (Box){WIDTH/2-300,KB_TOP+4*KB_ROW_H+20,290,60};}
static Box kb_done_box(void){return (Box){WIDTH/2+10,KB_TOP+4*KB_ROW_H+20,290,60};}
static Box kb_query_box(void){return (Box){(WIDTH-600)/2,90,600,55};}

static char *active_query(App*a){return a->screen==SCREEN_STOPS?a->stop_query:a->route_query;}

static void draw_keyboard(App*a){
    char *query=active_query(a);
    header(a,a->screen==SCREEN_STOPS?tr(a->language,T_TAP_STOPS):tr(a->language,T_SELECT_ROUTE));
    fill(a,(Box){0,70,WIDTH,HEIGHT-70},0xE9F4F6);
    Box qbox=kb_query_box();
    fill(a,qbox,0xFFFFFF);
    char shown[QUERY_CAP+2];snprintf(shown,sizeof shown,"%s_",query);
    text_f(a,latin_font(a),shown,qbox.x+16,qbox.y+14,0x073B4C);
    for(int row=0;row<4;row++)for(int col=0;col<(int)strlen(KB_ROWS[row]);col++){
        char label[2]={KB_ROWS[row][col],0};
        button_f(a,latin_font(a),kb_key_box(row,col),label,0x118AB2);
    }
    button_f(a,latin_font(a),kb_backspace_box(),"<-",0x6C7A89);
    /* Search is English-only for now (see ci_contains()/filter_*'s doc
     * comments), so its own chrome -- Search/Clear/Done -- is plain English
     * too, always drawn with the Latin font: calling tr() here previously
     * fed Devanagari/Gujarati text through latin_font() and rendered as
     * tofu boxes outside English mode. */
    button_f(a,latin_font(a),kb_clear_box(),"Clear",0x6C7A89);
    button_f(a,latin_font(a),kb_done_box(),"Done",0x118AB2);
}
static void click_keyboard(App*a,int x,int y){
    char *query=active_query(a);
    if(inside(kb_done_box(),x,y)){a->searching=false;return;}
    if(inside(kb_clear_box(),x,y)){query[0]=0;return;}
    if(inside(kb_backspace_box(),x,y)){size_t len=strlen(query);if(len>0)query[len-1]=0;return;}
    for(int row=0;row<4;row++)for(int col=0;col<(int)strlen(KB_ROWS[row]);col++){
        if(inside(kb_key_box(row,col),x,y)){
            size_t len=strlen(query);
            if(len+1<QUERY_CAP){query[len]=KB_ROWS[row][col];query[len+1]=0;}
            return;
        }
    }
}

static void render(App*a){
    color(a->renderer,0xFFFFFF);SDL_RenderClear(a->renderer);
    if(a->searching)draw_keyboard(a);
    else switch(a->screen){case SCREEN_ROUTE:draw_route(a);break;case SCREEN_STOPS:draw_stops(a);break;case SCREEN_PAYMENT:draw_payment(a);break;case SCREEN_TICKET:draw_ticket(a);break;case SCREEN_HELP:draw_help(a);break;default:break;}
    SDL_RenderPresent(a->renderer);
}

static void center_camera_on_route(App*a,const Route*r){
    if(r->stop_count==0)return;
    long sx=0,sy=0;
    for(size_t i=0;i<r->stop_count;i++){sx+=r->stops[i].x;sy+=r->stops[i].y;}
    camera_pan_to(&a->camera,(int)(sx/(long)r->stop_count),(int)(sy/(long)r->stop_count));
    a->camera.zoom_index=1;
}

static bool handle_map_click(App*a,int x,int y){
    if(inside(zoom_in_box(),x,y)){camera_zoom_in(&a->camera);return true;}
    if(inside(zoom_out_box(),x,y)){camera_zoom_out(&a->camera);return true;}
    return false;
}

static void click(App*a,int x,int y){
    /* Checked before the a->searching gate below: the language button is
     * drawn (via header()) on the search-keyboard screen too, so it must
     * stay clickable there -- it previously went dead/unresponsive the
     * moment searching started, since click_keyboard() never tested for
     * it and the early return skipped this check entirely. */
    if(inside((Box){845,15,155,40},x,y)){a->language=(Language)((a->language+1)%LANG_COUNT);return;}
    if(a->searching){click_keyboard(a,x,y);return;}
    if(a->screen==SCREEN_ROUTE){
        if(handle_map_click(a,x,y))return;
        if(inside(route_help_box(),x,y)){a->screen=SCREEN_HELP;return;}
        if(inside(route_search_box(),x,y)){a->searching=true;return;}
        size_t n;const Route*r=catalog_routes(&n);
        size_t idx[MAX_ROUTES];
        int matches=filter_routes(a->route_query,r,n,idx,MAX_ROUTES);
        int max_scroll=matches>ROUTE_ROWS?matches-ROUTE_ROWS:0;
        if(inside(route_scroll_up_box(),x,y)){if(a->route_scroll>0)a->route_scroll-=ROUTE_ROWS;if(a->route_scroll<0)a->route_scroll=0;return;}
        if(inside(route_scroll_down_box(),x,y)){if(a->route_scroll<max_scroll)a->route_scroll+=ROUTE_ROWS;if(a->route_scroll>max_scroll)a->route_scroll=max_scroll;return;}
        for(int row=0;row<ROUTE_ROWS;row++){
            int i=a->route_scroll+row;
            if(i>=matches)break;
            if(inside(route_row_box(row),x,y)){size_t ri=idx[i];a->booking.route=&r[ri];a->booking.start_stop=a->booking.end_stop=-1;a->stop_scroll=0;a->stop_query[0]=0;a->screen=SCREEN_STOPS;center_camera_on_route(a,&r[ri]);return;}
        }
        if(inside(map_box(),x,y)){int ix,iy;Box b=map_box();camera_screen_to_image(&a->camera,b.x,b.y,b.w,b.h,x,y,&ix,&iy);camera_pan_to(&a->camera,ix,iy);}
    }
    else if(a->screen==SCREEN_STOPS){
        if(handle_map_click(a,x,y))return;
        if(inside(stop_back_box(),x,y)){a->screen=SCREEN_ROUTE;camera_init(&a->camera);return;}
        if(inside(stop_search_box(),x,y)){a->searching=true;return;}
        if(inside(stop_continue_box(),x,y)&&booking_validate(&a->booking)==BOOKING_OK){a->screen=SCREEN_PAYMENT;return;}
        size_t idx[MAX_STOPS];
        int matches=filter_stops(a->stop_query,a->booking.route,idx,MAX_STOPS);
        int max_scroll=matches>STOP_ROWS?matches-STOP_ROWS:0;
        if(inside(stop_scroll_up_box(),x,y)){if(a->stop_scroll>0)a->stop_scroll-=STOP_ROWS;if(a->stop_scroll<0)a->stop_scroll=0;return;}
        if(inside(stop_scroll_down_box(),x,y)){if(a->stop_scroll<max_scroll)a->stop_scroll+=STOP_ROWS;if(a->stop_scroll>max_scroll)a->stop_scroll=max_scroll;return;}
        int s=-1;
        if(inside(map_box(),x,y))s=stop_screen_hit(a,a->booking.route,x,y,16);
        for(int row=0;row<STOP_ROWS;row++){
            int i=a->stop_scroll+row;
            if(i>=matches)break;
            if(inside(stop_row_box(row),x,y))s=(int)idx[i];
        }
        if(s>=0){if(a->booking.start_stop<0)a->booking.start_stop=s;else a->booking.end_stop=s;}
        else if(inside(map_box(),x,y)){int ix,iy;Box b=map_box();camera_screen_to_image(&a->camera,b.x,b.y,b.w,b.h,x,y,&ix,&iy);camera_pan_to(&a->camera,ix,iy);}
    }
    else if(a->screen==SCREEN_PAYMENT){if(inside((Box){20,640,120,45},x,y)){a->screen=SCREEN_STOPS;return;}if(inside((Box){190,280,190,80},x,y))a->booking.payment=PAY_CASH;else if(inside((Box){417,280,190,80},x,y))a->booking.payment=PAY_CARD;else if(inside((Box){644,280,190,80},x,y))a->booking.payment=PAY_UPI;else if(inside((Box){365,440,294,65},x,y)){BookingError e=booking_complete(&a->booking);if(e==BOOKING_OK){a->notice=NULL;a->screen=SCREEN_TICKET;}else if(e==BOOKING_PRINTER_EMPTY)a->notice=tr(a->language,T_PRINTER_EMPTY);}}
    else if(a->screen==SCREEN_TICKET){
        if(inside(ticket_save_box(),x,y)){export_ticket_pdf(a,0);return;}
        if(inside(ticket_print_box(),x,y)){export_ticket_pdf(a,1);return;}
        if(inside(ticket_new_box(),x,y)){a->screen=SCREEN_ROUTE;a->notice=NULL;camera_init(&a->camera);}
    }else if(a->screen==SCREEN_HELP&&inside((Box){420,470,180,55},x,y))a->screen=SCREEN_ROUTE;
}

static TTF_Font *open_font(const char *path){TTF_Font *f=TTF_OpenFont(path,18);if(!f)fprintf(stderr,"Failed to open font %s: %s\n",path,TTF_GetError());return f;}

int ui_run(void){
    if(SDL_Init(SDL_INIT_VIDEO)!=0||TTF_Init()!=0)return 1;
    App a={0};
    a.window=SDL_CreateWindow("BEST Ticket Kiosk",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIDTH,HEIGHT,0);
    a.renderer=SDL_CreateRenderer(a.window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!a.window||!a.renderer)return 1;
    a.map_texture=load_map_texture(a.renderer);
    a.fonts[LANG_EN]=open_font("assets/fonts/NotoSans-Regular.ttf");
    a.fonts[LANG_MR]=open_font("assets/fonts/NotoSansDevanagari-Regular.ttf");
    a.fonts[LANG_HI]=open_font("assets/fonts/NotoSansDevanagari-Regular.ttf");
    a.fonts[LANG_GU]=open_font("assets/fonts/NotoSansGujarati-Regular.ttf");
    for(int i=0;i<LANG_COUNT;i++)if(!a.fonts[i])return 1;
    camera_init(&a.camera);
    booking_init(&a.booking);
    a.screen=SCREEN_ROUTE;
    bool run=true;
    while(run){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT)run=false;
            if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)click(&a,e.button.x,e.button.y);
        }
        render(&a);
    }
    for(int i=0;i<LANG_COUNT;i++)TTF_CloseFont(a.fonts[i]);
    SDL_DestroyTexture(a.map_texture);
    SDL_DestroyRenderer(a.renderer);
    SDL_DestroyWindow(a.window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
