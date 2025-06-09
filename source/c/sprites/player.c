#include "source/c/neslib.h"
#include "source/c/sprites/player.h"
#include "source/c/library/bank_helpers.h"
#include "source/c/globals.h"
#include "source/c/map/map.h"
#include "graphics/palettes/palettes.config.h"
#include "source/c/configuration/game_states.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/sprites/collision.h"
#include "source/c/sprites/sprite_definitions.h"
#include "source/c/sprites/map_sprites.h"
#include "source/c/menus/error.h"
#include "source/c/graphics/hud.h"
#include "source/c/graphics/game_text.h"
#include "source/c/sprites/map_sprites.h"

CODE_BANK(PRG_BANK_PLAYER_SPRITE);

// Some useful global variables
ZEROPAGE_DEF(int, playerXPosition);
ZEROPAGE_DEF(int, playerYPosition);
ZEROPAGE_DEF(int, playerXVelocity);
ZEROPAGE_DEF(int, playerYVelocity);
ZEROPAGE_DEF(int, nextPlayerXPosition);
ZEROPAGE_DEF(int, nextPlayerYPosition);
ZEROPAGE_DEF(unsigned char, playerControlsLockTime);
ZEROPAGE_DEF(unsigned char, playerInvulnerabilityTime);
ZEROPAGE_DEF(unsigned char, playerDirection);
ZEROPAGE_DEF(unsigned char, swordPosition);
ZEROPAGE_DEF(unsigned char, shieldActive);
ZEROPAGE_DEF(unsigned char, hasSword);
ZEROPAGE_DEF(unsigned char, hasSwordUpgrade);
ZEROPAGE_DEF(unsigned char, hasShield);

// Huge pile of temporary variables
#define rawXPosition tempChar1
#define rawYPosition tempChar2
#define rawTileId tempChar3
#define collisionTempX tempChar4
#define collisionTempY tempChar5
#define collisionTempXRight tempChar6
#define collisionTempYBottom tempChar7
#define collisionTempDirection tempChar8
#define enemyDamageDealt tempChar9

#define tempSpriteCollisionX tempInt1
#define tempSpriteCollisionY tempInt2

#define collisionTempXInt tempInt3
#define collisionTempYInt tempInt4

 const unsigned char* shieldText1 = 
                                "Legends tell of a magical     "
                                "shield hidden in a cave       "
                                "nearby!                       ";
 const unsigned char* shieldText2 = 
                                "Nice, you got the shield!     "
                                "                              "
                                "                              "
                                "Look out for heart containers!"
                                "They increase your health     "
                                "permanently!                  "
                                "Hope you're having fun!       "
                                "                              "
                                "- Mako Infused";
 const unsigned char* shieldText3 = 
                                "You can now defend yourself   "
                                "from enemy attacks!           "
                                "                              "
                                "You can protect yourself from "
                                "flames too.                   "
                                "                              "
                                "All damage is reduced to a max"
                                "of 2 damage per hit, even if  "
                                "your shield is down!          "
                                "                              "
                                "Try holding the 'B' button.   ";
                                
const unsigned char* swordText1 = 
                                "I droppped my sword in the    "
                                "cave of interconnected webs.  "
                                "It is to the west of here.    "
                                "I fear one the trolls snatched"
                                "it, if you find it you can    "
                                "keep it for yourself.         ";
const unsigned char* swordText2 = 
                                "You can now attack enemies    "
                                "in order to slay them!        "
                                "                              "
                                "                              "
                                "Try pressing the 'A' button.  ";
const unsigned char* swordText3 = 
                                "Great job, you found my sword!"
                                "If you bring me $100, I can   "
                                "make it deal twice the damage!";
const unsigned char* swordText4 = 
                                "I've upgraded your sword!     "
                                "Now go forth and vanquish     "
                                "those foul fiends!            ";

 const unsigned char* boyText = 
                                "Welcome to this grand epoch!  " 
                                "I hope thy farest most well.  ";
 const unsigned char* girlText = 
                                "You look strange traveller.   " 
                                "Where are you from?           "
                                "Bosnia you say?               "
                                "I've never of it.             ";

 const unsigned char* dukeText = 
                                "My kingdom ... all in ruins.  " 
                                "                              "
                                "                              "
                                "Full of trolls now, just like "
                                "the caves of interconnected   "
                                "webs.                         ";

 const unsigned char* ladyErinText = 
                                "I don't needing saving, but..." 
                                "I'm glad you were here!       "
                                "                              "
                                "Let's go and make the world   "
                                "a better place... together.   "
                                "What's that, darling?         "
                                "                              "
                                "I love you too!               ";

