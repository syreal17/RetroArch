/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __VIDEO_DRIVER__H
#define __VIDEO_DRIVER__H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#include <boolean.h>
#include <retro_common_api.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#ifdef HAVE_OVERLAY
#include "../input/input_overlay.h"
#endif

#include "video_defines.h"
#include "video_filter.h"
#include "video_shader_parse.h"

#include "../input/input_driver.h"

RETRO_BEGIN_DECLS

typedef struct video_info
{
   /* Width of window. 
    * If fullscreen mode is requested, 
    * a width of 0 means the resolution of the 
    * desktop should be used. */
   unsigned width;

   /* Height of window. 
    * If fullscreen mode is requested, 
    * a height of 0 means the resolutiof the desktop should be used.
    */
   unsigned height;

   /* Launch in fullscreen mode instead of windowed mode. */
   bool fullscreen;

   /* Start with V-Sync enabled. */
   bool vsync;

   /* If true, the output image should have the aspect ratio 
    * as set in aspect_ratio. */
   bool force_aspect;

   unsigned swap_interval;

   bool font_enable;

#ifdef GEKKO
   /* TODO - we can't really have driver system-specific
    * variables in here. There should be some
    * kind of publicly accessible driver implementation
    * video struct for specific things like this.
    */

   /* Wii-specific settings. Ignored for everything else. */
   unsigned viwidth;
   bool vfilter;
#endif

   /* If true, applies bilinear filtering to the image,
    * otherwise nearest filtering. */
   bool smooth;

   /* 
    * input_scale defines the maximum size of the picture that will
    * ever be used with the frame callback.
    *
    * The maximum resolution is a multiple of 256x256 size (RARCH_SCALE_BASE),
    * so an input scale of 2 means you should allocate a texture or of 512x512.
    *
    * Maximum input size: RARCH_SCALE_BASE * input_scale 
    */
   unsigned input_scale;

   /* Use 32bit RGBA rather than native RGB565/XBGR1555. 
    *
    * XRGB1555 format is 16-bit and has byte ordering: 0RRRRRGGGGGBBBBB,
    * in native endian.
    *
    * ARGB8888 is AAAAAAAARRRRRRRRGGGGGGGGBBBBBBBB, native endian.
    * Alpha channel should be disregarded.
    * */
   bool rgb32;
   
#ifndef RARCH_INTERNAL
   uintptr_t parent;
#endif
} video_info_t;

typedef struct video_frame_info
{
   float menu_wallpaper_opacity;
   float refresh_rate;
   bool shared_context;
   bool black_frame_insertion;
   bool hard_sync;
   unsigned hard_sync_frames;
   bool fps_show;
   bool scale_integer;
   unsigned aspect_ratio_idx;
   bool post_filter_record;
   unsigned max_swapchain_images;
   bool windowed_fullscreen;
   bool fullscreen;
   unsigned monitor_index;
   bool font_enable;
   char fps_text[128];
   uint64_t frame_count;

   unsigned width;
   unsigned height;

   float font_msg_pos_x;
   float font_msg_pos_y;
   float font_msg_color_r;
   float font_msg_color_g;
   float font_msg_color_b;
   bool use_rgba;
   bool libretro_running;

   float menu_header_opacity;
   float menu_footer_opacity;

   bool xmb_shadows_enable;
   float xmb_alpha_factor;
   unsigned xmb_theme;
   unsigned xmb_color_theme;
   unsigned menu_shader_pipeline;

   unsigned materialui_color_theme;

   bool battery_level_enable;
   bool timedate_enable;
   bool runloop_is_slowmotion;
   bool runloop_is_idle;
   bool runloop_is_paused;
   bool is_perfcnt_enable;

   bool menu_is_alive;
} video_frame_info_t;

/* Optionally implemented interface to poke more
 * deeply into video driver. */

