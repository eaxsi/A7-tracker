// A7-tracker
// GPRS enabled GPS-tracker

// Eero Silfverberg 2018

#include <ArduinoJson.h>

#include "A7.h"
#include "config.h"

AiA7 modem(true); // enable WDT

//Globals
byte nullCounter = 0;

unsigned int update_interval = 20;
unsigned int connection_interval = 60;
unsigned int settings_interval = 60;

long position_timestamp;
long connection_timestamp;
long settings_timestamp;

AiA7::device_state state = AiA7::device_state::IDLE;
AiA7::node_settings set;

void setup()
{
  // init serial
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial.println(F("Hello!"));
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(6, OUTPUT);

  if (modem.communicates())
  {
    Serial.println(F("Modem soft-restart!"));
    modem.restart();
  }
  else
  {
    Serial.println(F("Modem hard-restart!"));
    modem.turn_on();
  }
  Serial.println(F("Modem initialized"));
  modem.initModem();

  while (!modem.produceNodeId()) // wait until we have a nodeID
    ;

  Serial.print("Node ID:");
  Serial.println(modem.get_nodeId());

  modem.update_signal_strength();
  modem.update_battery_level();
}

void loop()
{
  switch (state)
  {
  case AiA7::device_state::CHECK_CONNECTION:
    Serial.print(F("Checking connection: "));
    //Serial.println(F("Checking connection"));
    connection_timestamp = millis();
    if (modem.communicates())
    {
      if (modem.is_connected())
      {
        Serial.println(F("success!"));
        connection_timestamp = millis();
      }
      else
      {
        Serial.println(F("no connection"));
        nullCounter++;
      }
    }
    else
    {
      Serial.println(F("not responding"));
      nullCounter = nullCounter + 30;
    }

    modem.update_signal_strength();
    modem.update_battery_level();
    break;

  case AiA7::device_state::UPDATE_POSITION:
    Serial.print(F("Updating position: "));
    modem.startGPS();
    modem.startGPRS();
    if (modem.getGpsData())
    {
      Serial.println(F("success!"));
      position_timestamp = millis();
      if (nullCounter > 0)
        nullCounter--;
      modem.stopGPS();
    }
    else
    {
      Serial.println(F("fail!"));
      nullCounter++;
    }

    modem.stopGPRS();
    break;

  case AiA7::device_state::GET_SETTINGS:
    Serial.print(F("Getting settings: "));
    modem.startGPRS();

    set = modem.get_settings();
    if (set.success)
    {
      Serial.println(F("success!"));
      update_interval = set.loc_interval;
      settings_interval = set.set_interval;
      settings_timestamp = millis();
    }
    else
    {
      Serial.println(F("fail!"));
      //nullCounter++;
    }
    modem.stopGPRS();
    settings_timestamp = millis();
    break;

  case AiA7::device_state::RESTART:
    Serial.println(F("Restarting the modem"));
    nullCounter = 0;
    modem.turn_on();
    modem.restart();
    modem.initModem();
    break;
  case AiA7::device_state::IDLE:
    modem.nb_delay(10);
    break;

  default:
    break;
  }

  if (nullCounter > 20)
  {
    state = AiA7::device_state::RESTART;
  }
  else if (millis() > connection_timestamp + (long)connection_interval * 1000)
    state = AiA7::device_state::CHECK_CONNECTION;

  else if (millis() > settings_timestamp + (long)settings_interval * 1000)
    state = AiA7::device_state::GET_SETTINGS;

  else if (millis() > position_timestamp + (long)update_interval * 1000)
    state = AiA7::device_state::UPDATE_POSITION;

  else
    state = AiA7::device_state::IDLE;

  modem.nb_delay(500);
}
