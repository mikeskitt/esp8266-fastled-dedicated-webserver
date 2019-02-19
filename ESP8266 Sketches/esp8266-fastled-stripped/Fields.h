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
uint8_t currentPaletteIndex = 0;
CRGB solidColor = CRGB::Blue;
uint8_t hueincSpeed = 113 / 8;
uint8_t paramInt1 = 0;
uint8_t paramInt2 = 0;
uint8_t paramInt3 = 0;
uint8_t paramInt4 = 0;

//getter and setters
String getPower() {
  return String(power);
}
void setPower(String value) {
  uint8_t val = value.toInt();
  power = val == 0 ? 0 : 1;
  broadcastInt("power", power);
}

String getBrightness() {
  return String(brightness);
}
void setBrightness(String val) {
  uint8_t value = val.toInt();
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
void setPattern(String val) {
  uint8_t value = val.toInt();
  if (value >= patternCount)
    value = patternCount - 1;
  currentPatternIndex = value;

  //reset each parameter value to the default for the new pattern
  for (int i = 0; i < patterns[currentPatternIndex].params.size(); i++) {
    fieldParams[patterns[currentPatternIndex].params[i]].setValue(String(fieldParams[patterns[currentPatternIndex].params[i]].defaultVal));
    Serial.println("Changing " + fieldParams[patterns[currentPatternIndex].params[i]].name + " to " + String(fieldParams[patterns[currentPatternIndex].params[i]].defaultVal));
  }
  
  broadcastInt("pattern", currentPatternIndex);
}
void setPatternName(String name) {
  for(uint8_t i = 0; i < patternCount; i++) {
    if(patterns[i].name == name) {
      setPattern(String(i));
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
void setPalette(String val) {
  uint8_t value = val.toInt();
  if (value >= paletteCount)
    value = paletteCount - 1;
  currentPaletteIndex = value;
  
  broadcastInt("palette", currentPaletteIndex);
}
void setPaletteName(String name) {
  for(uint8_t i = 0; i < paletteCount; i++) {
    if(paletteNames[i] == name) {
      setPalette(String(i));
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
void setZone(String val) {
  uint8_t value = val.toInt();
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
void setAutoplay(String val) {
  uint8_t value = val.toInt();
  autoplay = value == 0 ? 0 : 1;
  broadcastInt("autoplay", autoplay);
}

String getAutoplayDuration() {
  return String(autoplayDuration);
}
void setAutoplayDuration(String val) {
  uint8_t value = val.toInt();
  autoplayDuration = value;
  autoPlayTimeout = millis() + (autoplayDuration * 1000);

  broadcastInt("autoplayDuration", autoplayDuration);
}

String getSolidColor() {
  return String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b);
}
void setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
  solidColor = CRGB(r, g, b);

  broadcastString("solidColor", String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
}
void setSolidColor(CRGB color) {
  setSolidColor(color.r, color.g, color.b);
}


void setParamInt(uint8_t &paramInt, String val) {     //helper function
  uint8_t value = val.toInt();
  paramInt = value;
}

String getParamInt1() {
  return String(paramInt1);
}
void setParamInt1(String val) {
  setParamInt(paramInt1, val);
}
String getParamInt2() {
  return String(paramInt2);
}
void setParamInt2(String val) {
  setParamInt(paramInt2, val);
}
String getParamInt3() {
  return String(paramInt3);
}
void setParamInt3(String val) {
  setParamInt(paramInt3, val);
}
String getParamInt4() {
  return String(paramInt4);
}
void setParamInt4(String val) {
  setParamInt(paramInt4, val);
}
