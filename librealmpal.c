/**
 * librealmpal.c - Universal Image to 8-bit Palette Library Implementation
 * 
 * Core library functions for image quantization, dithering, and palette management.
 * This file contains all the algorithmic implementations from the original realmpal.c
 * refactored into a clean, reusable library interface.
 */

// Feature test macros for POSIX functions
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "librealmpal.h"

#define STBI_ONLY_PNG
#define STBI_ONLY_BMP

#ifdef _MSC_VER
  #define _CRT_SECURE_NO_WARNINGS
  #pragma warning(push)
  #pragma warning(disable:4996) // fopen etc.
#endif

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-function"
  #pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>

#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#endif

#ifdef _MSC_VER
  #ifndef strcasecmp
    #define strcasecmp  _stricmp
  #endif
  #ifndef strncasecmp
    #define strncasecmp _strnicmp
  #endif
#endif

// ----------------------------- Internal Constants and Macros -----------------------------
#define WU_SIDE 33
#define CACHE_SIZE 1024

// round() fallback if missing (MSVC old or strict C99 libs)
static inline double pb_round(double x){
#if defined(_MSC_VER) && _MSC_VER < 1800
    return (x>=0.0) ? floor(x+0.5) : ceil(x-0.5);
#else
    return round(x);
#endif
}

// ----------------------------- Internal State and Caching -----------------------------

// Performance: Pre-computed perceptual weights lookup table
static double perceptual_lut[256];
static bool perceptual_lut_initialized = false;

// Color distance cache for frequently used colors
typedef struct {
    uint32_t color; // RGB packed as 24-bit
    uint8_t best_index;
    bool valid;
} ColorCache;
static ColorCache color_cache[CACHE_SIZE];
static bool cache_initialized = false;

// Wu quantizer working arrays
static unsigned long *wu_wt, *wu_mr, *wu_mg, *wu_mb;
static double *wu_m2;

// Error tracking
static char last_error[256] = {0};

// ----------------------------- Internal Helper Functions -----------------------------

static inline int WIDX(int r,int g,int b){ 
    return (r*WU_SIDE + g)*WU_SIDE + b; 
}