// NOTE: This uses tempChar1 through tempChar3; the caller must not use these.
void update_player_sprite(void) {
    // Calculate the position of the player itself, then use these variables to build it up with 4 8x8 NES sprites.
    rawXPosition = (playerXPosition >> PLAYER_POSITION_SHIFT);
    rawYPosition = (playerYPosition >> PLAYER_POSITION_SHIFT);
    rawTileId = PLAYER_SPRITE_TILE_ID + playerDirection;

    if (playerXVelocity != 0 || playerYVelocity != 0) {
        // Does some math with the current NES frame to add either 2 or 0 to the tile id, animating the sprite.
        rawTileId += ((frameCount >> SPRITE_ANIMATION_SPEED_DIVISOR) & 0x01) << 1;
    }

    // Clamp the player's sprite X Position to 0 to make sure we don't loop over, even if technically we have.
    if (rawXPosition > (SCREEN_EDGE_RIGHT + 4)) {
        rawXPosition = 0;
    }
    
    if (playerInvulnerabilityTime && frameCount & PLAYER_INVULNERABILITY_BLINK_MASK) {
        // If the player is invulnerable, we hide their sprite about half the time to do a flicker animation.
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, rawTileId, 0x00, PLAYER_SPRITE_INDEX);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, rawTileId + 1, 0x00, PLAYER_SPRITE_INDEX+4);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, rawTileId + 16, 0x00, PLAYER_SPRITE_INDEX+8);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, rawTileId + 17, 0x00, PLAYER_SPRITE_INDEX+12);

    } else {
        oam_spr(rawXPosition, rawYPosition, rawTileId, 0x00, PLAYER_SPRITE_INDEX);
        oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition, rawTileId + 1, 0x00, PLAYER_SPRITE_INDEX+4);
        oam_spr(rawXPosition, rawYPosition + NES_SPRITE_HEIGHT, rawTileId + 16, 0x00, PLAYER_SPRITE_INDEX+8);
        oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition + NES_SPRITE_HEIGHT, rawTileId + 17, 0x00, PLAYER_SPRITE_INDEX+12);
    }

    if (swordPosition) {
        switch (playerDirection) {
            case SPRITE_DIRECTION_RIGHT:
                oam_spr(rawXPosition + swordPosition, rawYPosition + 6, PLAYER_SWORD_TILE_ID_H1, 0x00, PLAYER_EQUIP_OAM_LOCATION);
                oam_spr(rawXPosition + swordPosition + NES_SPRITE_WIDTH, rawYPosition + 6, PLAYER_SWORD_TILE_ID_H2, 0x00, PLAYER_EQUIP_OAM_LOCATION + 0x04);
                break;
            case SPRITE_DIRECTION_LEFT:
                oam_spr(rawXPosition - swordPosition, rawYPosition + 6, PLAYER_SWORD_TILE_ID_H2, OAM_FLIP_H, PLAYER_EQUIP_OAM_LOCATION);
                oam_spr(rawXPosition - swordPosition + NES_SPRITE_WIDTH, rawYPosition + 6, PLAYER_SWORD_TILE_ID_H1, OAM_FLIP_H, PLAYER_EQUIP_OAM_LOCATION + 0x04);
                break;
            case SPRITE_DIRECTION_DOWN:
                oam_spr(rawXPosition + 6, rawYPosition + swordPosition, PLAYER_SWORD_TILE_ID_V1, OAM_FLIP_V, PLAYER_EQUIP_OAM_LOCATION);
                oam_spr(rawXPosition + 6, rawYPosition + swordPosition + NES_SPRITE_WIDTH, PLAYER_SWORD_TILE_ID_V2, OAM_FLIP_V, PLAYER_EQUIP_OAM_LOCATION + 0x04);
                break;
            case SPRITE_DIRECTION_UP:
                oam_spr(rawXPosition + 2, rawYPosition - swordPosition + 2, PLAYER_SWORD_TILE_ID_V2, 0x00, PLAYER_EQUIP_OAM_LOCATION);
                oam_spr(rawXPosition + 2, rawYPosition - swordPosition + 2 + NES_SPRITE_WIDTH, PLAYER_SWORD_TILE_ID_V1, 0x00, PLAYER_EQUIP_OAM_LOCATION + 0x04);
                break;
        }
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V1, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V2, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x04);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V3, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x08);
        oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V4, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x0C);
    } else {
        if (shieldActive) {
            switch (playerDirection) {
                case SPRITE_DIRECTION_RIGHT:
                    oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition + 6, PLAYER_SHIELD_TILE_ID_H1, OAM_FLIP_H, PLAYER_EQUIP_OAM_LOCATION);
                    oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition + 6, PLAYER_SHIELD_TILE_ID_H2, OAM_FLIP_H, PLAYER_EQUIP_OAM_LOCATION + 0x04);

                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V1, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V2, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x04);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V3, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x08);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V4, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x0C);
                    break;
                case SPRITE_DIRECTION_LEFT:
                    oam_spr(rawXPosition, rawYPosition + 6, PLAYER_SHIELD_TILE_ID_H2, 0x00, PLAYER_EQUIP_OAM_LOCATION);
                    oam_spr(rawXPosition, rawYPosition + 6, PLAYER_SHIELD_TILE_ID_H1, 0x00, PLAYER_EQUIP_OAM_LOCATION + 0x04);

                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V1, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V2, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x04);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V3, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x08);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V4, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x0C);
                    break;
                case SPRITE_DIRECTION_DOWN:
                    oam_spr(rawXPosition, rawYPosition - 4 + NES_SPRITE_HEIGHT, PLAYER_SHIELD_TILE_ID_V1, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION);
                    oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition - 4 + NES_SPRITE_HEIGHT, PLAYER_SHIELD_TILE_ID_V2, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x04);
                    oam_spr(rawXPosition, rawYPosition - 4 + NES_SPRITE_HEIGHT + NES_SPRITE_HEIGHT, PLAYER_SHIELD_TILE_ID_V3, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x08);
                    oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition - 4 + NES_SPRITE_HEIGHT + NES_SPRITE_HEIGHT, PLAYER_SHIELD_TILE_ID_V4, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x0C);
                    
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_H1, 0x00, PLAYER_EQUIP_OAM_LOCATION);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_H2, 0x00, PLAYER_EQUIP_OAM_LOCATION + 0x04);
                    break;
                case SPRITE_DIRECTION_UP:
                    oam_spr(rawXPosition, rawYPosition + 4 - NES_SPRITE_HEIGHT, PLAYER_SHIELD_TILE_ID_V1, 0x00, PLAYER_EQUIP_OAM_LOCATION);
                    oam_spr(rawXPosition + NES_SPRITE_WIDTH, rawYPosition + 4 - NES_SPRITE_HEIGHT, PLAYER_SHIELD_TILE_ID_V2, 0x00, PLAYER_EQUIP_OAM_LOCATION + 0x04);

                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V1, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V2, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x04);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V3, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x08);
                    oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V4, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x0C);
                    break;
            }
        } else {
            
            oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V1, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION);
            oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V2, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x04);
            oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V3, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x08);
            oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_V4, 0x00, PLAYER_EQUIP_ABOVE_OAM_LOCATION + 0x0C);
            oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_H1, 0x00, PLAYER_EQUIP_OAM_LOCATION);
            oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, PLAYER_SHIELD_TILE_ID_H2, 0x00, PLAYER_EQUIP_OAM_LOCATION + 0x04);
        }
    }
}