typedef struct video_poke_interface
{
   uintptr_t (*load_texture)(void *video_data, void *data,
         bool threaded, enum texture_filter_type filter_type);
   void (*unload_texture)(void *data, uintptr_t id);
   void (*set_video_mode)(void *data, unsigned width,
         unsigned height, bool fullscreen);
   void (*set_filtering)(void *data, unsigned index, bool smooth);
   void (*get_video_output_size)(void *data,
         unsigned *width, unsigned *height);

   /* Move index to previous resolution */
   void (*get_video_output_prev)(void *data);

   /* Move index to next resolution */
   void (*get_video_output_next)(void *data);

   uintptr_t (*get_current_framebuffer)(void *data);
   retro_proc_address_t (*get_proc_address)(void *data, const char *sym);
   void (*set_aspect_ratio)(void *data, unsigned aspectratio_index);
   void (*apply_state_changes)(void *data);

#ifdef HAVE_MENU
   /* Update texture. */
   void (*set_texture_frame)(void *data, const void *frame, bool rgb32,
         unsigned width, unsigned height, float alpha);
#endif
   /* Enable or disable rendering. */
   void (*set_texture_enable)(void *data, bool enable, bool full_screen);
   void (*set_osd_msg)(void *data, const char *msg,
         const void *params, void *font);

   void (*show_mouse)(void *data, bool state);
   void (*grab_mouse_toggle)(void *data);

   struct video_shader *(*get_current_shader)(void *data);
   bool (*get_current_software_framebuffer)(void *data,
         struct retro_framebuffer *framebuffer);
   bool (*get_hw_render_interface)(void *data,
         const struct retro_hw_render_interface **iface);
} video_poke_interface_t;

typedef struct video_viewport
{
   int x;
   int y;
   unsigned width;
   unsigned height;
   unsigned full_width;
   unsigned full_height;
} video_viewport_t;

/* msg is for showing a message on the screen 
 * along with the video frame. */
typedef bool (*video_driver_frame_t)(void *data,
      const void *frame, unsigned width,
      unsigned height, uint64_t frame_count,
      unsigned pitch, const char *msg, video_frame_info_t *video_info);

typedef struct video_driver
{
   /* Should the video driver act as an input driver as well?
    * The video initialization might preinitialize an input driver
    * to override the settings in case the video driver relies on
    * input driver for event handling. */
   void *(*init)(const video_info_t *video,
         const input_driver_t **input,
         void **input_data);

   /* Updates frame on the screen. 
    * Frame can be either XRGB1555, RGB565 or ARGB32 format
    * depending on rgb32 setting in video_info_t. 
    * Pitch is the distance in bytes between two scanlines in memory. 
    * 
    * When msg is non-NULL, 
    * it's a message that should be displayed to the user. */
   video_driver_frame_t frame;

   /* Should we care about syncing to vblank? Fast forwarding. 
    *
    * Requests nonblocking operation. 
    *
    * True = VSync is turned off. 
    * False = VSync is turned on.
    * */
   void (*set_nonblock_state)(void *data, bool toggle);

   /* Is the window still active? */
   bool (*alive)(void *data);

   /* Does the window have focus? */
   bool (*focus)(void *data);

   /* Should the screensaver be suppressed? */
   bool (*suppress_screensaver)(void *data, bool enable);

   /* Does the graphics context support windowed mode? */
   bool (*has_windowed)(void *data);

   /* Sets shader. Might not be implemented. Will be moved to
    * poke_interface later. */
   bool (*set_shader)(void *data, enum rarch_shader_type type,
         const char *path);

   /* Frees driver. */
   void (*free)(void *data);

   /* Human-readable identifier. */
   const char *ident;

   void (*set_viewport)(void *data, unsigned width, unsigned height,
         bool force_full, bool allow_rotate);

   void (*set_rotation)(void *data, unsigned rotation);
   void (*viewport_info)(void *data, struct video_viewport *vp);

   /* Reads out in BGR byte order (24bpp). */
   bool (*read_viewport)(void *data, uint8_t *buffer, bool is_idle);

   /* Returns a pointer to a newly allocated buffer that can
    * (and must) be passed to free() by the caller, containing a
    * copy of the current raw frame in the active pixel format
    * and sets width, height and pitch to the correct values. */
   void* (*read_frame_raw)(void *data, unsigned *width,
   unsigned *height, size_t *pitch);

#ifdef HAVE_OVERLAY
   void (*overlay_interface)(void *data,
         const video_overlay_interface_t **iface);
#endif
   void (*poke_interface)(void *data, const video_poke_interface_t **iface);
   unsigned (*wrap_type_to_enum)(enum gfx_wrap_type type);
} video_driver_t;

