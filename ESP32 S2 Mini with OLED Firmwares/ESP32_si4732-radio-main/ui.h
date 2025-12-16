#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

#include "global.h"
#include "icons.h"
#include "images/welcome.h"

#include "fonts/font_4x6mr.h"
#include "fonts/font_5x8.h"
#include "fonts/font_6x10.h"
#include "fonts/Segment7_12.h"
#include "fonts/Segment7_14.h"


enum class TextAlign {
    LEFT,
    CENTER,
    RIGHT
};

enum class Font {
    FONT_4X6MR,
    FONT_5X8,
    FONT_6X10,
    SEGMENT7_12,
    SEGMENT7_14
};

#define BLACK 0
#define WHITE 1

#ifndef UI_H
#define UI_H

#define W 128
#define H 64

class UI {
public:
    UI() :
        u8g2(U8G2_R0, /* I2C Communication */ U8X8_PIN_NONE, U8X8_PIN_NONE)  // I2C constructor        
    {
        //lcd()->begin();
        //lcd()->setBusClock(2800000);

        //hal_extcom_start();

        lcd()->setColorIndex(WHITE);
        lcd()->clearBuffer();
    };

    U8G2_SSD1306_128X64_NONAME_F_HW_I2C* lcd() { return &u8g2; };

    uint8_t message_result = 0;

    uint8_t menu_pos = 1;

    void clearDisplay() {
        lcd()->setColorIndex(BLACK);
        lcd()->drawBox(0, 0, W, H);
        lcd()->sendBuffer();
    };

    void updateDisplay() {
        lcd()->sendBuffer();
    };

    void setFont(Font font) {
        switch (font) {
        case Font::FONT_4X6MR:
            lcd()->setFont(font_4x6mr);
            break;
        case Font::FONT_5X8:
            lcd()->setFont(font_5x8);
            break;
        case Font::FONT_6X10:
            lcd()->setFont(font_6x10);
            break;   
        case Font::SEGMENT7_12:
            lcd()->setFont(Segment7_12);
            break;
        case Font::SEGMENT7_14:
            lcd()->setFont(Segment7_14);
            break;
        }
    };

    void setBlackColor() {
        lcd()->setColorIndex(BLACK);
    };

    void setWhiteColor() {
        lcd()->setColorIndex(WHITE);
    };

    void drawStrf(u8g2_uint_t x, u8g2_uint_t y, const char* str, ...) {
        char text[52] = { 0 };

        va_list va;
        va_start(va, str);
        vsnprintf(text, sizeof(text), str, va);
        va_end(va);

        lcd()->drawStr(x, y, text);
    };

    void drawString(TextAlign tAlign, u8g2_uint_t xstart, u8g2_uint_t xend, u8g2_uint_t y, bool isBlack, bool isFill, bool isBox, const char* str) {

        u8g2_uint_t startX = xstart;
        u8g2_uint_t endX = xend;
        u8g2_uint_t stringWidth = lcd()->getStrWidth(str);

        u8g2_uint_t xx, yy, ww, hh;

        u8g2_uint_t border_width = 1;

        u8g2_uint_t padding_h = 2;
        u8g2_uint_t padding_v = 2;

        u8g2_uint_t h = lcd()->getAscent();

        if (endX > startX) {
            if (tAlign == TextAlign::CENTER) {
                if (stringWidth < (endX - startX)) {
                    startX = ((startX + endX) / 2) - (stringWidth / 2);
                    endX = stringWidth;
                }
            }
            else if (tAlign == TextAlign::RIGHT) {
                startX = endX - stringWidth;
            }
        }

        xx = startX;
        xx -= padding_h;
        xx -= border_width;
        ww = (endX > startX ? (endX - startX) : stringWidth) + (2 * padding_h) + (2 * border_width);

        yy = y;
        yy -= h;
        yy -= padding_v;
        yy -= border_width;
        hh = h + (2 * padding_v) + (2 * border_width);

        /*lcd()->setColorIndex(isBlack ? WHITE : BLACK);
        lcd()->drawBox(xx, yy, ww, hh);*/

        lcd()->setColorIndex(isBlack ? BLACK : WHITE);
        if (isFill) {
            lcd()->drawRBox(xx, yy, ww, hh, 5);
            lcd()->setColorIndex(isBlack ? WHITE : BLACK);
        }
        else if (isBox) {
            lcd()->drawRFrame(xx, yy, ww, hh, 5);
        }

        lcd()->drawStr(startX, y, str);

    };

