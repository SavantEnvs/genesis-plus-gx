/* Minimal libretro frontend shared by the fuzz harnesses and the oracle probe. */
#ifndef GPGX_GLUE_H
#define GPGX_GLUE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "libretro.h"

/* Core globals freed by retro_unload_game(); retro_load_game() leaks them when loading fails. */
extern void *md_ntsc;
extern void *sms_ntsc;

static uint16_t g_pad;
static unsigned g_vid_w, g_vid_h;
static uint16_t g_vid_center;
static const void *g_rom_ptr;
static size_t g_rom_len;
static char g_ext_buf[8];
static char g_full_path[32];
static struct retro_game_info_ext g_info_ext;

static bool gpgx_env(unsigned cmd, void *data)
{
   switch (cmd)
   {
      case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
         g_info_ext.data = g_rom_ptr;
         g_info_ext.size = g_rom_len;
         g_info_ext.full_path = g_full_path;
         g_info_ext.dir = "/nonexistent";
         g_info_ext.name = "rom";
         g_info_ext.ext = g_ext_buf;
         g_info_ext.meta = NULL;
         g_info_ext.file_in_archive = false;
         g_info_ext.persistent_data = false;
         *(struct retro_game_info_ext **)data = &g_info_ext;
         return true;
      case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
         return true;
      default:
         return false;
   }
}

static void gpgx_video(const void *data, unsigned w, unsigned h, size_t pitch)
{
   g_vid_w = w;
   g_vid_h = h;
   if (data && w && h)
      g_vid_center = ((const uint16_t *)((const uint8_t *)data + (h / 2) * pitch))[w / 2];
}

static void gpgx_audio(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t gpgx_audio_batch(const int16_t *d, size_t frames) { (void)d; return frames; }
static void gpgx_poll(void) {}

static int16_t gpgx_input(unsigned port, unsigned device, unsigned index, unsigned id)
{
   (void)index;
   if (port != 0 || device != RETRO_DEVICE_JOYPAD)
      return 0;
   if (id == RETRO_DEVICE_ID_JOYPAD_MASK)
      return (int16_t)g_pad;
   return id < 16 ? (g_pad >> id) & 1 : 0;
}

static void gpgx_init(void)
{
   static int done;
   if (done)
      return;
   done = 1;
   retro_set_environment(gpgx_env);
   retro_set_video_refresh(gpgx_video);
   retro_set_audio_sample(gpgx_audio);
   retro_set_audio_sample_batch(gpgx_audio_batch);
   retro_set_input_poll(gpgx_poll);
   retro_set_input_state(gpgx_input);
   retro_init();
}

/* ext is the 2-3 character content extension (selects MD/SMS/GG/SG hardware). */
static bool gpgx_load(const void *rom, size_t len, const char *ext)
{
   struct retro_game_info info;
   bool ok;

   strncpy(g_ext_buf, ext, sizeof(g_ext_buf) - 1);
   g_ext_buf[sizeof(g_ext_buf) - 1] = 0;
   strcpy(g_full_path, "/nonexistent/rom.");
   strcat(g_full_path, g_ext_buf);
   g_rom_ptr = rom;
   g_rom_len = len;
   memset(&info, 0, sizeof(info));
   info.path = g_full_path;
   ok = retro_load_game(&info);
   if (!ok)
   {
      free(md_ntsc);
      free(sms_ntsc);
      md_ntsc = NULL;
      sms_ntsc = NULL;
   }
   return ok;
}

#endif
