/**
 * librealmpal.h - Universal Image to 8-bit Palette Library
 * 
 * A comprehensive library for converting images to 8-bit indexed color formats
 * with advanced quantization, dithering, and palette management features.
 * 
 * Features:
 * - Wu and Median-cut quantization algorithms
 * - Multiple dithering modes (Floyd-Steinberg, serpentine, perceptual, etc.)
 * - External palette loading from BMP, PNG, PCX files
 * - Transparency handling with configurable alpha thresholds
 * - Performance optimizations with caching and lookup tables
 * - Support for BMP and PCX output formats
 * 
 * Usage:
 *   #include "librealmpal.h"
 *   // Initialize library (optional, called automatically)
 *   realmpal_init();
 *   
 *   // Load image
 *   RGBA8 *pixels;
 *   int w, h;
 *   if (!realmpal_load_image("input.png", &pixels, &w, &h)) {
 *       // Handle error
 *   }
 *   
 *   // Quantize to palette
 *   RGB8 palette[256];
 *   int colors = realmpal_quantize_wu(pixels, w * h, 256, 0, palette);
 *   
 *   // Apply dithering and map to indices
 *   uint8_t *indices = malloc(w * h);
 *   realmpal_map_floyd_steinberg(pixels, w, h, palette, indices, false, 1.0, -1, 128);
 *   
 *   // Save result
 *   realmpal_write_bmp8("output.bmp", w, h, indices, palette);
 *   
 *   // Cleanup
 *   realmpal_free_image(pixels);
 *   free(indices);
 */

#ifndef LIBREALMPAL_H
#define LIBREALMPAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------- Version Information -----------------------------
#define REALMPAL_VERSION_MAJOR 1
#define REALMPAL_VERSION_MINOR 0
#define REALMPAL_VERSION_PATCH 0
#define REALMPAL_VERSION_STRING "1.0.0"

// ----------------------------- Core Types -----------------------------

/**
 * RGBA8 - 8-bit RGBA pixel structure
 */
typedef struct { 
    uint8_t r, g, b, a; 
} RGBA8;

/**
 * RGB8 - 8-bit RGB color structure (used for palettes)
 */
typedef struct { 
    uint8_t r, g, b; 
} RGB8;

/**
 * Single index range or constraint
 */
typedef struct {
    int start;              /**< Start index (for single index, start == end) */
    int end;                /**< End index (for single index, start == end) */
} RealmpalIndexRange;

/**
 * Collection of index constraints for quantization
 */
typedef struct {
    RealmpalIndexRange ranges[32];  /**< Array of allowed ranges/indices (max 32) */
    int count;                      /**< Number of ranges/indices specified */
    bool enabled;                   /**< Whether constraints are active */
} RealmpalIndexConstraints;

/**
 * Dithering algorithms supported by the library
 */
typedef enum { 
    REALMPAL_DITHER_NONE = 0,      /**< No dithering (nearest neighbor) */
    REALMPAL_DITHER_FS = 1,        /**< Floyd-Steinberg error diffusion */
    REALMPAL_DITHER_FS_SERP = 2,   /**< Floyd-Steinberg with serpentine scanning */
    REALMPAL_DITHER_ORDERED = 3,   /**< Ordered (Bayer matrix) dithering */
    REALMPAL_DITHER_PERCEPTUAL = 4,/**< Perceptually weighted nearest neighbor */
    REALMPAL_DITHER_BLUE = 5       /**< Blue noise dithering */
} RealmpalDitherMode;

/**
 * Color quantization algorithms
 */
typedef enum { 
    REALMPAL_QUANT_WU = 0,      /**< Wu's color quantizer (default, best quality) */
    REALMPAL_QUANT_MEDIAN = 1   /**< Median-cut quantizer (faster) */
} RealmpalQuantMode;

/**
 * Output file formats
 */
typedef enum { 
    REALMPAL_FORMAT_BMP,        /**< Windows Bitmap (.bmp) */
    REALMPAL_FORMAT_PCX         /**< ZSoft PCX (.pcx) */
} RealmpalOutputFormat;

/**
 * Library operation mode
 */
typedef enum { 
    REALMPAL_MODE_AUTO = 0,     /**< Automatic quantization */
    REALMPAL_MODE_PALETTE = 1   /**< Use external palette */
} RealmpalOpMode;

// ----------------------------- Library Initialization -----------------------------

