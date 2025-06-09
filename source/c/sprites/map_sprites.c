#include "source/c/neslib.h"
#include "source/c/globals.h"
#include "source/c/configuration/system_constants.h"
#include "source/c/sprites/map_sprites.h"
#include "source/c/sprites/player.h"
#include "source/c/sprites/sprite_definitions.h"
#include "source/c/map/map.h"
#include "source/c/library/bank_helpers.h"
#include "source/c/menus/error.h"
#include "source/c/sprites/collision.h"

CODE_BANK(PRG_BANK_MAP_SPRITES);

#define currentSpriteSize tempChar2
#define currentSpriteTileId tempChar3
#define oamMapSpriteIndex tempChar4
#define currentSpriteType tempChar5
#define currentSpriteData tempChar6
#define sprX8 tempChar7
#define sprY8 tempChar8
#define sprX tempInt1
#define sprY tempInt2
#define tempMapSpriteIndex tempChar9
// NOTE: width = height for our examples, so both are set to the same value.
// If you change this, be sure to assign it in the for loop below as well.
#define currentSpriteFullWidth tempInt3
#define currentSpriteFullHeight tempInt4
// Same story, but corrected for tile collisions
#define currentSpriteFullTileCollisionWidth tempInt5
#define currentSpriteFullTileCollisionHeight tempInt6

// sword temporary
#define playerSwordX tempInt7
#define playerSwordY tempInt8
#define playerSwordWidth tempChar9
#define playerSwordHeight tempCharA
#define enemyFireIndex tempInt9
#define enemyFireId tempIntA

ZEROPAGE_DEF(unsigned char, lastPlayerSpriteCollisionId);
ZEROPAGE_DEF(unsigned char, lastPlayerWeaponCollisionId);
ZEROPAGE_DEF(unsigned char, currentMapSpriteIndex);
ZEROPAGE_DEF(unsigned char, currentSpriteSizeName);
ZEROPAGE_DEF(unsigned char, currentSpriteDirection);

// Forward definition of this method; code is at the bottom of this file. Ignore this for now!
void do_sprite_movement_with_collision(void);