static void set_error(const char* message) {
    strncpy(last_error, message, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

static void clear_error_internal(void) {
    last_error[0] = '\0';
}

// Performance: Initialize perceptual lookup table
static void init_perceptual_lut(void) {
    if (perceptual_lut_initialized) return;
    for (int i = 0; i < 256; i++) {
        perceptual_lut[i] = pow(i / 255.0, 2.2);
    }
    perceptual_lut_initialized = true;
}

// Performance: Initialize color cache
static void init_color_cache(void) {
    if (cache_initialized) return;
    memset(color_cache, 0, sizeof(color_cache));
    cache_initialized = true;
}

// Performance: Pack RGB into 24-bit for caching
static inline uint32_t pack_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static inline uint32_t be32(const uint8_t* p){ 
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | (uint32_t)p[3]; 
}

// Wu quantizer memory management
static void wu_free_hist(void){
    free(wu_wt); free(wu_mr); free(wu_mg); free(wu_mb); free(wu_m2);
    wu_wt=wu_mr=wu_mg=wu_mb=NULL; wu_m2=NULL;
}

// ----------------------------- Public API Implementation -----------------------------

int realmpal_init(void) {
    init_perceptual_lut();
    init_color_cache();
    clear_error_internal();
    return 1;
}

const char* realmpal_get_version(int *major, int *minor, int *patch) {
    if (major) *major = REALMPAL_VERSION_MAJOR;
    if (minor) *minor = REALMPAL_VERSION_MINOR;
    if (patch) *patch = REALMPAL_VERSION_PATCH;
    return REALMPAL_VERSION_STRING;
}

const char* realmpal_get_last_error(void) {
    return last_error[0] ? last_error : "No error";
}

void realmpal_clear_error(void) {
    clear_error_internal();
}

// ----------------------------- Image Loading and Management -----------------------------

int realmpal_load_image(const char *path, RGBA8 **out_pixels, int *w, int *h) {
    if (!path || !out_pixels || !w || !h) {
        set_error("Invalid arguments to realmpal_load_image");
        return 0;
    }
    
    int n;
    unsigned char *img = stbi_load(path, w, h, &n, 4);
    if (!img) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to load image: %s", stbi_failure_reason());
        set_error(error_msg);
        return 0;
    }
    
    *out_pixels = (RGBA8*)img;
    clear_error_internal();
    return 1;
}

void realmpal_free_image(RGBA8 *pixels) {
    if (pixels) {
        stbi_image_free((void*)pixels);
    }
}

void realmpal_apply_transparency(RGBA8* pixels, int count, RGB8 matte_color, int alpha_threshold) {
    if (!pixels || count <= 0) return;
    
    for(int i = 0; i < count; i++) {
        if(pixels[i].a < alpha_threshold) {
            pixels[i].r = matte_color.r;
            pixels[i].g = matte_color.g;
            pixels[i].b = matte_color.b;
        }
    }
}

// ----------------------------- Palette Management -----------------------------

int realmpal_read_bmp_palette(const char* path, RGB8 out_palette[256]){
    if (!path || !out_palette) {
        set_error("Invalid arguments to realmpal_read_bmp_palette");
        return 0;
    }
    
    FILE *f=fopen(path,"rb");
    if(!f) {
        set_error("Could not open BMP file");
        return 0;
    }

    uint8_t sig[2];
    if(fread(sig,1,2,f)!=2){ fclose(f); set_error("Invalid BMP file"); return 0; }
    if(sig[0]!='B'||sig[1]!='M'){ fclose(f); set_error("Not a BMP file"); return 0; }

    if(fseek(f,14,SEEK_SET)!=0){ fclose(f); set_error("BMP seek error"); return 0; }
    uint32_t biSize=0;
    if(fread(&biSize,4,1,f)!=1){ fclose(f); set_error("BMP read error"); return 0; }
    if(biSize < 40){ fclose(f); set_error("Unsupported BMP format"); return 0; }

    uint8_t buf40[40];
    if(fseek(f,14,SEEK_SET)!=0){ fclose(f); set_error("BMP seek error"); return 0; }
    if(fread(buf40,1,40,f)!=40){ fclose(f); set_error("BMP read error"); return 0; }
    uint16_t bpp = *(uint16_t*)&buf40[14];
    uint32_t clrUsed = *(uint32_t*)&buf40[32];

    if(clrUsed==0){
        if(bpp==1) clrUsed = 2;
        else if(bpp==4) clrUsed = 16;
        else if(bpp==8) clrUsed = 256;
        else clrUsed = 0;
    }
    if(clrUsed>256) clrUsed = 256;

    long pal_pos = 14 + (long)biSize;
    if(fseek(f, pal_pos, SEEK_SET)!=0){ fclose(f); set_error("BMP palette seek error"); return 0; }

    for(uint32_t i=0;i<256;i++){
        uint8_t bgra[4] = {0,0,0,0};
        if(i < clrUsed){
            if(fread(bgra,1,4,f)!=4){ fclose(f); set_error("BMP palette read error"); return 0; }
        }
        out_palette[i].b=bgra[0]; out_palette[i].g=bgra[1]; out_palette[i].r=bgra[2];
    }
    fclose(f);
    clear_error_internal();
    return (int)(clrUsed ? clrUsed : 0);
}

int realmpal_read_png_palette(const char* path, RGB8 out_palette[256]){
    if (!path || !out_palette) {
        set_error("Invalid arguments to realmpal_read_png_palette");
        return 0;
    }
    
    FILE *f=fopen(path,"rb");
    if(!f) {
        set_error("Could not open PNG file");
        return 0;
    }
    
    uint8_t sig[8];
    if(fread(sig,1,8,f)!=8){ fclose(f); set_error("Invalid PNG file"); return 0; }
    static const uint8_t pngsig[8]={137,80,78,71,13,10,26,10};
    if(memcmp(sig,pngsig,8)!=0){ fclose(f); set_error("Not a PNG file"); return 0; }
    
    int found=0;
    for(;;){
        uint8_t lenb[4], type[4];
        if(fread(lenb,1,4,f)!=4){ break; }
        uint32_t len=be32(lenb);
        if(fread(type,1,4,f)!=4){ break; }
        if(memcmp(type,"PLTE",4)==0 && len>=3 && len<=256*3 && (len%3)==0){
            uint8_t buf[256*3];
            if(fread(buf,1,len,f)!=len){ break; }
            for(unsigned i=0;i<len/3;i++){
                out_palette[i].r=buf[i*3+0];
                out_palette[i].g=buf[i*3+1];
                out_palette[i].b=buf[i*3+2];
            }
            if(fseek(f,4,SEEK_CUR)!=0){ break; } // skip CRC
            for(unsigned i=len/3;i<256;i++){ out_palette[i].r=out_palette[i].g=out_palette[i].b=0; }
            found = (int)(len/3);
            break;
        } else {
            if(fseek(f, (long)len + 4, SEEK_CUR)!=0){ break; } // data + crc
        }
    }
    fclose(f);
    
    if (found > 0) {
        clear_error_internal();
    } else {
        set_error("No palette found in PNG file");
    }
    return found;
}

int realmpal_read_pcx_palette(const char* path, RGB8 out_palette[256]) {
    if (!path || !out_palette) {
        set_error("Invalid arguments to realmpal_read_pcx_palette");
        return 0;
    }
    
    FILE *f = fopen(path, "rb");
    if (!f) {
        set_error("Could not open PCX file");
        return 0;
    }
    
    // Read and verify PCX header
    uint8_t header[128];
    if (fread(header, sizeof(header), 1, f) != 1) {
        fclose(f);
        set_error("Could not read PCX header");
        return 0;
    }
    
    // Verify it's a valid PCX file
    if (header[0] != 0x0A || header[1] != 5 || header[3] != 8) {
        fclose(f);
        set_error("Not a valid 256-color PCX file");
        return 0;
    }
    
    // Skip image data by seeking to palette position
    // PCX palette is at the end: file_size - 769 bytes (1 marker + 256*3 RGB)
    if (fseek(f, -769, SEEK_END) != 0) {
        fclose(f);
        set_error("Could not seek to PCX palette");
        return 0;
    }
    
    // Check for palette marker (0x0C)
    uint8_t marker;
    if (fread(&marker, 1, 1, f) != 1 || marker != 0x0C) {
        fclose(f);
        set_error("PCX palette marker not found");
        return 0;
    }
    
    // Read RGB palette
    for (int i = 0; i < 256; i++) {
        uint8_t rgb[3];
        if (fread(rgb, 1, 3, f) != 3) {
            fclose(f);
            set_error("Could not read PCX palette data");
            return 0;
        }
        out_palette[i].r = rgb[0];
        out_palette[i].g = rgb[1];
        out_palette[i].b = rgb[2];
    }
    
    fclose(f);
    clear_error_internal();
    return 256;
}

int realmpal_read_any_palette(const char* path, RGB8 out_palette[256], int max_colors) {
    if (!path || !out_palette) {
        set_error("Invalid arguments to realmpal_read_any_palette");
        return 0;
    }
    
    // Try reading palette directly first
    int colors = realmpal_read_bmp_palette(path, out_palette);
    if (colors > 0) return colors;
    
    colors = realmpal_read_png_palette(path, out_palette);
    if (colors > 0) return colors;
    
    colors = realmpal_read_pcx_palette(path, out_palette);
    if (colors > 0) return colors;
    
    // If no palette found, try loading as image and quantizing
    RGBA8 *pixels;
    int w, h;
    if (!realmpal_load_image(path, &pixels, &w, &h)) {
        return 0; // Error already set by load_image
    }
    
    int want = (max_colors > 0 && max_colors <= 256) ? max_colors : 256;
    colors = realmpal_quantize_wu(pixels, w * h, want, 0, out_palette);
    realmpal_free_image(pixels);
    
    return colors;
}

// ----------------------------- Wu Quantizer Implementation -----------------------------

static int wu_build_hist(const RGBA8 *p, int count){
    size_t N = (size_t)WU_SIDE*WU_SIDE*WU_SIDE;
    wu_wt = (unsigned long*)calloc(N,sizeof(unsigned long));
    wu_mr = (unsigned long*)calloc(N,sizeof(unsigned long));
    wu_mg = (unsigned long*)calloc(N,sizeof(unsigned long));
    wu_mb = (unsigned long*)calloc(N,sizeof(unsigned long));
    wu_m2 = (double*)calloc(N,sizeof(double));
    if(!wu_wt||!wu_mr||!wu_mg||!wu_mb||!wu_m2){ 
        wu_free_hist(); 
        set_error("Out of memory during Wu quantization");
        return 0; 
    }

    for(int i=0;i<count;i++){
        int r = (p[i].r >> 3) + 1;
        int g = (p[i].g >> 3) + 1;
        int b = (p[i].b >> 3) + 1;
        int ind = WIDX(r,g,b);
        wu_wt[ind] += 1;
        wu_mr[ind] += p[i].r;
        wu_mg[ind] += p[i].g;
        wu_mb[ind] += p[i].b;
        wu_m2[ind] += (double)p[i].r*p[i].r + (double)p[i].g*p[i].g + (double)p[i].b*p[i].b;
    }

    for(int r=1;r<WU_SIDE;r++){
        for(int g=1;g<WU_SIDE;g++){
            for(int b=1;b<WU_SIDE;b++){
                int ind = WIDX(r,g,b);
                unsigned long wt = wu_wt[ind] + wu_wt[WIDX(r-1,g,b)] + wu_wt[WIDX(r,g-1,b)] + wu_wt[WIDX(r,g,b-1)]
                                 - wu_wt[WIDX(r-1,g-1,b)] - wu_wt[WIDX(r-1,g,b-1)] - wu_wt[WIDX(r,g-1,b-1)]
                                 + wu_wt[WIDX(r-1,g-1,b-1)];
                unsigned long mr = wu_mr[ind] + wu_mr[WIDX(r-1,g,b)] + wu_mr[WIDX(r,g-1,b)] + wu_mr[WIDX(r,g,b-1)]
                                 - wu_mr[WIDX(r-1,g-1,b)] - wu_mr[WIDX(r-1,g,b-1)] - wu_mr[WIDX(r,g-1,b-1)]
                                 + wu_mr[WIDX(r-1,g-1,b-1)];
                unsigned long mg = wu_mg[ind] + wu_mg[WIDX(r-1,g,b)] + wu_mg[WIDX(r,g-1,b)] + wu_mg[WIDX(r,g,b-1)]
                                 - wu_mg[WIDX(r-1,g-1,b)] - wu_mg[WIDX(r-1,g,b-1)] - wu_mg[WIDX(r,g-1,b-1)]
                                 + wu_mg[WIDX(r-1,g-1,b-1)];
                unsigned long mb = wu_mb[ind] + wu_mb[WIDX(r-1,g,b)] + wu_mb[WIDX(r,g-1,b)] + wu_mb[WIDX(r,g,b-1)]
                                 - wu_mb[WIDX(r-1,g-1,b)] - wu_mb[WIDX(r-1,g,b-1)] - wu_mb[WIDX(r,g-1,b-1)]
                                 + wu_mb[WIDX(r-1,g-1,b-1)];
                double m2 = wu_m2[ind] + wu_m2[WIDX(r-1,g,b)] + wu_m2[WIDX(r,g-1,b)] + wu_m2[WIDX(r,g,b-1)]
                          - wu_m2[WIDX(r-1,g-1,b)] - wu_m2[WIDX(r-1,g,b-1)] - wu_m2[WIDX(r,g-1,b-1)]
                          + wu_m2[WIDX(r-1,g-1,b-1)];
                wu_wt[ind]=wt; wu_mr[ind]=mr; wu_mg[ind]=mg; wu_mb[ind]=mb; wu_m2[ind]=m2;
            }
        }
    }
    return 1;
}

typedef struct { int r0,r1,g0,g1,b0,b1; } WBox;

static inline unsigned long V_wt(const WBox *c){
    return   wu_wt[WIDX(c->r1,c->g1,c->b1)] - wu_wt[WIDX(c->r0,c->g1,c->b1)] - wu_wt[WIDX(c->r1,c->g0,c->b1)] - wu_wt[WIDX(c->r1,c->g1,c->b0)]
           + wu_wt[WIDX(c->r0,c->g0,c->b1)] + wu_wt[WIDX(c->r0,c->g1,c->b0)] + wu_wt[WIDX(c->r1,c->g0,c->b0)] - wu_wt[WIDX(c->r0,c->g0,c->b0)];
}

static double wu_variance(const WBox *c){
    double w = (double)V_wt(c);
    if(w<=0.0) return 0.0;
    
    double mr = (double)(wu_mr[WIDX(c->r1,c->g1,c->b1)] - wu_mr[WIDX(c->r0,c->g1,c->b1)] - wu_mr[WIDX(c->r1,c->g0,c->b1)] - wu_mr[WIDX(c->r1,c->g1,c->b0)]
                       + wu_mr[WIDX(c->r0,c->g0,c->b1)] + wu_mr[WIDX(c->r0,c->g1,c->b0)] + wu_mr[WIDX(c->r1,c->g0,c->b0)] - wu_mr[WIDX(c->r0,c->g0,c->b0)]);
    double mg = (double)(wu_mg[WIDX(c->r1,c->g1,c->b1)] - wu_mg[WIDX(c->r0,c->g1,c->b1)] - wu_mg[WIDX(c->r1,c->g0,c->b1)] - wu_mg[WIDX(c->r1,c->g1,c->b0)]
                       + wu_mg[WIDX(c->r0,c->g0,c->b1)] + wu_mg[WIDX(c->r0,c->g1,c->b0)] + wu_mg[WIDX(c->r1,c->g0,c->b0)] - wu_mg[WIDX(c->r0,c->g0,c->b0)]);
    double mb = (double)(wu_mb[WIDX(c->r1,c->g1,c->b1)] - wu_mb[WIDX(c->r0,c->g1,c->b1)] - wu_mb[WIDX(c->r1,c->g0,c->b1)] - wu_mb[WIDX(c->r1,c->g1,c->b0)]
                       + wu_mb[WIDX(c->r0,c->g0,c->b1)] + wu_mb[WIDX(c->r0,c->g1,c->b0)] + wu_mb[WIDX(c->r1,c->g0,c->b0)] - wu_mb[WIDX(c->r0,c->g0,c->b0)]);
    double m2 = wu_m2[WIDX(c->r1,c->g1,c->b1)] - wu_m2[WIDX(c->r0,c->g1,c->b1)] - wu_m2[WIDX(c->r1,c->g0,c->b1)] - wu_m2[WIDX(c->r1,c->g1,c->b0)]
              + wu_m2[WIDX(c->r0,c->g0,c->b1)] + wu_m2[WIDX(c->r0,c->g1,c->b0)] + wu_m2[WIDX(c->r1,c->g0,c->b0)] - wu_m2[WIDX(c->r0,c->g0,c->b0)];
    
    double sum = mr*mr + mg*mg + mb*mb;
    return m2 - sum / w;
}

static int wu_try_cut(const WBox *c, int axis, int *cutpos, double *totvar){
    int best_k = -1; double best_var = -1.0;
    double var_total = wu_variance(c);
    int kstart = (axis==0? c->r0+1 : axis==1? c->g0+1 : c->b0+1);
    int kend   = (axis==0? c->r1   : axis==1? c->g1   : c->b1  );
    for(int k=kstart; k<kend; k++){
        WBox c1=*c, c2=*c;
        if(axis==0){ c1.r1=k; c2.r0=k; }
        else if(axis==1){ c1.g1=k; c2.g0=k; }
        else { c1.b1=k; c2.b0=k; }
        double v1 = wu_variance(&c1);
        double v2 = wu_variance(&c2);
        double gain = var_total - (v1+v2);
        if(gain>best_var){ best_var=gain; best_k=k; }
    }
    if(best_k<0) return 0;
    *cutpos = best_k; *totvar = best_var; return 1;
}

static int wu_cut_box(const WBox *in, WBox *out1, WBox *out2){
    int k; double g, best_g=-1.0; int best_axis=-1, best_k=-1;
    if(wu_try_cut(in,0,&k,&g) && g>best_g){ best_g=g; best_axis=0; best_k=k; }
    if(wu_try_cut(in,1,&k,&g) && g>best_g){ best_g=g; best_axis=1; best_k=k; }
    if(wu_try_cut(in,2,&k,&g) && g>best_g){ best_g=g; best_axis=2; best_k=k; }
    if(best_axis<0) return 0;
    *out1 = *in; *out2 = *in;
    if(best_axis==0){ out1->r1=best_k; out2->r0=best_k; }
    else if(best_axis==1){ out1->g1=best_k; out2->g0=best_k; }
    else { out1->b1=best_k; out2->b0=best_k; }
    return 1;
}

int realmpal_quantize_wu(const RGBA8 *pixels, int count, int max_colors, int offset, RGB8 out_palette[256]){
    if (!pixels || count <= 0 || !out_palette) {
        set_error("Invalid arguments to realmpal_quantize_wu");
        return 0;
    }
    
    if(max_colors<1) max_colors = 1;
    if(max_colors>256) max_colors=256;
    if(offset < 0) offset = 0;
    if(offset >= 256) offset = 255;
    
    if(!wu_build_hist(pixels, count)) return 0;

    WBox *boxes = (WBox*)malloc(sizeof(WBox)*(size_t)max_colors);
    if(!boxes){ 
        wu_free_hist(); 
        set_error("Out of memory during Wu quantization");
        return 0; 
    }

    int nboxes=1;
    boxes[0] = (WBox){1, WU_SIDE-1, 1, WU_SIDE-1, 1, WU_SIDE-1};

    while(nboxes<max_colors){
        int best_i=-1; double best_v=0.0;
        for(int i=0;i<nboxes;i++){
            double v = wu_variance(&boxes[i]);
            if(v>best_v && V_wt(&boxes[i])>0){ best_v=v; best_i=i; }
        }
        if(best_i<0) break;

        WBox a,b;
        if(!wu_cut_box(&boxes[best_i], &a, &b)) break;
        boxes[best_i]=a;
        boxes[nboxes++]=b;
    }

    int produced=0;
    for(int i=0;i<nboxes && produced<max_colors;i++){
        WBox c=boxes[i];
        unsigned long wt = V_wt(&c);
        int idx = offset + produced;
        if(wt==0){ 
            if(idx<256){ out_palette[idx]=(RGB8){0,0,0}; } 
            produced++; 
            continue; 
        }
        
        unsigned long r = wu_mr[WIDX(c.r1,c.g1,c.b1)] - wu_mr[WIDX(c.r0,c.g1,c.b1)] - wu_mr[WIDX(c.r1,c.g0,c.b1)] - wu_mr[WIDX(c.r1,c.g1,c.b0)]
                        + wu_mr[WIDX(c.r0,c.g0,c.b1)] + wu_mr[WIDX(c.r0,c.g1,c.b0)] + wu_mr[WIDX(c.r1,c.g0,c.b0)] - wu_mr[WIDX(c.r0,c.g0,c.b0)];
        unsigned long g = wu_mg[WIDX(c.r1,c.g1,c.b1)] - wu_mg[WIDX(c.r0,c.g1,c.b1)] - wu_mg[WIDX(c.r1,c.g0,c.b1)] - wu_mg[WIDX(c.r1,c.g1,c.b0)]
                        + wu_mg[WIDX(c.r0,c.g0,c.b1)] + wu_mg[WIDX(c.r0,c.g1,c.b0)] + wu_mg[WIDX(c.r1,c.g0,c.b0)] - wu_mg[WIDX(c.r0,c.g0,c.b0)];
        unsigned long b = wu_mb[WIDX(c.r1,c.g1,c.b1)] - wu_mb[WIDX(c.r0,c.g1,c.b1)] - wu_mb[WIDX(c.r1,c.g0,c.b1)] - wu_mb[WIDX(c.r1,c.g1,c.b0)]
                        + wu_mb[WIDX(c.r0,c.g0,c.b1)] + wu_mb[WIDX(c.r0,c.g1,c.b0)] + wu_mb[WIDX(c.r1,c.g0,c.b0)] - wu_mb[WIDX(c.r0,c.g0,c.b0)];
        
        RGB8 col = { (uint8_t)realmpal_clamp_int((int)(r/(int)wt),0,255),
                     (uint8_t)realmpal_clamp_int((int)(g/(int)wt),0,255),
                     (uint8_t)realmpal_clamp_int((int)(b/(int)wt),0,255) };
        if(idx<256) out_palette[idx]=col;
        produced++;
    }
    
    // Zero out unused palette entries
    for(int i=0;i<offset && i<256;i++){
        if(!(out_palette[i].r||out_palette[i].g||out_palette[i].b)) out_palette[i]=(RGB8){0,0,0};
    }
    for(int i=offset+produced;i<256;i++){
        if(!(out_palette[i].r||out_palette[i].g||out_palette[i].b)) out_palette[i]=(RGB8){0,0,0};
    }

    free(boxes);
    wu_free_hist();
    clear_error_internal();
    return produced;
}

// ----------------------------- Median-Cut Quantizer Implementation -----------------------------

typedef struct {
    RGBA8 *pixels;
    int count;
    int r_min, r_max;
    int g_min, g_max;
    int b_min, b_max;
} MedianCutBox;

static int compare_red(const void *a, const void *b) {
    RGBA8 *pa = (RGBA8*)a;
    RGBA8 *pb = (RGBA8*)b;
    return pa->r - pb->r;
}

static int compare_green(const void *a, const void *b) {
    RGBA8 *pa = (RGBA8*)a;
    RGBA8 *pb = (RGBA8*)b;
    return pa->g - pb->g;
}

static int compare_blue(const void *a, const void *b) {
    RGBA8 *pa = (RGBA8*)a;
    RGBA8 *pb = (RGBA8*)b;
    return pa->b - pb->b;
}

static void median_cut_find_bounds(MedianCutBox *box) {
    if (box->count == 0) return;
    
    box->r_min = box->r_max = box->pixels[0].r;
    box->g_min = box->g_max = box->pixels[0].g;
    box->b_min = box->b_max = box->pixels[0].b;
    
    for (int i = 1; i < box->count; i++) {
        if (box->pixels[i].r < box->r_min) box->r_min = box->pixels[i].r;
        if (box->pixels[i].r > box->r_max) box->r_max = box->pixels[i].r;
        if (box->pixels[i].g < box->g_min) box->g_min = box->pixels[i].g;
        if (box->pixels[i].g > box->g_max) box->g_max = box->pixels[i].g;
        if (box->pixels[i].b < box->b_min) box->b_min = box->pixels[i].b;
        if (box->pixels[i].b > box->b_max) box->b_max = box->pixels[i].b;
    }
}

static RGB8 median_cut_average_color(MedianCutBox *box) {
    if (box->count == 0) return (RGB8){0, 0, 0};
    
    unsigned long r_sum = 0, g_sum = 0, b_sum = 0;
    for (int i = 0; i < box->count; i++) {
        r_sum += box->pixels[i].r;
        g_sum += box->pixels[i].g;
        b_sum += box->pixels[i].b;
    }
    
    return (RGB8){
        (uint8_t)(r_sum / box->count),
        (uint8_t)(g_sum / box->count),
        (uint8_t)(b_sum / box->count)
    };
}

static int median_cut_split_box(MedianCutBox *box, MedianCutBox *box1, MedianCutBox *box2) {
    if (box->count < 2) return 0;
    
    median_cut_find_bounds(box);
    
    // Find the axis with the largest range
    int r_range = box->r_max - box->r_min;
    int g_range = box->g_max - box->g_min;
    int b_range = box->b_max - box->b_min;
    
    int axis = 0; // 0=red, 1=green, 2=blue
    if (g_range >= r_range && g_range >= b_range) axis = 1;
    else if (b_range >= r_range && b_range >= g_range) axis = 2;
    
    // Sort pixels along the chosen axis
    switch (axis) {
        case 0: qsort(box->pixels, box->count, sizeof(RGBA8), compare_red); break;
        case 1: qsort(box->pixels, box->count, sizeof(RGBA8), compare_green); break;
        case 2: qsort(box->pixels, box->count, sizeof(RGBA8), compare_blue); break;
    }
    
    // Split at median
    int median = box->count / 2;
    
    *box1 = (MedianCutBox){
        .pixels = box->pixels,
        .count = median
    };
    
    *box2 = (MedianCutBox){
        .pixels = box->pixels + median,
        .count = box->count - median
    };
    
    return 1;
}

int realmpal_quantize_median_cut(const RGBA8 *pixels, int count, int max_colors, int offset, RGB8 out_palette[256]) {
    if (!pixels || count <= 0 || !out_palette) {
        set_error("Invalid arguments to realmpal_quantize_median_cut");
        return 0;
    }
    
    if (max_colors < 1) max_colors = 1;
    if (max_colors > 256) max_colors = 256;
    if (offset < 0) offset = 0;
    if (offset >= 256) offset = 255;
    
    // Create a copy of pixels for sorting
    RGBA8 *pixel_copy = (RGBA8*)malloc(sizeof(RGBA8) * count);
    if (!pixel_copy) {
        set_error("Out of memory during median-cut quantization");
        return 0;
    }
    memcpy(pixel_copy, pixels, sizeof(RGBA8) * count);
    
    // Initialize box list with all pixels
    MedianCutBox *boxes = (MedianCutBox*)malloc(sizeof(MedianCutBox) * max_colors);
    if (!boxes) {
        free(pixel_copy);
        set_error("Out of memory during median-cut quantization");
        return 0;
    }
    
    int num_boxes = 1;
    boxes[0] = (MedianCutBox){
        .pixels = pixel_copy,
        .count = count
    };
    
    // Split boxes until we have enough colors
    while (num_boxes < max_colors) {
        // Find the box with the most pixels
        int largest_box = 0;
        int largest_count = boxes[0].count;
        
        for (int i = 1; i < num_boxes; i++) {
            if (boxes[i].count > largest_count) {
                largest_count = boxes[i].count;
                largest_box = i;
            }
        }
        
        // Can't split further if the largest box has only 1 pixel
        if (largest_count <= 1) break;
        
        // Split the largest box
        MedianCutBox new_box1, new_box2;
        if (!median_cut_split_box(&boxes[largest_box], &new_box1, &new_box2)) {
            break;
        }
        
        // Replace the original box with the two new boxes
        boxes[largest_box] = new_box1;
        boxes[num_boxes] = new_box2;
        num_boxes++;
    }
    
    // Generate palette from boxes
    int produced = 0;
    for (int i = 0; i < num_boxes && produced < max_colors; i++) {
        int idx = offset + produced;
        if (idx < 256) {
            out_palette[idx] = median_cut_average_color(&boxes[i]);
        }
        produced++;
    }
    
    // Zero out unused palette entries
    for (int i = 0; i < offset && i < 256; i++) {
        if (!(out_palette[i].r || out_palette[i].g || out_palette[i].b)) {
            out_palette[i] = (RGB8){0, 0, 0};
        }
    }
    for (int i = offset + produced; i < 256; i++) {
        if (!(out_palette[i].r || out_palette[i].g || out_palette[i].b)) {
            out_palette[i] = (RGB8){0, 0, 0};
        }
    }
    
    free(pixel_copy);
    free(boxes);
    clear_error_internal();
    return produced;
}

// ----------------------------- Color Mapping and Distance Functions -----------------------------

double realmpal_color_distance_rgb(RGB8 c1, RGB8 c2) {
    double dr = (double)c1.r - c2.r;
    double dg = (double)c1.g - c2.g;
    double db = (double)c1.b - c2.b;
    return sqrt(dr*dr + dg*dg + db*db);
}

double realmpal_color_distance_perceptual(RGB8 c1, RGB8 c2) {
    init_perceptual_lut();
    
    double R1 = perceptual_lut[c1.r];
    double G1 = perceptual_lut[c1.g];
    double B1 = perceptual_lut[c1.b];
    double R2 = perceptual_lut[c2.r];
    double G2 = perceptual_lut[c2.g];
    double B2 = perceptual_lut[c2.b];
    
    double dr = R1 - R2;
    double dg = G1 - G2;
    double db = B1 - B2;
    
    // Weighted by luminance perception
    return sqrt(0.2126*dr*dr + 0.7152*dg*dg + 0.0722*db*db);
}

static int best_index_linear_cached(uint8_t r, uint8_t g, uint8_t b,
                                    const RGB8 *pal, int skip_index) {
    init_color_cache();

    uint32_t packed = pack_rgb(r, g, b);
    uint32_t hash = packed % CACHE_SIZE;

    // Use cache only when NOT skipping any index
    if (skip_index < 0 && color_cache[hash].valid && color_cache[hash].color == packed) {
        return color_cache[hash].best_index;
    }

    RGB8 transparency_color = {0, 0, 0}; // Default to black
    if (skip_index >= 0 && skip_index < 256) {
        transparency_color = pal[skip_index];
    }

    int best = -1;
    int bestd = INT_MAX;
    
    for (int i = 0; i < 256; i++) {
        // Skip the transparency index itself
        if (skip_index >= 0 && i == skip_index) continue;
        
        // Skip any index that has the same color as transparency index
        if (skip_index >= 0 && 
            pal[i].r == transparency_color.r && 
            pal[i].g == transparency_color.g && 
            pal[i].b == transparency_color.b) {
            continue;
        }
        
        // Optional: Skip pure black and pure white for safety (can be disabled)
        // Only avoid pure black/white if they're not explicitly part of a small palette
        bool avoid_pure_colors = true; // Make this configurable if needed
        if (avoid_pure_colors && 
            ((pal[i].r == 0 && pal[i].g == 0 && pal[i].b == 0) ||
             (pal[i].r == 255 && pal[i].g == 255 && pal[i].b == 255))) {
            continue;
        }
        
        int dr = (int)r - pal[i].r;
        int dg = (int)g - pal[i].g;
        int db = (int)b - pal[i].b;
        int d = dr*dr + dg*dg + db*db;
        
        if (best == -1 || d < bestd) {
            bestd = d;
            best = i;
        }
    }

    // Fallback: if no valid colors found, we need to find the "least bad" option
    if (best == -1) {
        // Find any non-transparency index, even if it's black/white
        for (int i = 0; i < 256; i++) {
            if (skip_index >= 0 && i == skip_index) continue;
            
            // At least avoid the exact transparency color match
            if (skip_index >= 0 && 
                pal[i].r == transparency_color.r && 
                pal[i].g == transparency_color.g && 
                pal[i].b == transparency_color.b) {
                continue;
            }
            
            best = i;
            break;
        }
        
        // Last resort fallback
        if (best == -1) {
            best = (skip_index == 0) ? 1 : 0;
        }
    }

    // Don't cache results that depend on skip_index
    if (skip_index < 0) {
        color_cache[hash].color = packed;
        color_cache[hash].best_index = (uint8_t)best;
        color_cache[hash].valid = true;
    }
    return best;
}

static int best_index_perceptual_optimized(uint8_t r, uint8_t g, uint8_t b,
                                           const RGB8 *pal, int skip_index) {
    init_perceptual_lut();
    init_color_cache();

    uint32_t packed = pack_rgb(r, g, b);
    uint32_t hash = (packed + 1) % CACHE_SIZE;

    // Use cache only when NOT skipping any index
    if (skip_index < 0 && color_cache[hash].valid && color_cache[hash].color == packed) {
        return color_cache[hash].best_index;
    }

    double R = perceptual_lut[r];
    double G = perceptual_lut[g];
    double B = perceptual_lut[b];

    RGB8 transparency_color = {0, 0, 0}; // Default to black
    if (skip_index >= 0 && skip_index < 256) {
        transparency_color = pal[skip_index];
    }

    int best = -1;
    double bestd = 1e99;
    
    for (int i = 0; i < 256; i++) {
        // Skip the transparency index itself
        if (skip_index >= 0 && i == skip_index) continue;
        
        // Skip any index that has the same color as transparency index
        if (skip_index >= 0 && 
            pal[i].r == transparency_color.r && 
            pal[i].g == transparency_color.g && 
            pal[i].b == transparency_color.b) {
            continue;
        }
        
        // Optional: Skip pure black and pure white for safety
        if ((pal[i].r == 0 && pal[i].g == 0 && pal[i].b == 0) ||
            (pal[i].r == 255 && pal[i].g == 255 && pal[i].b == 255)) {
            continue;
        }
        
        double pr = perceptual_lut[pal[i].r];
        double pg = perceptual_lut[pal[i].g];
        double pb = perceptual_lut[pal[i].b];
        double dr = R - pr, dg = G - pg, db = B - pb;
        double d = 0.2126*dr*dr + 0.7152*dg*dg + 0.0722*db*db;
        
        if (best == -1 || d < bestd) {
            bestd = d;
            best = i;
        }
    }

    // Fallback: if no valid colors found, find the "least bad" option
    if (best == -1) {
        // Find any non-transparency index, even if it's black/white
        for (int i = 0; i < 256; i++) {
            if (skip_index >= 0 && i == skip_index) continue;
            
            // At least avoid the exact transparency color match
            if (skip_index >= 0 && 
                pal[i].r == transparency_color.r && 
                pal[i].g == transparency_color.g && 
                pal[i].b == transparency_color.b) {
                continue;
            }
            
            best = i;
            break;
        }
        
        // Last resort fallback
        if (best == -1) {
            best = (skip_index == 0) ? 1 : 0;
        }
    }

    // Don't cache results that depend on skip_index
    if (skip_index < 0) {
        color_cache[hash].color = packed;
        color_cache[hash].best_index = (uint8_t)best;
        color_cache[hash].valid = true;
    }
    return best;
}

// ----------------------------- Index Constraints -----------------------------

int realmpal_parse_index_constraints(const char* constraint_string, RealmpalIndexConstraints* constraints) {
    if (!constraint_string || !constraints) {
        return 0;
    }
    
    // Initialize constraints
    memset(constraints, 0, sizeof(RealmpalIndexConstraints));
    constraints->enabled = 0;
    constraints->count = 0;
    
    // Skip empty strings
    while (*constraint_string && isspace(*constraint_string)) {
        constraint_string++;
    }
    if (!*constraint_string) {
        return 1; // Empty string is valid (no constraints)
    }
    
    constraints->enabled = 1;
    
    // Create a working copy of the string
    char* work_string = (char*)malloc(strlen(constraint_string) + 1);
    if (!work_string) {
        set_error("Out of memory parsing constraints");
        return 0;
    }
    strcpy(work_string, constraint_string);
    
    // Parse comma-separated tokens
    char* token = strtok(work_string, ",");
    while (token && constraints->count < 32) {
        // Trim whitespace
        while (*token && isspace(*token)) token++;
        char* end = token + strlen(token) - 1;
        while (end > token && isspace(*end)) *end-- = '\0';
        
        if (strlen(token) == 0) {
            token = strtok(NULL, ",");
            continue;
        }
        
        // Check for range (contains '-')
        char* dash = strchr(token, '-');
        if (dash) {
            // Range format: "start-end"
            *dash = '\0';
            dash++;
            
            // Trim whitespace around dash
            char* start_str = token;
            char* end_str = dash;
            while (*start_str && isspace(*start_str)) start_str++;
            while (*end_str && isspace(*end_str)) end_str++;
            
            char* start_end = start_str + strlen(start_str) - 1;
            while (start_end > start_str && isspace(*start_end)) *start_end-- = '\0';
            char* end_end = end_str + strlen(end_str) - 1;
            while (end_end > end_str && isspace(*end_end)) *end_end-- = '\0';
            
            int start_idx = atoi(start_str);
            int end_idx = atoi(end_str);
            
            // Validate range
            if (start_idx < 0 || start_idx > 255 || end_idx < 0 || end_idx > 255) {
                free(work_string);
                set_error("Index out of range (must be 0-255)");
                return 0;
            }
            
            // Ensure start <= end
            if (start_idx > end_idx) {
                int temp = start_idx;
                start_idx = end_idx;
                end_idx = temp;
            }
            
            constraints->ranges[constraints->count].start = start_idx;
            constraints->ranges[constraints->count].end = end_idx;
        } else {
            // Single index
            int index = atoi(token);
            if (index < 0 || index > 255) {
                free(work_string);
                set_error("Index out of range (must be 0-255)");
                return 0;
            }
            
            constraints->ranges[constraints->count].start = index;
            constraints->ranges[constraints->count].end = index;
        }
        
        constraints->count++;
        token = strtok(NULL, ",");
    }
    
    free(work_string);
    
    if (constraints->count == 0) {
        constraints->enabled = 0;
    }
    
    clear_error_internal();
    return 1;
}

int realmpal_index_allowed_by_constraints(int index, const RealmpalIndexConstraints* constraints, int transparency_index) {
    // Check if index is transparency index first
    if (transparency_index >= 0 && index == transparency_index) {
        return 0;
    }
    
    if (!constraints || !constraints->enabled || constraints->count == 0) {
        // No constraints = all indices allowed (except transparency)
        return 1;
    }
    
    // Check if index falls within any of the allowed ranges
    for (int i = 0; i < constraints->count; i++) {
        if (index >= constraints->ranges[i].start && index <= constraints->ranges[i].end) {
            return 1;
        }
    }
    
    return 0;
}

int realmpal_count_constraint_indices(const RealmpalIndexConstraints* constraints, int transparency_index) {
    if (!constraints || !constraints->enabled || constraints->count == 0) {
        // No constraints = all indices available
        int total = 256;
        if (transparency_index >= 0 && transparency_index <= 255) {
            total--;
        }
        return total;
    }
    
    int total = 0;
    for (int i = 0; i < constraints->count; i++) {
        int range_size = constraints->ranges[i].end - constraints->ranges[i].start + 1;
        
        // Check if transparency index is in this range
        if (transparency_index >= 0 && 
            transparency_index >= constraints->ranges[i].start && 
            transparency_index <= constraints->ranges[i].end) {
            range_size--;
        }
        
        total += range_size;
    }
    
    return total;
}

int realmpal_constraints_to_string(const RealmpalIndexConstraints* constraints, char* output_buffer, int buffer_size) {
    if (!constraints || !output_buffer || buffer_size <= 0) {
        return 0;
    }
    
    if (!constraints->enabled || constraints->count == 0) {
        if (buffer_size >= 1) {
            output_buffer[0] = '\0';
            return 1;
        }
        return 0;
    }
    
    int pos = 0;
    for (int i = 0; i < constraints->count && pos < buffer_size - 1; i++) {
        if (i > 0) {
            // Add comma separator
            if (pos + 2 >= buffer_size) break;
            output_buffer[pos++] = ',';
            output_buffer[pos++] = ' ';
        }
        
        if (constraints->ranges[i].start == constraints->ranges[i].end) {
            // Single index
            int written = snprintf(output_buffer + pos, buffer_size - pos, "%d", constraints->ranges[i].start);
            if (written >= buffer_size - pos) break;
            pos += written;
        } else {
            // Range
            int written = snprintf(output_buffer + pos, buffer_size - pos, "%d-%d", 
                                 constraints->ranges[i].start, constraints->ranges[i].end);
            if (written >= buffer_size - pos) break;
            pos += written;
        }
    }
    
    output_buffer[pos] = '\0';
    return 1;
}

// Helper function to convert legacy range to new constraints format
static void convert_legacy_constraints_internal(RealmpalConfig* config) {
    if (config->enforce_mapping_range && 
        config->mapping_min_index >= 0 && config->mapping_max_index >= 0) {
        
        // Convert legacy single range to new format
        config->index_constraints.enabled = 1;
        config->index_constraints.count = 1;
        config->index_constraints.ranges[0].start = config->mapping_min_index;
        config->index_constraints.ranges[0].end = config->mapping_max_index;
        
        // Disable legacy format
        config->enforce_mapping_range = 0;
    }
}

static int realmpal_find_best_color_match(uint8_t r, uint8_t g, uint8_t b,
                                         const RGB8 *pal, int transparency_index,
                                         const RealmpalIndexConstraints* constraints,
                                         bool use_perceptual) {
    int best = -1;
    double best_distance = 1e99;
    
    if (use_perceptual) {
        init_perceptual_lut();
        double R = perceptual_lut[r];
        double G = perceptual_lut[g];
        double B = perceptual_lut[b];
        
        for (int i = 0; i < 256; i++) {
            if (!realmpal_index_allowed_by_constraints(i, constraints, transparency_index)) {
                continue;
            }
            
            double pr = perceptual_lut[pal[i].r];
            double pg = perceptual_lut[pal[i].g];
            double pb = perceptual_lut[pal[i].b];
            double dr = R - pr, dg = G - pg, db = B - pb;
            double d = 0.2126*dr*dr + 0.7152*dg*dg + 0.0722*db*db;
            
            if (best == -1 || d < best_distance) {
                best_distance = d;
                best = i;
            }
        }
    } else {
        for (int i = 0; i < 256; i++) {
            if (!realmpal_index_allowed_by_constraints(i, constraints, transparency_index)) {
                continue;
            }
            
            int dr = (int)r - pal[i].r;
            int dg = (int)g - pal[i].g;
            int db = (int)b - pal[i].b;
            double d = dr*dr + dg*dg + db*db;
            
            if (best == -1 || d < best_distance) {
                best_distance = d;
                best = i;
            }
        }
    }
    
    // Fallback if no valid index found
    if (best == -1) {
        // Find any allowed index
        for (int i = 0; i < 256; i++) {
            if (realmpal_index_allowed_by_constraints(i, constraints, transparency_index)) {
                best = i;
                break;
            }
        }
        
        // Ultimate fallback
        if (best == -1) {
            best = (transparency_index == 0) ? 1 : 0;
        }
    }
    
    return best;
}

// ----------------------------- Dithering Implementation -----------------------------

void realmpal_map_nearest_neighbor(const RGBA8* src, int w, int h, const RGB8* palette, 
                                  uint8_t *out, int transparency_index, int alpha_threshold,
                                  const RealmpalIndexConstraints* constraints) {
    if (!src || !palette || !out || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_map_nearest_neighbor");
        return;
    }
    
    for (int y = 0; y < h; y++) {
        const RGBA8* s = src + (size_t)y * w;
        uint8_t* d = out + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                d[x] = (uint8_t)transparency_index;
            } else {
                d[x] = (uint8_t)realmpal_find_best_color_match(s[x].r, s[x].g, s[x].b, palette, 
                                                             transparency_index, constraints, false);
            }
        }
    }
    clear_error_internal();
}

