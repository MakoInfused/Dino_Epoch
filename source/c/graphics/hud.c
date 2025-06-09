#include "source/c/neslib.h"
#include "source/c/graphics/hud.h"
#include "source/c/globals.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/library/itoa.h"

CODE_BANK(PRG_BANK_HUD);

char number[3];

void draw_hud(void) {
    vram_adr(NAMETABLE_A + HUD_POSITION_START);
    for (i = 0; i != 160; ++i) {
        vram_put(HUD_TILE_BLANK);
    }
    vram_put(HUD_TILE_BORDER_BL);
    for (i = 0; i != 30; ++i) {
        vram_put(HUD_TILE_BORDER_HORIZONTAL);
    }
    vram_put(HUD_TILE_BORDER_BR);

    vram_adr(NAMETABLE_A + HUD_ATTRS_START);
    for (i = 0; i != 16; ++i) {
        vram_put(0xff);
    }
}

void update_hud(void) {
    // Make sure we don't update vram while editing the screenBuffer array, or we could get visual glitches.
    set_vram_update(NULL);

    if ((frameCount & 0x0f) != 0) {
        // This sets up screenBuffer to print x hearts, then x more empty hearts.
        // You give it the address, tell it the direction to write, the length to write,
        // then follow up with Ids, ending with NT_UPD_EOF
        
        // We use i for the index on screen buffer, so we don't have to shift things around
        // as we add values.
        i = 0;

        // First, draw the player name
        screenBuffer[i++] = MSB(NAMETABLE_A + HUD_POSITION_PLAYER_TEXT_START) | NT_UPD_HORZ;
        screenBuffer[i++] = LSB(NAMETABLE_A + HUD_POSITION_PLAYER_TEXT_START);
        screenBuffer[i++] = 6;
        // Next, we write a quick loop to go over each character of player and write it.
        for (j = 0; j != 6; ++j) {
            screenBuffer[i++] = HUD_TILE_PLAYER_TEXT + j;
        }

        // Next, draw the player hearts.
        screenBuffer[i++] = MSB(NAMETABLE_A + HUD_HEART_START) | NT_UPD_HORZ;
        screenBuffer[i++] = LSB(NAMETABLE_A + HUD_HEART_START);
        screenBuffer[i++] = playerMaxHealth;
        // Add one heart for every health the player has
        for (j = 0; j != playerHealth; ++j) {
            screenBuffer[i++] = HUD_TILE_HEART;
        }
        // Using the same variable, add empty hearts up to max health
        for (; j != playerMaxHealth; ++j) {
            screenBuffer[i++] = HUD_TILE_HEART_EMPTY;
        }

        // Next, draw the money count, using the money tile, and our money count variable
        screenBuffer[i++] = MSB(NAMETABLE_A + HUD_MONEY_START) | NT_UPD_HORZ;
        screenBuffer[i++] = LSB(NAMETABLE_A + HUD_MONEY_START);
        screenBuffer[i++] = 4;
        screenBuffer[i++] = HUD_TILE_MONEY;
        itoa(playerMoneyCount, number);
        for (j = 0; j != 3; ++j) {
            screenBuffer[i++] = HUD_TILE_NUMBER + number[j] + 0xD0;
        }

        // Next, draw the key count, using the key tile, and our key count variable
        screenBuffer[i++] = MSB(NAMETABLE_A + HUD_KEY_START) | NT_UPD_HORZ;
        screenBuffer[i++] = LSB(NAMETABLE_A + HUD_KEY_START);
        screenBuffer[i++] = 2;
        screenBuffer[i++] = HUD_TILE_KEY;
        screenBuffer[i++] = HUD_TILE_NUMBER + playerKeyCount;

        screenBuffer[i++] = NT_UPD_EOF;
        set_vram_update(screenBuffer);
    } else {
        // Every 32 frames, we replace the entire "ocean" tile in the background
        // with a different one from our `ocean_tiles` array.

        screenBuffer[0] = MSB(PPU_PATTERN_TABLE_0_ADDRESS + 0x880) | NT_UPD_HORZ;
        screenBuffer[1] = LSB(PPU_PATTERN_TABLE_0_ADDRESS + 0x880);
        screenBuffer[2] = 16;
        
        // j will be our index off of ocean_tiles - if we're on an odd frame, we
        // will set this to 0, otherwise we will set it to the length of 1 
        // tile (16 bytes) so we get the second one.
        if ((frameCount & 0x10) == 0x10) {
            j = 0;
        } else {
            j = 16;
        }

        for (i = 0; i != 16; ++i) {
            screenBuffer[i+3] = ocean_tiles[j++];
        }
        screenBuffer[i+3] = NT_UPD_EOF;
        set_vram_update(screenBuffer);
    }

}