; CHR RAM data
;
; The files below will be loaded on the fly into chr ram as required.
;

; Loads our main ascii and tiles chr data into code bank 6 (currently unused)
; must match the same values in main.c, example: bank_push(6)
.export _main_tiles, _main_sprites, _ascii_tiles
.segment "ROM_06"
        _ascii_tiles:
                .incbin "graphics/ascii.chr"
        _main_tiles: 
                .incbin "graphics/tiles.chr"
	_main_sprites: 
                .incbin "graphics/sprites.chr"

.export _boss_01
.segment "ROM_07"
        _boss_01:
                .incbin "graphics/boss_01.chr"

.export _ocean_tiles
.segment "CODE"
	_ocean_tiles: 
		.incbin "graphics/ocean_tiles.chr"