struct aspect_ratio_elem
{
   char name[64];
   float value;
};

extern struct aspect_ratio_elem aspectratio_lut[ASPECT_RATIO_END];

#define video_driver_is_alive()   ((current_video) ? current_video->alive(video_driver_data) : true)

#define video_driver_is_focused() (current_video->focus(video_driver_data))

#if defined(RARCH_CONSOLE) || defined(RARCH_MOBILE)
#define video_driver_has_windowed() (false)
#else
#define video_driver_has_windowed() (current_video->has_windowed && current_video->has_windowed(video_driver_data))
#endif

#define video_driver_cached_frame_has_valid_framebuffer() (frame_cache_data ? (frame_cache_data == RETRO_HW_FRAME_BUFFER_VALID) : false)

void video_driver_destroy(void);
void video_driver_set_cached_frame_ptr(const void *data);
void video_driver_set_stub_frame(void);
void video_driver_unset_stub_frame(void);
bool video_driver_supports_recording(void);
bool video_driver_supports_viewport_read(void);
bool video_driver_supports_read_frame_raw(void);
void video_driver_set_viewport_config(void);
void video_driver_set_viewport_square_pixel(void);
void video_driver_set_viewport_core(void);
void video_driver_reset_custom_viewport(void);
void video_driver_set_rgba(void);
void video_driver_unset_rgba(void);
bool video_driver_supports_rgba(void);
bool video_driver_get_next_video_out(void);
bool video_driver_get_prev_video_out(void);
bool video_driver_init(void);
void video_driver_destroy_data(void);
void video_driver_free(void);
void video_driver_free_hw_context(void);
void video_driver_monitor_reset(void);
void video_driver_set_aspect_ratio(void);
void video_driver_show_mouse(void);
void video_driver_hide_mouse(void);
void video_driver_set_nonblock_state(bool toggle);
bool video_driver_find_driver(void);
void video_driver_apply_state_changes(void);
bool video_driver_read_viewport(uint8_t *buffer, bool is_idle);
bool video_driver_cached_frame(void);
uint64_t video_driver_get_frame_count(void);
bool video_driver_frame_filter_alive(void);
bool video_driver_frame_filter_is_32bit(void);
void video_driver_default_settings(void);
void video_driver_load_settings(config_file_t *conf);
void video_driver_save_settings(config_file_t *conf);
void video_driver_set_own_driver(void);
void video_driver_unset_own_driver(void);
bool video_driver_owns_driver(void);
bool video_driver_is_hw_context(void);
struct retro_hw_render_callback *video_driver_get_hw_context(void);
const struct retro_hw_render_context_negotiation_interface 
*video_driver_get_context_negotiation_interface(void);
void video_driver_set_context_negotiation_interface(const struct 
      retro_hw_render_context_negotiation_interface *iface);
bool video_driver_is_video_cache_context(void);
void video_driver_set_video_cache_context_ack(void);
bool video_driver_is_video_cache_context_ack(void);
void video_driver_set_active(void);
bool video_driver_is_active(void);
bool video_driver_gpu_record_init(unsigned size);
void video_driver_gpu_record_deinit(void);
bool video_driver_get_current_software_framebuffer(struct 
      retro_framebuffer *fb);
bool video_driver_get_hw_render_interface(const struct 
      retro_hw_render_interface **iface);
bool video_driver_get_viewport_info(struct video_viewport *viewport);
void video_driver_set_title_buf(void);
void video_driver_monitor_adjust_system_rates(void);
bool video_driver_is_threaded(void);

/**
 * video_driver_find_handle:
 * @index              : index of driver to get handle to.
 *
 * Returns: handle to video driver at index. Can be NULL
 * if nothing found.
 **/