void realmpal_map_floyd_steinberg(const RGBA8* src, int w, int h, const RGB8* palette, 
                                 uint8_t *out, bool serpentine, double strength,
                                 int transparency_index, int alpha_threshold,
                                 const RealmpalIndexConstraints* constraints) {
    if (!src || !palette || !out || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_map_floyd_steinberg");
        return;
    }
    
    // Allocate error buffers
    double *er = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *eg = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *eb = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *nr = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *ng = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *nb = (double*)calloc((size_t)(w + 2), sizeof(double));
    
    if (!er || !eg || !eb || !nr || !ng || !nb) {
        free(er); free(eg); free(eb); free(nr); free(ng); free(nb);
        set_error("Out of memory in Floyd-Steinberg dithering");
        return;
    }

    const double ERR_CLAMP = 64.0 * strength;
    const double fs_7_16 = 7.0/16.0 * strength;
    const double fs_3_16 = 3.0/16.0 * strength;
    const double fs_5_16 = 5.0/16.0 * strength;
    const double fs_1_16 = 1.0/16.0 * strength;

    for (int y = 0; y < h; y++) {
        // Clear next row errors
        for (int i = 0; i < w + 2; i++) {
            nr[i] = ng[i] = nb[i] = 0.0;
        }

        const RGBA8* s = src + (size_t)y * w;
        uint8_t* d = out + (size_t)y * w;

        if (!serpentine || (y % 2 == 0)) {
            // Left to right
            for (int x = 0; x < w; x++) {
                if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                    d[x] = (uint8_t)transparency_index;
                    continue;
                }
                
                double R = s[x].r + er[x];
                double G = s[x].g + eg[x];
                double B = s[x].b + eb[x];
                
                R = realmpal_clamp_double(R, 0.0, 255.0);
                G = realmpal_clamp_double(G, 0.0, 255.0);
                B = realmpal_clamp_double(B, 0.0, 255.0);

                int idx = realmpal_find_best_color_match((uint8_t)R, (uint8_t)G, (uint8_t)B, palette, 
                                                       transparency_index, constraints, true);
                d[x] = (uint8_t)idx;

                double dr = R - palette[idx].r;
                double dg = G - palette[idx].g;
                double db = B - palette[idx].b;

                dr = realmpal_clamp_double(dr, -ERR_CLAMP, ERR_CLAMP);
                dg = realmpal_clamp_double(dg, -ERR_CLAMP, ERR_CLAMP);
                db = realmpal_clamp_double(db, -ERR_CLAMP, ERR_CLAMP);

                er[x + 1] += dr * fs_7_16; eg[x + 1] += dg * fs_7_16; eb[x + 1] += db * fs_7_16;
                
                int xm1 = (x > 0) ? x - 1 : x;
                nr[xm1] += dr * fs_3_16; ng[xm1] += dg * fs_3_16; nb[xm1] += db * fs_3_16;
                nr[x] += dr * fs_5_16; ng[x] += dg * fs_5_16; nb[x] += db * fs_5_16;
                nr[x + 1] += dr * fs_1_16; ng[x + 1] += dg * fs_1_16; nb[x + 1] += db * fs_1_16;
            }
        } else {
            // Right to left (serpentine)
            for (int x = w - 1; x >= 0; x--) {
                if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                    d[x] = (uint8_t)transparency_index;
                    continue;
                }
                
                double R = s[x].r + er[x];
                double G = s[x].g + eg[x];
                double B = s[x].b + eb[x];
                
                R = realmpal_clamp_double(R, 0.0, 255.0);
                G = realmpal_clamp_double(G, 0.0, 255.0);
                B = realmpal_clamp_double(B, 0.0, 255.0);

                int idx = realmpal_find_best_color_match((uint8_t)R, (uint8_t)G, (uint8_t)B, palette, 
                                                       transparency_index, constraints, true);
                d[x] = (uint8_t)idx;

                double dr = R - palette[idx].r;
                double dg = G - palette[idx].g;
                double db = B - palette[idx].b;

                dr = realmpal_clamp_double(dr, -ERR_CLAMP, ERR_CLAMP);
                dg = realmpal_clamp_double(dg, -ERR_CLAMP, ERR_CLAMP);
                db = realmpal_clamp_double(db, -ERR_CLAMP, ERR_CLAMP);

                if (x - 1 >= 0) {
                    er[x - 1] += dr * fs_7_16; eg[x - 1] += dg * fs_7_16; eb[x - 1] += db * fs_7_16;
                } else {
                    er[0] += dr * fs_7_16; eg[0] += dg * fs_7_16; eb[0] += db * fs_7_16;
                }

                int xp1 = (x < w - 1) ? x + 1 : x;
                nr[xp1] += dr * fs_3_16; ng[xp1] += dg * fs_3_16; nb[xp1] += db * fs_3_16;
                nr[x] += dr * fs_5_16; ng[x] += dg * fs_5_16; nb[x] += db * fs_5_16;
                int xm1 = (x > 0) ? x - 1 : 0;
                nr[xm1] += dr * fs_1_16; ng[xm1] += dg * fs_1_16; nb[xm1] += db * fs_1_16;
            }
        }

        // Swap error buffers
        double *temp_r = er; er = nr; nr = temp_r;
        double *temp_g = eg; eg = ng; ng = temp_g;
        double *temp_b = eb; eb = nb; nb = temp_b;
    }

    free(er); free(eg); free(eb); free(nr); free(ng); free(nb);
    clear_error_internal();
}

