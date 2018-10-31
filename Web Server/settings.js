const server = require('./static_server.js')
const patterns = require('./patterns.js')
//artificial settings
exports.power = 1;
exports.brightnessMap = [16, 32, 64, 128, 255];
exports.brightnessIndex = 0;
exports.brightness = this.brightnessMap[this.brightnessIndex];
exports.currentPaletteIndex = 0;

exports.cooling = 49;
exports.sparking = 60;
exports.speed = 30;
exports.currentPatternIndex = 0;
exports.autoplay = 0;
exports.autoplayDuration = 10;
exports.twinkleSpeed = 4;
exports.twinkleDensity = 5;
exports.solidColor = {r: 128, g: 127, b: 126};


exports.setPower = function(value) {
    this.power = value == 0 ? 0 : 1;

    server.broadcastInt("power", this.power);
}

exports.setCooling = function(value) {
    this.cooling = value;

    server.broadcastInt("cooling", this.cooling);
}
exports.setSparking = function(value) {
    this.sparking = value;

    server.broadcastInt("sparking", this.sparking);
}
exports.setSpeed = function(value) {
    this.speed = value;

    server.broadcastInt("speed", this.speed);
}
exports.setTwinkleSpeed = function(value) {
    this.twinkleSpeed = value;
    if(this.twinkleSpeed < 0) this.twinkleSpeed = 0;  
    else if (this.twinkleSpeed > 8) this.twinkleSpeed = 8;

    server.broadcastInt("twinkleSpeed", this.twinkleSpeed);
}
exports.setTwinkleDensity = function(value) {
    this.twinkleDensity = value;
    if(this.twinkleDensity < 0) this.twinkleDensity = 0;
    else if (this.twinkleDensity > 8) this.twinkleDensity = 8;

    server.broadcastInt("twinkleDensity", this.twinkleDensity);
}

exports.setAutoplay = function(value) {
    this.autoplay = value == 0 ? 0 : 1;

    server.broadcastInt("autoplay", this.autoplay);
}

exports.setAutoplayDuration = function(value) {
    const hrTime = process.hrtime();
    let millis = hrTime[0] * 1000 + hrTime[1] / 1000000;
    this.autoplayDuration = value;
    this.autoPlayTimeout = millis + (this.autoplayDuration * 1000);

    server.broadcastInt("autoplayDuration", this.autoplayDuration);
}

exports.setSolidColor = function(color) {
    setSolidColor(color.r, color.g, color.b);
}

exports.setSolidColor = function(r, g, b) {
    this.solidColor.r = r;
    this.solidColor.g = g;
    this.solidColor.b = b;
    this.setPattern(patterns.patternCount - 1);

    server.broadcastString("color", String(this.solidColor.r) + "," + String(this.solidColor.g) + "," + String(this.solidColor.b));
}

// increase or decrease the current pattern number, and wrap around at the ends
exports.adjustPattern = function(up) {
    if (up)
        this.currentPatternIndex++;
    else
        this.currentPatternIndex--;

    if (this.currentPatternIndex < 0)
        this.currentPatternIndex = patterns.patternCount - 1;
    if (this.currentPatternIndex >= patterns.patternCount)
        this.currentPatternIndex = 0;

    server.broadcastInt("pattern", this.currentPatternIndex);
}

exports.setPattern = function(value) {
    if (value >= patterns.patternCount)
        value = patterns.patternCount - 1;
    
    this.currentPatternIndex = value;

    server.broadcastInt("pattern", this.currentPatternIndex);
}

exports.setPatternName = function(name) {
    for(i = 0; i < patterns.patternCount; i++) {
        if(patterns[i].name == name) {
            setPattern(i);
            break;
        }
    }
}

exports.setPalette = function(value) {
    if (value >= patterns.paletteCount)
        value = patterns.paletteCount - 1;

    this.currentPaletteIndex = value;

    server.broadcastInt("palette", this.currentPaletteIndex);
}

exports.setPaletteName = function(name) {
    for(i = 0; i < patterns.paletteCount; i++) {
        if(patterns.paletteNames[i] == name) {
            setPalette(i);
            break;
        }
    }
}

exports.adjustBrightness = function(up) {
    if (up && this.brightnessIndex < this.brightnessCount - 1)
        this.brightnessIndex++;
    else if (!up && this.brightnessIndex > 0)
        this.brightnessIndex--;

    this.brightness = this.brightnessMap[this.brightnessIndex];

    //FastLED.setBrightness(brightness);

    server.broadcastInt("brightness", this.brightness);
}

exports.setBrightness = function(value) {
    if (value > 255)
        value = 255;
    else if (value < 0) value = 0;

    this.brightness = value;

    //FastLED.setBrightness(brightness);

    server.broadcastInt("brightness", this.brightness);
}