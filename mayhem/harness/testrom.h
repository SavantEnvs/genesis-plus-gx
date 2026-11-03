/* A tiny hand-assembled Mega Drive ROM with known observable behavior:
 *   d0 = 1+2+...+100 stored long at $FF0000 (work RAM), CRAM[0] = $0E00, display on, then spin. */
#ifndef GPGX_TESTROM_H
#define GPGX_TESTROM_H

#include <stdint.h>
#include <string.h>

#define TESTROM_SIZE 0x400

static void testrom_build(uint8_t *rom)
{
   static const uint16_t code[] = {
      0x41F9, 0x00FF, 0x0000,       /* lea    $FF0000.l,a0       */
      0x7000,                       /* moveq  #0,d0              */
      0x7201,                       /* moveq  #1,d1              */
      0xD081,                       /* loop:  add.l d1,d0        */
      0x5281,                       /*        addq.l #1,d1       */
      0xB2BC, 0x0000, 0x0065,       /*        cmp.l #101,d1      */
      0x66F4,                       /*        bne   loop         */
      0x2080,                       /* move.l d0,(a0)            */
      0x43F9, 0x00C0, 0x0004,       /* lea    $C00004.l,a1       */
      0x32BC, 0x8144,               /* move.w #$8144,(a1)  display on, MD mode */
      0x32BC, 0x8700,               /* move.w #$8700,(a1)  backdrop = CRAM[0]  */
      0x22BC, 0xC000, 0x0000,       /* move.l #$C0000000,(a1)  CRAM write @0   */
      0x337C, 0x0E00, 0xFFFC,       /* move.w #$0E00,-4(a1)    CRAM[0] = blue  */
      0x60FE                        /* spin:  bra.s spin         */
   };
   size_t i;

   memset(rom, 0, TESTROM_SIZE);
   rom[0] = 0x00; rom[1] = 0xFF; rom[2] = 0xFE; rom[3] = 0x00;     /* initial SSP = $00FFFE00 */
   rom[4] = 0x00; rom[5] = 0x00; rom[6] = 0x02; rom[7] = 0x00;     /* initial PC  = $00000200 */
   memcpy(rom + 0x100, "SEGA MEGA DRIVE ", 16);
   memcpy(rom + 0x120, "MAYHEM TEST ROM                                 ", 48);
   memcpy(rom + 0x150, "MAYHEM TEST ROM                                 ", 48);
   memcpy(rom + 0x180, "GM 00000000-00", 14);
   rom[0x1A0] = 0; rom[0x1A1] = 0; rom[0x1A2] = 0; rom[0x1A3] = 0;
   rom[0x1A4] = 0; rom[0x1A5] = 0; rom[0x1A6] = 0x03; rom[0x1A7] = 0xFF;   /* ROM end */
   rom[0x1A8] = 0x00; rom[0x1A9] = 0xFF; rom[0x1AA] = 0x00; rom[0x1AB] = 0x00;
   rom[0x1AC] = 0x00; rom[0x1AD] = 0xFF; rom[0x1AE] = 0xFF; rom[0x1AF] = 0xFF; /* RAM range */
   memcpy(rom + 0x1F0, "U               ", 16);
   for (i = 0; i < sizeof(code) / sizeof(code[0]); i++)
   {
      rom[0x200 + 2 * i] = code[i] >> 8;
      rom[0x200 + 2 * i + 1] = code[i] & 0xFF;
   }
}

#endif