void realmpal_map_ordered_dither(const RGBA8* src, int w, int h, const RGB8* palette,
                                uint8_t *out, int matrix_size,
                                int transparency_index, int alpha_threshold,
                                const RealmpalIndexConstraints* constraints) {
    if (!src || !palette || !out || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_map_ordered_dither");
        return;
    }
    
    // Simple 4x4 Bayer matrix
    static const int bayer4[4][4] = {
        { 0,  8,  2, 10},
        {12,  4, 14,  6},
        { 3, 11,  1,  9},
        {15,  7, 13,  5}
    };
    
    int size = (matrix_size == 2 || matrix_size == 8) ? 4 : 4; // Default to 4x4
    
    for (int y = 0; y < h; y++) {
        const RGBA8* s = src + (size_t)y * w;
        uint8_t* d = out + (size_t)y * w;
        
        for (int x = 0; x < w; x++) {
            if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                d[x] = (uint8_t)transparency_index;
                continue;
            }
            
            // Get threshold from Bayer matrix
            int threshold = bayer4[y % size][x % size];
            double factor = (threshold / 15.0 - 0.5) * 32.0; // Scale factor
            
            int r = realmpal_clamp_int((int)(s[x].r + factor), 0, 255);
            int g = realmpal_clamp_int((int)(s[x].g + factor), 0, 255);
            int b = realmpal_clamp_int((int)(s[x].b + factor), 0, 255);
            
            d[x] = (uint8_t)realmpal_find_best_color_match((uint8_t)r, (uint8_t)g, (uint8_t)b, palette, 
                                                         transparency_index, constraints, false);
        }
    }
    clear_error_internal();
}

void realmpal_map_perceptual(const RGBA8* src, int w, int h, const RGB8* palette,
                            uint8_t *out, int transparency_index, int alpha_threshold,
                            const RealmpalIndexConstraints* constraints) {
    if (!src || !palette || !out || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_map_perceptual");
        return;
    }
    
    for (int y = 0; y < h; y++) {
        const RGBA8* s = src + (size_t)y * w;
        uint8_t* d = out + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                d[x] = (uint8_t)transparency_index;
            } else {
                d[x] = (uint8_t)realmpal_find_best_color_match(s[x].r, s[x].g, s[x].b, palette, 
                                                             transparency_index, constraints, true);
            }
        }
    }
    clear_error_internal();
}

// ----------------------------- File Output Implementation -----------------------------

RealmpalOutputFormat realmpal_get_output_format(const char* filename) {
    if (!filename) return REALMPAL_FORMAT_BMP;
    
    const char* ext = strrchr(filename, '.');
    if (!ext) return REALMPAL_FORMAT_BMP;
    
    if (strcasecmp(ext, ".pcx") == 0) return REALMPAL_FORMAT_PCX;
    return REALMPAL_FORMAT_BMP;
}

int realmpal_write_bmp8(const char* path, int w, int h, const uint8_t *indices, const RGB8 *palette) {
    if (!path || !indices || !palette || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_write_bmp8");
        return 0;
    }
    
    FILE *f = fopen(path, "wb");
    if(!f){ 
        set_error("Could not create BMP file");
        return 0; 
    }

#pragma pack(push,1)
    struct BMPFILEHDR {
        uint16_t bfType;      // 'BM'
        uint32_t bfSize;
        uint16_t bfReserved1;
        uint16_t bfReserved2;
        uint32_t bfOffBits;
    } filehdr;
    struct BMPINFOHDR {
        uint32_t biSize;      // 40
        int32_t  biWidth;
        int32_t  biHeight;    // positive => bottom-up
        uint16_t biPlanes;    // 1
        uint16_t biBitCount;  // 8
        uint32_t biCompression; // 0=BI_RGB
        uint32_t biSizeImage;
        int32_t  biXPelsPerMeter;
        int32_t  biYPelsPerMeter;
        uint32_t biClrUsed;   // 256
        uint32_t biClrImportant;
    } infohdr;
#pragma pack(pop)

    const int palette_bytes = 256*4; // RGBQUAD
    int rowbytes = ((w + 3) & ~3);   // pad to 4
    uint32_t pixel_bytes = (uint32_t)rowbytes * (uint32_t)h;
    uint32_t offbits = (uint32_t)(sizeof(filehdr)+sizeof(infohdr)+palette_bytes);
    filehdr.bfType = 0x4D42;
    filehdr.bfSize = offbits + pixel_bytes;
    filehdr.bfReserved1 = 0;
    filehdr.bfReserved2 = 0;
    filehdr.bfOffBits = offbits;

    infohdr.biSize = 40;
    infohdr.biWidth = w;
    infohdr.biHeight = h; // bottom-up
    infohdr.biPlanes = 1;
    infohdr.biBitCount = 8;
    infohdr.biCompression = 0;
    infohdr.biSizeImage = pixel_bytes;
    infohdr.biXPelsPerMeter = 2835; // ~72 dpi
    infohdr.biYPelsPerMeter = 2835;
    infohdr.biClrUsed = 256;
    infohdr.biClrImportant = 0;

    if(fwrite(&filehdr,sizeof(filehdr),1,f)!=1) goto fail;
    if(fwrite(&infohdr,sizeof(infohdr),1,f)!=1) goto fail;

    // palette in BGR0
    for(int i=0;i<256;i++){
        uint8_t bgrx[4] = { palette[i].b, palette[i].g, palette[i].r, 0 };
        if(fwrite(bgrx,1,4,f)!=4) goto fail;
    }

    // pixel data (bottom-up)
    uint8_t *rowbuf = (uint8_t*)malloc((size_t)rowbytes);
    if(!rowbuf) goto fail;
    for(int y=h-1; y>=0; y--){
        const uint8_t *src = indices + (size_t)y * (size_t)w;
        memcpy(rowbuf, src, (size_t)w);
        if(rowbytes>w) memset(rowbuf+w, 0, (size_t)(rowbytes-w));
        if(fwrite(rowbuf,1,(size_t)rowbytes,f)!=(size_t)rowbytes){ free(rowbuf); goto fail; }
    }
    free(rowbuf);
    fclose(f);
    clear_error_internal();
    return 1;
fail:
    fclose(f);
    set_error("Failed to write BMP file");
    return 0;
}

