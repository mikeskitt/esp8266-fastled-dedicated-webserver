
#include <WebSockets.h>
//#include <WebSocketsClient.h>
#include <WebSocketsServer.h>


#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FRAMES_PER_SECOND 40
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define LED_TYPE      WS2811
#define COLOR_ORDER   RGB

//1 living, 2 kitchen, 3 garage, 4 foosball room, 5 party room, 6 stairs, 7 lighthouse, 20 calibration, 21 dads basement
//252       251        250                                                                                
#define CURRENT_ROOM 1

#include <FastLED.h>
#define DATA_PIN      4
#include <vector>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>

#include <ArduinoJson.h>  //v6.8 beta works
#include "GradientPalettes.h"
#include "TypeDefs.h"
#include "Secret.h"
#include "RoomSpecific.h"
#include "Palettes.h"
int lastSat;
CHSV tintAmt(0,0,90);

ESP8266WebServer webServer(80);
WebSocketsServer webSocketsServer = WebSocketsServer(81);
//ESP8266WiFiMulti WiFiMulti; //!going to regular wifi so it actually connects every time. wifiMulti BUG?
WiFiUDP Udp;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming udp packet,

DynamicJsonDocument doc(1024);

IPAddress gateway(192, 168, 1, 1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network
const IPAddress multicastIpAddr = IPAddress(239,0,0,222); // multicast ip address to match audio server
unsigned int multicastPort = 2222;  // local multicast port to listen on

CRGB leds[NUM_LEDS];
CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );
CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

const uint8_t apCount = 3;
const uint8_t zoneCount = ARRAY_SIZE(zones);
uint8_t brightness = 250;
uint8_t power = 0;
uint8_t secondsPerPalette = 10;
uint8_t gCurrentPaletteNumber = 0;
uint8_t currentPatternIndex = 0;
uint8_t currentZoneIndex = 0;
uint8_t autoplay = 0;
uint8_t autoplayDuration = 10;
uint8_t audioGain = 0;
unsigned long autoPlayTimeout = 0;
unsigned long gHue = 0;

/*#define AUDIO_DROP_RATE 20
double lowFreq = 0;
double low40Freq = 0;
double low80Freq = 0;
double low120Freq = 0;
double low160Freq = 0;
double midFreq = 0;
double highFreq = 0;
double* currentFreqBand = &lowFreq;
uint8_t bass;*/
int shift1 = 0;                                     //for large jumps in loudness

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
  Ice_p,
  Lemony_p,
  SpForest_p
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
   "Ice",
   "Lemony",
   "Spring Forest"
 };

PatternAndNameList patterns = {
  { pride,                  "Pride", {7, 8, 9}},
  { colorWaves,             "Color Waves", {8, 9} },

  // TwinkleFOX patterns
  { showTwinkles,           "Twinkles", {0, 5, 6} },

  { rainbow,                "Rainbow", {1, 10, 14} },
  { rainbowWithGlitter,     "Rainbow With Glitter", {1, 10} },
  { rainbowSolid,           "Solid Rainbow", {1, 14} },
  { confetti,               "Confetti", {0, 12} },
  { sinelon,                "Sinelon", {0, 1, 12} },
  { bpm,                    "Beat", {0, 1} },
  { juggle,                 "Juggle", {} },
  { fire,                   "Fire", {3, 4} },
  { water,                  "Water", {3, 4} },

  { showSolidColor,         "Solid Color" , {2}},

/*  { audioSolidColor,        "(Audio) Solid Color", {2, 13}},
  { audioRainbowEdge,       "(Audio) Rainbow Edge", {13}},
  { audioFadeEdge,          "(Audio) Fade Edge", {13}},
  { audioFadeRedEdge,       "(Audio) Red Edge", {13}},
  { audioRedWhiteEdge,      "(Audio) White Edge", {13}},
  { audioRainbowMid,        "(Audio) Rainbow Mid", {13}},
  { audioFadeMid,           "(Audio) Fade Mid", {13}},
  { audioFadeRedMid,        "(Audio) Red Mid", {13}},
  { audioRedWhiteMid,       "(Audio) White Mid", {13}},
  { audioRainbowSpeed,      "(Audio) Rainbow Speed", {13}},
  { audioRainbowSpeedBack,  "(Audio) Rainbow Speed 2", {13}},
  { audioRainbowSpeedLong,  "(Audio) Rainbow Speed Long", {13}},
  { audioRainbowSpeedLongBack,"(Audio) Rainbow Speed Long 2", {13}},
  { audioConfetti,          "(Audio) Confetti", {13}},
  { audioConfettiDense,     "(Audio) Confetti Dense", {13}}*/
};
const uint8_t patternCount = ARRAY_SIZE(patterns);

