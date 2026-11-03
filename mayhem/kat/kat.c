/* Known-answer probe: boots the built-in test ROM through the real core and prints what it observed. */
#include <stdio.h>
#include "../harness/glue.h"
#include "../harness/testrom.h"

int main(int argc, char **argv)
{
   static uint8_t rom[TESTROM_SIZE];
   static uint8_t s1[0xfd000], s2[0xfd000];
   const uint16_t *ram;
   uint32_t sum;
   int i;

   testrom_build(rom);
   if (argc > 2 && !strcmp(argv[1], "--dump-rom"))
   {
      FILE *f = fopen(argv[2], "wb");
      fputc(0, f);
      fputc(0, f);
      fputc(0, f);
      fwrite(rom, 1, sizeof(rom), f);
      fclose(f);
      return 0;
   }

   gpgx_init();
   if (!gpgx_load(rom, sizeof(rom), "md"))
   {
      puts("load=fail");
      return 1;
   }
   for (i = 0; i < 4; i++)
      retro_run();
   ram = retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
   sum = ((uint32_t)ram[0] << 16) | ram[1];
   printf("sum=%u\n", sum);
   printf("video=%ux%u pixel=0x%04x\n", g_vid_w, g_vid_h, g_vid_center);

   if (retro_serialize_size() != sizeof(s1) || !retro_serialize(s1, sizeof(s1)))
   {
      puts("serialize=fail");
      return 1;
   }
   for (i = 0; i < 5; i++)
      retro_run();
   if (!retro_unserialize(s1, sizeof(s1)) || !retro_serialize(s2, sizeof(s2)))
   {
      puts("unserialize=fail");
      return 1;
   }
   printf("state_roundtrip=%s\n", memcmp(s1, s2, sizeof(s1)) ? "differs" : "identical");
   ram = retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
   printf("sum_after_restore=%u\n", ((uint32_t)ram[0] << 16) | ram[1]);
   retro_unload_game();
   return 0;
}
