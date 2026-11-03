/* Loads an arbitrary ROM image through the libretro API and emulates a few frames.
 * Input: byte0 = content type (bits0-2) | frames-1 (bits3-5) | reset (bit6); bytes1-2 = joypad mask; rest = ROM. */
#include <stddef.h>
#include "glue.h"

#define MAX_ROM 0x500000

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   static const char *const exts[8] = { "md", "bin", "smd", "sms", "gg", "sg", "mdx", "gen" };
   unsigned cfg, frames, i;

   if (size < 4 || size > MAX_ROM)
      return 0;
   gpgx_init();

   cfg = data[0];
   frames = 1 + ((cfg >> 3) & 7);
   g_pad = (uint16_t)(data[1] | (data[2] << 8));

   if (!gpgx_load(data + 3, size - 3, exts[cfg & 7]))
      return 0;
   if (cfg & 0x40)
      retro_reset();
   for (i = 0; i < frames; i++)
      retro_run();
   retro_unload_game();
   return 0;
}