void prepare_player_movement(void) {
    // Using a variable, so we can change the velocity based on pressing a button, having a special item,
    // or whatever you like!
    int maxVelocity = PLAYER_MAX_VELOCITY;
    lastControllerState = controllerState;
    controllerState = pad_poll(0);

    if (swordPosition == 0 && hasSword && controllerState & PAD_A && !(lastControllerState & PAD_A)) {
        swordPosition = PLAYER_SWORD_POSITION_FULLY_EXTENDED;
    } else if (swordPosition != 0) {
        swordPosition -= 1;
    }

    if (swordPosition == 0 && hasShield && controllerState & PAD_B) {
        shieldActive = 1;
    } else {
        shieldActive = 0;
    }

    // If Start is pressed now, and was not pressed before...
    if (controllerState & PAD_START && !(lastControllerState & PAD_START)) {
        gameState = GAME_STATE_PAUSED;
        sfx_play(SFX_PAUSE, SFX_CHANNEL_1);
        return;
    }
    if (playerControlsLockTime) {
        // If your controls are locked, just tick down the timer until they stop being locked. Don't read player input.
        playerControlsLockTime--;
    } else {
        if (controllerState & PAD_RIGHT && playerXVelocity >= (0 - PLAYER_VELOCITY_NUDGE)) {
            // If you're moving right, and you're not at max, speed up.
            if (playerXVelocity < maxVelocity) {
                playerXVelocity += PLAYER_VELOCITY_ACCEL;
            // If you're over max somehow, we'll slow you down a little.
            } else if (playerXVelocity > maxVelocity) {
                playerXVelocity -= PLAYER_VELOCITY_ACCEL;
            }
        } else if (controllerState & PAD_LEFT && playerXVelocity <= (0 + PLAYER_VELOCITY_NUDGE)) {
            // If you're moving left, and you're not at max, speed up.
            if (ABS(playerXVelocity) < maxVelocity) {
                playerXVelocity -= PLAYER_VELOCITY_ACCEL;
            // If you're over max, slow you down a little...
            } else if (ABS(playerXVelocity) > maxVelocity) { 
                playerXVelocity += PLAYER_VELOCITY_ACCEL;
            }
        } else if (playerXVelocity != 0) {
            // Not pressing anything? Let's slow you back down...
            if (playerXVelocity > 0) {
                playerXVelocity -= PLAYER_VELOCITY_ACCEL;
            } else {
                playerXVelocity += PLAYER_VELOCITY_ACCEL;
            }
        }

        if (controllerState & PAD_UP && playerYVelocity <= (0 + PLAYER_VELOCITY_NUDGE)) {
            if (ABS(playerYVelocity) < maxVelocity) {
                playerYVelocity -= PLAYER_VELOCITY_ACCEL;
            } else if (ABS(playerYVelocity) > maxVelocity) {
                playerYVelocity += PLAYER_VELOCITY_ACCEL;
            }
        } else if (controllerState & PAD_DOWN && playerYVelocity >= (0 - PLAYER_VELOCITY_NUDGE)) {
            if (playerYVelocity < maxVelocity) {
                playerYVelocity += PLAYER_VELOCITY_ACCEL;
            } else if (playerYVelocity > maxVelocity) {
                playerYVelocity -= PLAYER_VELOCITY_ACCEL;
            }
        } else { 
            if (playerYVelocity > 0) {
                playerYVelocity -= PLAYER_VELOCITY_ACCEL;
            } else if (playerYVelocity < 0) {
                playerYVelocity += PLAYER_VELOCITY_ACCEL;
            }
        }
        
        // Now, slowly adjust to the grid as possible, if the player isn't pressing a direction. 
        #if PLAYER_MOVEMENT_STYLE == MOVEMENT_STYLE_GRID
            if (playerYVelocity != 0 && playerXVelocity == 0 && (controllerState & (PAD_LEFT | PAD_RIGHT)) == 0) {
                if ((char)((playerXPosition + PLAYER_POSITION_TILE_MASK_MIDDLE) & PLAYER_POSITION_TILE_MASK) > (char)(PLAYER_POSITION_TILE_MASK_MIDDLE)) {
                    playerXVelocity = 0-PLAYER_VELOCITY_NUDGE;
                } else if ((char)((playerXPosition + PLAYER_POSITION_TILE_MASK_MIDDLE) & PLAYER_POSITION_TILE_MASK) < (char)(PLAYER_POSITION_TILE_MASK_MIDDLE)) {
                    playerXVelocity = PLAYER_VELOCITY_NUDGE;
                } // If equal, do nothing. That's our goal.
            }

            if (playerXVelocity != 0 && playerYVelocity == 0 && (controllerState & (PAD_UP | PAD_DOWN)) == 0) {
                if ((char)((playerYPosition + PLAYER_POSITION_TILE_MASK_MIDDLE + PLAYER_POSITION_TILE_MASK_EXTRA_NUDGE) & PLAYER_POSITION_TILE_MASK) > (char)(PLAYER_POSITION_TILE_MASK_MIDDLE)) {
                    playerYVelocity = 0-PLAYER_VELOCITY_NUDGE;
                } else if ((char)((playerYPosition + PLAYER_POSITION_TILE_MASK_MIDDLE + PLAYER_POSITION_TILE_MASK_EXTRA_NUDGE) & PLAYER_POSITION_TILE_MASK) < (char)(PLAYER_POSITION_TILE_MASK_MIDDLE)) {
                    playerYVelocity = PLAYER_VELOCITY_NUDGE;
                } // If equal, do nothing. That's our goal.
            }
        #endif

    }

    // While we're at it, tick down the invulnerability timer if needed
    if (playerInvulnerabilityTime) {
        playerInvulnerabilityTime--;
    }

    nextPlayerXPosition = playerXPosition + playerXVelocity;
    nextPlayerYPosition = playerYPosition + playerYVelocity;

}

