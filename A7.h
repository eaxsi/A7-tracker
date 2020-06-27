// A7.h

#ifndef A7_h
#define A7_h

#include "Arduino.h"
#include <avr/wdt.h>
#include <ArduinoJson.h>

#include "config.h"

#define EMPTY_LINE "\r\n"

class AiA7
{

public:
  AiA7(bool enabled) : wdt_enabled{enabled} { wdt_enable(WDTO_8S); };

  struct node_settings
  {
    unsigned int loc_interval = 0;
    unsigned int set_interval = 0;
    bool success = false;
  };

  enum http_method
  {
    GET,
    POST
  };

  void nb_delay(uint32_t time);

  void turn_on();
  void restart();

  void waitUntilConnected();
  void initModem();
  void startGPS();
  void stopGPS();
  bool getGpsData();
  void startGPRS();
  void stopGPRS();
  boolean produceNodeId();
  bool communicates();
  bool is_connected();
  bool httpRequest(http_method, char *path, JsonObject &data);
  node_settings get_settings();

  byte get_signal_strength();
  byte get_battery_level();
  char *get_nodeId();

  void update_signal_strength();
  void update_battery_level();

  enum device_state
  {
    IDLE,
    GET_SETTINGS,
    UPDATE_POSITION,
    CHECK_CONNECTION,
    RESTART
  };

private:
  void sendAT(String command, const int timeout, boolean debug);
  void sendAT(String command, const int timeout, boolean debug, boolean wait_for_response);
  void serialFlush();

  char nodeId[7] = "";
  bool wdt_enabled = false;
  boolean cleared = false;

  char response[500];
  StaticJsonBuffer<220> jsonBuffer;

  node_settings settings;

  char url[50];

  int nullCounter = 0;
  byte signalStrength = 0;
  byte batteryLevel = 0;
};
#endif