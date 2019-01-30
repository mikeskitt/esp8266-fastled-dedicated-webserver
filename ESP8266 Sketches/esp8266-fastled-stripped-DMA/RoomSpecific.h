//253 living, 252 kitchen, 251 pong, 250 foosball room, 249 party room, 248 stairs
IPAddress ip(192, 168, 1, 248); // where last octet is the desired IP Address

ZoneList zones = {
  {0, 2, "Left"},
  {3, 10, "Middle"},
  {11, 20, "Right"}
};

#define NUM_LEDS      649
#define MILLI_AMPS    40000     // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