// PCX RLE compression
static int pcx_compress_line(const uint8_t* src, int len, uint8_t* dst) {
    int dst_pos = 0;
    int src_pos = 0;
    
    while (src_pos < len) {
        uint8_t current = src[src_pos];
        int run_length = 1;
        
        // Count consecutive identical bytes (max 63)
        while (src_pos + run_length < len && 
               src[src_pos + run_length] == current && 
               run_length < 63) {
            run_length++;
        }
        
        // If run length > 1 or byte has high bits set, encode as RLE
        if (run_length > 1 || (current & 0xC0) == 0xC0) {
            dst[dst_pos++] = 0xC0 | run_length;
            dst[dst_pos++] = current;
        } else {
            // Single byte, store as-is
            dst[dst_pos++] = current;
        }
        
        src_pos += run_length;
    }
    
    return dst_pos;
}

int realmpal_write_pcx8(const char* path, int w, int h, const uint8_t *indices, const RGB8 *palette) {
    if (!path || !indices || !palette || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_write_pcx8");
        return 0;
    }
    
    FILE *f = fopen(path, "wb");
    if (!f) {
        set_error("Could not create PCX file");
        return 0;
    }
    
#pragma pack(push,1)
    struct PCXHEADER {
        uint8_t  manufacturer;    // 0x0A
        uint8_t  version;         // 5 for 256-color
        uint8_t  encoding;        // 1 for RLE
        uint8_t  bits_per_pixel;  // 8
        uint16_t xmin, ymin;      // 0, 0
        uint16_t xmax, ymax;      // width-1, height-1
        uint16_t hres, vres;      // DPI (usually 72)
        uint8_t  ega_palette[48]; // unused for 256-color
        uint8_t  reserved1;       // 0
        uint8_t  num_planes;      // 1
        uint16_t bytes_per_line;  // must be even
        uint16_t palette_info;    // 1 for color
        uint16_t hscreen_size;    // 0
        uint16_t vscreen_size;    // 0
        uint8_t  reserved2[54];   // zeros
    };
#pragma pack(pop)
    
    // Prepare PCX header
    struct PCXHEADER header;
    memset(&header, 0, sizeof(header));
    
    header.manufacturer = 0x0A;
    header.version = 5;           // Version 5 for 256-color
    header.encoding = 1;          // RLE encoding
    header.bits_per_pixel = 8;
    header.xmin = 0;
    header.ymin = 0;
    header.xmax = (uint16_t)(w - 1);
    header.ymax = (uint16_t)(h - 1);
    header.hres = 72;             // 72 DPI
    header.vres = 72;
    header.num_planes = 1;
    header.bytes_per_line = (uint16_t)((w + 1) & ~1); // Must be even
    header.palette_info = 1;      // Color palette
    
    // Write header
    if (fwrite(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        set_error("Failed to write PCX header");
        return 0;
    }
    
    // Allocate compression buffer
    int line_buffer_size = header.bytes_per_line * 2;
    uint8_t *line_buffer = (uint8_t*)malloc(line_buffer_size);
    uint8_t *padded_line = (uint8_t*)malloc(header.bytes_per_line);
    
    if (!line_buffer || !padded_line) {
        free(line_buffer);
        free(padded_line);
        fclose(f);
        set_error("Out of memory writing PCX file");
        return 0;
    }
    
    // Write compressed image data
    for (int y = 0; y < h; y++) {
        const uint8_t *src_line = indices + (size_t)y * w;
        
        // Copy line and pad to even width if necessary
        memcpy(padded_line, src_line, w);
        if (header.bytes_per_line > w) {
            memset(padded_line + w, 0, header.bytes_per_line - w);
        }
        
        // Compress line
        int compressed_size = pcx_compress_line(padded_line, header.bytes_per_line, line_buffer);
        
        // Write compressed line
        if (fwrite(line_buffer, 1, compressed_size, f) != compressed_size) {
            free(line_buffer);
            free(padded_line);
            fclose(f);
            set_error("Failed to write PCX image data");
            return 0;
        }
    }
    
    // Write 256-color palette
    // PCX palette starts with 0x0C marker
    uint8_t palette_marker = 0x0C;
    if (fwrite(&palette_marker, 1, 1, f) != 1) {
        free(line_buffer);
        free(padded_line);
        fclose(f);
        set_error("Failed to write PCX palette marker");
        return 0;
    }
    
    // Write RGB palette (256 colors * 3 bytes each)
    for (int i = 0; i < 256; i++) {
        uint8_t rgb[3] = { palette[i].r, palette[i].g, palette[i].b };
        if (fwrite(rgb, 1, 3, f) != 3) {
            free(line_buffer);
            free(padded_line);
            fclose(f);
            set_error("Failed to write PCX palette data");
            return 0;
        }
    }
    
    free(line_buffer);
    free(padded_line);
    fclose(f);
    clear_error_internal();
    return 1;
}

int realmpal_write_auto(const char* path, int w, int h, const uint8_t *indices, const RGB8 *palette) {
    if (!path) {
        set_error("Invalid path in realmpal_write_auto");
        return 0;
    }
    
    RealmpalOutputFormat format = realmpal_get_output_format(path);
    
    switch (format) {
        case REALMPAL_FORMAT_PCX:
            return realmpal_write_pcx8(path, w, h, indices, palette);
        case REALMPAL_FORMAT_BMP:
        default:
            return realmpal_write_bmp8(path, w, h, indices, palette);
    }
}

// ----------------------------- High-Level Configuration and Conversion -----------------------------

void realmpal_config_init(RealmpalConfig *config) {
    if (!config) return;
    
    memset(config, 0, sizeof(RealmpalConfig));
    
    // Set defaults
    config->mode = REALMPAL_MODE_AUTO;
    config->dither = REALMPAL_DITHER_FS;
    config->quantizer = REALMPAL_QUANT_WU;
    config->num_colors = 256;
    config->index_offset = 0;
    config->extra_offset = -1;
    config->extra_colors = -1;
    config->matte_color = (RGB8){0, 0, 0};
    config->transparency_index = 255;
    config->alpha_threshold = 128;
    config->alpha_color = (RGB8){255, 0, 255};
    config->use_alpha_color = false;
    config->fs_strength = 1.0;
    config->ordered_matrix_size = 4;
    
    // Initialize constraints
    memset(&config->index_constraints, 0, sizeof(RealmpalIndexConstraints));
    config->index_constraints.enabled = false;
    config->index_constraints.count = 0;
    
    // Legacy compatibility (deprecated)
    config->mapping_min_index = -1;
    config->mapping_max_index = -1;
    config->enforce_mapping_range = false;
}

int realmpal_convert_image(const RealmpalConfig *config) {
    if (!config || !config->input_file || !config->output_file) {
        set_error("Invalid configuration");
        return REALMPAL_ERROR_INVALID_ARGS;
    }
    
    // Create a working copy of config to handle legacy conversion
    RealmpalConfig working_config = *config;
    convert_legacy_constraints_internal(&working_config);
    
    // Load input image
    RGBA8 *pixels = NULL;
    int w, h;
    if (!realmpal_load_image(working_config.input_file, &pixels, &w, &h)) {
        return REALMPAL_ERROR_FILE_NOT_FOUND;
    }

    // Apply transparency handling if enabled
    if (working_config.transparency_index >= 0) {
        realmpal_apply_transparency(pixels, w * h, working_config.matte_color, working_config.alpha_threshold);
    }

    // Build base palette
    RGB8 palette[256];
    memset(palette, 0, sizeof(palette));
    int have_colors = 0;

    if (working_config.mode == REALMPAL_MODE_PALETTE && working_config.palette_file) {
        have_colors = realmpal_read_any_palette(working_config.palette_file, palette, 256);
        if (have_colors <= 0) {
            realmpal_free_image(pixels);
            return REALMPAL_ERROR_INVALID_PALETTE;
        }
        
        // Shift palette if offset specified
        if (working_config.index_offset > 0) {
            RGB8 tmp[256];
            memset(tmp, 0, sizeof(tmp));
            for (int i = 0; i < have_colors && (working_config.index_offset + i) < 256; i++) {
                tmp[working_config.index_offset + i] = palette[i];
            }
            memcpy(palette, tmp, sizeof(tmp));
        }
    } else {
        // Auto quantization
        int want = working_config.num_colors;
        if (want < 1) want = 1;
        if (want > 256) want = 256;
        
        if (working_config.quantizer == REALMPAL_QUANT_WU) {
            have_colors = realmpal_quantize_wu(pixels, w * h, want, working_config.index_offset, palette);
        } else {
            have_colors = realmpal_quantize_median_cut(pixels, w * h, want, working_config.index_offset, palette);
        }
    }

    // Optional extra palette injection
    if (working_config.extra_palette_file && working_config.extra_offset >= 0) {
        RGB8 extra[256];
        int ec = realmpal_read_any_palette(working_config.extra_palette_file, extra, 
                                          working_config.extra_colors > 0 ? working_config.extra_colors : 16);
        if (ec > 0) {
            if (working_config.extra_colors > 0 && working_config.extra_colors < ec) ec = working_config.extra_colors;
            for (int i = 0; i < ec; i++) {
                int idx = working_config.extra_offset + i;
                if (idx >= 0 && idx < 256) palette[idx] = extra[i];
            }
        }
    }

    // Apply alpha color assignment if specified
    if (working_config.use_alpha_color && working_config.transparency_index >= 0 && working_config.transparency_index < 256) {
        palette[working_config.transparency_index] = working_config.alpha_color;
    }

    // Map pixels to indices
    uint8_t *indices = (uint8_t*)malloc((size_t)w * (size_t)h);
    if (!indices) {
        realmpal_free_image(pixels);
        set_error("Out of memory");
        return REALMPAL_ERROR_OUT_OF_MEMORY;
    }

    // Apply dithering with enhanced constraints
    const RealmpalIndexConstraints* constraints = working_config.index_constraints.enabled ? 
                                                  &working_config.index_constraints : NULL;
    
    switch (working_config.dither) {
        case REALMPAL_DITHER_NONE:
            realmpal_map_nearest_neighbor(pixels, w, h, palette, indices, 
                                         working_config.transparency_index, working_config.alpha_threshold,
                                         constraints);
            break;
        case REALMPAL_DITHER_FS_SERP:
            realmpal_map_floyd_steinberg(pixels, w, h, palette, indices, true, working_config.fs_strength,
                                        working_config.transparency_index, working_config.alpha_threshold,
                                        constraints);
            break;
        case REALMPAL_DITHER_FS:
            realmpal_map_floyd_steinberg(pixels, w, h, palette, indices, false, working_config.fs_strength,
                                        working_config.transparency_index, working_config.alpha_threshold,
                                        constraints);
            break;
        case REALMPAL_DITHER_ORDERED:
            realmpal_map_ordered_dither(pixels, w, h, palette, indices, working_config.ordered_matrix_size,
                                       working_config.transparency_index, working_config.alpha_threshold,
                                       constraints);
            break;
        case REALMPAL_DITHER_PERCEPTUAL:
            realmpal_map_perceptual(pixels, w, h, palette, indices,
                                   working_config.transparency_index, working_config.alpha_threshold,
                                   constraints);
            break;
        default:
            realmpal_map_floyd_steinberg(pixels, w, h, palette, indices, false, working_config.fs_strength,
                                        working_config.transparency_index, working_config.alpha_threshold,
                                        constraints);
            break;
    }

    // Save output
    int ok = realmpal_write_auto(working_config.output_file, w, h, indices, palette);

    // Cleanup
    free(indices);
    realmpal_free_image(pixels);
    
    if (!ok) {
        return REALMPAL_ERROR_WRITE_FAILED;
    }

    return REALMPAL_SUCCESS;
}

// ----------------------------- Helper Functions for Advanced Palette Management -----------------------------

// Helper function: Convert RGB to HSL
static void rgb_to_hsl_palette(RGB8 rgb, double *h, double *s, double *l) {
    double r = rgb.r / 255.0;
    double g = rgb.g / 255.0;
    double b = rgb.b / 255.0;
    
    double max_val = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    double min_val = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    double diff = max_val - min_val;
    
    // Lightness
    *l = (max_val + min_val) / 2.0;
    
    if (diff < 1e-6) {
        *h = *s = 0.0; // Achromatic
    } else {
        // Saturation
        *s = (*l > 0.5) ? diff / (2.0 - max_val - min_val) : diff / (max_val + min_val);
        
        // Hue
        if (max_val == r) {
            *h = (g - b) / diff + (g < b ? 6.0 : 0.0);
        } else if (max_val == g) {
            *h = (b - r) / diff + 2.0;
        } else {
            *h = (r - g) / diff + 4.0;
        }
        *h /= 6.0;
    }
}

// Helper function for HSL to RGB conversion
static double hue_to_rgb_helper_palette(double p, double q, double t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0/2.0) return q;
    if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
    return p;
}

// Helper function: Convert HSL to RGB
static RGB8 hsl_to_rgb_palette(double h, double s, double l) {
    double r, g, b;
    
    if (s < 1e-6) {
        r = g = b = l; // Achromatic
    } else {
        double q = (l < 0.5) ? l * (1.0 + s) : l + s - l * s;
        double p = 2.0 * l - q;
        
        r = hue_to_rgb_helper_palette(p, q, h + 1.0/3.0);
        g = hue_to_rgb_helper_palette(p, q, h);
        b = hue_to_rgb_helper_palette(p, q, h - 1.0/3.0);
    }
    
    return (RGB8){
        (uint8_t)realmpal_clamp_int((int)(r * 255.0 + 0.5), 0, 255),
        (uint8_t)realmpal_clamp_int((int)(g * 255.0 + 0.5), 0, 255),
        (uint8_t)realmpal_clamp_int((int)(b * 255.0 + 0.5), 0, 255)
    };
}

