#include "source/c/menus/pause.h"
#include "graphics/palettes/palettes.config.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/globals.h"
#include "source/c/neslib.h"
#include "source/c/configuration/game_states.h"
#include "source/c/menus/text_helpers.h"
#include "source/c/menus/input_helpers.h"
#include "source/c/sprites/player.h"
#include "source/c/map/map.h"
#include "source/c/library/itoa.h"

CODE_BANK(PRG_BANK_PAUSE_MENU);

#define currentMenuItem tempChar1

char positionBuffer[10];

void draw_pause_screen(void) {
    ppu_off();
    clear_screen_with_border();
    // We reuse the title palette here, though we have the option of making our own if needed.
    pal_bg(titlePalette);
    pal_spr(titlePalette);
    scroll(0, 0);

	set_chr_bank_0(CHR_BANK_MENU);

    // Just write "- Paused -" on the screen... there's plenty of nicer things you could do if you wanna spend time!
    put_str(NTADR_A(11, 13), "- Paused -");

    put_str(NTADR_A(10, 18), "Continue");
    put_str(NTADR_A(10, 20), "Reset");

    // itoa(playerOverworldPosition, positionBuffer);
    // put_str(NTADR_A(10, 22), positionBuffer);
    // itoa(playerXPosition, positionBuffer);
    // put_str(NTADR_A(14, 22), positionBuffer);
    // itoa(playerYPosition, positionBuffer);
    // put_str(NTADR_A(20, 22), positionBuffer);

    // We purposely leave sprites off, so they do not clutter the view. 
    // This means all menu drawing must be done with background tiles - if you want to use sprites (eg for a menu item),
    // you will have to hide all sprites, then put them back after. 
    ppu_on_bg();
}

void handle_pause_input(void) {
    currentMenuItem = 0;

    // Fill screenBuffer with the information required to update the menu every tile - we'll then edit this
    // array in-line each frame.
    i = 0;
    screenBuffer[i++] = MSB(NTADR_A(8, 18)) | NT_UPD_VERT;
    screenBuffer[i++] = LSB(NTADR_A(8, 18));
    screenBuffer[i++] = 6; // Number of tiles to draw
    screenBuffer[i++] = MENU_ARROW_TILE;
    for (; i != 9; ++i) {
        screenBuffer[i] = MENU_BLANK_TILE;
    }
    screenBuffer[i] = NT_UPD_EOF;

    set_vram_update(screenBuffer);

    while (1) {
        lastControllerState = controllerState;
        controllerState = pad_poll(0);

        // If Start is pressed now, and was not pressed before...
        if (controllerState & PAD_START && !(lastControllerState & PAD_START)) {
            if (currentMenuItem == 0) { // Resume game by exiting the game loop.
                break;
            } else if (currentMenuItem == 1) { // Reset game
                // This function resets the entire game; use it with extreme caution.
                // No code past this function call will run.
                reset();
            }
        }

        if (controllerState & PAD_UP && !(lastControllerState & PAD_UP)) {
            if (currentMenuItem != 0) {
                --currentMenuItem;
            }
        }

        if (controllerState & PAD_DOWN && !(lastControllerState & PAD_DOWN)) {
            if (currentMenuItem != 1) {
                ++currentMenuItem;
            }
        }

        screenBuffer[3] = (currentMenuItem == 0 ? MENU_ARROW_TILE : MENU_BLANK_TILE);
        screenBuffer[5] = (currentMenuItem == 1 ? MENU_ARROW_TILE : MENU_BLANK_TILE);

        ppu_wait_nmi();
    }

    sfx_play(SFX_PAUSE, SFX_CHANNEL_1);
    gameState = GAME_STATE_RUNNING;
    set_vram_update(NULL);
}