    void drawStringf(TextAlign tAlign, u8g2_uint_t xstart, u8g2_uint_t xend, u8g2_uint_t y, bool isBlack, bool isFill, bool isBox, const char* str, ...) {
        char text[52] = { 0 };

        va_list va;
        va_start(va, str);
        vsnprintf(text, sizeof(text), str, va);
        va_end(va);

        drawString(tAlign, xstart, xend, y, isBlack, isFill, isBox, text);
    };

    void clearScreenAnimationCircleFromRight() {
        int curr = 0;
        float targ = 80;
        while (targ < W) {
            targ *= 1.6;
            lcd()->setColorIndex(WHITE);
            for (; curr < targ; curr += 1)
            {
                lcd()->drawCircle(/*x0*/ 400, /*y0*/ H / 2, /*rad*/ curr, /*opt*/ U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_LOWER_LEFT); // drawDisc     U8G2_DRAW_ALL
            }
            lcd()->setColorIndex(BLACK);
            lcd()->drawCircle(/*x0*/ 400, /*y0*/ H / 2, /*rad*/ curr + 2, /*opt*/ U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_LOWER_LEFT); // drawDisc     U8G2_DRAW_ALL
            lcd()->sendBuffer();
        }
    };

#define LINE_SPACING_MULTILINE_TEXT_PX 1

    uint32_t draw_string_multi_line(const char* str, uint8_t max_chr_per_line, int32_t x, int32_t y) {
        uint32_t string_length_bytes = strlen(str);
        char partial_string[60 + 1] = { '\0' };

        uint8_t line_start = 0;
        uint8_t line_end = 0;
        uint8_t max_chr_h = lcd()->getMaxCharHeight();
        uint8_t string_length = lcd()->getStrWidth(str);

        while (line_start < string_length_bytes) {
            memset(partial_string, 0, max_chr_per_line + 1);
            line_end = line_start + max_chr_per_line - 1;

            uint8_t i;
            uint8_t last_space_idx = line_start; /* used to keep track of the last space */
            /* for strings with multi-byte utf8 characters, these two values will be different */
            uint8_t partial_str_len = 0;
            uint8_t partial_str_len_bytes = 0;

            /**
             * Parse the string until a new line is found or we process the max number of characters
             * that can fit in a single line. Save the index of the last space/new line in order not to
             * break the word in the middle in case it cannot fit in the line completely.
             */
            for (i = line_start; (partial_str_len <= max_chr_per_line) && (str[i] != '\0'); i++) {
                if (str[i] == '\n') {
                    last_space_idx = i;
                    break;
                }
                else if (str[i] == ' ') {
                    last_space_idx = i;
                    partial_str_len++;
                }
                else {
                    if ((str[i] & 0xc0) != 0x80) {
                        /**
                         * Count only the first byte of each glyph for the length in characters.
                         * UTF8 glyphs can be multi-byte and continuation bytes will have
                         * 0x10xxxxxx (0x80) as leftmost bits
                         */
                        partial_str_len++;
                    }
                    partial_str_len_bytes++;
                }
            }

            /**
             * If new line was found, or a word would be partially cut off, force the last known
             * space/new line as the end of the first line to be drawn;
             * otherwise, take the calculated length of the string that can fit the single line and
             * discard the beginning of the word that cannot fully fit.
             */
            if ((str[i] == '\n') || (partial_str_len > max_chr_per_line)) {
                line_end = last_space_idx;
            }
            else {
                line_end = partial_str_len + line_start;
                while (line_end < (string_length - 1) && !isspace((unsigned char)str[line_end]) &&
                    (line_end > line_start)) {
                    line_end--;
                }
            }

            /**
             * If a word is longer than the max num of characters that can be shown,
             * we can't rely on spaces and new lines so we split the word at the line limit
             */
            if (line_end == line_start) {
                line_end = line_start + max_chr_per_line;
            }
            else if (line_end == (string_length - 1) &&
                !isspace((unsigned char)str[line_end])) {
                line_end = line_start + max_chr_per_line - 1;
            }

            strncpy(partial_string, &str[line_start], line_end - line_start);
            lcd()->drawUTF8(x, y, partial_string);

            /* Prepare the coordinates and indices for the next line */
            y += max_chr_h + LINE_SPACING_MULTILINE_TEXT_PX;

            line_start = line_end;
            while (line_start < string_length_bytes && isspace((unsigned char)str[line_start])) {
                line_start++;
            }
        }

        /* Remove the spacing that was added to the last line */
        return y - max_chr_h - LINE_SPACING_MULTILINE_TEXT_PX;
    }

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - */

