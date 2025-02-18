#include "stats.hpp"
#include "defines.hpp"
#include "font.hpp"
#include "gfx.hpp"
#include "kbd.hpp"
#include "tonccpy.h"

#include "bgBottom.h"
#include "numbers_large_nftr.h"
#include "numbers_small_nftr.h"
#include "statsBottom.h"

#include <array>
#include <algorithm>
#include <nds.h>
#include <qrencode.h>

std::vector<TilePalette> check(const std::u16string &guess, Kbd *kbd);
std::string shareMessage(const Config &config);


void showQr(const Config &config) {
	// Ensure the game is done
	std::vector<TilePalette> allCorrect;
	for(int i = 0; i < WORD_LEN; i++)
		allCorrect.push_back(TilePalette::green);
	if(config.boardState().size() < MAX_GUESSES && check(Font::utf8to16(config.boardState().back()), nullptr) != allCorrect)
		return;

	std::string str = shareMessage(config);

	QRcode *qr = QRcode_encodeString(str.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, true);

	// Draw QR
	int scale = SCREEN_HEIGHT / qr->width;
	u8 *dst = (u8 *)bgGetGfxPtr(BG_SUB(2)) + (SCREEN_HEIGHT - qr->width * scale) / 2 * SCREEN_WIDTH + (SCREEN_WIDTH - qr->width * scale) / 2;
	for(int y = 0; y < qr->width; y++) {
		for(int i = 0; i < scale; i++) // Fill line with white
			toncset(dst + (y * scale + i) * SCREEN_WIDTH - 4, 0xF0, qr->width * scale + 8);

		for(int x = 0; x < qr->width; x++) {
			if(qr->data[y * qr->width + x] & 1) { // If black, draw pixel
				for(int i = 0; i < scale; i++)
					toncset(dst + (y * scale + i) * SCREEN_WIDTH + (x * scale), 0xF3, scale);
			}
		}
	}

	// Pad above and below with white
	for(int i = 0; i < 4; i++) {
		toncset(dst - (i + 1) * SCREEN_WIDTH - 4, 0xF0, qr->width * scale + 8);
		toncset(dst + ((qr->width * scale + i) * SCREEN_WIDTH) - 4, 0xF0, qr->width * scale + 8);
	}

	QRcode_free(qr);

	// Wait for input
	do {
		swiWaitForVBlank();
		scanKeys();
	} while(!(keysDown() & (KEY_A | KEY_B | KEY_TOUCH)));
}

void statsMenu(const Config &config, bool won) {
	// Change to stats menu background
	swiWaitForVBlank();
	tonccpy(bgGetGfxPtr(BG_SUB(0)), statsBottomTiles, statsBottomTilesLen);
	tonccpy(BG_PALETTE_SUB, statsBottomPal, statsBottomPalLen);
	tonccpy(bgGetMapPtr(BG_SUB(0)), statsBottomMap, statsBottomMapLen);

	// Load fonts
	Font largeFont(numbers_large_nftr, numbers_large_nftr_size), smallFont(numbers_small_nftr, numbers_small_nftr_size);
	largeFont.palette(TEXT_BLACK);
	smallFont.palette(TEXT_BLACK);

	while(1) {
		// Print scores
		largeFont.print(-96, 32, false, config.gamesPlayed(), Alignment::center);
		largeFont.print(-32, 32, false, std::count_if(config.guessCounts().begin(), config.guessCounts().end(), [](int a) { return a <= MAX_GUESSES; }) * 100 / config.gamesPlayed(), Alignment::center);
		largeFont.print(32, 32, false, config.streak(), Alignment::center);
		largeFont.print(96, 32, false, config.maxStreak(), Alignment::center).update(false);

		// Draw guess percentage bars
		int highestCount = 0;
		for(int i = 1; i <= MAX_GUESSES; i++)
			highestCount = std::max(highestCount, std::count(config.guessCounts().begin(), config.guessCounts().end(), i));

		for(int i = 1; i <= MAX_GUESSES; i++) {
			int count = std::count(config.guessCounts().begin(), config.guessCounts().end(), i);
			int width = (10 + (216 * count / highestCount));
			u8 palette = (i == config.guessCounts().back() && won) ? TEXT_GREEN : TEXT_WHITE;
			u8 *dst = (u8 *)bgGetGfxPtr(BG_SUB(2)) + ((256 * (90 + (14 * i))) + 20);

			smallFont.print(8, 90 - 1 + i * 14, false, i);
			for(int j = 0; j < 12; j++) {
				int adjust = (j == 0 || j == 11) ? 1 : 0;
				toncset(dst + 256 * j + adjust, palette, width - adjust * 2);
			}

			smallFont.palette(palette).print(20 + width - 1, 90 - 1 + i * 14, false, count, Alignment::right).palette(TEXT_BLACK);
		}
		smallFont.update(false, true);

		u16 pressed;
		touchPosition touch;
		do {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
			touchRead(&touch);
		} while(!(pressed & (KEY_A | KEY_B | KEY_TOUCH)));

		if(pressed & (KEY_A | KEY_B)) {
			break;
		} else if((pressed & KEY_TOUCH) && touch.py < 24) {
			if(touch.px < 24) {
				showQr(config);
				largeFont.clear(false).update(false);
			} else if(touch.px > 232) {
				break;
			}
		}
	}

	largeFont.clear(false).update(false);

	// Restore normal BG and letterSprites
	swiWaitForVBlank();
	tonccpy(bgGetGfxPtr(BG_SUB(0)), bgBottomTiles, bgBottomTilesLen);
	tonccpy(BG_PALETTE_SUB, bgBottomPal, bgBottomPalLen);
	tonccpy(bgGetMapPtr(BG_SUB(0)), bgBottomMap, bgBottomMapLen);
}