void do_player_movement(void) {

    // This will knock out the player's speed if they hit anything.
    test_player_tile_collision();
    // If the new player position hit any sprites, we'll find that out and knock it out here.
    handle_player_sprite_collision();

    playerXPosition += playerXVelocity;
    playerYPosition += playerYVelocity;


    rawXPosition = (playerXPosition >> PLAYER_POSITION_SHIFT);
    rawYPosition = (playerYPosition >> PLAYER_POSITION_SHIFT);
    // The X Position has to wrap to allow us to snap to the grid. This makes us shift when you wrap around to 255, down to 240-ish
    if (rawXPosition > (SCREEN_EDGE_RIGHT+4) && rawXPosition < SCREEN_EDGE_LEFT_WRAPPED) {
        // We use sprite direction to determine which direction to scroll in, so be sure this is set properly.
        if (playerInvulnerabilityTime) {
            playerXPosition -= playerXVelocity;
            rawXPosition = (playerXPosition >> PLAYER_POSITION_SHIFT);
        } else {
            playerDirection = SPRITE_DIRECTION_LEFT;
            gameState = GAME_STATE_SCREEN_SCROLL;
            playerOverworldPosition--;
        }
    } else if (rawXPosition > SCREEN_EDGE_RIGHT && rawXPosition < (SCREEN_EDGE_RIGHT+4)) {
        if (playerInvulnerabilityTime) {
            playerXPosition -= playerXVelocity;
            rawXPosition = (playerXPosition >> PLAYER_POSITION_SHIFT);
        } else {
            playerDirection = SPRITE_DIRECTION_RIGHT;
            gameState = GAME_STATE_SCREEN_SCROLL;
            playerOverworldPosition++;
        }
    } else if (rawYPosition > SCREEN_EDGE_BOTTOM) {
        if (playerInvulnerabilityTime) {
            playerYPosition -= playerYVelocity;
            rawYPosition = (playerYPosition >> PLAYER_POSITION_SHIFT);
        } else {
            playerDirection = SPRITE_DIRECTION_DOWN;
            gameState = GAME_STATE_SCREEN_SCROLL;
            playerOverworldPosition += 8;
        }
    } else if (rawYPosition < SCREEN_EDGE_TOP) {
        if (playerInvulnerabilityTime) {
            playerYPosition -= playerYVelocity;
            rawYPosition = (playerYPosition >> PLAYER_POSITION_SHIFT);
        } else {
            playerDirection = SPRITE_DIRECTION_UP;
            gameState = GAME_STATE_SCREEN_SCROLL;
            playerOverworldPosition -= 8;
        }
    }
}