    void draw_ic24_battery75(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawBitmap(x, y, 3, 24, ic24_battery75); };

    void draw_start(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, start_icon_width, start_icon_height, start_icon_bits); };
    void draw_finish(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, finish_icon_width, finish_icon_height, finish_icon_bits); };
    void draw_stereo(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, stereo_icon_width, stereo_icon_height, stereo_icon_bits); };
    void draw_ic24_sound_on(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, sound_width, sound_height, sound_bits); };

    void draw_ic24_save(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, save_icon_width, save_icon_height, save_icon_bits); };

    void draw_ic24_step(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, step_icon_width, step_icon_height, step_icon_bits); };
    void draw_ic24_bandwidth(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, bandwidth_icon_width, bandwidth_icon_height, bandwidth_icon_bits); };
    void draw_ic24_agc(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, agc_icon_width, agc_icon_height, agc_icon_bits); };

    void draw_ic_mode(int x, int y, bool color) { lcd()->setColorIndex(color);  lcd()->drawXBM(x, y, mode_icon_width, mode_icon_height, mode_icon_bits); };
    

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - */

    void showStatusScreen(const char* title, const char* message ) {
        int popupHigh = 40;

        setFont(Font::FONT_6X10);

        u8g2_uint_t string_length = lcd()->getStrWidth(message);

        int popupWidth = 1 + string_length;

        setBlackColor();
        lcd()->drawRBox((W / 3) - (popupWidth / 3), (H / 2) - (popupHigh / 2) - 3, popupWidth, popupHigh, 8);

        setWhiteColor();
        lcd()->drawRFrame((W / 3) - (popupWidth / 3), (H / 2) - (popupHigh / 2) - 3, popupWidth +1, popupHigh + 1, 8);        
        //lcd()->drawRFrame((W / 2) - (popupWidth / 2) - 1, (H / 2) - (popupHigh / 2) - 1, popupWidth + 2, popupHigh + 2, 8);
        //lcd()->drawRFrame((W / 2) - (popupWidth / 2) + 2, (H / 2) - (popupHigh / 2) + 2, popupWidth - 4, popupHigh - 4, 8);

	setWhiteColor();
	setFont(Font::FONT_6X10);
	drawString(TextAlign::CENTER, 1, 20, 5, false, false, false, title);

        drawString(TextAlign::CENTER, (W / 2) - (popupWidth / 2), (W / 2) - (popupWidth / 2) + popupWidth, (H / 2) - (popupHigh / 2) + (popupHigh / 2) + 4, false, false, false, message);
        
    }

    void drawLoading() {
        showStatusScreen("", "Loading SSB ...");
    }

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - */

    void drawStatus(uint8_t volume, bool itIsTimeToSave) {
        setBlackColor();
        lcd()->drawBox(0, 0, W, 9);

        //draw_ic24_battery75(1, 0, WHITE);
        setWhiteColor();
        setFont(Font::FONT_4X6MR);
        //lcd()->drawStr(2, 7, "75%  8.7V");
        //lcd()->drawStr(35, 18, "joaquim.org");

        if(itIsTimeToSave) {
            draw_ic24_save(64, 1, WHITE);
        }        

        //draw_ic24_sound_on(120, 0, WHITE);
        drawStringf(TextAlign::RIGHT, 0, 122, 7, false, false, false, "%02u %%", map(volume, 0, 63, 0, 80));

        //lcd()->drawHLine(0, 14, W);
    };

    void clearMain() {
        setBlackColor();
        lcd()->drawBox(0, 20, W, H);
    };

    void drawFrequencyBig(uint32_t freq, int16_t bfo, uint8_t currentBandType, uint8_t currentMode, u8g2_uint_t xend, u8g2_uint_t y) {

        setFont(Font::SEGMENT7_14);

        if (currentBandType == FM_BAND_TYPE) {
            drawStringf(TextAlign::RIGHT, 0, xend, y, false, false, false, "%02u.%02u", (freq / 100), (freq % 100));
        }
        else {

            if (currentMode == LSB || currentMode == USB) {

                uint32_t cFrequency  = (uint32_t(freq) * 1000) + bfo;

                drawStringf(TextAlign::RIGHT, 0, xend-12, y+1, false, false, false, "%2u.%3.3u", (cFrequency / 1000000), (cFrequency % 1000000) / 1000);

                setWhiteColor();

                setFont(Font::SEGMENT7_12);
                drawStrf(xend-6, y+1, "%3.3d", (cFrequency % 1000));                

            }
            else {
                drawStringf(TextAlign::RIGHT, 0, xend, y+1, false, false, false, "%2u.%03u", (freq / 1000), (freq % 1000));
            }

        }
    };

    void drawFrequency(uint32_t freq, u8g2_uint_t x, u8g2_uint_t y) {

        drawStringf(TextAlign::LEFT, x, 0, y, false, false, false, "%2u.%03u MHz", (freq / 1000), (freq % 1000));

    };

    long map(long x, long in_min, long in_max, long out_min, long out_max) {
        const long run = in_max - in_min;
        if (run == 0) {
            return -1; // AVR returns -1, SAM returns 0
        }
        const long rise = out_max - out_min;
        const long delta = x - in_min;
        return (delta * rise) / run + out_min;
    }

    void drawRSSI(int rssi, int vu, int snr, u8g2_uint_t x, u8g2_uint_t y) {

        setWhiteColor();
        setFont(Font::FONT_6X10);
        lcd()->drawStr(x+6, y-12, "S");

        setFont(Font::FONT_4X6MR);
        lcd()->drawStr(x + 15, y-17, "1.3.5.7.9.20.40.60");

        uint16_t sMultiplier;
        uint8_t sWidth;
        for (int i = 0; i < 15; i++) {
            sMultiplier = (i * 4);
            sWidth = 3;
            if (i > 9) {
                sMultiplier += (i - 9) * 2;
                sWidth = 5;
            }
            lcd()->drawVLine(x + 16 + sMultiplier, y-16, 5);
            if (i + 1 < vu) {
                if (i > 9) {
                    sMultiplier -= 1;
                }
                lcd()->drawBox(x + 15 + sMultiplier, y-16, sWidth, 5);
            }
        }

        setFont(Font::FONT_5X8);
        drawStringf(TextAlign::LEFT, x + 14, 0, y-3, false, false, false, "%2udBuV", rssi);
        drawStringf(TextAlign::LEFT, x + 46, 0, y-3, false, false, false, "%2udB SNR", snr);

    }

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - */
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - */

    u8g2_uint_t drawSelectionListLine(u8sl_t* u8sl, u8g2_uint_t y, uint8_t idx, const char* s) {
        //u8g2_uint_t yy;
        uint8_t border_size = 1;
        uint8_t is_invert = 0;

        u8g2_uint_t line_height = lcd()->getAscent() - lcd()->getDescent();

        /* check whether this is the current cursor line */
        if (idx == u8sl->current_pos)
        {
            border_size = 0;
            is_invert = 1;
        }

        /* get the line from the array */
        s = u8x8_GetStringLineStart(idx, s);

        /* draw the line */
        if (s == NULL)
            s = "";
        //u8g2_DrawUTF8Line(u8g2, MY_BORDER_SIZE, y, u8g2_GetDisplayWidth(u8g2) - 2 * MY_BORDER_SIZE, s, border_size, is_invert);
        if (is_invert) {
            setFont(Font::FONT_6X10);
            drawStringf(TextAlign::LEFT, 54, 20, y-8, false, true, false, s);
        }
        else {
            setFont(Font::FONT_6X10);
            drawStringf(TextAlign::LEFT, 54, 20, y-8, false, false, false, s);
        }

        return line_height;
    }

    void drawList(u8sl_t* u8sl, u8g2_uint_t y, const char* s) {
        uint8_t i;
        for (i = 0; i < u8sl->visible; i++) {
            y += drawSelectionListLine(u8sl, y, i + u8sl->first_pos, s);
        }
    }

    u8sl_t u8sl;
    const char* slines;

    void listNext() {
        u8sl.current_pos++;
        if (u8sl.current_pos >= u8sl.total) {
            u8sl.current_pos = 0;
            u8sl.first_pos = 0;
        }
        else {
            uint8_t middle = u8sl.visible / 2;
            if (u8sl.current_pos >= middle && u8sl.current_pos < u8sl.total - middle) {
                u8sl.first_pos = u8sl.current_pos - middle;
            }
            else if (u8sl.current_pos >= u8sl.total - middle) {
                u8sl.first_pos = u8sl.total - u8sl.visible;
            }
        }
    }

    void listPrev() {
        if (u8sl.current_pos == 0) {
            u8sl.current_pos = u8sl.total - 1;
            u8sl.first_pos = (u8sl.total > u8sl.visible) ? (u8sl.total - u8sl.visible) : 0;
        }
        else {
            u8sl.current_pos--;
            uint8_t middle = u8sl.visible / 2;
            if (u8sl.current_pos >= middle && u8sl.current_pos < u8sl.total - middle) {
                u8sl.first_pos = u8sl.current_pos - middle;
            }
            else if (u8sl.current_pos < middle) {
                u8sl.first_pos = 0;
            }
        }
    }

    void drawSelectionList(uint8_t startPos, uint8_t displayLines, const char* sl) {

        u8sl.visible = displayLines;

        u8sl.total = u8x8_GetStringLineCnt(sl);
        if (u8sl.total <= u8sl.visible)
            u8sl.visible = u8sl.total;

        // Calculate the middle position
        uint8_t middlePos = u8sl.visible / 2;

        // Set the current position
        u8sl.current_pos = startPos;

        // Adjust first_pos to center the current_pos if possible
        if (u8sl.current_pos >= middlePos) {
            u8sl.first_pos = u8sl.current_pos - middlePos;
        }
        else {
            u8sl.first_pos = 0;
        }

        // Ensure first_pos does not exceed the total lines
        if (u8sl.first_pos + u8sl.visible > u8sl.total) {
            u8sl.first_pos = u8sl.total - u8sl.visible;
        }

        // Ensure current_pos is within the valid range
        if (u8sl.current_pos >= u8sl.total) {
            u8sl.current_pos = u8sl.total - 1;
        }

        slines = sl;
    }

    void setMenu(uint8_t startPos, const char* sl, uint8_t displayLines) {
        drawSelectionList(startPos, displayLines, sl);
    }

    void drawMenu() {
        lcd()->setFontPosBaseline();
        drawList(&u8sl, 18, slines);
    }

    uint8_t getListPos() {
        return u8sl.current_pos;
    }

    void setListPos(uint8_t pos) {
        u8sl.current_pos = pos;
        uint8_t middlePos = u8sl.visible / 2;

        // Adjust first_pos to center the current_pos if possible
        if (u8sl.current_pos >= middlePos) {
            u8sl.first_pos = u8sl.current_pos - middlePos;
        } else {
            u8sl.first_pos = 0;
        }

        // Ensure first_pos does not exceed the total lines
        if (u8sl.first_pos + u8sl.visible > u8sl.total) {
            u8sl.first_pos = u8sl.total - u8sl.visible;
        }

        // Ensure current_pos is within the valid range
        if (u8sl.current_pos >= u8sl.total) {
            u8sl.current_pos = u8sl.total - 1;
        }
    }

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - */
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - */

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;  // Use the I2C-specific constructor for a 128x64 display

};

#endif // UI_H