// Helper function: Calculate luminance
static double calculate_luminance_palette(RGB8 color) {
    return 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

// Helper function: Update indices after palette change
static void update_indices_mapping_palette(uint8_t *indices, int pixel_count, const int *remap_table) {
    if (!indices || !remap_table) return;
    
    for (int i = 0; i < pixel_count; i++) {
        indices[i] = (uint8_t)remap_table[indices[i]];
    }
}

// ----------------------------- Advanced Palette Management Implementation -----------------------------

int realmpal_palette_copy_range(RealmpalPaletteContext *ctx, int src_start, int src_end, int dst_start) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (src_start < 0 || src_start > 255 || src_end < 0 || src_end > 255 || src_start > src_end ||
        dst_start < 0 || dst_start > 255) {
        return 0;
    }
    
    int range_size = src_end - src_start + 1;
    if (dst_start + range_size > 256) {
        return 0;
    }
    
    // Copy the range
    memmove(&ctx->palette[dst_start], &ctx->palette[src_start], sizeof(RGB8) * range_size);
    
    return 1;
}

int realmpal_palette_swap_ranges(RealmpalPaletteContext *ctx, int range1_start, int range1_end,
                                int range2_start, int range2_end) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (range1_start < 0 || range1_start > 255 || range1_end < 0 || range1_end > 255 || 
        range2_start < 0 || range2_start > 255 || range2_end < 0 || range2_end > 255 ||
        range1_start > range1_end || range2_start > range2_end) {
        return 0;
    }
    
    int size1 = range1_end - range1_start + 1;
    int size2 = range2_end - range2_start + 1;
    
    // Check for overlap
    if ((range1_start <= range2_end && range1_end >= range2_start)) {
        return 0;
    }
    
    if (size1 != size2) {
        return 0;
    }
    
    // Swap the ranges
    RGB8 *temp = (RGB8*)malloc(sizeof(RGB8) * size1);
    if (!temp) {
        return 0;
    }
    
    memcpy(temp, &ctx->palette[range1_start], sizeof(RGB8) * size1);
    memcpy(&ctx->palette[range1_start], &ctx->palette[range2_start], sizeof(RGB8) * size1);
    memcpy(&ctx->palette[range2_start], temp, sizeof(RGB8) * size1);
    
    // Update indices if requested
    if (ctx->update_indices) {
        int remap_table[256];
        for (int i = 0; i < 256; i++) remap_table[i] = i;
        
        for (int i = 0; i < size1; i++) {
            remap_table[range1_start + i] = range2_start + i;
            remap_table[range2_start + i] = range1_start + i;
        }
        
        update_indices_mapping_palette(ctx->indices, ctx->width * ctx->height, remap_table);
    }
    
    free(temp);
    return 1;
}

int realmpal_palette_reverse_range(RealmpalPaletteContext *ctx, int start, int end) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    int orig_start = start, orig_end = end;
    
    // Reverse the palette range
    while (start < end) {
        RGB8 temp = ctx->palette[start];
        ctx->palette[start] = ctx->palette[end];
        ctx->palette[end] = temp;
        start++;
        end--;
    }
    
    // Update indices if requested
    if (ctx->update_indices) {
        int remap_table[256];
        for (int i = 0; i < 256; i++) remap_table[i] = i;
        
        for (int i = orig_start; i <= orig_end; i++) {
            remap_table[i] = orig_end - (i - orig_start);
        }
        
        update_indices_mapping_palette(ctx->indices, ctx->width * ctx->height, remap_table);
    }
    
    return 1;
}

// Comparison function globals for sorting
static RealmpalSortCriteria current_sort_criteria;
static bool current_sort_ascending;

static int compare_colors_palette(const void *a, const void *b) {
    RGB8 *color_a = (RGB8*)a;
    RGB8 *color_b = (RGB8*)b;
    double val_a, val_b;
    
    switch (current_sort_criteria) {
        case REALMPAL_SORT_LUMINANCE:
            val_a = calculate_luminance_palette(*color_a);
            val_b = calculate_luminance_palette(*color_b);
            break;
        case REALMPAL_SORT_HUE: {
            double h_a, s_a, l_a, h_b, s_b, l_b;
            rgb_to_hsl_palette(*color_a, &h_a, &s_a, &l_a);
            rgb_to_hsl_palette(*color_b, &h_b, &s_b, &l_b);
            val_a = h_a; val_b = h_b;
            break;
        }
        case REALMPAL_SORT_SATURATION: {
            double h_a, s_a, l_a, h_b, s_b, l_b;
            rgb_to_hsl_palette(*color_a, &h_a, &s_a, &l_a);
            rgb_to_hsl_palette(*color_b, &h_b, &s_b, &l_b);
            val_a = s_a; val_b = s_b;
            break;
        }
        case REALMPAL_SORT_RED:
            val_a = color_a->r; val_b = color_b->r;
            break;
        case REALMPAL_SORT_GREEN:
            val_a = color_a->g; val_b = color_b->g;
            break;
        case REALMPAL_SORT_BLUE:
            val_a = color_a->b; val_b = color_b->b;
            break;
        default:
            val_a = val_b = 0;
    }
    
    if (current_sort_ascending) {
        return (val_a < val_b) ? -1 : (val_a > val_b) ? 1 : 0;
    } else {
        return (val_a > val_b) ? -1 : (val_a < val_b) ? 1 : 0;
    }
}

int realmpal_palette_sort_range(RealmpalPaletteContext *ctx, int start, int end,
                               RealmpalSortCriteria criteria, bool ascending) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    // Create array of (color, original_index) pairs for tracking
    typedef struct { RGB8 color; int orig_index; } ColorIndexPair;
    int range_size = end - start + 1;
    ColorIndexPair *pairs = (ColorIndexPair*)malloc(sizeof(ColorIndexPair) * range_size);
    if (!pairs) {
        return 0;
    }
    
    // Fill pairs array
    for (int i = 0; i < range_size; i++) {
        pairs[i].color = ctx->palette[start + i];
        pairs[i].orig_index = start + i;
    }
    
    // Set global sort parameters (not thread-safe, but simple)
    current_sort_criteria = criteria;
    current_sort_ascending = ascending;
    
    // Sort colors only
    RGB8 *colors_only = (RGB8*)malloc(sizeof(RGB8) * range_size);
    if (!colors_only) {
        free(pairs);
        return 0;
    }
    
    for (int i = 0; i < range_size; i++) {
        colors_only[i] = pairs[i].color;
    }
    
    qsort(colors_only, range_size, sizeof(RGB8), compare_colors_palette);
    
    // Copy sorted colors back to palette
    for (int i = 0; i < range_size; i++) {
        ctx->palette[start + i] = colors_only[i];
    }
    
    // Create remap table for indices if needed
    if (ctx->update_indices) {
        int remap_table[256];
        for (int i = 0; i < 256; i++) remap_table[i] = i;
        
        // Find where each original index ended up
        for (int i = 0; i < range_size; i++) {
            RGB8 sorted_color = colors_only[i];
            // Find which original color this was
            for (int j = 0; j < range_size; j++) {
                if (memcmp(&pairs[j].color, &sorted_color, sizeof(RGB8)) == 0) {
                    remap_table[pairs[j].orig_index] = start + i;
                    // Mark as used to avoid duplicates
                    pairs[j].color = (RGB8){255, 255, 255}; // Invalid marker
                    break;
                }
            }
        }
        
        update_indices_mapping_palette(ctx->indices, ctx->width * ctx->height, remap_table);
    }
    
    free(pairs);
    free(colors_only);
    return 1;
}

int realmpal_palette_create_gradient(RealmpalPaletteContext *ctx, int start_index, int end_index,
                                    bool interpolate_through_hsl) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start_index < 0 || start_index > 255 || end_index < 0 || end_index > 255) {
        return 0;
    }
    
    if (start_index == end_index) {
        return 1; // Nothing to do
    }
    
    int step = (start_index < end_index) ? 1 : -1;
    int steps = abs(end_index - start_index);
    
    RGB8 start_color = ctx->palette[start_index];
    RGB8 end_color = ctx->palette[end_index];
    
    if (interpolate_through_hsl) {
        double h1, s1, l1, h2, s2, l2;
        rgb_to_hsl_palette(start_color, &h1, &s1, &l1);
        rgb_to_hsl_palette(end_color, &h2, &s2, &l2);
        
        // Handle hue wraparound
        if (fabs(h2 - h1) > 0.5) {
            if (h1 > h2) h2 += 1.0;
            else h1 += 1.0;
        }
        
        for (int i = 1; i < steps; i++) {
            double t = (double)i / steps;
            double h = h1 + t * (h2 - h1);
            double s = s1 + t * (s2 - s1);
            double l = l1 + t * (l2 - l1);
            
            if (h >= 1.0) h -= 1.0;
            if (h < 0.0) h += 1.0;
            
            int index = start_index + i * step;
            ctx->palette[index] = hsl_to_rgb_palette(h, s, l);
        }
    } else {
        // RGB interpolation
        for (int i = 1; i < steps; i++) {
            double t = (double)i / steps;
            int index = start_index + i * step;
            
            ctx->palette[index] = (RGB8){
                (uint8_t)(start_color.r + t * (end_color.r - start_color.r)),
                (uint8_t)(start_color.g + t * (end_color.g - start_color.g)),
                (uint8_t)(start_color.b + t * (end_color.b - start_color.b))
            };
        }
    }
    
    return 1;
}

int realmpal_palette_adjust_brightness_contrast(RealmpalPaletteContext *ctx, int start, int end,
                                               double brightness, double contrast) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    brightness = realmpal_clamp_double(brightness, -1.0, 1.0);
    contrast = realmpal_clamp_double(contrast, -1.0, 1.0);
    
    double contrast_factor = (contrast >= 0) ? (1.0 + contrast) : (1.0 / (1.0 - contrast));
    
    for (int i = start; i <= end; i++) {
        RGB8 *color = &ctx->palette[i];
        
        // Apply contrast first (around midpoint)
        double r = (color->r / 255.0 - 0.5) * contrast_factor + 0.5;
        double g = (color->g / 255.0 - 0.5) * contrast_factor + 0.5;
        double b = (color->b / 255.0 - 0.5) * contrast_factor + 0.5;
        
        // Apply brightness
        r += brightness;
        g += brightness;
        b += brightness;
        
        // Clamp and convert back
        color->r = (uint8_t)realmpal_clamp_int((int)(r * 255.0), 0, 255);
        color->g = (uint8_t)realmpal_clamp_int((int)(g * 255.0), 0, 255);
        color->b = (uint8_t)realmpal_clamp_int((int)(b * 255.0), 0, 255);
    }
    
    return 1;
}

int realmpal_palette_adjust_hue_saturation(RealmpalPaletteContext *ctx, int start, int end,
                                          double hue_shift, double saturation_factor) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    hue_shift = fmod(hue_shift, 360.0) / 360.0; // Convert to 0-1 range
    saturation_factor = realmpal_clamp_double(saturation_factor, 0.0, 2.0);
    
    for (int i = start; i <= end; i++) {
        double h, s, l;
        rgb_to_hsl_palette(ctx->palette[i], &h, &s, &l);
        
        // Adjust hue
        h += hue_shift;
        if (h >= 1.0) h -= 1.0;
        if (h < 0.0) h += 1.0;
        
        // Adjust saturation
        s *= saturation_factor;
        s = realmpal_clamp_double(s, 0.0, 1.0);
        
        ctx->palette[i] = hsl_to_rgb_palette(h, s, l);
    }
    
    return 1;
}

int realmpal_palette_find_unused(const RGB8 *palette, const uint8_t *indices, int pixel_count,
                                int unused_indices[256]) {
    if (!palette || !unused_indices) {
        return 0;
    }
    
    bool used[256] = {false};
    
    // Mark indices as used if we have pixel data
    if (indices && pixel_count > 0) {
        for (int i = 0; i < pixel_count; i++) {
            used[indices[i]] = true;
        }
    } else {
        // If no pixel data, consider colors that are not black as "used"
        for (int i = 0; i < 256; i++) {
            if (palette[i].r != 0 || palette[i].g != 0 || palette[i].b != 0) {
                used[i] = true;
            }
        }
    }
    
    // Collect unused indices
    int unused_count = 0;
    for (int i = 0; i < 256; i++) {
        if (!used[i]) {
            unused_indices[unused_count++] = i;
        }
    }
    
    return unused_count;
}

int realmpal_palette_compact(RealmpalPaletteContext *ctx, bool preserve_order) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (!ctx->update_indices) {
        return 0;
    }
    
    bool used[256] = {false};
    int pixel_count = ctx->width * ctx->height;
    
    // Mark used colors
    for (int i = 0; i < pixel_count; i++) {
        used[ctx->indices[i]] = true;
    }
    
    // Create remap table
    int remap_table[256];
    int new_index = 0;
    
    if (preserve_order) {
        // Keep original order, just remove gaps
        for (int i = 0; i < 256; i++) {
            if (used[i]) {
                remap_table[i] = new_index++;
            } else {
                remap_table[i] = 0; // Map unused to black
            }
        }
        
        // Compact palette
        new_index = 0;
        for (int i = 0; i < 256; i++) {
            if (used[i]) {
                if (new_index != i) {
                    ctx->palette[new_index] = ctx->palette[i];
                }
                new_index++;
            }
        }
        
        // Clear unused entries
        for (int i = new_index; i < 256; i++) {
            ctx->palette[i] = (RGB8){0, 0, 0};
        }
    } else {
        // Pack efficiently (could reorder colors)
        for (int i = 0; i < 256; i++) {
            if (used[i]) {
                remap_table[i] = new_index;
                ctx->palette[new_index] = ctx->palette[i];
                new_index++;
            } else {
                remap_table[i] = 0;
            }
        }
        
        // Clear unused entries
        for (int i = new_index; i < 256; i++) {
            ctx->palette[i] = (RGB8){0, 0, 0};
        }
    }
    
    // Update indices
    update_indices_mapping_palette(ctx->indices, pixel_count, remap_table);
    
    return new_index;
}

int realmpal_palette_find_closest_color_advanced(const RGB8 *palette, RGB8 color,
                                                bool use_perceptual, int exclude_index) {
    if (!palette) {
        return 0;
    }
    
    int best_index = (exclude_index == 0) ? 1 : 0;
    double best_distance = 1e99;
    
    for (int i = 0; i < 256; i++) {
        if (i == exclude_index) continue;
        
        double distance;
        if (use_perceptual) {
            distance = realmpal_color_distance_perceptual(color, palette[i]);
        } else {
            distance = realmpal_color_distance_rgb(color, palette[i]);
        }
        
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }
    
    return best_index;
}