/**
 * Initialize the realmpal library.
 * This function sets up internal lookup tables and caches for optimal performance.
 * It's called automatically by other functions but can be called explicitly.
 * 
 * @return 1 on success, 0 on failure
 */
int realmpal_init(void);

/**
 * Get library version information
 * 
 * @param major Pointer to store major version (can be NULL)
 * @param minor Pointer to store minor version (can be NULL) 
 * @param patch Pointer to store patch version (can be NULL)
 * @return Version string (e.g., "1.0.0")
 */
const char* realmpal_get_version(int *major, int *minor, int *patch);

// ----------------------------- Image Loading and Management -----------------------------

/**
 * Load an image file (PNG, BMP, etc.) into RGBA8 format.
 * Uses stb_image internally to support multiple formats.
 * 
 * @param path Path to image file
 * @param out_pixels Pointer to store allocated pixel array (caller must free with realmpal_free_image)
 * @param w Pointer to store image width
 * @param h Pointer to store image height
 * @return 1 on success, 0 on failure
 */
int realmpal_load_image(const char *path, RGBA8 **out_pixels, int *w, int *h);

/**
 * Free memory allocated by realmpal_load_image.
 * 
 * @param pixels Pointer returned by realmpal_load_image
 */
void realmpal_free_image(RGBA8 *pixels);

/**
 * Apply transparency handling to an image.
 * Replaces transparent pixels (alpha < threshold) with the specified matte color.
 * 
 * @param pixels Image pixel array
 * @param count Number of pixels in array
 * @param matte_color Color to use for transparent pixels
 * @param alpha_threshold Alpha value threshold (0-255)
 */
void realmpal_apply_transparency(RGBA8* pixels, int count, RGB8 matte_color, int alpha_threshold);

// ----------------------------- Palette Management -----------------------------

/**
 * Load palette from a BMP file.
 * Reads the color table from an 8-bit or lower BMP file.
 * 
 * @param path Path to BMP file
 * @param out_palette Output palette array (256 entries)
 * @return Number of colors loaded, or 0 on failure
 */
int realmpal_read_bmp_palette(const char* path, RGB8 out_palette[256]);

/**
 * Load palette from a PNG PLTE chunk.
 * Extracts the palette from a PNG file's PLTE chunk.
 * 
 * @param path Path to PNG file
 * @param out_palette Output palette array (256 entries)
 * @return Number of colors loaded, or 0 on failure
 */
int realmpal_read_png_palette(const char* path, RGB8 out_palette[256]);

/**
 * Load palette from a PCX file.
 * Reads the 256-color palette from the end of a PCX file.
 * 
 * @param path Path to PCX file
 * @param out_palette Output palette array (256 entries)
 * @return Number of colors loaded (256), or 0 on failure
 */
int realmpal_read_pcx_palette(const char* path, RGB8 out_palette[256]);

/**
 * Load palette from any supported image format.
 * This function tries multiple methods:
 * 1. Read palette directly if the image has one
 * 2. Quantize the image colors if it's a full-color image
 * 
 * @param path Path to image file
 * @param out_palette Output palette array (256 entries)
 * @param max_colors Maximum number of colors to extract/quantize
 * @return Number of colors loaded, or 0 on failure
 */
int realmpal_read_any_palette(const char* path, RGB8 out_palette[256], int max_colors);

// ----------------------------- Advanced Palette Management -----------------------------

/**
 * Palette operation structure for complex operations
 */
typedef struct {
    RGB8 *palette;              /**< Target palette (256 entries) */
    uint8_t *indices;           /**< Optional index array to update (can be NULL) */
    int width, height;          /**< Dimensions if indices provided */
    bool update_indices;        /**< Whether to update index array */
} RealmpalPaletteContext;

/**
 * Initialize a palette context for operations
 * 
 * @param ctx Context structure to initialize
 * @param palette Palette array (256 entries, required)
 * @param indices Index array to update (optional, can be NULL)
 * @param width Image width (if indices provided)
 * @param height Image height (if indices provided)
 */
void realmpal_palette_context_init(RealmpalPaletteContext *ctx, RGB8 *palette, 
                                  uint8_t *indices, int width, int height);

