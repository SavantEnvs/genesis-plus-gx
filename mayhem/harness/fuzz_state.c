/* Feeds patched savestates to retro_unserialize(). The input is a list of patch records applied to a
 * valid base state: [off0 off1 off2 len data...], offset = 24-bit mod state size, length = (len & 31) + 1. */
#include <stddef.h>
#include "glue.h"
#include "testrom.h"

static uint8_t *g_base;
static uint8_t *g_work;
static size_t g_state_size;

static void state_setup(void)
{
   static uint8_t rom[TESTROM_SIZE];
   int i;

   gpgx_init();
   testrom_build(rom);
   if (!gpgx_load(rom, sizeof(rom), "md"))
      abort();
   for (i = 0; i < 3; i++)
      retro_run();
   g_state_size = retro_serialize_size();
   g_base = malloc(g_state_size);
   g_work = malloc(g_state_size);
   if (!g_base || !g_work || !retro_serialize(g_base, g_state_size))
      abort();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   size_t pos = 0;

   if (!g_base)
      state_setup();

   memcpy(g_work, g_base, g_state_size);
   while (size - pos >= 5)
   {
      size_t off = ((size_t)data[pos] | (data[pos + 1] << 8) | ((size_t)data[pos + 2] << 16)) % g_state_size;
      size_t len = (data[pos + 3] & 31) + 1;
      pos += 4;
      if (len > size - pos)
         len = size - pos;
      if (len > g_state_size - off)
         len = g_state_size - off;
      memcpy(g_work + off, data + pos, len);
      pos += len;
   }

   if (retro_unserialize(g_work, g_state_size))
      retro_run();

   if (!retro_unserialize(g_base, g_state_size))
      abort();
   return 0;
}
