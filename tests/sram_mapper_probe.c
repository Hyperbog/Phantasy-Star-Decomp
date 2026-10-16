/* Small compiled hardware test. Not part of the game binary, no ROM data. */
#include "glue.h"
#include "sms_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Dispatch is irrelevant to a memory-mapper-only test. */
void call_by_address(uint16_t address){ (void)address; }

static void check(int good, const char *msg){
    if (!good){ fprintf(stderr,"SRAM TEST FAILURE: %s\n",msg); exit(1); }
}

int main(int argc,char **argv){
    if (argc < 3){ fprintf(stderr,"usage: %s <synthetic-rom> <path to SRAM>\n",argv[0]); return 2; }
    check(glue_load_rom(argv[1]),"open synthetic ROM");
    check(glue_load_sram(argv[2]),"initial SRAM image");
    glue_init(false,0);
    if (argc > 3 && !strcmp(argv[3],"reload")){
        sms_write8(0xFFFC,0x08);
        check(sms_read8(0x8000)==0x11 && sms_read8(0xBFFF)==0x22,"persisted bank0");
        sms_write8(0xFFFC,0x0C);
        check(sms_read8(0x8000)==0x44 && sms_read8(0xBFFF)==0x55,"persisted bank1");
        check(glue_flush_sram(),"no-op flush");
        puts("SRAM persisted reload: PASS");
        return 0;
    }
    if (argc > 3 && !strcmp(argv[3],"mirror")){
        sms_write8(0xFFFC,0x08);
        check(sms_read8(0x8000)==0xA0,"8K SRAM image loaded");
        check(sms_read8(0xA000)==0xA0,"8K SRAM mirrors at A000");
        sms_write8(0xFFFC,0x0C);
        check(sms_read8(0x8000)==0xA0,"8K SRAM bank select aliases same physical chip");
        sms_write8(0x8000,0x5A);
        sms_write8(0xFFFC,0x08);
        check(sms_read8(0x8000)==0x5A && sms_read8(0xA000)==0x5A,"mirror writes persist");
        check(glue_flush_sram(),"8K flush");
        puts("8K SRAM mirroring: PASS");
        return 0;
    }
    check(sms_read8(0x0000)==0x00,"fixed vector ROM bank0");
    check(sms_read8(0x8000)==0x02,"ROM slot2 active at reset");
    sms_write8(0x8000,0x33);
    check(sms_read8(0x8000)==0x02,"ROM write remains protected");
    sms_write8(0xFFFC,0x08);
    check(sms_read8(0xFFFC)==0x08,"mapper control mirrored into system RAM");
    check(sms_read8(0x8000)==0xFF,"unwritten cartridge RAM is erased");
    check(sms_slot_bank(0x8000)==-1,"SRAM is not ROM code bank");
    sms_write8(0x8000,0x11);
    sms_write8(0xBFFF,0x22);
    sms_write8(0xFFFC,0x0C);
    check(sms_read8(0x8000)==0xFF,"SRAM bank1 distinct from bank0");
    sms_write8(0x8000,0x44);
    sms_write8(0xBFFF,0x55);
    sms_write8(0xFFFC,0x08);
    check(sms_read8(0x8000)==0x11 && sms_read8(0xBFFF)==0x22,"SRAM bank0 survives switch");
    sms_write8(0xFFFC,0x0C);
    check(sms_read8(0x8000)==0x44 && sms_read8(0xBFFF)==0x55,"SRAM bank1 survives switch");
    sms_write8(0xFFFC,0x00);
    check(sms_read8(0x8000)==0x02,"ROM restored after SRAM disabled");
    check(sms_slot_bank(0x8000)==2,"ROM bank restored to slot2");
    sms_write8(0xFFFD,3);
    check(sms_read8(0x0000)==0x00 && sms_read8(0x0400)==3,"fixed 1KiB and switched rest of slot0");
    check(glue_flush_sram(),"write atomic save image");
    puts("SRAM mapper and persistence: PASS");
    return 0;
}