const void *video_driver_find_handle(int index);

/**
 * video_driver_find_ident:
 * @index              : index of driver to get handle to.
 *
 * Returns: Human-readable identifier of video driver at index. 
 * Can be NULL if nothing found.
 **/
const char *video_driver_find_ident(int index);

/**
 * config_get_video_driver_options:
 *
 * Get an enumerated list of all video driver names, separated by '|'.
 *
 * Returns: string listing of all video driver names, separated by '|'.
 **/
const char* config_get_video_driver_options(void);

/**
 * video_driver_get_ptr:
 *
 * Use this if you need the real video driver
 * and driver data pointers.
 *
 * Returns: video driver's userdata.
 **/
void *video_driver_get_ptr(bool force_nonthreaded_data);

/**
 * video_driver_get_current_framebuffer:
 *
 * Gets pointer to current hardware renderer framebuffer object.
 * Used by RETRO_ENVIRONMENT_SET_HW_RENDER.
 *
 * Returns: pointer to hardware framebuffer object, otherwise 0.
 **/
uintptr_t video_driver_get_current_framebuffer(void);

retro_proc_address_t video_driver_get_proc_address(const char *sym);

bool video_driver_set_shader(enum rarch_shader_type type,
      const char *shader);

bool video_driver_set_rotation(unsigned rotation);

bool video_driver_set_video_mode(unsigned width,
      unsigned height, bool fullscreen);

bool video_driver_get_video_output_size(
      unsigned *width, unsigned *height);

void video_driver_set_osd_msg(const char *msg,
      const void *params, void *font);

void video_driver_set_texture_enable(bool enable, bool full_screen);

void video_driver_set_texture_frame(const void *frame, bool rgb32,
      unsigned width, unsigned height, float alpha);

#ifdef HAVE_OVERLAY
bool video_driver_overlay_interface(
      const video_overlay_interface_t **iface);
#endif

void * video_driver_read_frame_raw(unsigned *width,
   unsigned *height, size_t *pitch);

void video_driver_set_filtering(unsigned index, bool smooth);

const char *video_driver_get_ident(void);

bool video_driver_set_viewport(unsigned width, unsigned height,
      bool force_fullscreen, bool allow_rotate);

void video_driver_get_size(unsigned *width, unsigned *height);

void video_driver_set_size(unsigned *width, unsigned *height);

void video_driver_unset_video_cache_context_ack(void);

float video_driver_get_aspect_ratio(void);

void video_driver_set_aspect_ratio_value(float value);

rarch_softfilter_t *video_driver_frame_filter_get_ptr(void);

enum retro_pixel_format video_driver_get_pixel_format(void);

void video_driver_set_pixel_format(enum retro_pixel_format fmt);

void video_driver_cached_frame_set(const void *data, unsigned width,
      unsigned height, size_t pitch);

void video_driver_cached_frame_get(const void **data, unsigned *width,
      unsigned *height, size_t *pitch);

void video_driver_menu_settings(void **list_data, void *list_info_data,
      void *group_data, void *subgroup_data, const char *parent_group);

/**
 * video_viewport_get_scaled_integer:
 * @vp            : Viewport handle
 * @width         : Width.
 * @height        : Height.
 * @aspect_ratio  : Aspect ratio (in float).
 * @keep_aspect   : Preserve aspect ratio?
 *
 * Gets viewport scaling dimensions based on 
 * scaled integer aspect ratio.
 **/
void video_viewport_get_scaled_integer(struct video_viewport *vp,
      unsigned width, unsigned height,
      float aspect_ratio, bool keep_aspect);

struct retro_system_av_info *video_viewport_get_system_av_info(void);

struct video_viewport *video_viewport_get_custom(void);

/**
 * video_monitor_set_refresh_rate:
 * @hz                 : New refresh rate for monitor.
 *
 * Sets monitor refresh rate to new value.
 **/
void video_monitor_set_refresh_rate(float hz);

