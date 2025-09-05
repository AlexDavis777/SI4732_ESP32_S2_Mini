#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

const DCfont* LastFont = DEFAULT_FONT;

void oledSetFont(const DCfont* font)
{
    if (font && LastFont != font)
    {
        LastFont = font;
        oled.setFont(font);
    }
}

void oledPrint(const char* text, int offX = -1, int offY = -1, const DCfont* font = LastFont, bool invert = false)
{
    oledSetFont(font);
    if (invert)
        oled.invertOutput(invert);
    if (offX >= 0 && offY >= 0)
        oled.setCursor(offX, offY);
    oled.print(text);
    if (invert)
        oled.invertOutput(false);
}

void oledPrint(uint16_t u, int offX = -1, int offY = -1, const DCfont* font = LastFont, bool invert = false)
{
    oledSetFont(font);
    if (invert)
        oled.invertOutput(invert);
    if (offX >= 0 && offY >= 0)
        oled.setCursor(offX, offY);
    oled.print(u);
    if (invert)
        oled.invertOutput(false);
}

//Faster alternative for convertToChar
void utoa(char* out, uint16_t num)
{
    char* p = out;
    if (num == 0)
        *p++ = '0';
    else
    {
        for (uint16_t base = 10000; base > 0; base /= 10)
        {
            if (num >= base)
            {
                *p++ = '0' + num / base;
                num %= base;
            }
            else if (p != out)
                *p++ = '0';
        }
    }

    *p = '\0';
}

//Better than sprintf which has overwhelmingly large overhead, it helps to reduce binary size
void convertToChar(char* strValue, uint16_t value, uint8_t len, uint8_t dot = 0, uint8_t separator = 0, uint8_t space = ' ')
{
    char d;
    int8_t i;
    for (i = (len - 1); i >= 0; i--)
    {
        d = value % 10;
        value = value / 10;
        strValue[i] = d + 48;
    }
    strValue[len] = '\0';

    if (dot > 0)
    {
        for (int i = len; i >= dot; i--)
        {
            strValue[i + 1] = strValue[i];
        }
        strValue[dot] = separator;
        len = dot;
    }
    i = 0;
    len--;

    while ((i < len) && ('0' == strValue[i]))
    {
        strValue[i++] = space;
    }
}

//Measure integer digit length
int ilen(uint16_t n)
{
    if (n < 10)
        return 1;
    else if (n < 100)
        return 2;
    else if (n < 1000)
        return 3;
    else if (n < 10000)
        return 4;
    else
        return 5;
}

//Split KHz frequency + BFO to KHz and .00 tail
void splitFreq(uint16_t& khz, uint16_t& tail)
{
    int32_t freq = (uint32_t(g_currentFrequency) * 1000) + g_currentBFO;
    khz = freq / 1000;
    tail = abs(freq % 1000) / 10;
}

uint8_t strlen8(const char* str)
{
    uint8_t n = 0;
    while (str[n] != '\0')
        n++;
    return n;
}

void bleep(int freq, int time) //Generate a freq tone for time duration
{
  tone(speakerPin, freq);  // Generate a tone in Hz on pin 1
  delay(time);             // Wait for number milliseconds
  noTone(speakerPin);      // Stop the tone
}

// Function to scan around curr_freq and display signal strength bars
void quickScan(int curr_freq, int step_khz) {
  const int center_x = SCREEN_WIDTH / 2 - 1;  // 63
  int max_strength = 0;
  int strong_freq = curr_freq;
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  oled.clear();

  // Measure center frequency signal strength
  g_si4735.setFrequency(curr_freq);
  g_si4735.setSeekAmRssiThreshold(10); // default is 25
  g_si4735.setSeekAmSNRThreshold(3); // default is 5
  g_si4735.getCurrentReceivedSignalQuality();
  int center_strength = g_si4735.getCurrentRSSI();
  max_strength = center_strength;
  

  // Plot center frequency
  int h = signalStrengthToHeight(center_strength);
  display.setCursor(56, 8);
  display.print(curr_freq);
  display.drawLine(center_x, SCREEN_HEIGHT + 4, center_x, SCREEN_HEIGHT + 4 - h, WHITE);
  display.display();

  // LEFT side (lower freqs)
  int x = center_x - 1;
  for (int f = curr_freq - step_khz; f >= g_bandList[g_bandIndex].minimumFreq && x >= 0; f -= step_khz, x--) {
    g_si4735.setFrequency(f);
    g_si4735.setSeekAmRssiThreshold(10); // default is 25
    g_si4735.setSeekAmSNRThreshold(3); // default is 5
    g_si4735.getCurrentReceivedSignalQuality();
    int strength = g_si4735.getCurrentRSSI();
    h = signalStrengthToHeight(strength);
    display.drawLine(x, SCREEN_HEIGHT + 4, x, SCREEN_HEIGHT + 4 - h, WHITE);
    display.display();
    if (strength > max_strength) {
      max_strength = strength;
      strong_freq = f;
    }
  }
  // RIGHT side (higher freqs)
  x = center_x + 1;
  for (int f = curr_freq + step_khz; f <= g_bandList[g_bandIndex].maximumFreq && x < SCREEN_WIDTH; f += step_khz, x++) {
    g_si4735.setFrequency(f);
    g_si4735.setSeekAmRssiThreshold(10); // default is 25
    g_si4735.setSeekAmSNRThreshold(3); // default is 5
    g_si4735.getCurrentReceivedSignalQuality();
    int strength = g_si4735.getCurrentRSSI();
    h = signalStrengthToHeight(strength);
    display.drawLine(x, SCREEN_HEIGHT + 4, x, SCREEN_HEIGHT + 4 - h, WHITE);
    display.display();
    if (strength > max_strength) {
      max_strength = strength;
      strong_freq = f;
    }
  } 
    // --- Draw marker for strongest signal ---
  if (strong_freq != curr_freq) {
    int diff = (strong_freq - curr_freq) / step_khz;
    int marker_x = center_x + diff;

    if (marker_x >= 1 && marker_x < SCREEN_WIDTH-1) {
      display.setTextColor(WHITE,BLACK); // Set text to plot foreground and background colours
      display.setCursor(40, 8);
      display.print("        ");
      display.setTextColor(WHITE);
      display.drawCircle(marker_x, 12, 1, WHITE); // small marker at top
      g_si4735.setFrequency(strong_freq);
    }
  }
  if (strong_freq == curr_freq)
  {
    g_si4735.setFrequency(curr_freq);
  }
  display.display();
  delay(3000);
  display.clearDisplay();
  display.setCursor(10, 20);
  display.printf("Freq Centre= ");
  display.print(curr_freq);
  display.setCursor(45, 32);
  display.print("<-->");
  display.setCursor(10, 44);
  display.printf(" New Freq= ");
  display.print(strong_freq);
  display.display();
  delay(3000);
  // Return frequency if it's stronger than current
   if (center_strength < max_strength)
   {
    curr_freq = strong_freq;
   }
   g_currentFrequency = curr_freq;
   g_si4735.setFrequency(g_currentFrequency);
   oled.clear();
}
//=======================================================================================
