/*
 * ESP8266 + FastLED + IR Remote: https://github.com/jasoncoon/esp8266-fastled-webserver
 * Copyright (C) 2015-2016 Jason Coon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 2
#define FRAMES_PER_SECOND 40
#include <FastLED.h>
#include <vector>
FASTLED_USING_NAMESPACE

extern "C" {
#include "user_interface.h"
}

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "GradientPalettes.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#include "Field.h"

ESP8266WebServer webServer(80);
WebSocketsServer webSocketsServer = WebSocketsServer(81);

ESP8266WiFiMulti wifiMulti;
typedef struct {
  char* name;
  char* password;
} AP;
typedef AP APList[];
#include "Secret.h"
const uint8_t apCount = ARRAY_SIZE(wifiAPs);

typedef struct {
  int start;
  int end;
  String name;
} Zone;
typedef Zone ZoneList[];

#include "RoomSpecific.h"
const uint8_t zoneCount = ARRAY_SIZE(zones);

#define DATA_PIN      3
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB

CRGB leds[NUM_LEDS];

IPAddress gateway(192, 168, 1, 1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network

uint8_t brightness = 64;
uint8_t power = 1;
// ten seconds per color palette makes a good demo, 20-120 is better for deployment
uint8_t secondsPerPalette = 10;

uint8_t cooling = 50;
uint8_t sparking = 60;
uint8_t speed = 30;

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );
CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

uint8_t currentPatternIndex = 0; // Index number of which pattern is current
uint8_t currentZoneIndex = 0; // Index number of which zone is active
uint8_t currentPaletteIndex = 0;

uint8_t autoplay = 0;
uint8_t autoplayDuration = 10;
unsigned long autoPlayTimeout = 0;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

CRGB solidColor = CRGB::Blue;

typedef struct {
  CRGBPalette16 palette;
   String name;
 } PaletteAndName;
typedef PaletteAndName PaletteAndNameList[];

#include "Palettes.h"
const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  LavaColors_p,
  OceanColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p,
  RedGreenWhite_p,
  Holly_p,
  RedWhite_p,
  BlueWhite_p,
  FairyLight_p,
  Snow_p,
  RetroC9_p,
  Ice_p
};
const uint8_t paletteCount = ARRAY_SIZE(palettes);

const String paletteNames[paletteCount] = {
  "Rainbow",
  "Rainbow Stripe",
  "Cloud",
  "Lava",
  "Ocean",
   "Forest",
  "Party",
   "Heat",
   "Red Green White",
   "Holly",
   "Red White",
   "Blue White",
   "Fairy Light",
   "Snow",
   "Retro C9",
   "Ice"
 };
#include "TwinkleFOX.h"

#include "Fields.h"
void pride();
void colorWaves();
void rainbow();
void rainbowWithGlitter();
void rainbowSolid();
void confetti();
void sinelon();
void bpm();
void juggle();
void fire();
void water();
void showSolidColor();
typedef void (*Pattern)();
typedef Pattern PatternList[];
typedef struct {
  Pattern pattern;
  String name;
  std::vector<int> params;
} PatternAndName;
typedef PatternAndName PatternAndNameList[];

PatternAndNameList patterns = {
  { pride,                  "Pride", {0, 1}},
  { colorWaves,             "Color Waves", {2, 3, 4, 5} },

  // TwinkleFOX patterns
  { showTwinkles,           "Twinkles", {0} },

  { rainbow,                "Rainbow", {0, 1, 5} },
  { rainbowWithGlitter,     "Rainbow With Glitter", {0, 1} },
  { rainbowSolid,           "Solid Rainbow", {0, 1} },
  { confetti,               "Confetti", {0, 1} },
  { sinelon,                "Sinelon", {0, 1} },
  { bpm,                    "Beat", {0, 1} },
  { juggle,                 "Juggle", {0, 1} },
  { fire,                   "Fire", {0, 1} },
  { water,                  "Water", {0, 1} },

  { showSolidColor,         "Solid Color" , {0, 1}}
};
const uint8_t patternCount = ARRAY_SIZE(patterns);

String getPatterns() {
  String json = "";

  for (uint8_t i = 0; i < patternCount; i++) {
    json += "\"" + patterns[i].name + "\"";
    if (i < patternCount - 1)
      json += ",";
  }

  return json;
}

FieldList fields = {
  { "zone", "Zone", SelectHeaderFieldType, 0, zoneCount, getZone, getZones },
  { "power", "Power", BooleanFieldType, 0, 1, getPower },
  { "brightness", "Brightness", NumberFieldType, 1, 255, getBrightness },
  { "pattern", "Pattern", SelectFieldType, 0, patternCount, getPattern, getPatterns },
  { "autoplay", "Autoplay", SectionFieldType },
  { "autoplay", "Autoplay", BooleanFieldType, 0, 1, getAutoplay },
  { "autoplayDuration", "Autoplay Duration", NumberFieldType, 0, 255, getAutoplayDuration },
  { "parameters", "Parameters", SectionFieldType }
};

uint8_t fieldCount = ARRAY_SIZE(fields);

void setup() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(1000);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);         // for WS2812 (Neopixel)
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);

  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);

  int count = 2;
  for (int i = 0; i < apCount; i++) {
    wifiMulti.addAP(wifiAPs[i].name, wifiAPs[i].password);
  }
  while (wifiMulti.run() != WL_CONNECTED) {
    for (int i = 0; i < 5;i++) {
      count++;
      leds[count%NUM_LEDS] = CRGB(255, 0, 0);
      leds[(count-1)%NUM_LEDS] = CRGB(128, 0, 0);
      leds[(count-3)%NUM_LEDS] = CRGB(64, 0, 0);
      leds[(count-3)%NUM_LEDS] = CRGB(0, 0, 0);
      FastLED.show();
      delay(100);
    }
  }

  webServer.on("/all", HTTP_GET, []() {
    String json = getFieldsJson(fields, fieldCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", json);
  });
  
  webServer.on("/parameters", HTTP_GET, []() {
    String json = getFieldsJsonVec(patterns[currentPatternIndex].params);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", json);
  });

  webServer.on("/fieldValue", HTTP_GET, []() {
    String name = webServer.arg("name");
    String value = getFieldValue(name, fields, fieldCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", value);
  });

  webServer.on("/fieldValue", HTTP_POST, []() {
    String name = webServer.arg("name");
    String value = webServer.arg("value");
    String newValue = setFieldValue(name, value, fields, fieldCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", newValue);
  });
  
  webServer.on("/zone", HTTP_POST, []() {
    String value = webServer.arg("value");
    setZone(value.toInt());
    sendInt(power);
  });

  webServer.on("/power", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPower(value.toInt());
    sendInt(power);
  });

  webServer.on("/cooling", HTTP_POST, []() {
    String value = webServer.arg("value");
    cooling = value.toInt();
    broadcastInt("cooling", cooling);
    sendInt(cooling);
  });

  webServer.on("/sparking", HTTP_POST, []() {
    String value = webServer.arg("value");
    sparking = value.toInt();
    broadcastInt("sparking", sparking);
    sendInt(sparking);
  });

  webServer.on("/speed", HTTP_POST, []() {
    String value = webServer.arg("value");
    speed = value.toInt();
    broadcastInt("speed", speed);
    sendInt(speed);
  });

  webServer.on("/twinkleSpeed", HTTP_POST, []() {
    String value = webServer.arg("value");
    twinkleSpeed = value.toInt();
    if(twinkleSpeed < 0) twinkleSpeed = 0;
    else if (twinkleSpeed > 8) twinkleSpeed = 8;
    broadcastInt("twinkleSpeed", twinkleSpeed);
    sendInt(twinkleSpeed);
  });

  webServer.on("/twinkleDensity", HTTP_POST, []() {
    String value = webServer.arg("value");
    twinkleDensity = value.toInt();
    if(twinkleDensity < 0) twinkleDensity = 0;
    else if (twinkleDensity > 8) twinkleDensity = 8;
    broadcastInt("twinkleDensity", twinkleDensity);
    sendInt(twinkleDensity);
  });

  webServer.on("/solidColor", HTTP_POST, []() {
    String r = webServer.arg("r");
    String g = webServer.arg("g");
    String b = webServer.arg("b");
    setSolidColor(r.toInt(), g.toInt(), b.toInt());
    sendString(String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
  });

  webServer.on("/pattern", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPattern(value.toInt());
    sendInt(currentPatternIndex);
  });

  webServer.on("/patternName", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPatternName(value);
    sendInt(currentPatternIndex);
  });

  webServer.on("/palette", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPalette(value.toInt());
    sendInt(currentPaletteIndex);
  });

  webServer.on("/paletteName", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPaletteName(value);
    sendInt(currentPaletteIndex);
  });

  webServer.on("/brightness", HTTP_POST, []() {
    String value = webServer.arg("value");
    setBrightness(value.toInt());
    sendInt(brightness);
  });

  webServer.on("/autoplay", HTTP_POST, []() {
    String value = webServer.arg("value");
    setAutoplay(value.toInt());
    sendInt(autoplay);
  });

  webServer.on("/autoplayDuration", HTTP_POST, []() {
    String value = webServer.arg("value");
    setAutoplayDuration(value.toInt());
    sendInt(autoplayDuration);
  });

  webServer.begin();

  webSocketsServer.begin();
  webSocketsServer.onEvent(webSocketEvent);

  autoPlayTimeout = millis() + (autoplayDuration * 1000);
}

void sendInt(uint8_t value) {
  sendString(String(value));
}

void sendString(String value) {
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "text/plain", value);
}

void broadcastInt(String name, uint8_t value) {
  String json = "{\"name\":\"" + name + "\",\"value\":" + String(value) + "}";
  webSocketsServer.broadcastTXT(json);
}

void broadcastString(String name, String value) {
  String json = "{\"name\":\"" + name + "\",\"value\":\"" + String(value) + "\"}";
  webSocketsServer.broadcastTXT(json);
}

void loop() {
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(65535));

  webSocketsServer.loop();
  webServer.handleClient();

  if (power == 0) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
    return;
  }

  // change to a new cpt-city gradient palette
  EVERY_N_SECONDS( secondsPerPalette ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  EVERY_N_MILLISECONDS(40) {
    // slowly blend the current palette to the next
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 8);
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  if (autoplay && (millis() > autoPlayTimeout)) {
    adjustPattern(true);
    autoPlayTimeout = millis() + (autoplayDuration * 1000);
  }

  // Call the current pattern function once, updating the 'leds' array
  patterns[currentPatternIndex].pattern();

  FastLED.show();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocketsServer.remoteIP(num);

        // send message to client
        // webSocketsServer.sendTXT(num, "Connected");
      }
      break;

    case WStype_TEXT:

      // send message to client
      // webSocketsServer.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocketsServer.broadcastTXT("message here");
      break;

    case WStype_BIN:
      hexdump(payload, length);

      // send message to client
      // webSocketsServer.sendBIN(num, payload, lenght);
      break;
  }
}

void setZone(uint8_t value) {
  if (value >= zoneCount)
    value = zoneCount - 1;
  currentZoneIndex = value;
  broadcastInt("zone", currentZoneIndex);
}

void setPower(uint8_t value) {
  power = value == 0 ? 0 : 1;
  broadcastInt("power", power);
}

void setAutoplay(uint8_t value) {
  autoplay = value == 0 ? 0 : 1;
  broadcastInt("autoplay", autoplay);
}

void setAutoplayDuration(uint8_t value) {
  autoplayDuration = value;
  autoPlayTimeout = millis() + (autoplayDuration * 1000);

  broadcastInt("autoplayDuration", autoplayDuration);
}

void setSolidColor(CRGB color) {
  setSolidColor(color.r, color.g, color.b);
}

void setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
  solidColor = CRGB(r, g, b);
  setPattern(patternCount - 1);

  broadcastString("solidColor", String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
}

// increase or decrease the current pattern number, and wrap around at the ends
void adjustPattern(bool up) {
  if (up)
    currentPatternIndex++;
  else
    currentPatternIndex--;

  // wrap around at the ends
  if (currentPatternIndex < 0)
    currentPatternIndex = patternCount - 1;
  if (currentPatternIndex >= patternCount)
    currentPatternIndex = 0;
    
  broadcastInt("pattern", currentPatternIndex);
}

void setPattern(uint8_t value) {
  if (value >= patternCount)
    value = patternCount - 1;
  currentPatternIndex = value;
  
  broadcastInt("pattern", currentPatternIndex);
}

void setPatternName(String name) {
  for(uint8_t i = 0; i < patternCount; i++) {
    if(patterns[i].name == name) {
      setPattern(i);
      break;
    }
  }
}

void setPalette(uint8_t value) {
  if (value >= paletteCount)
    value = paletteCount - 1;
  currentPaletteIndex = value;
  
  broadcastInt("palette", currentPaletteIndex);
}

void setPaletteName(String name) {
  for(uint8_t i = 0; i < paletteCount; i++) {
    if(paletteNames[i] == name) {
      setPalette(i);
      break;
    }
  }
}

void setBrightness(uint8_t value) {
  if (value > 255)
    value = 255;
  else if (value < 0) value = 0;

  brightness = value;

  FastLED.setBrightness(brightness);
  
  broadcastInt("brightness", brightness);
}

void strandTest() {
  static uint8_t i = 0;

  EVERY_N_SECONDS(1)
  {
    i++;
    if (i >= NUM_LEDS)
      i = 0;
  }

  fill_solid(leds, NUM_LEDS, CRGB::Black);

  leds[i] = solidColor;
}

void showSolidColor() {
  fill_solid(leds, NUM_LEDS, solidColor);
}

// Patterns from FastLED example DemoReel100: https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 255 / NUM_LEDS);
}

void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void rainbowSolid() {
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  // leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64));
}

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(speed, 0, NUM_LEDS);
  static int prevpos = 0;
  CRGB color = ColorFromPalette(palettes[currentPaletteIndex], gHue, 255);
  if( pos < prevpos ) {
    fill_solid( leds+pos, (prevpos-pos)+1, color);
  } else {
    fill_solid( leds+prevpos, (pos-prevpos)+1, color);
  }
  prevpos = pos;
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t beat = beatsin8( speed, 64, 255);
  CRGBPalette16 palette = palettes[currentPaletteIndex];
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle() {
  static uint8_t    numdots =   4; // Number of dots in use.
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails.
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot.
  static uint8_t    thishue =   0; // Starting hue.
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour.
  static uint8_t thisbright = 255; // How bright should the LED/display be.
  static uint8_t   basebeat =   5; // Higher = faster movement.

  static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand) { // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    switch (secondHand) {
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30: break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for ( int i = 0; i < numdots; i++) {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat + i + numdots, 0, NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }
}

void fire() {
  heatMap(HeatColors_p, true);
}

void water() {
  heatMap(IceColors_p, false);
}

// Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }
}

void radialPaletteShift() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // leds[i] = ColorFromPalette( gCurrentPalette, gHue + sin8(i*16), brightness);
    leds[i] = ColorFromPalette(gCurrentPalette, i + gHue, 255, LINEARBLEND);
  }
}

// based on FastLED example Fire2012WithPalette: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino
void heatMap(CRGBPalette16 palette, bool up) {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  byte colorindex;

  // Step 1.  Cool down every cell a little
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( uint16_t k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < sparking ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( uint16_t j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    colorindex = scale8(heat[j], 190);

    CRGB color = ColorFromPalette(palette, colorindex);

    if (up) {
      leds[j] = color;
    }
    else {
      leds[(NUM_LEDS - 1) - j] = color;
    }
  }
}

void addGlitter( uint8_t chanceOfGlitter) {
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

uint8_t beatsaw8( accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255,
                  uint32_t timebase = 0, uint8_t phase_offset = 0)
{
  uint8_t beat = beat8( beats_per_minute, timebase);
  uint8_t beatsaw = beat + phase_offset;
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8( beatsaw, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}

void colorWaves() {
  colorwaves( leds, NUM_LEDS, gCurrentPalette);
}

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette) {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if ( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds - 1) - pixelnumber;
    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

// Alternate rendering function just scrolls the current palette
// across the defined LED strip.
void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette) {
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}