#include "Fields.h"
#include "Field.h"
#include "TwinkleFOX.h"

void setup() {
  Serial.setDebugOutput(true);
  Serial.begin(115200);
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
/*  WiFiMulti.addAP("TP-LINK_C9701C", "6c4a61bbbd");
  WiFiMulti.addAP("linksys38_RPT", "6c4a61bbbd");
  WiFiMulti.addAP("linksys38", "6c4a61bbbd");*/
   WiFi.begin(STASSID, STAPSK); 
  //https://github.com/FastLED/FastLED/issues/367
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  //Serial.printf("Connecting to %s\n", wifiAPs[1].name);

  int count = 2;
  Serial.print("Current AP List: ");
/*for (int i = 0; i < apCount; i++) {
    WiFiMulti.addAP(wifiAPs[i].name, wifiAPs[i].password);
    Serial.print(wifiAPs[i].name);
    Serial.print(" ");
  }*/
  Serial.println();
/*  while (WiFiMulti.run() != WL_CONNECTED) {
  /*WiFi.disconnect();
  WiFi.begin(wifiAPs[1].name, wifiAPs[1].password);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    for (int i = 0; i < 5;i++) {
      count++;
      leds[count%NUM_LEDS] = CRGB(255, 0, 0);
      leds[(count-1)%NUM_LEDS] = CRGB(128, 0, 0);
      leds[(count-3)%NUM_LEDS] = CRGB(64, 0, 0);
      leds[(count-3)%NUM_LEDS] = CRGB(0, 0, 0);
      FastLED.show();
      delay(100);
    }*/
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }  
  
  Serial.println();
  Serial.print("Connected to ");
  Serial.print(WiFi.SSID());
  Serial.print(", ip ");
  Serial.println(WiFi.localIP());

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

  webServer.on("/formParametersValue", HTTP_GET, []() {
    String name = webServer.arg("name");
    String value = getFieldValue(name, fieldParams, fieldParamsCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", value);
  });

  webServer.on("/formParametersValue", HTTP_POST, []() {
    String name = webServer.arg("name");
    String value = webServer.arg("value");
    String newValue = setFieldValue(name, value, fieldParams, fieldParamsCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", newValue);
  });
  webServer.on("/formGeneralValue", HTTP_GET, []() {
    String name = webServer.arg("name");
    String value = getFieldValue(name, fields, fieldCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", value);
  });

  webServer.on("/formGeneralValue", HTTP_POST, []() {
    String name = webServer.arg("name");
    String value = webServer.arg("value");
    String newValue = setFieldValue(name, value, fields, fieldCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", newValue);
  });
  
/*  webServer.on("/solidColor", HTTP_POST, []() {
    String r = webServer.arg("r");
    String g = webServer.arg("g");
    String b = webServer.arg("b");
    setSolidColor(r.toInt(), g.toInt(), b.toInt());
    sendString(String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
  }); */
  
  webServer.begin();
  Serial.println("HTTP web server started");

  webSocketsServer.begin();
  webSocketsServer.onEvent(webSocketEvent);
  Serial.println("Web socket server started");
  
/*  Udp.beginMulticast(WiFi.localIP(), multicastIpAddr, multicastPort);
  Serial.printf("Listening to multicast IP %s UDP port %d\n for music", multicastIpAddr.toString().c_str(), multicastPort);
  Udp.stop();*/

  autoPlayTimeout = millis() + (autoplayDuration * 1000);
}

void loop() {
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(65535));

  webSocketsServer.loop();
  webServer.handleClient();
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    // read the packet into packetBufffer
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    //Serial.println(String(packetBuffer)); //always print out before, deserialize destroys
    
     DeserializationError error = deserializeJson(doc, packetBuffer);
/*    if (error) {
      Serial.print("Error: Bad parse: ");
      Serial.println(error.c_str());
    }
    else {
      
      int temp = doc["low40hz"];
      temp *= (audioGain / 255.0);
      if (temp > low40Freq - AUDIO_DROP_RATE) low40Freq = temp;
      else low40Freq -= AUDIO_DROP_RATE;
      
      temp = doc["low80hz"];
      temp *= (audioGain / 255.0);
      if (temp > low80Freq - AUDIO_DROP_RATE) low80Freq = temp;
      else low80Freq -= AUDIO_DROP_RATE;
      
      temp = doc["low120hz"];
      temp *= (audioGain / 255.0);
      if (temp > low120Freq - AUDIO_DROP_RATE) low120Freq = temp;
      else low120Freq -= AUDIO_DROP_RATE;
      
      temp = doc["low160hz"];
      temp *= (audioGain / 255.0);
      if (temp > low160Freq - AUDIO_DROP_RATE) low160Freq = temp;
      else low160Freq -= AUDIO_DROP_RATE;
      
      temp = doc["mid"];
      temp *= (audioGain / 255.0);
      if (temp > midFreq - AUDIO_DROP_RATE) midFreq = temp;
      else midFreq -= AUDIO_DROP_RATE;
      
      temp = doc["high"];
      temp *= (audioGain / 255.0);
      if (temp > highFreq - AUDIO_DROP_RATE) highFreq = temp;
      else highFreq -= AUDIO_DROP_RATE;
      
      int previousLow = lowFreq;
      lowFreq = low40Freq + low80Freq + low120Freq + low160Freq / 4;
      if (lowFreq - previousLow > 30) shift1 += (lowFreq - previousLow - 30) / 3; //set the shift higher on large bass hits
      
      constrainDouble(lowFreq, 0, NUM_LEDS / 2);
      constrainDouble(low40Freq, 0, 255);
      constrainDouble(low80Freq, 0, 255);
      constrainDouble(low120Freq, 0, 255);
      constrainDouble(low160Freq, 0, 255);
      constrainDouble(midFreq, 0, 255);
      constrainDouble(highFreq, 0, 255);
    }*/
  }
  if (power == 0) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    return;
  }
  /*EVERY_N_SECONDS(2) {
     Serial.print( F("Heap: ") );
     Serial.println(system_get_free_heap_size());
  }*/
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

//server stuff
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
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocketsServer.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocketsServer.sendTXT(num, "Connected");
      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      // send message to client
      webSocketsServer.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocketsServer.broadcastTXT("message here");
      break;

    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);

      // send message to client
      webSocketsServer.sendBIN(num, payload, length);
      break;
  }
}

