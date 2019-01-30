/*
   ESP8266 + FastLED + IR Remote: https://github.com/jasoncoon/esp8266-fastled-webserver
   Copyright (C) 2016 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//param definitions
uint8_t cooling = 50;
uint8_t sparking = 60;
uint8_t speed = 30;
uint8_t twinkleSpeed = 4;
uint8_t twinkleDensity = 5;
uint8_t satSpeed = 87 / 8;
CRGB solidColor = CRGB::Blue;
uint8_t brightDepthSpeed = 341 / 8;
uint8_t msmultiplierSpeed = 147 / 8;
uint8_t hueincSpeed = 113 / 8;

//getter and setters
String getPower() {
  return String(power);
}
void setPower(uint8_t value) {
  power = value == 0 ? 0 : 1;
  broadcastInt("power", power);
}

String getBrightness() {
  return String(brightness);
}
void setBrightness(uint8_t value) {
  if (value > 255)
    value = 255;
  else if (value < 0) value = 0;

  brightness = value;

  FastLED.setBrightness(brightness);
  
  broadcastInt("brightness", brightness);
}

String getPattern() {
  return String(currentPatternIndex);
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

String getPatterns() {
  String json = "";

  for (uint8_t i = 0; i < patternCount; i++) {
    json += "\"" + patterns[i].name + "\"";
    if (i < patternCount - 1)
      json += ",";
  }

  return json;
}

String getPalette() {
  return String(currentPaletteIndex);
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

String getPalettes() {
  String json = "";

  for (uint8_t i = 0; i < paletteCount; i++) {
    json += "\"" + paletteNames[i] + "\"";
    if (i < paletteCount - 1)
      json += ",";
  }

  return json;
}

String getZone() {
  return String(currentZoneIndex);
}
void setZone(uint8_t value) {
  if (value >= zoneCount)
    value = zoneCount - 1;
  currentZoneIndex = value;
  broadcastInt("zone", currentZoneIndex);
}

String getZones() {
  String json = "";

  for (uint8_t i = 0; i < zoneCount; i++) {
    json += "\"" + zones[i].name + "\"";
    if (i < zoneCount - 1)
      json += ",";
  }

  return json;
}

String getAutoplay() {
  return String(autoplay);
}
void setAutoplay(uint8_t value) {
  autoplay = value == 0 ? 0 : 1;
  broadcastInt("autoplay", autoplay);
}

String getAutoplayDuration() {
  return String(autoplayDuration);
}
void setAutoplayDuration(uint8_t value) {
  autoplayDuration = value;
  autoPlayTimeout = millis() + (autoplayDuration * 1000);

  broadcastInt("autoplayDuration", autoplayDuration);
}

String getSolidColor() {
  return String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b);
}
void setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
  solidColor = CRGB(r, g, b);
  setPattern(patternCount - 1);

  broadcastString("solidColor", String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
}
void setSolidColor(CRGB color) {
  setSolidColor(color.r, color.g, color.b);
}

String getCooling() {
  return String(cooling);
}

String getSparking() {
  return String(sparking);
}

String getSpeed() {
  return String(speed);
}

String getTwinkleSpeed() {
  return String(twinkleSpeed);
}

String getTwinkleDensity() {
  return String(twinkleDensity);
}