/**
 * Shift a range of palette colors and optionally update pixel indices
 * Moves colors from [start, end] to a new position, with wraparound support
 * 
 * @param ctx Palette context
 * @param start Start index of range to move (0-255)
 * @param end End index of range to move (0-255, inclusive)
 * @param new_start New position for the range
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_shift_range(RealmpalPaletteContext *ctx, int start, int end, int new_start);

/**
 * Copy a range of palette colors to another location
 * 
 * @param ctx Palette context
 * @param src_start Source start index (0-255)
 * @param src_end Source end index (0-255, inclusive)
 * @param dst_start Destination start index
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_copy_range(RealmpalPaletteContext *ctx, int src_start, int src_end, int dst_start);

/**
 * Swap two ranges of palette colors
 * 
 * @param ctx Palette context
 * @param range1_start First range start index
 * @param range1_end First range end index (inclusive)
 * @param range2_start Second range start index
 * @param range2_end Second range end index (inclusive)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_swap_ranges(RealmpalPaletteContext *ctx, int range1_start, int range1_end,
                                int range2_start, int range2_end);

/**
 * Reverse the order of colors in a palette range
 * 
 * @param ctx Palette context
 * @param start Start index of range (0-255)
 * @param end End index of range (0-255, inclusive)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_reverse_range(RealmpalPaletteContext *ctx, int start, int end);

/**
 * Sort colors in a palette range by a specific criteria
 */
typedef enum {
    REALMPAL_SORT_LUMINANCE,    /**< Sort by perceived brightness */
    REALMPAL_SORT_HUE,          /**< Sort by hue (color wheel position) */
    REALMPAL_SORT_SATURATION,   /**< Sort by color saturation */
    REALMPAL_SORT_RED,          /**< Sort by red component */
    REALMPAL_SORT_GREEN,        /**< Sort by green component */
    REALMPAL_SORT_BLUE          /**< Sort by blue component */
} RealmpalSortCriteria;

/**
 * Sort colors in a palette range by specified criteria
 * 
 * @param ctx Palette context
 * @param start Start index of range (0-255)
 * @param end End index of range (0-255, inclusive)
 * @param criteria Sorting criteria
 * @param ascending Sort in ascending order (true) or descending (false)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_sort_range(RealmpalPaletteContext *ctx, int start, int end,
                               RealmpalSortCriteria criteria, bool ascending);

/**
 * Create a smooth gradient between two palette indices
 * 
 * @param ctx Palette context
 * @param start_index Start palette index
 * @param end_index End palette index
 * @param interpolate_through_hsl Use HSL interpolation (true) or RGB (false)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_create_gradient(RealmpalPaletteContext *ctx, int start_index, int end_index,
                                    bool interpolate_through_hsl);

/**
 * Adjust brightness/contrast of a palette range
 * 
 * @param ctx Palette context
 * @param start Start index of range (0-255)
 * @param end End index of range (0-255, inclusive)
 * @param brightness Brightness adjustment (-1.0 to 1.0, 0 = no change)
 * @param contrast Contrast adjustment (-1.0 to 1.0, 0 = no change)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_adjust_brightness_contrast(RealmpalPaletteContext *ctx, int start, int end,
                                               double brightness, double contrast);

/**
 * Adjust hue/saturation of a palette range
 * 
 * @param ctx Palette context
 * @param start Start index of range (0-255)
 * @param end End index of range (0-255, inclusive)
 * @param hue_shift Hue shift in degrees (-360 to 360)
 * @param saturation_factor Saturation multiplier (0.0 to 2.0, 1.0 = no change)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_adjust_hue_saturation(RealmpalPaletteContext *ctx, int start, int end,
                                          double hue_shift, double saturation_factor);

/**
 * Find unused palette indices
 * 
 * @param palette Palette to analyze
 * @param indices Index array to check usage against (can be NULL)
 * @param pixel_count Number of pixels in index array
 * @param unused_indices Output array for unused indices (256 entries)
 * @return Number of unused indices found
 */
int realmpal_palette_find_unused(const RGB8 *palette, const uint8_t *indices, int pixel_count,
                                int unused_indices[256]);

/**
 * Compact palette by removing unused colors and updating indices
 * 
 * @param ctx Palette context (indices required for this operation)
 * @param preserve_order Keep original color order (true) or pack efficiently (false)
 * @return Number of colors after compacting, or 0 on failure
 */
int realmpal_palette_compact(RealmpalPaletteContext *ctx, bool preserve_order);

/**
 * Find the closest match for a color in the palette
 * 
 * @param palette Palette to search (256 entries)
 * @param color Color to find
 * @param use_perceptual Use perceptual color distance
 * @param exclude_index Index to exclude from search (-1 for none)
 * @return Index of closest color (0-255)
 */