//helpers
void adjustPattern(bool up) {
  if (up) {
    setPattern(String(++currentPatternIndex));
  }
  else {
    setPattern(String(--currentPatternIndex));
  }

  broadcastString("pattern", String(currentPatternIndex));
}
void constrainDouble(double &value, int lowerBound, int upperBound) {
  if (value > upperBound) {
    value = upperBound;
  }
  else if (value < lowerBound) {
    value = lowerBound;
  }
}
//patterns-----------------------------------------------------------------------------------

void showSolidColor() {
  fill_solid(leds, NUM_LEDS, solidColor);
}
void rainbow() {        // Patterns from FastLED example DemoReel100: https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
  // FastLED's built-in rainbow generator
//  if(paramInt3!=lastSat){
    CHSV tintAmt(0,0,255-paramInt3);
/*    lastSat = paramInt3;
    }*/
  fill_rainbow( leds, NUM_LEDS, gHue, 255 / paramInt1);
  for (uint8_t i=0; i<NUM_LEDS; i++) {
      leds[i] += tintAmt;
    }
}
void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(paramInt2);
}
void rainbowSolid() {
  fill_solid(leds, NUM_LEDS, CHSV(gHue * (paramInt1 / 3), (paramInt3), 255));
}
void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, paramInt2/2);
  if(random16(10)>6) {
  int pos = random16(NUM_LEDS);
  // leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64));
  }
}
void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, paramInt2);
  int pos = beatsin16(paramInt1, 0, NUM_LEDS);
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
  uint8_t beat = beatsin8( paramInt1, 64, 255);
  CRGBPalette16 palette = palettes[currentPaletteIndex];
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}
void juggle() {
  static uint8_t    numdots =   8; // Number of dots in use.
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
void pride() {        // Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88(paramInt1 * 8, 220, 250);
  uint8_t brightdepth = beatsin88(paramInt2 * 8, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(paramInt3 * 8, 23, 60);

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
void heatMap(CRGBPalette16 palette, bool up) {        // based on FastLED example Fire2012WithPalette: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  byte colorindex;

  // Step 1.  Cool down every cell a little
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((paramInt1 * 10) / NUM_LEDS) + 2));
  }
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( uint16_t k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < paramInt2 ) {
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
void colorWaves() {
  colorwaves( leds, NUM_LEDS, gCurrentPalette);
}
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette) {// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88(paramInt2, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(paramInt3, 23, 60);

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
void Newconfetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 4);
  for(int x=0;x<8;x++){
  int pos = random16(NUM_LEDS);
  //leds[pos] += CHSV( 85 + random8(64), 100, 255);
  leds[pos] += CHSV( 85 + random8(8), 245, 255);
  }
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( 42, 255, 255);
}