// Updates all available map sprites (with movement every other frame)
void update_map_sprites(void) {
    lastPlayerSpriteCollisionId = NO_SPRITE_HIT;
    lastPlayerWeaponCollisionId = NO_SPRITE_HIT;

    if (swordPosition > 4) {
        playerSwordX = playerXPosition;
        playerSwordY = playerYPosition;

        switch (playerDirection) {
            case SPRITE_DIRECTION_RIGHT:
                playerSwordX += (PLAYER_WIDTH_EXTENDED + PLAYER_X_OFFSET_EXTENDED) + PLAYER_SWORD_SIZE_LONG;
                playerSwordY += ((PLAYER_HEIGHT_EXTENDED + PLAYER_Y_OFFSET_EXTENDED) / 2);
                playerSwordWidth = PLAYER_SWORD_SIZE_LONG;
                playerSwordHeight = PLAYER_SWORD_SIZE_SHORT;
                break;
            case SPRITE_DIRECTION_LEFT:
                playerSwordX -= PLAYER_WIDTH_EXTENDED + PLAYER_X_OFFSET_EXTENDED;
                playerSwordY += ((PLAYER_HEIGHT_EXTENDED + PLAYER_Y_OFFSET_EXTENDED) / 2);
                playerSwordWidth = PLAYER_SWORD_SIZE_LONG;
                playerSwordHeight = PLAYER_SWORD_SIZE_SHORT;
                break;
            case SPRITE_DIRECTION_DOWN:
                playerSwordX += ((PLAYER_WIDTH_EXTENDED + PLAYER_X_OFFSET_EXTENDED) / 2);
                playerSwordY += (PLAYER_HEIGHT_EXTENDED + PLAYER_Y_OFFSET_EXTENDED);
                playerSwordWidth = PLAYER_SWORD_SIZE_SHORT;
                playerSwordHeight = PLAYER_SWORD_SIZE_LONG;
                break;
            case SPRITE_DIRECTION_UP:
                playerSwordX += ((PLAYER_WIDTH_EXTENDED + PLAYER_X_OFFSET_EXTENDED) / 2);
                playerSwordY -= (PLAYER_HEIGHT_EXTENDED + PLAYER_Y_OFFSET_EXTENDED);
                playerSwordWidth = PLAYER_SWORD_SIZE_SHORT;
                playerSwordHeight = PLAYER_SWORD_SIZE_LONG;
                break;
        }
    }

    // To save some cpu time, we only update sprites every other frame - even sprites on even frames, odd sprites on odd frames.
    for (i = 0; i < MAP_MAX_SPRITES; ++i) {
        currentMapSpriteIndex = i << MAP_SPRITE_DATA_SHIFT;

        // This switches what position we write the sprite to regularly, so we can maintain a flicker effect instead
        // of having the sprite just randomly disappear. We use 0x02 so it flips every other frame, so flickering is less
        // likely to fail if we lose a frame. (If you have enough sprites to flicker, you may also see slowdown.)
        if (frameCount & 0x02) {
            oamMapSpriteIndex = (MAP_MAX_SPRITES-i-1) << MAP_SPRITE_OAM_SHIFT;
        } else {
            oamMapSpriteIndex = i << MAP_SPRITE_OAM_SHIFT;
        }
        oamMapSpriteIndex += FIRST_ENEMY_SPRITE_OAM_INDEX;
        sprX = ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X]) + ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X + 1]) << 8));
        sprY = ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y]) + ((currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y + 1]) << 8));
        currentSpriteSizeName = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_SIZE_PALETTE] & SPRITE_SIZE_MASK;
        currentSpriteTileId = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_TILE_ID];
        currentSpriteDirection = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION];

        if (currentSpriteSizeName == SPRITE_SIZE_8PX_8PX) {
            currentSpriteFullWidth = NES_SPRITE_WIDTH << PLAYER_POSITION_SHIFT;
            currentSpriteFullHeight = currentSpriteFullWidth;
            
            currentSpriteFullTileCollisionWidth = currentSpriteFullWidth;
            currentSpriteFullHeight = currentSpriteFullWidth;
        } else if (currentSpriteSizeName == SPRITE_SIZE_8PX_16PX) {
            currentSpriteFullWidth = NES_SPRITE_WIDTH << PLAYER_POSITION_SHIFT;
            currentSpriteFullHeight = (NES_SPRITE_HEIGHT*2) << (PLAYER_POSITION_SHIFT);

            currentSpriteFullTileCollisionWidth = currentSpriteFullWidth;
            // For 8x16 sprites, we reduce the sprite hitbox height a bit. We also subtract this offset from X/y in-place
            currentSpriteFullTileCollisionHeight = currentSpriteFullHeight - (2*SPRITE_TILE_HITBOX_OFFSET);

        } else if (currentSpriteSizeName == SPRITE_SIZE_16PX_8PX) {
            currentSpriteFullWidth = (NES_SPRITE_WIDTH*2) << (PLAYER_POSITION_SHIFT);
            currentSpriteFullHeight = NES_SPRITE_HEIGHT << PLAYER_POSITION_SHIFT;

            // For 16x16 sprites, we reduce the sprite hitbox width a bit. We also subtract this offset from X/y in-place
            currentSpriteFullTileCollisionWidth = currentSpriteFullWidth - (2*SPRITE_TILE_HITBOX_OFFSET);
            currentSpriteFullTileCollisionHeight = currentSpriteFullHeight;
        } else if (currentSpriteSizeName == SPRITE_SIZE_16PX_16PX) {
            currentSpriteFullWidth = (NES_SPRITE_WIDTH*2) << (PLAYER_POSITION_SHIFT);
            currentSpriteFullHeight = currentSpriteFullWidth;

            // For 16x16 sprites, we reduce the sprite hitbox a bit. We also subtract this offset from X/y in-place
            currentSpriteFullTileCollisionWidth = currentSpriteFullWidth - (2*SPRITE_TILE_HITBOX_OFFSET);
            currentSpriteFullTileCollisionHeight = currentSpriteFullTileCollisionWidth;

        } else if (currentSpriteSizeName == SPRITE_SIZE_32PX_32PX) {
            currentSpriteFullWidth = (NES_SPRITE_WIDTH*4) << PLAYER_POSITION_SHIFT;
            currentSpriteFullHeight = currentSpriteFullWidth;

            currentSpriteFullTileCollisionWidth = currentSpriteFullWidth - (10*SPRITE_TILE_HITBOX_OFFSET);
            currentSpriteFullTileCollisionHeight = currentSpriteFullTileCollisionWidth;
        }

        if (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_TYPE] == SPRITE_TYPE_OFFSCREEN) {
            if(currentSpriteSizeName == SPRITE_SIZE_32PX_32PX) {
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 32);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 36);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 40);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 44);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 48);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 52);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 56);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 60);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 64);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 68);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 72);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 76);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 80);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 84);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 88);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 92);
            } else {
                // Hide it and move on.
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 4);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 8);
                oam_spr(SPRITE_OFFSCREEN, SPRITE_OFFSCREEN, 0, 0, oamMapSpriteIndex + 12);
            }
            continue;
        }

        switch (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_ANIMATION_TYPE]) {
            case SPRITE_ANIMATION_SWAP:

                if (currentSpriteSizeName == SPRITE_SIZE_8PX_8PX) {
                    currentSpriteTileId += ((frameCount & 0x10) >> 4);
                } else if (currentSpriteSizeName == SPRITE_SIZE_16PX_16PX) {
                    currentSpriteTileId += ((frameCount & 0x10) >> 3);
                } else if (currentSpriteSizeName == SPRITE_SIZE_32PX_32PX) {
                    currentSpriteTileId += ((frameCount & 0x40) ? 4 : 0);
                }

                break;
            case SPRITE_ANIMATION_SWAP_FAST:
                if (currentSpriteSizeName == SPRITE_SIZE_8PX_8PX) {
                    currentSpriteTileId += ((frameCount & 0x08) >> 3);
                } else if (currentSpriteSizeName == SPRITE_SIZE_16PX_16PX) {
                    currentSpriteTileId += ((frameCount & 0x08) >> 2) * 0x10;
                }
                break;
            case SPRITE_ANIMATION_FULL:
                // This is for sprites that can face up/down/left/right, and are animated while they do so.
                if (currentSpriteDirection == SPRITE_DIRECTION_LEFT) {
                    if (currentSpriteSizeName == SPRITE_SIZE_32PX_32PX) {
                        currentSpriteTileId += 0x48;
                    } else if (currentSpriteSizeName == SPRITE_SIZE_8PX_8PX) {
                        currentSpriteTileId += 0x12;
                    } else {
                        currentSpriteTileId += 0x24;
                    }
                } else if (currentSpriteDirection == SPRITE_DIRECTION_RIGHT) {
                    if (currentSpriteSizeName == SPRITE_SIZE_32PX_32PX) {
                        currentSpriteTileId += 0x40;
                    } else if (currentSpriteSizeName == SPRITE_SIZE_8PX_8PX) {
                        currentSpriteTileId += 0x10;
                    } else {
                        currentSpriteTileId += 0x20;
                    }
                } else if (currentSpriteDirection == SPRITE_DIRECTION_UP) {
                    if (currentSpriteSizeName == SPRITE_SIZE_32PX_32PX) {
                        currentSpriteTileId += 0x08;
                    } else if (currentSpriteSizeName == SPRITE_SIZE_8PX_8PX) {
                        currentSpriteTileId += 0x02;
                    } else {
                        currentSpriteTileId += 0x04;
                    }

                } else {
                    // Else, you're facing down, which conveniently is in position zero for all sprites
                }

                // Next, let's animate based on the current frame.
                if (currentSpriteSizeName == SPRITE_SIZE_16PX_16PX) {
                    currentSpriteTileId += (frameCount & 0x10) >> 3;
                } else if (currentSpriteSizeName == SPRITE_SIZE_32PX_32PX) {
                    currentSpriteTileId += (((frameCount / 4) & 0x08) >> 3) * 4;
                } else {
                    currentSpriteTileId += (frameCount & 0x08) >> 3;
                }
                break;
            case SPRITE_ANIMATION_NONE:
            default:
                break;

        }
        // We only want to do movement once every other frame, to save some cpu time.
        // So, split this to update even sprites on even frames, odd sprites on odd frames
        if ((i & 0x01) == everyOtherCycle) {

            switch (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_MOVEMENT_TYPE]) {
                case SPRITE_MOVEMENT_LEFT_RIGHT:
                    // Get the speed to travel at
                    currentSpriteData = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_SLIDE_SPEED];

                    // If it's positive, add to X to get the right of the sprite
                    if ((signed char) currentSpriteData > 0) {
                        sprX += currentSpriteFullTileCollisionWidth;
                    }
                    // Add speed in
                    sprX += (signed char)currentSpriteData;
                    if (test_collision(currentMap[SPRITE_MAP_POSITION(sprX, sprY + SPRITE_TILE_HITBOX_OFFSET)], 0) || test_collision(currentMap[SPRITE_MAP_POSITION(sprX, sprY + currentSpriteFullTileCollisionHeight)], 0)) {
                        // Never mind... leave X position alone for now
                        sprX -= (signed char)currentSpriteData;
                        // Roll back our change to pick right of the sprite
                        if ((signed char) currentSpriteData > 0) {
                            sprX -= currentSpriteFullTileCollisionWidth;
                        }

                        // And... flip the direction!
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_SLIDE_SPEED] = 0 - (signed char)currentSpriteData;
                    } else {
                        // No collision! Roll back our change to pick right of the sprite
                        if ((signed char) currentSpriteData > 0) {
                            sprX -= currentSpriteFullTileCollisionWidth;
                        }


                        // And move the sprite over!
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X] = (sprX & 0xff);
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X+1] = (sprX >> 8);
                    }

                    break;
                case SPRITE_MOVEMENT_UP_DOWN:
                    // Get the speed to travel at
                    currentSpriteData = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_SLIDE_SPEED];

                    // If it's positive, add to X to get the right of the sprite
                    if ((signed char) currentSpriteData > 0) {
                        sprY += currentSpriteFullTileCollisionWidth;
                    }
                    // Add speed in
                    sprY += (signed char)currentSpriteData;
                    if (test_collision(currentMap[SPRITE_MAP_POSITION(sprX + SPRITE_TILE_HITBOX_OFFSET, sprY)], 0) || test_collision(currentMap[SPRITE_MAP_POSITION(sprX + currentSpriteFullTileCollisionWidth, sprY)], 0)) {
                        // Never mind... leave X position alone for now
                        sprY -= (signed char)currentSpriteData;
                        // Roll back our change to pick right of the sprite
                        if ((signed char) currentSpriteData > 0) {
                            sprY -= currentSpriteFullTileCollisionWidth;
                        }

                        // And... flip the direction!
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_SLIDE_SPEED] = 0 - (signed char)currentSpriteData;
                    } else {
                        // No collision! Roll back our change to pick right of the sprite
                        if ((signed char) currentSpriteData > 0) {
                            sprY -= currentSpriteFullTileCollisionWidth;
                        }


                        // And move the sprite over!
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y] = (sprY & 0xff);
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y+1] = (sprY >> 8);
                    }

                    break;
                case SPRITE_MOVEMENT_RANDOM_WANDER:
                case SPRITE_MOVEMENT_RANDOM_CHASE:
                    // Okay, we're going to simulate an intelligent enemy.

                    // First, how long have we been travelling in the same direction? Is it time for a swap?
                    if (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DIRECTION_TIME] == 0) {
                        // Yep. Figure out if direction is: none, left, right, up, or down we do this by getting a random number
                        // between 0 and 8 with bit masking. If it's 0, stop for a bit... if it's 1, left... 4 down, or 5-7, maintain.
                        switch (rand8() & 0x07) {
                            case 0:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_STATIONARY;
                                break;
                            case 1:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_LEFT;
                                break;
                            case 2:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_RIGHT;
                                break;
                            case 3:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_UP;
                                break;
                            case 4:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_DOWN;
                                break;
                            default:
                                // Do nothing - just carry on in the direction you're going for another cycle.
                                break;
                        }
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DIRECTION_TIME] = 20 + (rand8() & 31);

                        // TODO: Flames aren't working, would be cool if they did!
                        // if(currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_TYPE] == SPRITE_TYPE_BOSS) {
                        //     for (i = 0; i <= 2; i++)
                        //     {
                        //         enemyFireIndex = currentMapSpriteIndex + 32 + (i * 8);
                        //         // spawns a fireball
                        //         enemyFireId = 0x10 * SPRITE_DEFINITION_ITEM_SIZE;

                        //         // Copy the simple bytes from the sprite definition to replace the current enemy with one that has a different id.
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_TILE_ID] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_TILE_ID];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_TYPE] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_TYPE];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_SIZE_PALETTE] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_SIZE_PALETTE];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_HEALTH] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_HEALTH];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_ANIMATION_TYPE] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_ANIMATION_TYPE];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_MOVEMENT_TYPE] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_MOVEMENT_TYPE];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_MOVE_SPEED] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_MOVE_SPEED];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_DAMAGE] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_DAMAGE];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_INVULN_COUNTDOWN] = spriteDefinitions[enemyFireId + SPRITE_DEF_POSITION_MOVE_SPEED];

                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_X] = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X];
                        //         currentMapSpriteData[enemyFireIndex + MAP_SPRITE_DATA_POS_Y] = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y];
                        //     }
                        // }
                        
                    } else {
                        --currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DIRECTION_TIME];
                    }

                    do_sprite_movement_with_collision();
                    break;
                case SPRITE_MOVEMENT_PLAYER_MIMIC:
                    // This sprite is going to just move in the same direction as the player!

                    // First, is the player moving? If not, keep the player stationary.
                    if (playerXVelocity == 0 && playerYVelocity == 0) { 
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_STATIONARY; 
                    } else {
                        // Otherwise, we need to figure out the player's direction and mirror it.
                        switch (playerDirection) {
                            case SPRITE_DIRECTION_LEFT: 
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_RIGHT;
                                break;
                            case SPRITE_DIRECTION_RIGHT: 
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_LEFT;
                                break;
                            case SPRITE_DIRECTION_UP:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_DOWN;
                                break;
                            case SPRITE_DIRECTION_DOWN:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_UP;
                                break;
                        }
                    }

                    do_sprite_movement_with_collision();
                    break;
                case SPRITE_MOVEMENT_RANDOM_CHARGE:
                    // This sprite is going to just charge in a random direction then wait and do it again!
                     if (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DIRECTION_TIME] == 0) {
                        // Yep. Figure out if direction is: none, left, right, up, or down we do this by getting a random number
                        // between 0 and 8 with bit masking. If it's 0, stop for a bit... if it's 1, left... 4 down, or 5-7, maintain.
                        switch (rand8() & 0x03) {
                            case 0:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_LEFT;
                                break;
                            case 1:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_RIGHT;
                                break;
                            case 2:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_UP;
                                break;
                            default:
                            case 3:
                                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION] = SPRITE_DIRECTION_DOWN;
                                break;
                        }
                        currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DIRECTION_TIME] = 80 + (rand8() & 40);
                    } else {
                        --currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_DIRECTION_TIME];
                    }

                    do_sprite_movement_with_collision();
                    break;
                case SPRITE_MOVEMENT_NONE:
                default:
                    break;
            }
        }

        sprX8 = sprX >> SPRITE_POSITION_SHIFT;
        sprY8 = sprY >> SPRITE_POSITION_SHIFT;
        // If the sprite is not in the invulnerability countdown, or half the time when the sprite is in the countdown, draw the sprite.Add commentMore actions
        if (!currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_INVULN_COUNTDOWN] || (frameCount & 0x04)) {
            tempMapSpriteIndex = (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_SIZE_PALETTE] & SPRITE_PALETTE_MASK) >> 6;
            if (currentSpriteSizeName == SPRITE_SIZE_8PX_8PX) {
                oam_spr(
                    sprX8 + (NES_SPRITE_WIDTH/2),
                    sprY8 + (NES_SPRITE_HEIGHT/2),
                    currentSpriteTileId,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex
                );
            } else if (currentSpriteSizeName == SPRITE_SIZE_8PX_16PX) {
                oam_spr(
                    sprX8,
                    sprY8,
                    currentSpriteTileId,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex
                );

                oam_spr(
                    sprX8,
                    sprY8 + NES_SPRITE_HEIGHT,
                    currentSpriteTileId + 16,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 8
                );
            } else if (currentSpriteSizeName == SPRITE_SIZE_16PX_8PX) {
                oam_spr(
                    sprX8,
                    sprY8,
                    currentSpriteTileId,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex
                );
                
                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH,
                    sprY8,
                    currentSpriteTileId + 1,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 4
                );
            } else if (currentSpriteSizeName == SPRITE_SIZE_16PX_16PX) {
                oam_spr(
                    sprX8,
                    sprY8,
                    currentSpriteTileId,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex
                );
                
                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH,
                    sprY8,
                    currentSpriteTileId + 1,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 4
                );

                oam_spr(
                    sprX8,
                    sprY8 + NES_SPRITE_HEIGHT,
                    currentSpriteTileId + 16,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 8
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH,
                    sprY8 + NES_SPRITE_HEIGHT,
                    currentSpriteTileId + 17,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 12
                );
            } else if (currentSpriteSizeName == SPRITE_SIZE_32PX_32PX) {
                oam_spr(
                    sprX8,
                    sprY8,
                    currentSpriteTileId,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32
                );
                
                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH,
                    sprY8,
                    currentSpriteTileId + 1,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 4
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH * 2,
                    sprY8,
                    currentSpriteTileId + 2,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 8
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH * 3,
                    sprY8,
                    currentSpriteTileId + 3,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 12
                );

                oam_spr(
                    sprX8,
                    sprY8 + NES_SPRITE_HEIGHT,
                    currentSpriteTileId + 16,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 16
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH,
                    sprY8 + NES_SPRITE_HEIGHT,
                    currentSpriteTileId + 17,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 20
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH * 2,
                    sprY8 + NES_SPRITE_HEIGHT,
                    currentSpriteTileId + 18,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 24
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH * 3,
                    sprY8 + NES_SPRITE_HEIGHT,
                    currentSpriteTileId + 19,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 28
                );

                oam_spr(
                    sprX8,
                    sprY8 + NES_SPRITE_HEIGHT * 2,
                    currentSpriteTileId + 32,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 32
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH,
                    sprY8 + NES_SPRITE_HEIGHT * 2,
                    currentSpriteTileId + 33,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 36
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH * 2,
                    sprY8 + NES_SPRITE_HEIGHT * 2,
                    currentSpriteTileId + 34,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 40
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH * 3,
                    sprY8 + NES_SPRITE_HEIGHT * 2,
                    currentSpriteTileId + 35,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 44
                );

                oam_spr(
                    sprX8,
                    sprY8 + NES_SPRITE_HEIGHT * 3,
                    currentSpriteTileId + 48,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 48
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH,
                    sprY8 + NES_SPRITE_HEIGHT * 3,
                    currentSpriteTileId + 49,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 52
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH * 2,
                    sprY8 + NES_SPRITE_HEIGHT * 3,
                    currentSpriteTileId + 50,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 56
                );

                oam_spr(
                    sprX8 + NES_SPRITE_WIDTH * 3,
                    sprY8 + NES_SPRITE_HEIGHT * 3,
                    currentSpriteTileId + 51,
                    tempMapSpriteIndex,
                    oamMapSpriteIndex + 32 + 60
                );
            }
        }

        // Decrement the countdown for invulnerability if it is non-zero.
        if (currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_INVULN_COUNTDOWN]) {
            --currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_INVULN_COUNTDOWN];
        } else if(currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TILE_ID] == SPRITE_TYPE_EFFECT) {
            // Mark the sprite as collected, so we can't get it again.
            currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE] = SPRITE_TYPE_OFFSCREEN;
        }

        // While we have all the data above, let's see if the player hit us.

        // Only test collision for sprite types that collide.
        currentSpriteType = currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_TYPE];
        if (currentSpriteType != SPRITE_TYPE_NOTHING && currentSpriteType != SPRITE_TYPE_OFFSCREEN) {

            // If the player's sword is swinging, do some damage with it.
            if (swordPosition > 4) {
                if (
                    playerSwordX < sprX + currentSpriteFullTileCollisionWidth &&
                    playerSwordX + playerSwordWidth > sprX &&
                    playerSwordY < sprY + currentSpriteFullTileCollisionHeight &&
                    playerSwordY + playerSwordHeight > sprY
                ) {
                    lastPlayerWeaponCollisionId = i;
                }
            }

            // For 16x16 enemy sprites, make their hitbox a bit smaller
            if (currentSpriteType == SPRITE_TYPE_REGULAR_ENEMY || currentSpriteType == SPRITE_TYPE_INVULNERABLE_ENEMY) {
                sprX -= SPRITE_HITBOX_OFFSET;
                sprY -= SPRITE_HITBOX_OFFSET;
            }

            // Collision test... see here for a clear explanation: https://developer.mozilla.org/en-US/docs/Games/Techniques/2D_collision_detection
            // rect1=player position, rect2=sprite position
            if (
                (nextPlayerXPosition + PLAYER_X_OFFSET_EXTENDED) < sprX + currentSpriteFullWidth &&
                nextPlayerXPosition + (PLAYER_WIDTH_EXTENDED + PLAYER_X_OFFSET_EXTENDED) > sprX &&
                (nextPlayerYPosition + PLAYER_Y_OFFSET_EXTENDED) < sprY + currentSpriteFullHeight &&
                nextPlayerYPosition + (PLAYER_HEIGHT_EXTENDED + PLAYER_Y_OFFSET_EXTENDED) > sprY
            ) {
                lastPlayerSpriteCollisionId = i;
            }
        }


    }
}