/**
 * video_monitor_fps_statistics
 * @refresh_rate       : Monitor refresh rate.
 * @deviation          : Deviation from measured refresh rate.
 * @sample_points      : Amount of sampled points.
 *
 * Gets the monitor FPS statistics based on the current
 * runtime.
 *
 * Returns: true (1) on success.
 * false (0) if:
 * a) threaded video mode is enabled
 * b) less than 2 frame time samples.
 * c) FPS monitor enable is off.
 **/
bool video_monitor_fps_statistics(double *refresh_rate,
      double *deviation, unsigned *sample_points);

unsigned video_pixel_get_alignment(unsigned pitch);

const video_poke_interface_t *video_driver_get_poke(void);

/**
 * video_driver_frame:
 * @data                 : pointer to data of the video frame.
 * @width                : width of the video frame.
 * @height               : height of the video frame.
 * @pitch                : pitch of the video frame.
 *
 * Video frame render callback function.
 **/
void video_driver_frame(const void *data, unsigned width,
      unsigned height, size_t pitch);

#define video_driver_translate_coord_viewport_wrap(vp, mouse_x, mouse_y, res_x, res_y, res_screen_x, res_screen_y) \
   (video_driver_get_viewport_info(vp) ? video_driver_translate_coord_viewport(vp, mouse_x, mouse_y, res_x, res_y, res_screen_x, res_screen_y) : false)

/**
 * video_driver_translate_coord_viewport:
 * @mouse_x                        : Pointer X coordinate.
 * @mouse_y                        : Pointer Y coordinate.
 * @res_x                          : Scaled  X coordinate.
 * @res_y                          : Scaled  Y coordinate.
 * @res_screen_x                   : Scaled screen X coordinate.
 * @res_screen_y                   : Scaled screen Y coordinate.
 *
 * Translates pointer [X,Y] coordinates into scaled screen
 * coordinates based on viewport info.
 *
 * Returns: true (1) if successful, false if video driver doesn't support
 * viewport info.
 **/
bool video_driver_translate_coord_viewport(
      void *data,
      int mouse_x, int mouse_y,
      int16_t *res_x, int16_t *res_y, int16_t *res_screen_x,
      int16_t *res_screen_y);

uintptr_t video_driver_display_get(void);

enum rarch_display_type video_driver_display_type_get(void);

uintptr_t video_driver_window_get(void);

void video_driver_display_type_set(enum rarch_display_type type);

void video_driver_display_set(uintptr_t idx);

void video_driver_window_set(uintptr_t idx);

bool video_driver_texture_load(void *data,
      enum texture_filter_type  filter_type,
      uintptr_t *id);

bool video_driver_texture_unload(uintptr_t *id);

void video_driver_build_info(video_frame_info_t *video_info);

void video_driver_reinit(void);

void video_driver_get_window_title(char *buf, unsigned len);

void video_driver_get_record_status(
      bool *has_gpu_record, 
      uint8_t **gpu_buf);

void video_driver_get_status(uint64_t *frame_count, bool * is_alive,
      bool *is_focused);

extern video_driver_t video_gl;
extern video_driver_t video_vulkan;
extern video_driver_t video_psp1;
extern video_driver_t video_vita2d;
extern video_driver_t video_ctr;
extern video_driver_t video_d3d;
extern video_driver_t video_gx;
extern video_driver_t video_wiiu;
extern video_driver_t video_xenon360;
extern video_driver_t video_xvideo;
extern video_driver_t video_xdk_d3d;
extern video_driver_t video_sdl;
extern video_driver_t video_sdl2;
extern video_driver_t video_vg;
extern video_driver_t video_omap;
extern video_driver_t video_exynos;
extern video_driver_t video_dispmanx;
extern video_driver_t video_sunxi;
extern video_driver_t video_drm;
extern video_driver_t video_xshm;
extern video_driver_t video_caca;
extern video_driver_t video_gdi;
extern video_driver_t video_vga;
extern video_driver_t video_null;

extern const void *frame_cache_data;

extern void *video_driver_data;
extern video_driver_t *current_video;

RETRO_END_DECLS

#endif
