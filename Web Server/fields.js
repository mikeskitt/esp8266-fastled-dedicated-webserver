let patterns = require('./patterns.js');
let settings = require('./settings.js')

exports.NumberFieldType = "Number";
exports.BooleanFieldType = "Boolean";
exports.SelectFieldType = "Select";
exports.ColorFieldType = "Color";
exports.SectionFieldType = "Section";

function getPower() {
  return String(settings.power);
}

function getBrightness() {
  return String(settings.brightness);
}

function getPattern() {
  return String(settings.currentPatternIndex);
}

function getPatterns() {
  let json = "";

  for (let i = 0; i < patterns.patternCount; i++) {
    json += "\"" + patterns.patterns[i].name + "\"";
    if (i < patterns.patternCount - 1)
      json += ",";
  }

  return json;
}

function getPalette() {
  return String(settings.currentPaletteIndex);
}

function getPalettes() {
  let json = "";

  for (let i = 0; i < patterns.paletteCount; i++) {
    json += "\"" + patterns.paletteNames[i] + "\"";
    if (i < patterns.paletteCount - 1)
      json += ",";
  }

  return json;
}

function getAutoplay() {
  return String(settings.autoplay);
}

function getAutoplayDuration() {
  return String(settings.autoplayDuration);
}

//not gunna werk
function getSolidColor() {
  return String(settings.solidColor.r) + "," + String(settings.solidColor.g) + "," + String(settings.solidColor.b);
}

function getCooling() {
  return String(settings.cooling);
}

function getSparking() {
  return String(settings.sparking);
}

function getSpeed() {
  return settings.speed;
}

function getTwinkleSpeed() {
  return String(settings.twinkleSpeed);
}

function getTwinkleDensity() {
  return String(settings.twinkleDensity);
}
//name, label, type, min, max, getter
exports.fields = [
    { name:"power",           label:"Power",      type:this.BooleanFieldType, min:0, max:1, getValue:getPower },
    { name:"brightness",      label:"Brightness", type:this.NumberFieldType,  min:1, max:255, getValue:getBrightness },
    { name:"pattern",         label:"Pattern",    type:this.SelectFieldType,  min:0, max:patterns.patternCount, getValue:getPattern, getOptions:getPatterns },
    { name:"palette",         label:"Palette",    type:this.SelectFieldType,  min:0, max:patterns.paletteCount, getValue:getPalette, getOptions:getPalettes },
    { name:"speed",           label:"Speed",      type:this.NumberFieldType,  min:1, max:255, getValue:getSpeed },
    { name:"autoplay",        label:"Autoplay",   type:this.SectionFieldType },
    { name:"autoplay",        label:"Autoplay",   type:this.BooleanFieldType, min:0, max:1, getValue:getAutoplay },
    { name:"autoplayDuration",label:"Autoplay Duration", type:this.NumberFieldType, min:0, max:255, getValue:getAutoplayDuration },
    { name:"solidColor",      label:"Solid Color",type:this.SectionFieldType },
    { name:"solidColor",      label:"Color",      type:this.ColorFieldType,   min:0, max:255, getValue:getSolidColor },
    { name:"fire",            label:"Fire & Water", type:this.SectionFieldType },
    { name:"cooling",         label:"Cooling",    type:this.NumberFieldType,  min:0, max:255, getValue:getCooling },
    { name:"sparking",        label:"Sparking",   type:this.NumberFieldType,  min:0, max:255, getValue:getSparking },
    { name:"twinkles",        label:"Twinkles",   type:this.SectionFieldType },
    { name:"twinkleSpeed",    label:"Twinkle Speed", type:this.NumberFieldType, min:0, max:8, getValue:getTwinkleSpeed },
    { name:"twinkleDensity",  label:"Twinkle Density", type:this.NumberFieldType, min:0, max:8, getValue:getTwinkleDensity }
];
exports.fieldsCount = this.fields.length;