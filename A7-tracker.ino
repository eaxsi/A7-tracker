// A7-tracker
// GPRS enabled GPS-tracker

// Eero Silfverberg 2018

#define DEBUG true
#include <ArduinoJson.h>

//Globals
//char host[17] = "status.ehasa.org";
char host[10] = "eero.tech";
int port = 8000;
int updateInterval = 30;
int nullCounter = 0;

char response[500];
char httpBuffer[100];
char nodeId[7];
char url[50];
char jsonChar[150];
StaticJsonBuffer<150> jsonBuffer;

boolean cleared = false;

long lastUpdate;

void setup()
{
  delay(4000);
  // init serial
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial.println(F("Hello!"));
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(8, OUTPUT);
  digitalWrite(5, HIGH);
  digitalWrite(4, LOW);
  digitalWrite(8, LOW);
  delay(2000);
  digitalWrite(8, HIGH);
  delay(3000);
  digitalWrite(8, LOW);
  Serial.println(F("A7 powered"));
  delay(1000);

  restartModule();
  //getIMEI();
  boolean imeiStauts = false;
  while (!imeiStauts) {
    imeiStauts = getIMEI();
  }

  boolean settingsStauts = false;
  //while(!settingsStauts){
  //  settingsStauts = getSettings();
  //}
}

void loop()
{
  delay(1000);
  Serial.println(F("Loop!"));

  if (millis() > (updateInterval * 1000 + lastUpdate)) {
    lastUpdate = millis();
    getGpsData();
    //getSettings();
  }
  // restart gps if needed
  if (nullCounter > 30) {
    Serial.println(F("Restarting gprs module..."));
    restartModule();
    nullCounter = 0;
  }

}

void startGPS() {
  sendAT(F("AT+GPS=1"), 10000, DEBUG);
  //sendAT(F("AT+AGPS=1"), 15000, DEBUG);
  Serial.println(F("GPS powered"));
}

void getGpsData() {
  sendAT(F("AT+GPSRD=1"), 1500, DEBUG);

  // parse one line at time
  char * current = response;
  char * gpggaPointer;
  char * value;
  jsonBuffer.clear();
  JsonObject& data  =   jsonBuffer.createObject();

  current = strtok(response, "\n");
  while (current != NULL) {
    current = strtok(NULL, "\n");
    // search for correct line
    if (strstr(current, "GPGGA")) {
      // search for the start of the message
      gpggaPointer = strstr(current, "$GPGGA");
      //Serial.println(gpggaPointer);

      if (!gpggaPointer) {
        //pointer is NULL
        Serial.println(F("gpggaPointer is NULL!"));
        nullCounter++;
      }
      // split to values
      value = strtok(gpggaPointer, ",");
      data["id"] = nodeId;

      int i = 0;
      while (value != NULL)
      {
        value = strtok (NULL, ",");
        if (i == 0) {
          //strcpy(gps_time, value);
        }
        if (i == 1) {
          data["lat"] = value;
        }
        if (i == 2) {
          data["lat_d"] = "N";
        }
        if (i == 3) {
          data["lon"] = value;
        }
        if (i == 4) {
          data["lon_d"] = value;
        }
        if (i == 7) {
          data["acc"] = value;
        }
        if (i == 8) {
          data["alt"] = value;
        }
        i++;
      }
    }
  }

  if (strlen(data["lat"]) > 5) {
    sendAT(F("AT+GPSRD=0"), 2500, DEBUG);
    Serial.println(F("Valid Data!"));

    // create JsonObject
    data["spd"] = "0";
    data.printTo(jsonChar, sizeof(jsonChar));
    Serial.println(jsonChar);

    strcpy(url, "\0");
    strcpy(url, "/save_location_json.php?id=");
    strcat(url, nodeId);
    httpRequest("POST", url, jsonChar);
  }
  else {
    Serial.println(F("not valid data"));
    Serial.println(response);
    sendAT(F("AT+GPSRD=0"), 2500, DEBUG);
  }

}


boolean getIMEI() {
  serialFlush();
  sendAT(F("AT+CGSN"), 5000, DEBUG);
  if (strstr(response, "OK")) {
    strncpy(nodeId, response + 9, 6);
    nodeId[7] = '\0';
    Serial.print(F("Node ID: "));
    Serial.println(nodeId);
    return true;
  }
  else {
    Serial.println(F("NodeID error!"));
    return false;
  }
  delay(1000);
}

void initModem() {
  sendAT(F("AT+CGATT=1"), 5000, DEBUG);
  //sendAT("AT+CGDCONT=1,\"IP\",\"cmnet\"",3000,true);
  sendAT(F("AT+CGACT=1,1"), 8000, DEBUG);
  //sendAT("AT+CSTT=\"internet\"", 1000, true);
  //sendAT("AT+CIICR", 5000, true);
  //sendAT("AT+CIPSTATUS", 5000, true);
}

