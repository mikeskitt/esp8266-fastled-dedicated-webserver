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


const String NumberFieldType = "Number";
const String BooleanFieldType = "Boolean";
const String SelectFieldType = "Select";
const String SelectHeaderFieldType = "SelectHeader";
const String SelectButtonFieldType = "SelectButton";
const String ColorFieldType = "Color";
const String SectionFieldType = "Section";

FieldList fieldParams = {
  { "palette", "Palette", SelectFieldType, 0, paletteCount, 0, getPalette, setPalette, getPalettes },           //0
  { "speed", "Speed", NumberFieldType, 1, 255, 30, getParamInt1, setParamInt1 },                                //1
  { "solidColor", "Color", ColorFieldType, 0, 255, 0, getSolidColor},                                           //2
  { "cooling", "Cooling", NumberFieldType, 0, 255, 80, getParamInt1, setParamInt1 },                             //3
  { "sparking", "Sparking", NumberFieldType, 0, 255, 80, getParamInt2, setParamInt2},                           //4
  { "twinkleSpeed", "Twinkle Speed", NumberFieldType, 0, 8, 6, getParamInt1, setParamInt1},                     //5
  { "twinkleDensity", "Twinkle Density", NumberFieldType, 0, 8, 5, getParamInt2, setParamInt2},                 //6
  { "satSpeed", "Saturation Speed", NumberFieldType, 1, 255, 11, getParamInt1, setParamInt1},                   //7
  { "brightDepthSpeed", "Bright Depth Speed", NumberFieldType, 1, 255, 43, getParamInt2, setParamInt2},         //8
  { "msmultiplierSpeed", "msmultiplier Speed", NumberFieldType, 1, 255, 18, getParamInt3, setParamInt3},        //9
  { "length", "Length", NumberFieldType, 1, 255, 129, getParamInt2, setParamInt2},                              //10
  { "glitterChance", "Glitter Chance", NumberFieldType, 1, 255, 80, getParamInt2, setParamInt2},                //11
  { "fade", "Fade", NumberFieldType, 1, 255, 80, getParamInt2, setParamInt2},                                   //12
  { "audioGain", "Audio Gain", NumberFieldType, 0, 255, 100, getAudioGain, setAudioGain},                       //13
  { "saturation", "Saturation", NumberFieldType, 0, 255, 255, getParamInt3, setParamInt3}/*,                       //14
  { "freqBand", "Frequency Band", SelectHeaderFieldType, 0, 0, zoneCount, getFrequencyBand, setFrequencyBand, getFrequencyBands }//15*/
};
uint8_t fieldParamsCount = ARRAY_SIZE(fieldParams);

FieldList fields = {
  { "zone", "Zone", SelectHeaderFieldType, 0, 0, zoneCount, getZone, setZone, getZones },
  { "power", "Power", BooleanFieldType, 0, 1, 1, getPower, setPower},
  { "brightness", "Brightness", NumberFieldType, 1, 255, 100, getBrightness, setBrightness },
  { "pattern", "Pattern", SelectFieldType, 0, patternCount, 0, getPattern, setPattern, getPatterns},
  { "autoplaySection", "Autoplay", SectionFieldType },
  { "autoplay", "Autoplay", BooleanFieldType, 0, 1, 0, getAutoplay, setAutoplay},
  { "autoplayDuration", "Autoplay Duration", NumberFieldType, 1, 255, 60, getAutoplayDuration, setAutoplayDuration},
  { "parametersSection", "Parameters", SectionFieldType }
};
uint8_t fieldCount = ARRAY_SIZE(fields);

Field getField(String name, FieldList fields, uint8_t count) {
  Serial.print("get field ");
  Serial.println(name);
  for (uint8_t i = 0; i < count; i++) {
    Field field = fields[i];
    if (field.name == name) {
      Serial.println("found field, returning field");
      return field;
    }
  }
  return Field();
}

String getFieldValue(String name, FieldList fields, uint8_t count) {
  Field field = getField(name, fields, count);
  if (field.getValue) {
    return field.getValue();
  }
  return String();
}

String setFieldValue(String name, String value, FieldList fields, uint8_t count) {
  Field field = getField(name, fields, count);
  Serial.println(field.name);
  if (field.setValue) {
    Serial.println("field " + field.name + " set value to " + value);
    field.setValue(value);
    broadcastString(field.name, value);
    return value;
  }
  return String();
}

String getFieldsJson(FieldList fields, uint8_t count) {
  
  String json = "[";

  for (uint8_t i = 0; i < count; i++) {
    Field field = fields[i];

    json += "{\"name\":\"" + field.name + "\",\"label\":\"" + field.label + "\",\"type\":\"" + field.type + "\"";

    if(field.getValue) {
      if (field.type == ColorFieldType || field.type == "String") {
        json += ",\"value\":\"" + field.getValue() + "\"";
      }
      else {
        json += ",\"value\":" + field.getValue();
      }
    }

    if (field.type == NumberFieldType) {
      json += ",\"min\":" + String(field.min);
      json += ",\"max\":" + String(field.max);
    }

    if (field.getOptions) {
      json += ",\"options\":[";
      json += field.getOptions();
      json += "]";
    }

    json += "}";

    if (i < count - 1)
      json += ",";
  }

  json += "]";

  return json;
}

String getFieldsJsonVec(std::vector<int> fields) {
  String json = "[";

  for (uint8_t i = 0; i < fields.size(); i++) {
    Field field = fieldParams[fields[i]];

    json += "{\"name\":\"" + field.name + "\",\"label\":\"" + field.label + "\",\"type\":\"" + field.type + "\"";

    if(field.getValue) {
      if (field.type == ColorFieldType || field.type == "String") {
        json += ",\"value\":\"" + field.getValue() + "\"";
      }
      else {
        json += ",\"value\":" + field.getValue();
      }
    }

    if (field.type == NumberFieldType) {
      json += ",\"min\":" + String(field.min);
      json += ",\"max\":" + String(field.max);
    }

    if (field.getOptions) {
      json += ",\"options\":[";
      json += field.getOptions();
      json += "]";
    }

    json += "}";

    if (i < fields.size() - 1)
      json += ",";
  }

  json += "]";

  return json;
}
