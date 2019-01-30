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

typedef String (*FieldSetter)(String);
typedef String (*FieldGetter)();

const String NumberFieldType = "Number";
const String BooleanFieldType = "Boolean";
const String SelectFieldType = "Select";
const String SelectHeaderFieldType = "SelectHeader";
const String SelectButtonFieldType = "SelectButton";
const String ColorFieldType = "Color";
const String SectionFieldType = "Section";

typedef struct Field {
  String name;
  String label;
  String type;
  uint8_t min;
  uint8_t max;
  FieldGetter getValue;
  FieldGetter getOptions;
  FieldSetter setValue;
};

typedef Field FieldList[];

FieldList fieldParams = {
  { "palette", "Palette", SelectFieldType, 0, paletteCount, getPalette, getPalettes },  //0
  { "speed", "Speed", NumberFieldType, 1, 255, getSpeed },                              //1
  { "solidColor", "Color", ColorFieldType, 0, 255, getSolidColor },                     //2
  { "cooling", "Cooling", NumberFieldType, 0, 255, getCooling },                        //3
  { "sparking", "Sparking", NumberFieldType, 0, 255, getSparking },                     //4
  { "twinkleSpeed", "Twinkle Speed", NumberFieldType, 0, 8, getTwinkleSpeed },          //5
  { "twinkleDensity", "Twinkle Density", NumberFieldType, 0, 8, getTwinkleDensity }     //6
//  { "satSpeed", "Sat Speed", NumberFieldType, 1, 255, getSatSpeed }     //7
//  { "satSpeed", "Sat Speed", NumberFieldType, 1, 255, getSatSpeed }     //7
};

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

Field getField(String name, FieldList fields, uint8_t count) {
  Serial.println("get field");
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
  Serial.println("get field value");
  Field field = getField(name, fields, count);
  if (field.getValue) {
    Serial.println("field can get value, returning it");
    return field.getValue();
  }
  return String();
}

String setFieldValue(String name, String value, FieldList fields, uint8_t count) {
  Serial.println("set field value");
  Field field = getField(name, fields, count);
  Serial.println(field.name);
  if (field.setValue) {
    Serial.println("field can set value, returning it");
    return field.setValue(value);
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