void test_player_tile_collision(void) {

    if (playerYVelocity != 0) {
        collisionTempYInt = playerYPosition + PLAYER_Y_OFFSET_EXTENDED + playerYVelocity;
        collisionTempXInt = playerXPosition + PLAYER_X_OFFSET_EXTENDED;

        collisionTempY = ((collisionTempYInt) >> PLAYER_POSITION_SHIFT) - HUD_PIXEL_HEIGHT;
        collisionTempX = ((collisionTempXInt) >> PLAYER_POSITION_SHIFT);

        collisionTempYInt += PLAYER_HEIGHT_EXTENDED;
        collisionTempXInt += PLAYER_WIDTH_EXTENDED;

        collisionTempYBottom = ((collisionTempYInt) >> PLAYER_POSITION_SHIFT) - HUD_PIXEL_HEIGHT;
        collisionTempXRight = (collisionTempXInt) >> PLAYER_POSITION_SHIFT;
        
        // Due to how we are calculating the sprite's position, there is a slight chance we can exceed the area that our
        // map takes up near screen edges. To compensate for this, we clamp the Y position of our character to the 
        // window between 0 and 192 pixels, which we can safely test collision within.

        // If collisionTempY is > 240, it can be assumed we calculated a position less than zero, and rolled over to 255
        if (collisionTempY > 240) {
            collisionTempY = 0;
        }
        // The lowest spot we can test collision is at pixel 192 (the 12th 16x16 tile). If we're past that, bump ourselves back.
        if (collisionTempYBottom > 190) { 
            collisionTempYBottom = 190;
        }

        if (playerYVelocity < 0) {
            // We're going up - test the top left, and top right
            if (test_collision(currentMap[PLAYER_MAP_POSITION(collisionTempX, collisionTempY)], 1) || test_collision(currentMap[PLAYER_MAP_POSITION(collisionTempXRight, collisionTempY)], 1)) {
                playerYVelocity = 0;
                playerControlsLockTime = 0;
            }
            if (!playerControlsLockTime && playerYVelocity < (0-PLAYER_VELOCITY_NUDGE)) {
                playerDirection = SPRITE_DIRECTION_UP;
            }
        } else {
            // Okay, we're going down - test the bottom left and bottom right
            if (test_collision(currentMap[PLAYER_MAP_POSITION(collisionTempX, collisionTempYBottom)], 1) || test_collision(currentMap[PLAYER_MAP_POSITION(collisionTempXRight, collisionTempYBottom)], 1)) {
                playerYVelocity = 0;
                playerControlsLockTime = 0;

            }
            if (!playerControlsLockTime && playerYVelocity > PLAYER_VELOCITY_NUDGE) {
                playerDirection = SPRITE_DIRECTION_DOWN;
            }
        }
    }

    if (playerXVelocity != 0) {
        collisionTempXInt = playerXPosition + PLAYER_X_OFFSET_EXTENDED + playerXVelocity;
        collisionTempYInt = playerYPosition + PLAYER_Y_OFFSET_EXTENDED + playerYVelocity;
        collisionTempX = (collisionTempXInt) >> PLAYER_POSITION_SHIFT;
        collisionTempY = ((collisionTempYInt) >> PLAYER_POSITION_SHIFT) - HUD_PIXEL_HEIGHT;

        collisionTempXInt += PLAYER_WIDTH_EXTENDED;
        collisionTempYInt += PLAYER_HEIGHT_EXTENDED;

        collisionTempYBottom = ((collisionTempYInt) >> PLAYER_POSITION_SHIFT) - HUD_PIXEL_HEIGHT;
        collisionTempXRight = ((collisionTempXInt) >> PLAYER_POSITION_SHIFT);
        // The lowest spot we can test collision is at pixel 192 (the 12th 16x16 tile). If we're past that, bump ourselves back.
        if (collisionTempYBottom > 190) {
            collisionTempYBottom = 190;
        }

        // Depending on how far to the left/right the player is, there's a chance part of our bounding box falls into
        // the next column of tiles on the opposite side of the screen. This if statement prevents those collisions.
        if (collisionTempX > 2 && collisionTempX < 238) {
            if (playerXVelocity < 0) {
                // Okay, we're moving left. Need to test the top-left and bottom-left
                if (test_collision(currentMap[PLAYER_MAP_POSITION(collisionTempX, collisionTempY)], 1) || test_collision(currentMap[PLAYER_MAP_POSITION(collisionTempX, collisionTempYBottom)], 1)) {
                    playerXVelocity = 0;
                    playerControlsLockTime = 0;

                }
                if (!playerControlsLockTime && playerXVelocity < (0-PLAYER_VELOCITY_NUDGE)) {
                    playerDirection = SPRITE_DIRECTION_LEFT;
                }
            } else {
                // Going right - need to test top-right and bottom-right
                if (test_collision(currentMap[PLAYER_MAP_POSITION(collisionTempXRight, collisionTempY)], 1) || test_collision(currentMap[PLAYER_MAP_POSITION(collisionTempXRight, collisionTempYBottom)], 1)) {
                    playerXVelocity = 0;
                    playerControlsLockTime = 0;

                }
                if (!playerControlsLockTime && playerXVelocity > PLAYER_VELOCITY_NUDGE) {
                    playerDirection = SPRITE_DIRECTION_RIGHT;
                }
            }
        }
    }
}

