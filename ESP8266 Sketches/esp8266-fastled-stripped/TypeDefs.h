typedef struct {
  char* name;
  char* password;
} AP;
typedef AP APList[];

typedef struct {
  int start;
  int end;
  String name;
} Zone;
typedef Zone ZoneList[];

typedef void (*Pattern)();
typedef Pattern PatternList[];
typedef struct {
  Pattern pattern;
  String name;
  std::vector<int> params;
} PatternAndName;
typedef PatternAndName PatternAndNameList[];


//template functions to compile
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
void showTwinkles();
void broadcastInt(String, uint8_t);
void broadcastString(String, String);