int realmpal_palette_find_closest_color_advanced(const RGB8 *palette, RGB8 color,
                                                bool use_perceptual, int exclude_index);

/**
 * Replace all instances of one color with another in palette and indices
 * 
 * @param ctx Palette context
 * @param old_index Index of color to replace
 * @param new_color New color to use
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_replace_color(RealmpalPaletteContext *ctx, int old_index, RGB8 new_color);

/**
 * Merge similar colors in a palette range
 * 
 * @param ctx Palette context
 * @param start Start index of range (0-255)
 * @param end End index of range (0-255, inclusive)
 * @param threshold Color distance threshold for merging (0.0-1.0)
 * @param use_perceptual Use perceptual color distance
 * @return Number of colors after merging
 */
int realmpal_palette_merge_similar(RealmpalPaletteContext *ctx, int start, int end,
                                  double threshold, bool use_perceptual);

/**
 * Extract a sub-palette from specific indices
 * 
 * @param source_palette Source palette (256 entries)
 * @param indices_to_extract Array of indices to extract
 * @param count Number of indices to extract
 * @param output_palette Output palette (256 entries, will be zeroed first)
 * @param remap_table Optional output remapping table (can be NULL)
 * @return Number of colors extracted
 */
int realmpal_palette_extract_subpalette(const RGB8 *source_palette, const int *indices_to_extract,
                                       int count, RGB8 *output_palette, int *remap_table);

/**
 * Blend two palettes together
 * 
 * @param palette1 First palette (256 entries)
 * @param palette2 Second palette (256 entries)  
 * @param output_palette Output blended palette (256 entries)
 * @param blend_factor Blend factor (0.0 = all palette1, 1.0 = all palette2)
 * @param blend_mode Blending mode
 * @return 1 on success, 0 on failure
 */
typedef enum {
    REALMPAL_BLEND_LINEAR,      /**< Linear interpolation */
    REALMPAL_BLEND_HSL,         /**< HSL color space blending */
    REALMPAL_BLEND_OVERLAY,     /**< Overlay blend mode */
    REALMPAL_BLEND_MULTIPLY     /**< Multiply blend mode */
} RealmpalBlendMode;

int realmpal_palette_blend(const RGB8 *palette1, const RGB8 *palette2, RGB8 *output_palette,
                          double blend_factor, RealmpalBlendMode blend_mode);

// ----------------------------- Palette Analysis and Statistics -----------------------------

/**
 * Palette analysis results structure
 */
typedef struct {
    int unique_colors;          /**< Number of actually unique colors */
    int used_colors;            /**< Number of colors used in image (if indices provided) */
    RGB8 dominant_color;        /**< Most frequently used color */
    RGB8 average_color;         /**< Average color of the palette */
    double average_luminance;   /**< Average perceived brightness (0.0-1.0) */
    double color_variance;      /**< Color distribution variance */
    int darkest_index;          /**< Index of darkest color */
    int brightest_index;        /**< Index of brightest color */
    int most_saturated_index;   /**< Index of most saturated color */
} RealmpalPaletteStats;

/**
 * Analyze palette and optionally usage statistics
 * 
 * @param palette Palette to analyze (256 entries)
 * @param indices Optional index array for usage analysis (can be NULL)
 * @param pixel_count Number of pixels in index array
 * @param stats Output statistics structure
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_analyze(const RGB8 *palette, const uint8_t *indices, int pixel_count,
                            RealmpalPaletteStats *stats);

/**
 * Generate a random palette within specified constraints
 * 
 * @param palette Palette array to fill (256 entries)
 * @param start Start index for generation
 * @param end End index for generation (inclusive)
 * @param min_saturation Minimum saturation (0.0-1.0)
 * @param max_saturation Maximum saturation (0.0-1.0)
 * @param min_lightness Minimum lightness (0.0-1.0)
 * @param max_lightness Maximum lightness (0.0-1.0)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_generate_random(RGB8 *palette, int start, int end,
                                    double min_saturation, double max_saturation,
                                    double min_lightness, double max_lightness);

/**
 * Create a palette based on color harmony rules
 * 
 * @param palette Palette array (256 entries)
 * @param start Starting index for the harmony
 * @param base_color Base color for the harmony
 * @param harmony_type Harmony type (0=monochromatic, 1=analogous, 2=complementary, 3=triadic, 4=split-complementary)
 * @param num_colors Number of colors to generate
 * @return Number of colors generated, or 0 on failure
 */