void handle_player_sprite_collision(void) {
if (warpCooldownTime != 0) {
    --warpCooldownTime;
}

if (lastPlayerWeaponCollisionId != NO_SPRITE_HIT) {
        currentMapSpriteIndex = lastPlayerWeaponCollisionId<<MAP_SPRITE_DATA_SHIFT;
        switch (currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE]) {
            // If we hit an item, just use the normal method to collect it; let it override whatever we hit for a cycle.
            case SPRITE_TYPE_HEALTH:
            case SPRITE_TYPE_LIFE_UP:
            case SPRITE_TYPE_MONEY:
            case SPRITE_TYPE_KEY:
                // sfx_play(SFX_CONFIRM, SFX_CHANNEL_3);
                // lastPlayerSpriteCollisionId = lastPlayerWeaponCollisionId;
                break;
            case SPRITE_TYPE_DOOR:
            case SPRITE_TYPE_WARP_DOOR:
                lastPlayerSpriteCollisionId = lastPlayerWeaponCollisionId;
                break;
            case SPRITE_TYPE_REGULAR_ENEMY:
            case SPRITE_TYPE_BOSS:
                // If the sprite is already in the invulnerability animation, just skip out.
                if (currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_INVULN_COUNTDOWN]) {
                    break;
                }
                // Kinda complex logic warning: in the if statement below, we subtract 1 from the enemy's health, then
                // compare it with 0. If it is zero, we remove the sprite; otherwise we bump the invulnerability timer.
                sfx_play(SFX_ENEMY_HURT, SFX_CHANNEL_1);

                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH] -= hasSwordUpgrade ? 2 : 1;

                if (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH] == 0
                    || currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH] > 240) {
                    
                    if(currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_TYPE] == SPRITE_TYPE_BOSS) {
                        playerKeyCount++;
                        currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                        currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerWeaponCollisionId];
                    } else if((rand8() % 100) + 1 < 20) {
                        // Random chance to drop a heart, replacing the existing sprite.
                        // This has a bug which became a feature, where if the enemy drops a heart, and it doesn't get picked up:
                        // it will respawn the next time you reenter the map.
                        int spriteDefinitionIndex = 0x00;

                        // Copy the simple bytes from the sprite definition to replace the current enemy with one that has a different id.
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_TILE_ID] = spriteDefinitions[spriteDefinitionIndex + SPRITE_DEF_POSITION_TILE_ID];
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_TYPE] = spriteDefinitions[spriteDefinitionIndex + SPRITE_DEF_POSITION_TYPE];
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_SIZE_PALETTE] = spriteDefinitions[spriteDefinitionIndex + SPRITE_DEF_POSITION_SIZE_PALETTE];
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH] = spriteDefinitions[spriteDefinitionIndex + SPRITE_DEF_POSITION_HEALTH];
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_ANIMATION_TYPE] = spriteDefinitions[spriteDefinitionIndex + SPRITE_DEF_POSITION_ANIMATION_TYPE];
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_MOVEMENT_TYPE] = spriteDefinitions[spriteDefinitionIndex + SPRITE_DEF_POSITION_MOVEMENT_TYPE];
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_MOVE_SPEED] = spriteDefinitions[spriteDefinitionIndex + SPRITE_DEF_POSITION_MOVE_SPEED];
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DAMAGE] = spriteDefinitions[spriteDefinitionIndex + SPRITE_DEF_POSITION_DAMAGE];
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_INVULN_COUNTDOWN] = 0;
                    } else {
                        currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                        currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerWeaponCollisionId];
                    }
                } else {
                    // Bump the invulnerability timer for the enemy
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_INVULN_COUNTDOWN] = SPRITE_INVULNERABILITY_TIME;

                    if(currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] != SPRITE_TYPE_BOSS) {
                        // Also launch the enemy in the other direction
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DIRECTION_TIME] = SPRITE_INVULNERABILITY_TIME;

                        // Move them away from the player very quickly!
                        switch (playerDirection) {
                            case SPRITE_DIRECTION_LEFT:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_LEFT;
                                break;
                            case SPRITE_DIRECTION_RIGHT:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_RIGHT;
                                break;
                            case SPRITE_DIRECTION_DOWN:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_DOWN;
                                break;
                            case SPRITE_DIRECTION_UP:
                            default: // If they weren't heading in a direction, they can also go up.Add commentMore actions
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_UP;
                                break;
                        }
                    }

                    break;
                }
        }
    }

    // We store the last sprite hit when we update the sprites in `map_sprites.c`, so here all we have to do is react to it.
    if (lastPlayerSpriteCollisionId != NO_SPRITE_HIT) {
        currentMapSpriteIndex = lastPlayerSpriteCollisionId << MAP_SPRITE_DATA_SHIFT;
        switch (currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE]) {
            case SPRITE_TYPE_HEALTH:
                // This if statement ensures that we don't remove hearts if you don't need them yet.
                if (playerHealth < playerMaxHealth) {
                    playerHealth += currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH];
                    if (playerHealth > playerMaxHealth) {
                        playerHealth = playerMaxHealth;
                    }
                    // Hide the sprite now that it has been taken.
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;

                    // Play the heart sound!
                    sfx_play(SFX_HEART, SFX_CHANNEL_3);

                    // Mark the sprite as collected, so we can't get it again.
                    currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];
                }
                break;
            case SPRITE_TYPE_LIFE_UP:
                playerMaxHealth += 1;
                
                // Mark the sprite as collected, so we can't get it again.
                currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];

                sfx_play(SFX_CONFIRM, SFX_CHANNEL_3);
                break;
            case SPRITE_TYPE_KEY:
                if (playerKeyCount < MAX_KEY_COUNT) {
                    playerKeyCount++;

                    sfx_play(SFX_KEY, SFX_CHANNEL_3);

                    // Mark the sprite as collected, so we can't get it again.
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                    currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];
                }
                break;
            case SPRITE_TYPE_PALETTE_SWAP:
                switch (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH])
                {
                    case 0x01:
                        pal_spr(npcSpritePalette);
                        break;
                    case 0x00:
                    default:
                        pal_spr(mainSpritePalette);
                        break;
                }
                break;
            case SPRITE_TYPE_REGULAR_ENEMY:
            case SPRITE_TYPE_INVULNERABLE_ENEMY:
            case SPRITE_TYPE_EFFECT:
            case SPRITE_TYPE_BOSS:

                if (playerInvulnerabilityTime) {
                    return;
                }

                collisionTempDirection = 1;
                enemyDamageDealt = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DAMAGE];
                if(hasShield && enemyDamageDealt > 2) enemyDamageDealt = 2;
                
                // Attempts to damage player, if they don't have shield and aren't facing the enemy or the enemy is non-directional damage based one
                if(!shieldActive || currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] == SPRITE_TYPE_INVULNERABLE_ENEMY) {
                    playerHealth -= enemyDamageDealt;
                    // Since playerHealth is unsigned, we need to check for wraparound damage. 
                    // NOTE: If something manages to do more than 16 damage at once, this might fail.
                    if (playerHealth == 0 || playerHealth > 240) {
                        gameState = GAME_STATE_GAME_OVER;
                        music_stop();
                        sfx_play(SFX_GAMEOVER, SFX_CHANNEL_1);
                        return;
                    }
                    else if(enemyDamageDealt)
                    {
                        sfx_play(SFX_HURT, SFX_CHANNEL_2);
                        playerInvulnerabilityTime = PLAYER_DAMAGE_INVULNERABILITY_TIME;
                    }
                    else
                    {
                        return;
                    }
                }

                if(enemyDamageDealt) {
                    // Knock the player back
                    playerControlsLockTime = PLAYER_DAMAGE_CONTROL_LOCK_TIME;
                    if (playerDirection == SPRITE_DIRECTION_LEFT) {
                        // Punt them back in the opposite direction
                        playerXVelocity = PLAYER_MAX_VELOCITY;
                        // Reverse their velocity in the other direction, too.
                        playerYVelocity = 0 - playerYVelocity;
                    } else if (playerDirection == SPRITE_DIRECTION_RIGHT) {
                        playerXVelocity = 0-PLAYER_MAX_VELOCITY;
                        playerYVelocity = 0 - playerYVelocity;
                    } else if (playerDirection == SPRITE_DIRECTION_DOWN) {
                        playerYVelocity = 0-PLAYER_MAX_VELOCITY;
                        playerXVelocity = 0 - playerXVelocity;
                    } else { // Make being thrown downward into a catch-all, in case your direction isn't set or something.
                        playerYVelocity = PLAYER_MAX_VELOCITY;
                        playerXVelocity = 0 - playerXVelocity;
                    }
                }
                
                break;
            case SPRITE_TYPE_DOOR: 
                // Doors without locks are very simple - they just open! Hide the sprite until the user comes back...
                // note that we intentionally *don't* store this state, so it comes back next time.
                currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                break;
            case SPRITE_TYPE_LOCKED_DOOR:
                // First off, do you have a key? If so, let's just make this go away...
                if (playerKeyCount > 0) {
                    playerKeyCount--;
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;

                    // Mark the door as gone, so it doesn't come back.
                    currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];

                    break;
                }
                // So you don't have a key...
                // Okay, we collided with a door before we calculated the player's movement.
                // So, let's cancel it out and move on.
                playerXVelocity = 0;
                playerYVelocity = 0;
                playerControlsLockTime = 0;

                break;
            case SPRITE_TYPE_WARP_DOOR:
                // First, hide the sprite. We want to physically keep it around though, so just update the tile ids.
                currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TILE_ID] = SPRITE_TILE_ID_OFFSCREEN;

                // If we set a cooldown time, don't allow warping. This allows us to re-spawn the user into the doorway
                // in the other map without them immediately trying to teleport again.
                if (warpCooldownTime != 0) {
                    break;
                }

                // Next, figure out if the player is completely within the tile, and if so, teleport them.
                // Note that this isn't a normal collision test, so don't try to reuse it as one ;)
                
                // Calculate position of this sprite...
                tempSpriteCollisionX = ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X]) + ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X + 1]) << 8));
                tempSpriteCollisionY = ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y]) + ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y + 1]) << 8));

                // Test to see if the sprite is completely contained within the boundaries of this sprite. 
                // To make it a little loose, we make the sprite look like it is 2 pixels wider on all sides. 
                // (subtract 2 from x and y positions for comparison, then add 2 to the width and height.)
                if (
                    playerXPosition > tempSpriteCollisionX - (2 << PLAYER_POSITION_SHIFT) &&
                    playerXPosition + PLAYER_WIDTH_EXTENDED < tempSpriteCollisionX + (18 << PLAYER_POSITION_SHIFT) &&
                    playerYPosition > tempSpriteCollisionY - (2 << PLAYER_POSITION_SHIFT) &&
                    playerYPosition + PLAYER_HEIGHT_EXTENDED < tempSpriteCollisionY + (18 << PLAYER_POSITION_SHIFT)
                ) {
                    // Okay, we are going to warp the user!

                    // Set our cooldown timer, so when the user shows up on the new map inside the door, they are not teleported.
                    warpCooldownTime = DOOR_WARP_COOLDOWN_TIME;

                    // Determine which world map to place the player on based on the current map.
                    // NOTE: If you find yourself adding this logic somewhere else, consider putting it into a function.
                    // Additionally, this will not scale to having many maps. You may have to add a second array of characters
                    // and look up the world id like that.
                    if (currentWorldId == WORLD_OVERWORLD) {
                        // If you have more music, this would be a great place to switch!

                        // Swap worlds
                        currentWorldId = WORLD_UNDERWORLD;
                        // Look up what world map tile to place the player on, and switch to that tile.
                        playerOverworldPosition = playerOverworldPosition == OVERWORLD_SHIELD_ENTRANCE ? UNDERWORLD_SHIELD_ENTRANCE : UNDERWORLD_SWORD_ENTRANCE;
                    } else {
                        // If you have more music, this would be a great place to switch!

                        // Swap worlds
                        currentWorldId = WORLD_OVERWORLD;
                        // Look up what world map tile to place the player on, and switch to that tile.
                        playerOverworldPosition = playerOverworldPosition == UNDERWORLD_SHIELD_ENTRANCE ? OVERWORLD_SHIELD_ENTRANCE : OVERWORLD_SWORD_ENTRANCE;
                    }
                    // Switch to a new gameState that tells our game to animate your transition into this new world.
                    gameState = GAME_STATE_WORLD_TRANSITION;
                }
                break;
            case SPRITE_TYPE_MONEY:
                playerMoneyCount += currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH];
                sfx_play(SFX_CONFIRM, SFX_CHANNEL_3);

                // Mark the sprite as collected, so we can't get it again.
                currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];
                break;
            case SPRITE_TYPE_SHIELD:
                if (!hasShield) {
                    ++hasShield;

                    sfx_play(SFX_CONFIRM, SFX_CHANNEL_3);

                    // Mark the sprite as collected, so we can't get it again.
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                    currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];

                    trigger_game_text(shieldText3);
                }
                break;
            case SPRITE_TYPE_WEAPON:
                if (!hasSword) {
                    ++hasSword;

                    sfx_play(SFX_CONFIRM, SFX_CHANNEL_3);

                    // Mark the sprite as collected, so we can't get it again.
                    currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
                    currentMapSpritePersistance[playerOverworldPosition] |= bitToByte[lastPlayerSpriteCollisionId];

                    trigger_game_text(swordText2);
                }
                break;
            case SPRITE_TYPE_NPC:
                // Okay, we collided with this NPC before we calculated the player's movement. After being moved, does the 
                // new player position also collide? If so, stop it. Else, let it go.

                // Calculate position...
                tempSpriteCollisionX = ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X]) + ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X + 1]) << 8));
                tempSpriteCollisionY = ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y]) + ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y + 1]) << 8));

                if (controllerState & PAD_A && !(lastControllerState & PAD_A)) {
                    // Show the text for the player based on the NPC's type.
                    switch(currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_HEALTH])
                    {
                        default:
                        case 0x01:
                            if(!hasShield) {
                                trigger_game_text(shieldText1);
                            } else {
                                trigger_game_text(shieldText2);
                            }
                            break;
                        case 0x02:
                            if(!hasSword) {
                                trigger_game_text(swordText1);
                            } else {
                                if(!hasSwordUpgrade) {
                                    trigger_game_text(swordText3);
                                } else {
                                    if(playerMoneyCount > 100) {
                                        hasSwordUpgrade++;
                                        playerMoneyCount -= 100;
                                    }
                                    trigger_game_text(swordText4);
                                }
                            }
                            break;
                        case 0x03:
                            trigger_game_text(dukeText);
                            break;
                        case 0x04:
                            trigger_game_text(boyText);
                            break;
                        case 0x05:
                            trigger_game_text(girlText);
                            break;
                        case 0x06:
                            trigger_game_text(ladyErinText);
                            trigger_after_game_text(PRG_BANK_PLAYER_SPRITE, handle_end_game);
                            break;
                    }
                }
                break;
        }

    }
}

void handle_end_game(void) {
    gameState = GAME_STATE_CREDITS;
}
