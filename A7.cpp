// A7.cpp
#include "A7.h"
#include <avr/wdt.h>

void AiA7::turn_on()
{
  while (true)
  {
    // turn off
    digitalWrite(4, HIGH);
    nb_delay(3000);

    // turn on
    digitalWrite(5, HIGH);
    digitalWrite(4, LOW);
    digitalWrite(8, LOW);
    nb_delay(2000);
    digitalWrite(8, HIGH);
    nb_delay(3000);
    digitalWrite(8, LOW);
    Serial.println("Waiting for module to start up!");
    nb_delay(2000);
    if (communicates())
      break;
  }
}

void AiA7::nb_delay(uint32_t delay_time)
{
  uint32_t timestamp = millis();
  while ((timestamp + delay_time) > millis())
  {
    wdt_reset();
    delay(10);
  }
}


void AiA7::waitUntilConnected()
{
  while (!is_connected())
  {
    Serial.println(F("Searching for network..."));
    nb_delay(500);
  }
  Serial.println(F("Registered to network!"));
  sendAT(F("AT+CSQ"), 1000, DEBUG);
}

bool AiA7::is_connected()
{
  sendAT(F("AT+CREG?"), 1000, DEBUG);
  return strstr(response, "+CREG: 1,1\r");
}

bool AiA7::communicates()
{
  sendAT(F("AT"), 1500, DEBUG);
  return strstr(response, "OK");
}

void AiA7::initModem()
{
  // turn module command echo off
  sendAT(F("ATE0"), 1000, DEBUG);
  startGPS();
  waitUntilConnected();
  

  sendAT(F("AT+CGATT=1"), 10000, DEBUG); // GPRS attach
  startGPRS();

  //sendAT(F("AT+CGATT=1"), 5000, DEBUG);
  //sendAT("AT+CGDCONT=1,\"IP\", \"internet\"", 2000, true);
  //sendAT("AT+CGDCONT=1,\"IP\",\"cmnet\"",3000,true);
  //sendAT(F("AT+CGACT=1,1"), 8000, DEBUG);
  //sendAT("AT+CSTT=\"internet\"", 1000, true);
  //sendAT("AT+CIICR", 5000, true);
  //sendAT("AT+CIPSTATUS", 5000, true);
}

void AiA7::startGPS()
{
  sendAT(F("AT+GPS=1"), 10000, DEBUG);
  Serial.println(F("GPS powered"));
}

void AiA7::stopGPS()
{
  sendAT(F("AT+GPS=0"), 10000, DEBUG);
  Serial.println(F("GPS powered down"));
}

void AiA7::restart()
{
  sendAT(F("AT+RST=1"), 15000, DEBUG);
  while (!communicates())
  {
    nb_delay(100);
  }
}

boolean AiA7::produceNodeId()
{
  serialFlush();
  sendAT(F("AT+CGSN"), 5000, DEBUG);
  if (strstr(response, "OK"))
  {
    strncpy(nodeId, response + 9, 6);
    nodeId[7] = '\0';
    return true;
  }
  else
  {
    Serial.println(F("NodeID error!"));
    return false;
  }
  nb_delay(1000);
}

void AiA7::update_signal_strength()
{
  signalStrength = get_signal_strength();
}

byte AiA7::get_signal_strength()
{
  sendAT(F("AT+CSQ"), 2000, DEBUG);
  byte rssi;

  char *value;
  char *csqPointer = strstr(response, "+CSQ:");
  value = strtok(csqPointer, " ,");

  for (int i = 0; value; i++)
  {
    value = strtok(NULL, ",");
    switch (i)
    {
    case 0:
      rssi = atoi(value);
      break;
    }
  }
  return rssi;
}

void AiA7::update_battery_level()
{
  batteryLevel = get_battery_level();
}

byte AiA7::get_battery_level()
{
  sendAT(F("AT+CBC?"), 2000, DEBUG);
  byte level;

  char *value;
  char *cbcPointer = strstr(response, "+CBC:");
  value = strtok(cbcPointer, " ,");
  for (int i = 0; value; i++)
  {
    value = strtok(NULL, ",");
    switch (i)
    {
    case 1:
      level = atoi(value);
      break;
    }
  }
  return level;
}