void sendAT(String command, const int timeout, boolean debug)
{
  serialFlush();
  memset(response, 0, sizeof(response));
  strcpy(response, "\0");
  if (debug) Serial.println(command);
  //if (debug) Serial.println(strlen(response));
  int i = 0;
  int linecout = 0;

  Serial1.print(command);
  Serial1.println(F(""));
  Serial.flush();
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (Serial1.available()) {
      if (command == "" && !cleared) {
        Serial.println(F("Clearing serial buffer..."));
        for (int j = 0; j < 600; j++) {
          if (Serial1.available()) {
            char garbageChar = Serial1.read();
          }
        }
        cleared = true;
      }
      char currChar = Serial1.read();
      if (linecout >= 2) {
        // skip couple of lines from the response
        // A7 module echoes the sent commands back, we want only the response
        if (strlen(response) < sizeof(response) - 5) {
          response[i] = currChar;
          i++;
        }
        else {
          Serial.println(F("Response buffer overflow!"));
          Serial.println(strlen(response));
          i++;
          response[i] = '\0';
          return;
        }
      }
      if (currChar == '\r' || currChar == '\n') {
        linecout++;
      }
    }
  }
  i++;
  response[i] = '\0'; // end response correctly

  if (debug) Serial.println(response);
}

void serialFlush() {
  while (Serial1.available() > 0) {
    char t = Serial1.read();
  }
  Serial1.flush();
  Serial.flush();
}

void waitUntilConnected() {
  while (!strstr(response, "+CREG: 1,1\r")) {
    Serial.println(F("Searching for network..."));
    sendAT(F("AT+CREG?"), 1000, DEBUG);
    delay(1000);
  }
  Serial.println(F("Registered to network!"));
  sendAT(F("AT+CSQ"), 1000, DEBUG);
}

void restartModule() {
  sendAT(F("AT+RST=1"), 15000, DEBUG);
  // turn character echo off
  sendAT(F("ATE0"), 1000, DEBUG);

  waitUntilConnected();
  initModem();
  startGPS();
}

boolean getSettings() {

  // TODO use httpRequest here
  // 
  Serial.println(F("Getting settings..."));

  strcpy(url, "/gps_settings_json.php?id=");
  strcat(url, nodeId);

  httpRequest("GET", url, "");
  if (strlen(response) < 40) {
    return false;
  }
  Serial.print(F("Into parser: "));
  //Serial.println(response);

  //strcpy(strstr(httpBuffer, "}"), "}}");
  //strcat(httpBuffer, "\0");

  strncpy(strstr(response, "}"), "}}", 3);

  Serial.println(response);
  JsonObject& root = jsonBuffer.parseObject(strstr(response, "{"));
  JsonObject& node_settings = root["node_settings"];
  if (!root.success()) {
    Serial.println(F("Root parser fail!"));
    return false;
  }
  updateInterval = atol(node_settings["updateInterval"]);

  Serial.print(F("Update interval:"));
  Serial.println(updateInterval);
  return true;
}

int httpRequest(char method[5], char path[50], char data[100]) {
  Serial1.print(F("at+cipstart=\"TCP\",\""));
  Serial1.print(host);
  Serial1.print(F("\","));
  Serial1.print(port);
  sendAT(F(""), 8000, DEBUG);

  if (strstr(response, "ERROR")) {
    nullCounter++;
  }

  Serial.println(path);
  sendAT(F("AT+CIPSEND"), 3000, DEBUG);
  // GET or POST
  Serial1.print(method);
  Serial1.print(F(" "));
  Serial1.print(path);
  Serial1.print(F(" HTTP/1.0\r\n"));
  Serial1.print(F("Host: "));
  Serial1.print(host);
  Serial1.print(F("\r\n"));
  Serial1.print(F("Connection: close \r\n"));
  Serial1.flush();

  if (method == "POST") {
    Serial1.print(F("Content-Type: application/x-www-form-urlencoded\r\n"));
    Serial1.print(F("Content-Length: "));
    Serial1.print(strlen(data) + 10);
    Serial1.print(F("\r\n"));
    Serial1.print(F("\r\n"));
    Serial1.print(F("gps_event="));
    Serial1.print(data);
    //serialFlush();
    cleared = false;
  }
  else {
    cleared = false;
  }
  Serial1.print(F("\r\n"));
  Serial1.write(0x1A);
  serialFlush();
  sendAT("", 6500, DEBUG);
  Serial.println(response);
  if (strstr(response, "200 OK")) {
    // request is successfull
    Serial.println(F("HTTP request successfull"));
    // parse response

    int i = 0;
    char * current = strstr(response  , "\r\n");
    memset(httpBuffer, 0, sizeof(httpBuffer));
    strcpy(httpBuffer, "\0");


    current = strtok(current, "\n");
    while (current != NULL) {
      current = strtok(NULL, "\n");

      if (i >= 1) {
        if (strlen(httpBuffer) < sizeof(httpBuffer) - 5) {
          strcat(httpBuffer, current);
        }
      }
      if (strlen(current) == 1) {
        i++;
      }
    }
    Serial.print(F("HTTP response: "));
    Serial.println(httpBuffer);
    memset(response, 0, sizeof(response));
    strcpy(response, httpBuffer);
    //memset(httpBuffer, 0, sizeof(httpBuffer));
    //sendAT(F("AT+CIPCLOSE"), 3000, DEBUG);
    Serial1.print(F("AT+CIPCLOSE"));
    Serial1.println(F(""));
    Serial.flush();
    delay(2000);
    Serial1.print(F("AT+CIPSHUT"));
    Serial1.println(F(""));
    delay(1000);

  }
  else {
    nullCounter++;
  }
}