//audio anims----------------------------------------------------------------------------------
/*
void audioSolidColor() {
  CRGB newColor = solidColor;
  //Serial.println(newColor.r);
  //Serial.println(lowFreq);
  newColor.r *= (lowFreq / 255.0);
  newColor.g *= (lowFreq / 255.0);
  newColor.b *= (lowFreq / 255.0);
  //Serial.println(newColor.r);
  fill_solid(leds, NUM_LEDS, newColor);
}

void audioRainbowEdge() {
  //update background of both
  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = CHSV(lowFreq * 2, 200, 35);
    //leds2[dot] = CHSV((lowFreq * 2) + 128, 200, 35);
  }
  //update near (left) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[dot - 1] = CHSV((dot * 4), 255, 255);
    //leds2[dot - 1] = CHSV((dot*2.3) + (gHue)+128, 255, 255);
  }
  //update far (right) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS - dot] = CHSV((dot * 4), 255, 255);
    //leds2[NUM_LEDS - dot] = CHSV((dot)+(gHue)+128, 255, 255);
  }
}
void audioFadeEdge() {
  //fade effect
  fadeToBlackBy(leds, NUM_LEDS, 30);
  //fadeToBlackBy(leds2, NUM_LEDS, 30);

  //update near (left) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[dot - 1] = CHSV(shift1, 255, 255);
    //leds2[dot - 1] = CHSV(shift1 + 127, 255, 255);
  }
  //update far (right) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS - dot] = CHSV(shift1 + 63, 255, 255);
    //leds2[NUM_LEDS - dot] = CHSV(shift1 + 191, 255, 255);
  }
}
void audioFadeRedEdge() {
  //fade effect
  fadeToBlackBy(leds, NUM_LEDS, 30);
  //fadeToBlackBy(leds2, NUM_LEDS, 30);

  if (shift1 > 0) {
    shift1 /= 1.1;
  }
  if (shift1 > 127) {
    shift1 = 127;
  }
  //update near (left) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[dot - 1] = CRGB(255, shift1 * 2, shift1 * 2);
    //leds2[NUM_LEDS / 2 - (dot - 1)] = CRGB(255, shift1, shift1);
  }
  //update far (right) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS - dot] = CRGB(255, shift1 * 2, shift1 * 2);
    //leds2[NUM_LEDS / 2 + dot] = CRGB(255, shift1, shift1);
  }
}
void audioRedWhiteEdge() {
  //fade effect
  fadeToBlackBy(leds, NUM_LEDS, 30);
  //fadeToBlackBy(leds2, NUM_LEDS, 30);
 
  fill_solid(leds, NUM_LEDS, CRGB(165, 0, 0));
  //fill_solid(leds2, NUM_LEDS, CRGB(165, 0, 0));
  //update near (left) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[dot - 1] = CRGB(255, 255, 255);
    //leds2[NUM_LEDS / 2 - (dot - 1)] = CRGB(255, 255, 255);
  }
  //update far (right) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS - dot] = CRGB(255, 255, 255);
    //leds2[NUM_LEDS / 2 + dot] = CRGB(255, 255, 255);
  }
}

void audioRainbowMid() {
  //update all led background
  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = CHSV(lowFreq * 2, 200, 35);
    //leds2[dot] = CHSV((lowFreq * 2) + 128, 200, 35);
  }
  //update near (left) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS / 2 - (dot - 1)] = CHSV((dot * 4), 255, 255);
    //leds2[NUM_LEDS / 2 - (dot - 1)] = CHSV((dot*2.3) + (gHue)+128, 255, 255);
  }
  //update far (right) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS / 2 + dot] = CHSV((dot * 4), 255, 255);
    //leds2[NUM_LEDS / 2 + dot] = CHSV((dot)+(gHue)+128, 255, 255);
  }
}
void audioFadeMid() {
  //fade effect
  fadeToBlackBy(leds, NUM_LEDS, 30);
  //fadeToBlackBy(leds2, NUM_LEDS, 30);

  //update near (left) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS / 2 - (dot - 1)] = CHSV(shift1, 255, 255);
    //leds2[NUM_LEDS / 2 - (dot - 1)] = CHSV(shift1 + 127, 255, 255);
  }
  //update far (right) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS / 2 + dot] = CHSV(shift1 + 63, 255, 255);
    //leds2[NUM_LEDS / 2 + dot] = CHSV(shift1 + 191, 255, 255);
  }
}

void audioFadeRedMid() {
  //fade effect
  fadeToBlackBy(leds, NUM_LEDS, 30);
  //fadeToBlackBy(leds2, NUM_LEDS, 30);

  if (shift1 > 0) {
    shift1 /= 1.1;
  }
  if (shift1 > 127) {
    shift1 = 127;
  }
  //update near (left) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS / 2 - (dot - 1)] = CRGB(255, shift1 * 2, shift1 * 2);
    //leds2[NUM_LEDS / 2 - (dot - 1)] = CRGB(255, shift1, shift1);
  }
  //update far (right) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS / 2 + dot] = CRGB(255, shift1 * 2, shift1 * 2);
    //leds2[NUM_LEDS / 2 + dot] = CRGB(255, shift1, shift1);
  }
}
void audioRedWhiteMid() {
  //fade effect
  fadeToBlackBy(leds, NUM_LEDS, 30);
  //fadeToBlackBy(leds2, NUM_LEDS, 30);

  fill_solid(leds, NUM_LEDS, CRGB(165, 0, 0));
  //fill_solid(leds2, NUM_LEDS, CRGB(165, 0, 0));
  //update near (left) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS / 2 - (dot - 1)] = CRGB(255, 255, 255);
    //leds2[NUM_LEDS / 2 - (dot - 1)] = CRGB(255, 255, 255);
  }
  //update far (right) side
  for (int dot = 1; dot < lowFreq; dot++) {
    leds[NUM_LEDS / 2 + dot] = CRGB(255, 255, 255);
    //leds2[NUM_LEDS / 2 + dot] = CRGB(255, 255, 255);
  }
}

/*void audioRainbowSpeed() {
  gHue += lowFreq / 2 + 1;
  fill_rainbow(leds, NUM_LEDS, (gHue), 7);
}
void audioRainbowSpeedBack() {
  gHue += lowFreq / 2 - 5;
  fill_rainbow(leds, NUM_LEDS, (gHue), 7);
}
void audioRainbowSpeedLong() {
  gHue += lowFreq / 3 + 1;
  fill_rainbow(leds, NUM_LEDS, (gHue), 2);
}
void audioRainbowSpeedLongBack() {
  gHue += lowFreq / 3 - 5;
  fill_rainbow(leds, NUM_LEDS, (gHue), 7);
}
void audioConfetti() {
  fadeToBlackBy(leds, NUM_LEDS, 1+(lowFreq*lowFreq) / 70);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue + random8(64), 200, 255);
}
void audioConfettiDense() {
  fadeToBlackBy(leds, NUM_LEDS, 2 + (lowFreq*lowFreq) / 70);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue * 2 + random8(64), 200, 255);
  pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue * 2 + random8(64), 200, 255);
  pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue * 2 + random8(64), 200, 255);
}*/