void AiA7::serialFlush()
{
  while (Serial1.available() > 0)
  {
    char t = Serial1.read();
  }
  Serial1.flush();
  Serial.flush();
}
void AiA7::sendAT(String command, const int timeout, boolean debug)
{
  sendAT(command, timeout, debug, false);
}

void AiA7::sendAT(String command, const int timeout, boolean debug, boolean wait_for_response)
{
  long int timestamp;
  timestamp = millis();
  serialFlush();
  memset(response, 0, sizeof(response));
  strcpy(response, "\0");
  if (debug)
    Serial.println(command);

  // clear serial buffer
  while (millis() < timestamp + 100)
  {
    if (Serial1.available())
    {
      char garbageChar = Serial1.read();
    }
    else
      break;
    nb_delay(10);
  }
  // send AT-command
  Serial1.print(command);
  Serial1.println(F(""));
  Serial.flush();

  // wait until empty line is received
  while (millis() < timestamp + timeout)
  {
    if (Serial1.find(EMPTY_LINE))
      break;
    nb_delay(20);
  }

  //Serial.println("Escape from waiting loop");

  // read response
  char linebuffer[100];
  // read one line at a time
  while (Serial1.available() || millis() < (timestamp + timeout + 500))
  {
    memset(linebuffer, 0, sizeof(linebuffer));
    strcpy(linebuffer, "\0");
    Serial1.readBytesUntil('\n', linebuffer, sizeof(linebuffer) - 2);

    if (strlen(linebuffer) > 1)
    {
      strcat(linebuffer, "\n");
      if (strlen(response) + 5 + strlen(linebuffer) < sizeof(response))
      {
        strcat(response, linebuffer);
      }
      else
      {
        Serial.println(F("Response buffer full!"));
        break;
      }
    }
    // response ends when linebuffers first char is 0 and response contains ok or error
    if (linebuffer[0] == '\0' && (strstr(response, "OK") || strstr(response, "ERROR")))
      break;

    wdt_reset();
  }
  //Serial.println(strlen(response));
  Serial.println(response);
  
  //nb_delay(10);
}

bool AiA7::httpRequest(AiA7::http_method method, char * path, JsonObject &data)
{
  Serial1.print(F("at+cipstart=\"TCP\",\""));
  Serial1.print(SERVER_HOST);
  Serial1.print(F("\","));
  Serial1.print(SERVER_PORT);
  sendAT(F(""), 8000, DEBUG);

  if (strstr(response, "ERROR"))
  {
    nullCounter++;
  }

  Serial.println(path);
  sendAT(F("AT+CIPSEND"), 3000, DEBUG);
  // GET or POST
  switch (method)
  {
  case AiA7::http_method::GET:
    Serial1.print(F("GET"));
    break;
  case AiA7::http_method::POST:
    Serial1.print(F("POST"));
    break;
  }
  Serial1.print(F(" "));
  Serial1.print(path);
  Serial1.print(F(" HTTP/1.0\r\n"));
  Serial1.print(F("Host: "));
  Serial1.print(SERVER_HOST);
  Serial1.print(F("\r\n"));
  Serial1.print(F("Connection: close \r\n"));
  Serial1.flush();

  if (method == AiA7::http_method::POST)
  {
    Serial1.print(F("Content-Type: application/x-www-form-urlencoded\r\n"));
    Serial1.print(F("Content-Length: "));
    Serial1.print(data.measureLength() + 10);
    Serial1.print(F("\r\n"));
    Serial1.print(F("\r\n"));
    Serial1.print(F("gps_event="));
    data.printTo(Serial1);
    //Serial1.print(data);
    //serialFlush();
    cleared = false;
  }
  else
  {
    cleared = false;
  }
  Serial1.print(EMPTY_LINE);
  Serial1.write(0x1A);
  serialFlush();
  sendAT("", 7000, DEBUG);
  //Serial.println(response);
  if (strstr(response, "200 OK"))
  {
    // request is successfull
    Serial.println(F("HTTP request successfull"));
    // parse response

    int i = 0;
    char httpBuffer[100];
    char *current = strstr(strstr(response, "Connection: "), "\n") + 1;

    strcpy(httpBuffer, "\0");
    strncat(httpBuffer, current, sizeof(httpBuffer));

    Serial.print(F("HTTP response: "));
    Serial.println(httpBuffer);
    //memset(response, 0, sizeof(response));
    //strcpy(response, httpBuffer);

    sendAT(F("AT+CIPCLOSE"), 2000, DEBUG);
    sendAT(F("AT+CIPSHUT"), 2000, DEBUG);
    strcpy(response, httpBuffer);
    return true;
  }
  else
  {
    nullCounter++;
    return false;
  }
}