int realmpal_palette_create_harmony(RGB8 *palette, int start, RGB8 base_color, 
                                   int harmony_type, int num_colors);

/**
 * Create a color ramp between multiple key colors
 * 
 * @param ctx Palette context
 * @param key_indices Array of key color indices (must be in ascending order)
 * @param num_keys Number of key colors (minimum 2)
 * @param use_hsl Use HSL interpolation (true) or RGB (false)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_create_multi_gradient(RealmpalPaletteContext *ctx, 
                                          const int *key_indices, int num_keys,
                                          bool use_hsl);

/**
 * Convert a palette range to grayscale using luminance weights
 * 
 * @param ctx Palette context
 * @param start Start index of range (0-255)
 * @param end End index of range (0-255, inclusive)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_to_grayscale(RealmpalPaletteContext *ctx, int start, int end);

/**
 * Apply sepia tone effect to a palette range
 * 
 * @param ctx Palette context
 * @param start Start index of range (0-255)
 * @param end End index of range (0-255, inclusive)
 * @param intensity Sepia intensity (0.0-1.0, 0 = no effect, 1 = full sepia)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_sepia_tone(RealmpalPaletteContext *ctx, int start, int end, double intensity);

/**
 * Apply temperature adjustment (warm/cool) to a palette range
 * 
 * @param ctx Palette context
 * @param start Start index of range (0-255)
 * @param end End index of range (0-255, inclusive)
 * @param temperature Temperature adjustment (-1.0 to 1.0, negative = cooler, positive = warmer)
 * @return 1 on success, 0 on failure
 */
int realmpal_palette_adjust_temperature(RealmpalPaletteContext *ctx, int start, int end, 
                                       double temperature);

// ----------------------------- Color Quantization -----------------------------

/**
 * Quantize image colors using Wu's algorithm.
 * This is the highest quality quantization method, using variance-based 
 * color space subdivision to minimize color error.
 * 
 * @param pixels Source RGBA8 pixel array
 * @param count Number of pixels
 * @param max_colors Maximum colors to generate (1-256)
 * @param offset Starting index in palette (for palette injection)
 * @param out_palette Output palette array (256 entries)
 * @return Number of colors generated
 */
int realmpal_quantize_wu(const RGBA8 *pixels, int count, int max_colors, int offset, RGB8 out_palette[256]);

/**
 * Quantize image colors using median-cut algorithm.
 * Faster than Wu's algorithm but lower quality. Good for real-time applications.
 * 
 * @param pixels Source RGBA8 pixel array
 * @param count Number of pixels
 * @param max_colors Maximum colors to generate (1-256)
 * @param offset Starting index in palette (for palette injection)
 * @param out_palette Output palette array (256 entries)
 * @return Number of colors generated
 */
int realmpal_quantize_median_cut(const RGBA8 *pixels, int count, int max_colors, int offset, RGB8 out_palette[256]);

// ----------------------------- Color Mapping and Dithering -----------------------------

/**
 * Parse constraint string into IndexConstraints structure
 * Format: "12, 13, 20-40, 56, 60-65" (spaces optional)
 * 
 * @param constraint_string Input string with ranges and single indices
 * @param constraints Output constraints structure
 * @return 1 on success, 0 on parse error
 */
int realmpal_parse_index_constraints(const char* constraint_string, RealmpalIndexConstraints* constraints);

/**
 * Check if an index is allowed by the constraints
 * 
 * @param index Index to check (0-255)
 * @param constraints Constraints structure (NULL = no constraints)
 * @param transparency_index Transparency index to exclude (-1 = none)
 * @return 1 if allowed, 0 if not allowed
 */
int realmpal_index_allowed_by_constraints(int index, const RealmpalIndexConstraints* constraints, int transparency_index);

/**
 * Count total available indices in constraints
 * 
 * @param constraints Constraints structure (NULL = all indices)
 * @param transparency_index Transparency index to exclude (-1 = none)
 * @return Number of available indices
 */
int realmpal_count_constraint_indices(const RealmpalIndexConstraints* constraints, int transparency_index);

/**
 * Convert constraints to string representation
 * 
 * @param constraints Constraints structure
 * @param output_buffer Output buffer for string
 * @param buffer_size Size of output buffer
 * @return 1 on success, 0 if buffer too small
 */