int realmpal_palette_replace_color(RealmpalPaletteContext *ctx, int old_index, RGB8 new_color) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (old_index < 0 || old_index > 255) {
        return 0;
    }
    
    ctx->palette[old_index] = new_color;
    // Note: indices don't need updating since we're just changing the color value
    
    return 1;
}

int realmpal_palette_merge_similar(RealmpalPaletteContext *ctx, int start, int end,
                                  double threshold, bool use_perceptual) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    threshold = realmpal_clamp_double(threshold, 0.0, 1.0);
    threshold *= (use_perceptual ? 1.0 : 255.0); // Scale threshold appropriately
    
    bool merged[256] = {false};
    int remap_table[256];
    for (int i = 0; i < 256; i++) remap_table[i] = i;
    
    int colors_remaining = end - start + 1;
    
    for (int i = start; i <= end; i++) {
        if (merged[i]) continue;
        
        for (int j = i + 1; j <= end; j++) {
            if (merged[j]) continue;
            
            double distance;
            if (use_perceptual) {
                distance = realmpal_color_distance_perceptual(ctx->palette[i], ctx->palette[j]);
            } else {
                distance = realmpal_color_distance_rgb(ctx->palette[i], ctx->palette[j]);
            }
            
            if (distance <= threshold) {
                // Merge j into i
                remap_table[j] = i;
                merged[j] = true;
                colors_remaining--;
                
                // Average the colors
                ctx->palette[i] = (RGB8){
                    (uint8_t)((ctx->palette[i].r + ctx->palette[j].r) / 2),
                    (uint8_t)((ctx->palette[i].g + ctx->palette[j].g) / 2),
                    (uint8_t)((ctx->palette[i].b + ctx->palette[j].b) / 2)
                };
            }
        }
    }
    
    // Update indices if requested
    if (ctx->update_indices) {
        update_indices_mapping_palette(ctx->indices, ctx->width * ctx->height, remap_table);
    }
    
    return colors_remaining;
}

int realmpal_palette_extract_subpalette(const RGB8 *source_palette, const int *indices_to_extract,
                                       int count, RGB8 *output_palette, int *remap_table) {
    if (!source_palette || !indices_to_extract || !output_palette || count <= 0) {
        return 0;
    }
    
    // Clear output palette
    memset(output_palette, 0, sizeof(RGB8) * 256);
    
    // Create remap table
    if (remap_table) {
        for (int i = 0; i < 256; i++) remap_table[i] = 0; // Default to index 0
    }
    
    // Extract colors
    for (int i = 0; i < count && i < 256; i++) {
        int src_index = indices_to_extract[i];
        if (src_index >= 0 && src_index < 256) {
            output_palette[i] = source_palette[src_index];
            if (remap_table) {
                remap_table[src_index] = i;
            }
        }
    }
    
    return (count < 256) ? count : 256;
}

int realmpal_palette_blend(const RGB8 *palette1, const RGB8 *palette2, RGB8 *output_palette,
                          double blend_factor, RealmpalBlendMode blend_mode) {
    if (!palette1 || !palette2 || !output_palette) {
        return 0;
    }
    
    blend_factor = realmpal_clamp_double(blend_factor, 0.0, 1.0);
    
    for (int i = 0; i < 256; i++) {
        RGB8 c1 = palette1[i];
        RGB8 c2 = palette2[i];
        RGB8 result;
        
        switch (blend_mode) {
            case REALMPAL_BLEND_LINEAR:
                result = (RGB8){
                    (uint8_t)(c1.r + blend_factor * (c2.r - c1.r)),
                    (uint8_t)(c1.g + blend_factor * (c2.g - c1.g)),
                    (uint8_t)(c1.b + blend_factor * (c2.b - c1.b))
                };
                break;
                
            case REALMPAL_BLEND_HSL: {
                double h1, s1, l1, h2, s2, l2;
                rgb_to_hsl_palette(c1, &h1, &s1, &l1);
                rgb_to_hsl_palette(c2, &h2, &s2, &l2);
                
                // Handle hue wraparound
                if (fabs(h2 - h1) > 0.5) {
                    if (h1 > h2) h2 += 1.0;
                    else h1 += 1.0;
                }
                
                double h = h1 + blend_factor * (h2 - h1);
                double s = s1 + blend_factor * (s2 - s1);
                double l = l1 + blend_factor * (l2 - l1);
                
                if (h >= 1.0) h -= 1.0;
                if (h < 0.0) h += 1.0;
                
                result = hsl_to_rgb_palette(h, s, l);
                break;
            }
            
            case REALMPAL_BLEND_OVERLAY:
                result = (RGB8){
                    (uint8_t)realmpal_clamp_int((c1.r < 128) ? 
                        (2 * c1.r * c2.r) / 255 : 
                        255 - (2 * (255 - c1.r) * (255 - c2.r)) / 255, 0, 255),
                    (uint8_t)realmpal_clamp_int((c1.g < 128) ? 
                        (2 * c1.g * c2.g) / 255 : 
                        255 - (2 * (255 - c1.g) * (255 - c2.g)) / 255, 0, 255),
                    (uint8_t)realmpal_clamp_int((c1.b < 128) ? 
                        (2 * c1.b * c2.b) / 255 : 
                        255 - (2 * (255 - c1.b) * (255 - c2.b)) / 255, 0, 255)
                };
                result = (RGB8){
                    (uint8_t)(c1.r + blend_factor * (result.r - c1.r)),
                    (uint8_t)(c1.g + blend_factor * (result.g - c1.g)),
                    (uint8_t)(c1.b + blend_factor * (result.b - c1.b))
                };
                break;
                
            case REALMPAL_BLEND_MULTIPLY:
                result = (RGB8){
                    (uint8_t)realmpal_clamp_int((c1.r * c2.r) / 255, 0, 255),
                    (uint8_t)realmpal_clamp_int((c1.g * c2.g) / 255, 0, 255),
                    (uint8_t)realmpal_clamp_int((c1.b * c2.b) / 255, 0, 255)
                };
                result = (RGB8){
                    (uint8_t)(c1.r + blend_factor * (result.r - c1.r)),
                    (uint8_t)(c1.g + blend_factor * (result.g - c1.g)),
                    (uint8_t)(c1.b + blend_factor * (result.b - c1.b))
                };
                break;
                
            default:
                result = c1; // Fallback
        }
        
        output_palette[i] = result;
    }
    
    return 1;
}

// ----------------------------- Palette Analysis Implementation -----------------------------

int realmpal_palette_analyze(const RGB8 *palette, const uint8_t *indices, int pixel_count,
                            RealmpalPaletteStats *stats) {
    if (!palette || !stats) {
        return 0;
    }
    
    memset(stats, 0, sizeof(RealmpalPaletteStats));
    
    // Count unique colors
    bool seen[256] = {false};
    int usage_count[256] = {0};
    
    // Analyze usage if indices provided
    if (indices && pixel_count > 0) {
        for (int i = 0; i < pixel_count; i++) {
            usage_count[indices[i]]++;
            seen[indices[i]] = true;
        }
        
        // Find most used color
        int max_usage = 0;
        for (int i = 0; i < 256; i++) {
            if (usage_count[i] > max_usage) {
                max_usage = usage_count[i];
                stats->dominant_color = palette[i];
            }
            if (usage_count[i] > 0) {
                stats->used_colors++;
            }
        }
    } else {
        // No usage data, analyze palette directly
        for (int i = 0; i < 256; i++) {
            if (palette[i].r != 0 || palette[i].g != 0 || palette[i].b != 0) {
                seen[i] = true;
            }
        }
        stats->dominant_color = palette[0];
    }
    
    // Count unique colors and calculate statistics
    double total_luminance = 0.0;
    double min_luminance = 255.0, max_luminance = 0.0;
    double max_saturation = 0.0;
    long total_r = 0, total_g = 0, total_b = 0;
    
    for (int i = 0; i < 256; i++) {
        if (seen[i]) {
            stats->unique_colors++;
            
            // Luminance analysis
            double lum = calculate_luminance_palette(palette[i]);
            total_luminance += lum;
            
            if (lum < min_luminance) {
                min_luminance = lum;
                stats->darkest_index = i;
            }
            if (lum > max_luminance) {
                max_luminance = lum;
                stats->brightest_index = i;
            }
            
            // Saturation analysis
            double h, s, l;
            rgb_to_hsl_palette(palette[i], &h, &s, &l);
            if (s > max_saturation) {
                max_saturation = s;
                stats->most_saturated_index = i;
            }
            
            // Color averages
            total_r += palette[i].r;
            total_g += palette[i].g;
            total_b += palette[i].b;
        }
    }
    
    if (stats->unique_colors > 0) {
        stats->average_luminance = total_luminance / stats->unique_colors;
        stats->average_color = (RGB8){
            (uint8_t)(total_r / stats->unique_colors),
            (uint8_t)(total_g / stats->unique_colors),
            (uint8_t)(total_b / stats->unique_colors)
        };
        
        // Calculate color variance
        double variance_sum = 0.0;
        for (int i = 0; i < 256; i++) {
            if (seen[i]) {
                double lum = calculate_luminance_palette(palette[i]);
                double diff = lum - stats->average_luminance;
                variance_sum += diff * diff;
            }
        }
        stats->color_variance = variance_sum / stats->unique_colors;
    }
    
    return 1;
}

int realmpal_palette_get_histogram(const uint8_t *indices, int pixel_count, int histogram[256]) {
    if (!indices || !histogram || pixel_count <= 0) {
        return 0;
    }
    
    // Clear histogram
    memset(histogram, 0, sizeof(int) * 256);
    
    // Count occurrences
    for (int i = 0; i < pixel_count; i++) {
        histogram[indices[i]]++;
    }
    
    return 1;
}

// ----------------------------- Enhanced Shift Range Implementation -----------------------------

int realmpal_palette_shift_range(RealmpalPaletteContext *ctx, int start, int end, int new_start) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end ||
        new_start < 0 || new_start > 255) {
        return 0;
    }
    
    int range_size = end - start + 1;
    if (new_start + range_size > 256) {
        return 0;
    }
    
    // Create temporary copy of the range
    RGB8 *temp = (RGB8*)malloc(sizeof(RGB8) * range_size);
    if (!temp) {
        return 0;
    }
    
    // Copy the range to temp
    memcpy(temp, &ctx->palette[start], sizeof(RGB8) * range_size);
    
    // Create remap table for indices
    int remap_table[256];
    for (int i = 0; i < 256; i++) remap_table[i] = i;
    
    if (new_start < start) {
        // Shifting left - move intervening colors right
        memmove(&ctx->palette[new_start + range_size], &ctx->palette[new_start], 
                sizeof(RGB8) * (start - new_start));
        
        // Update remap table
        for (int i = start; i <= end; i++) {
            remap_table[i] = new_start + (i - start);
        }
        for (int i = new_start; i < start; i++) {
            remap_table[i] = i + range_size;
        }
    } else if (new_start > start) {
        // Shifting right - move intervening colors left
        memmove(&ctx->palette[start], &ctx->palette[end + 1], 
                sizeof(RGB8) * (new_start - end - 1));
        
        // Update remap table
        for (int i = start; i <= end; i++) {
            remap_table[i] = new_start + (i - start);
        }
        for (int i = end + 1; i < new_start + range_size; i++) {
            remap_table[i] = i - range_size;
        }
    }
    
    // Place the range at new position
    memcpy(&ctx->palette[new_start], temp, sizeof(RGB8) * range_size);
    
    // Update indices if requested
    if (ctx->update_indices) {
        update_indices_mapping_palette(ctx->indices, ctx->width * ctx->height, remap_table);
    }
    
    free(temp);
    return 1;
}

// ----------------------------- Additional Utility Functions -----------------------------

/**
 * Convert a palette to grayscale using luminance weights
 */
int realmpal_palette_to_grayscale(RealmpalPaletteContext *ctx, int start, int end) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    for (int i = start; i <= end; i++) {
        RGB8 *color = &ctx->palette[i];
        uint8_t gray = (uint8_t)calculate_luminance_palette(*color);
        color->r = color->g = color->b = gray;
    }
    
    return 1;
}

/**
 * Apply sepia tone effect to a palette range
 */
int realmpal_palette_sepia_tone(RealmpalPaletteContext *ctx, int start, int end, double intensity) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    intensity = realmpal_clamp_double(intensity, 0.0, 1.0);
    
    for (int i = start; i <= end; i++) {
        RGB8 *color = &ctx->palette[i];
        
        // Calculate sepia values
        double r = color->r;
        double g = color->g;
        double b = color->b;
        
        double sepia_r = (r * 0.393) + (g * 0.769) + (b * 0.189);
        double sepia_g = (r * 0.349) + (g * 0.686) + (b * 0.168);
        double sepia_b = (r * 0.272) + (g * 0.534) + (b * 0.131);
        
        // Blend with original
        color->r = (uint8_t)realmpal_clamp_int((int)(r + intensity * (sepia_r - r)), 0, 255);
        color->g = (uint8_t)realmpal_clamp_int((int)(g + intensity * (sepia_g - g)), 0, 255);
        color->b = (uint8_t)realmpal_clamp_int((int)(b + intensity * (sepia_b - b)), 0, 255);
    }
    
    return 1;
}

/**
 * Create a color ramp between multiple colors
 */
int realmpal_palette_create_multi_gradient(RealmpalPaletteContext *ctx, 
                                          const int *key_indices, int num_keys,
                                          bool use_hsl) {
    if (!ctx || !ctx->palette || !key_indices || num_keys < 2) {
        return 0;
    }
    
    // Validate key indices
    for (int i = 0; i < num_keys; i++) {
        if (key_indices[i] < 0 || key_indices[i] > 255) {
            return 0;
        }
        if (i > 0 && key_indices[i] <= key_indices[i-1]) {
            return 0; // Must be in ascending order
        }
    }
    
    // Create gradients between consecutive key colors
    for (int i = 0; i < num_keys - 1; i++) {
        int start_idx = key_indices[i];
        int end_idx = key_indices[i + 1];
        
        if (!realmpal_palette_create_gradient(ctx, start_idx, end_idx, use_hsl)) {
            return 0;
        }
    }
    
    return 1;
}

/**
 * Apply a temperature adjustment (warm/cool)
 */