bool AiA7::getGpsData()
{
  sendAT(F("AT+GPSRD=1"), 1500, DEBUG);

  //check response length
  if (strlen(response) < 10)
  {
    Serial.println(F("GPS message too short!"));
    nullCounter++;
  }

  // parse one line at time
  char *current = response;
  char *gpggaPointer;
  char *value;
  jsonBuffer.clear();
  JsonObject &data = jsonBuffer.createObject();

  data[F("id")] = get_nodeId();
  data[F("fix")] = 0;
  data[F("uptime")] = millis();
  data[F("signal")] = signalStrength;
  data[F("batt")] = batteryLevel;

  // find $GPGGA line
  // strtok until EOL
  // check that it has enough items
  // set data to JSON object

  gpggaPointer = strstr(response, "$GPGGA");

  value = strtok(gpggaPointer, ",");
  for (int i = 0; i < 10; i++)
  {
    value = strtok(NULL, ",\r");

    if (i == 1 && strlen(value) < 5)
    {
      data[F("fix")] = 0; // check that data gps-data is valid
      break;
    }
    else
      data[F("fix")] = 1;

    switch (i)
    {
    case 1:
      data[F("lat")] = value;
      break;
    case 2:
      data[F("lat_d")] = value;
      break;
    case 3:
      data[F("lon")] = value;
      break;
    case 4:
      data[F("lon_d")] = value;
      break;
    case 6:
      data[F("nSat")] = atoi(value);
      break;
    case 7:
      data[F("acc")] = atof(value) * 7;
      break;
      //case 8: data[F("alt")] = value; break;
      //case 13:data[F("head")] = value; break;
    }
  }
  sendAT(F("AT+GPSRD=0"), 2500, DEBUG);

  data.printTo(Serial);

  strcpy(url, "\0");
  strcpy(url, POSTION_PATH);
  strcat(url, get_nodeId());

  bool ret = httpRequest(AiA7::http_method::POST, url, data);
  
  return data[F("fix")] == 1 && ret;
}

char *AiA7::get_nodeId()
{
  return nodeId;
}

AiA7::node_settings AiA7::get_settings()
{
  jsonBuffer.clear();
  JsonObject &dummy_obj = jsonBuffer.createObject();

  // send request to server
  strcpy(url, SETTINGS_PATH);
  strcat(url, get_nodeId());
  httpRequest(AiA7::http_method::GET, url, dummy_obj);

  // process response
  jsonBuffer.clear();
  JsonObject &json_settings = jsonBuffer.parseObject(strstr(response, "{"));

  if (json_settings.success())
  {
    Serial.println(F("JSON Parser success!"));
    json_settings.printTo(Serial);

    settings.loc_interval = json_settings["loc_interval"];
    settings.set_interval = json_settings["set_interval"];
    settings.success = true;
  }
  else
  {
    settings.success = false;
    Serial.println(F("Root parser fail!"));
  }
  return settings;
}

void AiA7::startGPRS()
{
  //sendAT(F("AT+CGATT=1"), 10000, DEBUG); // GPRS attach
  sendAT(F("AT+CGACT=1,1"), 10000, DEBUG); // Activate PDP context
}

void AiA7::stopGPRS()
{
  sendAT(F("AT+CGACT=0,1"), 5000, DEBUG);
  //sendAT(F("AT+CGATT=0"), 8000, DEBUG);
}