int realmpal_constraints_to_string(const RealmpalIndexConstraints* constraints, char* output_buffer, int buffer_size);

/**
 * Map image to palette indices using Floyd-Steinberg error diffusion.
 * Provides excellent quality with good performance. The standard choice for most applications.
 * 
 * @param src Source RGBA8 pixel array
 * @param w Image width
 * @param h Image height
 * @param palette Color palette (256 entries)
 * @param out Output index array (w*h size, caller allocates)
 * @param serpentine Use serpentine (zigzag) scanning for better quality
 * @param strength Error diffusion strength (0.0-2.0, typically 1.0)
 * @param transparency_index Index to use for transparent pixels (-1 to disable)
 * @param alpha_threshold Alpha threshold for transparency (0-255)
 * @param constraints Index constraints (NULL = no constraints)
 */
void realmpal_map_floyd_steinberg(const RGBA8* src, int w, int h, const RGB8* palette, 
                                 uint8_t *out, bool serpentine, double strength,
                                 int transparency_index, int alpha_threshold,
                                 const RealmpalIndexConstraints* constraints);

/**
 * Map image using ordered (Bayer matrix) dithering.
 * Creates a characteristic crosshatch pattern. Good for print applications.
 * 
 * @param src Source RGBA8 pixel array
 * @param w Image width
 * @param h Image height
 * @param palette Color palette (256 entries)
 * @param out Output index array (w*h size, caller allocates)
 * @param matrix_size Size of Bayer matrix (2, 4, or 8)
 * @param transparency_index Index to use for transparent pixels (-1 to disable)
 * @param alpha_threshold Alpha threshold for transparency (0-255)
 * @param constraints Index constraints (NULL = no constraints)
 */
void realmpal_map_ordered_dither(const RGBA8* src, int w, int h, const RGB8* palette,
                                uint8_t *out, int matrix_size,
                                int transparency_index, int alpha_threshold,
                                const RealmpalIndexConstraints* constraints);

 /**
 * Map image using perceptually weighted nearest neighbor.
 * Uses human visual perception weights for better color matching.
 * 
 * @param src Source RGBA8 pixel array
 * @param w Image width
 * @param h Image height
 * @param palette Color palette (256 entries)
 * @param out Output index array (w*h size, caller allocates)
 * @param transparency_index Index to use for transparent pixels (-1 to disable)
 * @param alpha_threshold Alpha threshold for transparency (0-255)
 * @param constraints Index constraints (NULL = no constraints)
 */
void realmpal_map_perceptual(const RGBA8* src, int w, int h, const RGB8* palette,
                            uint8_t *out, int transparency_index, int alpha_threshold,
                            const RealmpalIndexConstraints* constraints);

// ----------------------------- File Output -----------------------------

/**
 * Write 8-bit indexed BMP file.
 * Creates a standard Windows Bitmap file with 256-color palette.
 * 
 * @param path Output file path
 * @param w Image width
 * @param h Image height
 * @param indices Pixel index array (w*h size)
 * @param palette Color palette (256 entries)
 * @return 1 on success, 0 on failure
 */
int realmpal_write_bmp8(const char* path, int w, int h, const uint8_t *indices, const RGB8 *palette);

/**
 * Write 8-bit PCX file with RLE compression.
 * Creates a ZSoft PCX file with 256-color palette and run-length encoding.
 * 
 * @param path Output file path
 * @param w Image width
 * @param h Image height
 * @param indices Pixel index array (w*h size)
 * @param palette Color palette (256 entries)
 * @return 1 on success, 0 on failure
 */
int realmpal_write_pcx8(const char* path, int w, int h, const uint8_t *indices, const RGB8 *palette);

/**
 * Determine output format from file extension.
 * 
 * @param filename File path or name
 * @return Format enum value
 */
RealmpalOutputFormat realmpal_get_output_format(const char* filename);

/**
 * Write 8-bit indexed image in automatically detected format.
 * Format is determined by file extension (.bmp, .pcx).
 * 
 * @param path Output file path  
 * @param w Image width
 * @param h Image height
 * @param indices Pixel index array (w*h size)
 * @param palette Color palette (256 entries)
 * @return 1 on success, 0 on failure
 */
int realmpal_write_auto(const char* path, int w, int h, const uint8_t *indices, const RGB8 *palette);

// ----------------------------- High-Level Conversion Functions -----------------------------

/**
 * Configuration structure for image conversion
 */