int realmpal_palette_adjust_temperature(RealmpalPaletteContext *ctx, int start, int end, 
                                       double temperature) {
    if (!ctx || !ctx->palette) {
        return 0;
    }
    
    if (start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    temperature = realmpal_clamp_double(temperature, -1.0, 1.0);
    
    for (int i = start; i <= end; i++) {
        RGB8 *color = &ctx->palette[i];
        
        if (temperature > 0) {
            // Warmer (more red/yellow)
            color->r = (uint8_t)realmpal_clamp_int((int)(color->r + temperature * (255 - color->r) * 0.3), 0, 255);
            color->g = (uint8_t)realmpal_clamp_int((int)(color->g + temperature * (255 - color->g) * 0.1), 0, 255);
        } else if (temperature < 0) {
            // Cooler (more blue)
            double cool_factor = -temperature;
            color->b = (uint8_t)realmpal_clamp_int((int)(color->b + cool_factor * (255 - color->b) * 0.3), 0, 255);
            color->r = (uint8_t)realmpal_clamp_int((int)(color->r - cool_factor * color->r * 0.1), 0, 255);
        }
    }
    
    return 1;
}

/**
 * Generate a random palette within specified constraints
 */
int realmpal_palette_generate_random(RGB8 *palette, int start, int end,
                                    double min_saturation, double max_saturation,
                                    double min_lightness, double max_lightness) {
    if (!palette || start < 0 || start > 255 || end < 0 || end > 255 || start > end) {
        return 0;
    }
    
    min_saturation = realmpal_clamp_double(min_saturation, 0.0, 1.0);
    max_saturation = realmpal_clamp_double(max_saturation, 0.0, 1.0);
    min_lightness = realmpal_clamp_double(min_lightness, 0.0, 1.0);
    max_lightness = realmpal_clamp_double(max_lightness, 0.0, 1.0);
    
    for (int i = start; i <= end; i++) {
        double h = (double)rand() / RAND_MAX;  // Random hue
        double s = min_saturation + ((double)rand() / RAND_MAX) * (max_saturation - min_saturation);
        double l = min_lightness + ((double)rand() / RAND_MAX) * (max_lightness - min_lightness);
        
        palette[i] = hsl_to_rgb_palette(h, s, l);
    }
    
    return 1;
}

/**
 * Create a palette based on color harmony rules
 */
int realmpal_palette_create_harmony(RGB8 *palette, int start, RGB8 base_color, 
                                   int harmony_type, int num_colors) {
    if (!palette || start < 0 || start > 255 || num_colors <= 0 || 
        start + num_colors > 256) {
        return 0;
    }
    
    double base_h, base_s, base_l;
    rgb_to_hsl_palette(base_color, &base_h, &base_s, &base_l);
    
    for (int i = 0; i < num_colors; i++) {
        double h = base_h;
        double s = base_s;
        double l = base_l;
        
        switch (harmony_type) {
            case 0: // Monochromatic - vary lightness
                l = 0.2 + (0.6 * i) / (num_colors - 1);
                break;
            case 1: // Analogous - nearby hues
                h = base_h + (i * 60.0 / 360.0) / num_colors;
                if (h > 1.0) h -= 1.0;
                break;
            case 2: // Complementary - opposite hues
                if (i % 2 == 1) {
                    h = base_h + 0.5;
                    if (h > 1.0) h -= 1.0;
                }
                break;
            case 3: // Triadic - 120° apart
                h = base_h + (i * 120.0 / 360.0);
                if (h > 1.0) h -= 1.0;
                break;
            case 4: // Split-complementary
                if (i == 1) h = base_h + 150.0/360.0;
                else if (i == 2) h = base_h + 210.0/360.0;
                if (h > 1.0) h -= 1.0;
                break;
        }
        
        palette[start + i] = hsl_to_rgb_palette(h, s, l);
    }
    
    return num_colors;
}

// ----------------------------- Index Constraint Utility Functions -----------------------------

int realmpal_validate_index_constraints(int *min_index, int *max_index, int transparency_index) {
    if (!min_index || !max_index) return 0;
    
    int original_start = *min_index;
    int original_end = *max_index;
    bool adjusted = false;
    
    // Handle no constraints
    if (*min_index < 0 && *max_index < 0) {
        *min_index = 0;
        *max_index = 255;
        return 1;
    }
    
    // Clamp to valid range
    if (*min_index < 0) *min_index = 0;
    if (*max_index < 0) *max_index = 255;
    if (*min_index > 255) { *min_index = 255; adjusted = true; }
    if (*max_index > 255) { *max_index = 255; adjusted = true; }
    
    // Ensure start <= end
    if (*min_index > *max_index) {
        int temp = *min_index;
        *min_index = *max_index;
        *max_index = temp;
        adjusted = true;
    }
    
    return (original_start == *min_index && original_end == *max_index) ? 1 : 0;
}

int realmpal_count_available_indices(int min_index, int max_index, int transparency_index) {
    if (min_index < 0) min_index = 0;
    if (max_index < 0) max_index = 255;
    if (min_index > max_index) return 0;
    
    int total = max_index - min_index + 1;
    
    // Subtract transparency index if it's in range
    if (transparency_index >= min_index && transparency_index <= max_index) {
        total--;
    }
    
    return total;
}

// Enhanced function to find best index with constraints
static int best_index_with_constraints(uint8_t r, uint8_t g, uint8_t b,
                                      const RGB8 *pal, int transparency_index,
                                      int min_index, int max_index, bool use_perceptual) {
    // Set up constraints
    if (min_index < 0) min_index = 0;
    if (max_index < 0) max_index = 255;
    if (min_index > max_index) {
        int temp = min_index;
        min_index = max_index;
        max_index = temp;
    }
    
    int best = -1;
    double best_distance = 1e99;
    
    if (use_perceptual) {
        init_perceptual_lut();
        double R = perceptual_lut[r];
        double G = perceptual_lut[g];
        double B = perceptual_lut[b];
        
        for (int i = min_index; i <= max_index; i++) {
            // Skip transparency index
            if (transparency_index >= 0 && i == transparency_index) continue;
            
            double pr = perceptual_lut[pal[i].r];
            double pg = perceptual_lut[pal[i].g];
            double pb = perceptual_lut[pal[i].b];
            double dr = R - pr, dg = G - pg, db = B - pb;
            double d = 0.2126*dr*dr + 0.7152*dg*dg + 0.0722*db*db;
            
            if (best == -1 || d < best_distance) {
                best_distance = d;
                best = i;
            }
        }
    } else {
        for (int i = min_index; i <= max_index; i++) {
            // Skip transparency index
            if (transparency_index >= 0 && i == transparency_index) continue;
            
            int dr = (int)r - pal[i].r;
            int dg = (int)g - pal[i].g;
            int db = (int)b - pal[i].b;
            double d = dr*dr + dg*dg + db*db;
            
            if (best == -1 || d < best_distance) {
                best_distance = d;
                best = i;
            }
        }
    }
    
    // Fallback if no valid index found
    if (best == -1) {
        // Find any valid index in range (excluding transparency)
        for (int i = min_index; i <= max_index; i++) {
            if (transparency_index >= 0 && i == transparency_index) continue;
            best = i;
            break;
        }
        
        // Last resort - use min_index if it's not transparency
        if (best == -1) {
            if (transparency_index != min_index) {
                best = min_index;
            } else if (min_index < max_index && transparency_index != min_index + 1) {
                best = min_index + 1;
            } else {
                best = 0; // Ultimate fallback
            }
        }
    }
    
    return best;
}

// ----------------------------- Constrained Mapping Functions -----------------------------

void realmpal_map_nearest_neighbor_constrained(const RGBA8* src, int w, int h, const RGB8* palette, 
                                              uint8_t *out, int transparency_index, int alpha_threshold,
                                              int min_index, int max_index) {
    if (!src || !palette || !out || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_map_nearest_neighbor_constrained");
        return;
    }
    
    // Validate constraints
    realmpal_validate_index_constraints(&min_index, &max_index, transparency_index);
    
    for (int y = 0; y < h; y++) {
        const RGBA8* s = src + (size_t)y * w;
        uint8_t* d = out + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                d[x] = (uint8_t)transparency_index;
            } else {
                d[x] = (uint8_t)best_index_with_constraints(s[x].r, s[x].g, s[x].b, palette, 
                                                           transparency_index, min_index, max_index, false);
            }
        }
    }
    clear_error_internal();
}

void realmpal_map_floyd_steinberg_constrained(const RGBA8* src, int w, int h, const RGB8* palette, 
                                             uint8_t *out, bool serpentine, double strength,
                                             int transparency_index, int alpha_threshold,
                                             int min_index, int max_index) {
    if (!src || !palette || !out || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_map_floyd_steinberg_constrained");
        return;
    }
    
    // Validate constraints
    realmpal_validate_index_constraints(&min_index, &max_index, transparency_index);
    
    // Allocate error buffers
    double *er = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *eg = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *eb = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *nr = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *ng = (double*)calloc((size_t)(w + 2), sizeof(double));
    double *nb = (double*)calloc((size_t)(w + 2), sizeof(double));
    
    if (!er || !eg || !eb || !nr || !ng || !nb) {
        free(er); free(eg); free(eb); free(nr); free(ng); free(nb);
        set_error("Out of memory in Floyd-Steinberg dithering");
        return;
    }

    const double ERR_CLAMP = 64.0 * strength;
    const double fs_7_16 = 7.0/16.0 * strength;
    const double fs_3_16 = 3.0/16.0 * strength;
    const double fs_5_16 = 5.0/16.0 * strength;
    const double fs_1_16 = 1.0/16.0 * strength;

    for (int y = 0; y < h; y++) {
        // Clear next row errors
        for (int i = 0; i < w + 2; i++) {
            nr[i] = ng[i] = nb[i] = 0.0;
        }

        const RGBA8* s = src + (size_t)y * w;
        uint8_t* d = out + (size_t)y * w;

        if (!serpentine || (y % 2 == 0)) {
            // Left to right
            for (int x = 0; x < w; x++) {
                if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                    d[x] = (uint8_t)transparency_index;
                    continue;
                }
                
                double R = s[x].r + er[x];
                double G = s[x].g + eg[x];
                double B = s[x].b + eb[x];
                
                R = realmpal_clamp_double(R, 0.0, 255.0);
                G = realmpal_clamp_double(G, 0.0, 255.0);
                B = realmpal_clamp_double(B, 0.0, 255.0);

                int idx = best_index_with_constraints((uint8_t)R, (uint8_t)G, (uint8_t)B, palette, 
                                                     transparency_index, min_index, max_index, true);
                d[x] = (uint8_t)idx;

                double dr = R - palette[idx].r;
                double dg = G - palette[idx].g;
                double db = B - palette[idx].b;

                dr = realmpal_clamp_double(dr, -ERR_CLAMP, ERR_CLAMP);
                dg = realmpal_clamp_double(dg, -ERR_CLAMP, ERR_CLAMP);
                db = realmpal_clamp_double(db, -ERR_CLAMP, ERR_CLAMP);

                er[x + 1] += dr * fs_7_16; eg[x + 1] += dg * fs_7_16; eb[x + 1] += db * fs_7_16;
                
                int xm1 = (x > 0) ? x - 1 : x;
                nr[xm1] += dr * fs_3_16; ng[xm1] += dg * fs_3_16; nb[xm1] += db * fs_3_16;
                nr[x] += dr * fs_5_16; ng[x] += dg * fs_5_16; nb[x] += db * fs_5_16;
                nr[x + 1] += dr * fs_1_16; ng[x + 1] += dg * fs_1_16; nb[x + 1] += db * fs_1_16;
            }
        } else {
            // Right to left (serpentine)
            for (int x = w - 1; x >= 0; x--) {
                if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                    d[x] = (uint8_t)transparency_index;
                    continue;
                }
                
                double R = s[x].r + er[x];
                double G = s[x].g + eg[x];
                double B = s[x].b + eb[x];
                
                R = realmpal_clamp_double(R, 0.0, 255.0);
                G = realmpal_clamp_double(G, 0.0, 255.0);
                B = realmpal_clamp_double(B, 0.0, 255.0);

                int idx = best_index_with_constraints((uint8_t)R, (uint8_t)G, (uint8_t)B, palette, 
                                                     transparency_index, min_index, max_index, true);
                d[x] = (uint8_t)idx;

                double dr = R - palette[idx].r;
                double dg = G - palette[idx].g;
                double db = B - palette[idx].b;

                dr = realmpal_clamp_double(dr, -ERR_CLAMP, ERR_CLAMP);
                dg = realmpal_clamp_double(dg, -ERR_CLAMP, ERR_CLAMP);
                db = realmpal_clamp_double(db, -ERR_CLAMP, ERR_CLAMP);

                if (x - 1 >= 0) {
                    er[x - 1] += dr * fs_7_16; eg[x - 1] += dg * fs_7_16; eb[x - 1] += db * fs_7_16;
                } else {
                    er[0] += dr * fs_7_16; eg[0] += dg * fs_7_16; eb[0] += db * fs_7_16;
                }

                int xp1 = (x < w - 1) ? x + 1 : x;
                nr[xp1] += dr * fs_3_16; ng[xp1] += dg * fs_3_16; nb[xp1] += db * fs_3_16;
                nr[x] += dr * fs_5_16; ng[x] += dg * fs_5_16; nb[x] += db * fs_5_16;
                int xm1 = (x > 0) ? x - 1 : 0;
                nr[xm1] += dr * fs_1_16; ng[xm1] += dg * fs_1_16; nb[xm1] += db * fs_1_16;
            }
        }

        // Swap error buffers
        double *temp_r = er; er = nr; nr = temp_r;
        double *temp_g = eg; eg = ng; ng = temp_g;
        double *temp_b = eb; eb = nb; nb = temp_b;
    }

    free(er); free(eg); free(eb); free(nr); free(ng); free(nb);
    clear_error_internal();
}

void realmpal_map_perceptual_constrained(const RGBA8* src, int w, int h, const RGB8* palette,
                                        uint8_t *out, int transparency_index, int alpha_threshold,
                                        int min_index, int max_index) {
    if (!src || !palette || !out || w <= 0 || h <= 0) {
        set_error("Invalid arguments to realmpal_map_perceptual_constrained");
        return;
    }
    
    // Validate constraints
    realmpal_validate_index_constraints(&min_index, &max_index, transparency_index);
    
    for (int y = 0; y < h; y++) {
        const RGBA8* s = src + (size_t)y * w;
        uint8_t* d = out + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            if (transparency_index >= 0 && s[x].a < alpha_threshold) {
                d[x] = (uint8_t)transparency_index;
            } else {
                d[x] = (uint8_t)best_index_with_constraints(s[x].r, s[x].g, s[x].b, palette, 
                                                           transparency_index, min_index, max_index, true);
            }
        }
    }
    clear_error_internal();
}