// Does movement for a sprite given x/y position.
void do_sprite_movement_with_collision(void) {
    // Set currentSpriteData to the sprite speed for now (NOTE: we overwrite this after the switch statement)
    // We'll then add/subtract it from sprX and sprY
    currentSpriteData = currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_MOVE_SPEED];
    // We temporarily double the enemy's speed when they are in their invulnerability animation, so they shoot backwards.
    if (currentMapSpriteData[(currentMapSpriteIndex) + MAP_SPRITE_DATA_POS_INVULN_COUNTDOWN]) {
        currentSpriteData <<= 2;
    }
    switch (currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_CURRENT_DIRECTION]) {
        case SPRITE_DIRECTION_LEFT:

            sprX -= currentSpriteData;
            if (sprX < SCREEN_EDGE_LEFT << SPRITE_POSITION_SHIFT) {
                // Roll back the position since we use sprX to place the sprite
                sprX += currentSpriteData;
                break;
            }

            // If we have not collided, save the new position. Else, just exit.
            if (!test_collision(currentMap[SPRITE_MAP_POSITION(sprX, sprY + SPRITE_TILE_HITBOX_OFFSET)], 0) && !test_collision(currentMap[SPRITE_MAP_POSITION(sprX, sprY + currentSpriteFullTileCollisionHeight)], 0)) {
                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X] = (sprX & 0xff);
                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X+1] = (sprX >> 8);
            } else {
                // Roll back the position since we use sprX to place the sprite
                sprX -= currentSpriteData;
            }

            break;
        case SPRITE_DIRECTION_RIGHT:
            // Set the X position to our new position, plus the full width of the sprite for the collision test
            sprX += currentSpriteData + currentSpriteFullTileCollisionWidth;
            if (sprX > SCREEN_EDGE_RIGHT << SPRITE_POSITION_SHIFT) {
                // Roll back the position since we use sprX to place the sprite
                sprX -= currentSpriteData + currentSpriteFullTileCollisionWidth;
                break;
            }

            // If we have not collided, save the new position. Else, just exit.
            if (!test_collision(currentMap[SPRITE_MAP_POSITION(sprX, sprY + SPRITE_TILE_HITBOX_OFFSET)], 0) && !test_collision(currentMap[SPRITE_MAP_POSITION(sprX, sprY + currentSpriteFullTileCollisionHeight)], 0)) {
                // If we did collide, we added the full width of the sprite to sprX; take that back out.
                sprX -= currentSpriteFullTileCollisionWidth;

                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X] = (sprX & 0xff);
                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_X+1] = (sprX >> 8);
            } else {
                // Roll back the position since we use sprX to place the sprite
                sprX -= currentSpriteData + currentSpriteFullTileCollisionWidth;
            }
            break;
        case SPRITE_DIRECTION_UP:
            sprY -= currentSpriteData;
            if (sprY < SCREEN_EDGE_TOP << SPRITE_POSITION_SHIFT) {
                // Roll back the position since we use sprY to place the sprite
                sprY += currentSpriteData;
                break;
            }

            // If we have not collided, save the new position. Else, just exit.
            if (!test_collision(currentMap[SPRITE_MAP_POSITION(sprX + SPRITE_TILE_HITBOX_OFFSET, sprY)], 0) && !test_collision(currentMap[SPRITE_MAP_POSITION(sprX + currentSpriteFullTileCollisionWidth, sprY)], 0)) {
                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y] = (sprY & 0xff);
                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y+1] = (sprY >> 8);
            } else {
                // Roll back the position since we use sprY to place the sprite
                sprY += currentSpriteData;
            }

            break;
        case SPRITE_DIRECTION_DOWN:
            // Set our Y position to the new position, plus the full height of the sprite for collisions
            sprY += currentSpriteData + currentSpriteFullTileCollisionHeight;

            if (sprY > SCREEN_EDGE_BOTTOM << SPRITE_POSITION_SHIFT) {
                // Roll back the position since we use sprY to place the sprite
                sprY -= currentSpriteData + currentSpriteFullTileCollisionHeight;
                break;
            }

            // If we have not collided, save the new position. Else, just exit.
            if (!test_collision(currentMap[SPRITE_MAP_POSITION(sprX + SPRITE_TILE_HITBOX_OFFSET, sprY)], 0) && !test_collision(currentMap[SPRITE_MAP_POSITION(sprX + currentSpriteFullTileCollisionWidth, sprY)], 0)) {
                // Reset sprY to the top of the sprite before we update.
                sprY -= currentSpriteFullTileCollisionHeight;

                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y] = (sprY & 0xff);
                currentMapSpriteData[currentMapSpriteIndex + MAP_SPRITE_DATA_POS_Y+1] = (sprY >> 8);
            } else {
                // Roll back the position since we use sprY to place the sprite
                sprY -= currentSpriteData + currentSpriteFullTileCollisionHeight;
            }

            break;
    }

}