typedef struct {
    // Input/Output
    const char *input_file;           /**< Input image path */
    const char *output_file;          /**< Output image path */
    const char *palette_file;         /**< External palette file (NULL for auto) */
    
    // Mode and Algorithm
    RealmpalOpMode mode;              /**< Operation mode */
    RealmpalDitherMode dither;        /**< Dithering algorithm */
    RealmpalQuantMode quantizer;      /**< Quantization algorithm */
    
    // Quantization Settings
    int num_colors;                   /**< Number of colors to generate (1-256) */
    int index_offset;                 /**< Starting palette index */
    
    // Extra Palette Injection
    const char *extra_palette_file;   /**< Additional palette file */
    int extra_offset;                 /**< Where to inject extra colors */
    int extra_colors;                 /**< Number of extra colors to use */
    
    // Transparency
    RGB8 matte_color;                 /**< Background color for transparency */
    int transparency_index;           /**< Palette index for transparent pixels (-1 to disable) */
    int alpha_threshold;              /**< Alpha threshold (0-255) */
    RGB8 alpha_color;                 /**< Color to assign to transparency index */
    bool use_alpha_color;             /**< Whether to set alpha_color in palette */
    
    // Dithering Parameters
    double fs_strength;               /**< Floyd-Steinberg strength (0.0-2.0) */
    int ordered_matrix_size;          /**< Ordered dither matrix size (2,4,8) */

    // Enhanced Index Mapping Constraints
    RealmpalIndexConstraints index_constraints;  /**< Multi-range index constraints */
} RealmpalConfig;

/**
 * Initialize configuration structure with default values.
 * 
 * @param config Configuration structure to initialize
 */
void realmpal_config_init(RealmpalConfig *config);

/**
 * High-level image conversion function.
 * Performs complete image conversion using the provided configuration.
 * 
 * @param config Conversion configuration
 * @return 0 on success, error code on failure
 */
int realmpal_convert_image(const RealmpalConfig *config);

// ----------------------------- Utility Functions -----------------------------

/**
 * Get human-readable error message for the last operation.
 * 
 * @return Error message string (do not free)
 */
const char* realmpal_get_last_error(void);

/**
 * Clear the last error message.
 */
void realmpal_clear_error(void);

/**
 * Calculate color distance between two RGB colors.
 * Uses Euclidean distance in RGB space.
 * 
 * @param c1 First color
 * @param c2 Second color
 * @return Distance value (lower = more similar)
 */
double realmpal_color_distance_rgb(RGB8 c1, RGB8 c2);

/**
 * Calculate perceptually weighted color distance.
 * Uses luminance-weighted distance that better matches human vision.
 * 
 * @param c1 First color
 * @param c2 Second color
 * @return Distance value (lower = more similar)
 */
double realmpal_color_distance_perceptual(RGB8 c1, RGB8 c2);

/**
 * Find the closest palette index for a given RGB color.
 * Uses optimized lookup with caching for performance.
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param palette Color palette (256 entries)
 * @param use_perceptual Use perceptual weighting
 * @return Best matching palette index (0-255)
 */
int realmpal_find_closest_color(uint8_t r, uint8_t g, uint8_t b, const RGB8 *palette, bool use_perceptual);

/**
 * Clamp integer value to specified range.
 * 
 * @param value Input value
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
static inline int realmpal_clamp_int(int value, int min, int max) {
    return value < min ? min : (value > max ? max : value);
}

/**
 * Clamp double value to specified range.
 * 
 * @param value Input value
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
static inline double realmpal_clamp_double(double value, double min, double max) {
    return value < min ? min : (value > max ? max : value);
}

// ----------------------------- Error Codes -----------------------------

#define REALMPAL_SUCCESS                0   /**< Operation successful */
#define REALMPAL_ERROR_INVALID_ARGS     1   /**< Invalid arguments provided */
#define REALMPAL_ERROR_FILE_NOT_FOUND   2   /**< Input file not found */
#define REALMPAL_ERROR_FILE_FORMAT      3   /**< Unsupported or invalid file format */
#define REALMPAL_ERROR_WRITE_FAILED     4   /**< Failed to write output file */
#define REALMPAL_ERROR_OUT_OF_MEMORY    5   /**< Memory allocation failed */
#define REALMPAL_ERROR_INVALID_PALETTE  6   /**< Invalid palette file or data */

#ifdef __cplusplus
}
#endif

#endif // LIBREALMPAL_H