// A7-tracker
// GPRS enabled GPS-tracker

// Eero Silfverberg 2018

#define DEBUG true

#include <ArduinoJson.h>

//Globals
char host[17] = "status.ehasa.org";
//char host[10] = "eero.tech";
int port = 80;
//int errorcounter = 0;
int updateInterval = 25;


char response[550];
char httpBuffer[100];
char nodeId[7];
char url[50];
char jsonChar[200];
StaticJsonBuffer<200> jsonBuffer;
JsonObject& data  =   jsonBuffer.createObject();

char gps_time[11];
char lat[11];
char lat_d[3];
char lon[11];
char lon_d[3];
char alt[6] = "0";
char spd[6] = "1";
char acc[6] = "1000";


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
	pinMode(8,OUTPUT);
	digitalWrite(5, HIGH); 
	digitalWrite(4, LOW); 
	digitalWrite(8, LOW); 
	delay(2000);
	digitalWrite(8, HIGH); 
	delay(3000);       
	digitalWrite(8, LOW);
	Serial.println(F("A7 powered"));
  delay(1000);

  //getIMEI();
  boolean imeiStauts = false;
  while(!imeiStauts){
    imeiStauts = getIMEI();
  }

  waitUntilConnected();
  //initModem();
  startGPS();
  
  
  boolean settingsStauts = false;
  //while(!settingsStauts){
  //  settingsStauts = getSettings();
  //}
}

void loop()
{
  delay(1000);
  Serial.println(F("Loop!"));

  if(millis() > updateInterval*1000 + lastUpdate) {
    lastUpdate = millis();
    getGpsData();
    getSettings();
  }
}


void startGPS() {
  sendAT(F("AT+GPS=1"), 10000, DEBUG);
  //sendAT(F("AT+AGPS=1"), 15000, DEBUG);
  Serial.println(F("GPS powered"));
}

void getGpsData() {
  sendAT(F("AT+GPSRD=1"), 1100, DEBUG);

  // parse one line at time
  char * current = response;
  char * gpggaPointer;
  char * value;
  current = strtok(response, "\n");
  while(current != NULL) {
    current = strtok(NULL, "\n");
    if (strstr(current, "GPGGA")) {
      // correct line
      // search for the start of the message
      gpggaPointer = strstr(current, "$GPGGA");
      // split to values
      value = strtok (gpggaPointer,",");

      int i = 0;
      while (value != NULL)
      {
        value = strtok (NULL, ",");
        if (i == 0){
          strcpy(gps_time, value);
        }
        if (i == 1) {
          strcpy(lat, value);
        }
        if (i == 2) {
          strcpy(lat_d, value);
        }
        if (i == 3) {
          strcpy(lon, value);
        }
        if (i == 4) {
          strcpy(lon_d, value);
        }
        if (i == 7) {
          strcpy(acc, value);
        }
         if (i == 8) {
          strcpy(alt, value);
        }
        i++;
      }
    }
  }

  if(strlen(lat)> 5){
    sendAT(F("AT+GPSRD=0"), 2500, DEBUG);
    Serial.println(F("Valid Data!"));
    // create JsonObject

    data["id"] = nodeId;
    data["lat"] = lat;
    data["lat_d"] = "N";
    data["lon"] = lon;
    data["lon_d"] = lon_d;
    data["acc"] = acc;
    data["alt"] = alt;
    data["spd"] = "0";

    data.printTo(jsonChar, sizeof(jsonChar));
    data.printTo(Serial);
    Serial.println(jsonChar);

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


boolean getIMEI(){
  sendAT(F("AT+CGSN"), 5000, DEBUG);
  if (strstr(response, "OK")) {
    strncpy(nodeId, response+9, 6);
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

void initModem(){
  sendAT(F("AT+CGATT=1"), 5000, DEBUG);
  //sendAT("AT+CGDCONT=1,\"IP\",\"cmnet\"",3000,true);
  sendAT(F("AT+CGACT=1,1"),8000, DEBUG);
  //sendAT("AT+CSTT=\"internet\"", 1000, true);
  //sendAT("AT+CIICR", 5000, true);
  //sendAT("AT+CIPSTATUS", 5000, true);
}

void waitUntilConnected(){
  while (!strstr(response, "+CREG: 1,1\r")){
    Serial.println(F("Searching for network..."));
    sendAT(F("AT+CREG?"), 1000, DEBUG);
    delay(1000);
  }
  Serial.println(F("Registered to network!"));
  sendAT(F("AT+CSQ"), 1000, DEBUG);
}

boolean getSettings(){
  Serial.println(F("Getting settings..."));

  
  strcpy(url, "/gps_settings_json.php?id=");
  strcat(url, nodeId);

  httpRequest("GET", url, "");
  //Serial.print(F("httpRequest returned: "));
  //Serial.println(response);

  strcat(response, "}");
  JsonObject& root = jsonBuffer.parseObject(strstr(response, "{"));
  JsonObject& node_settings = root["node_settings"];
  if(!root.success()){
    Serial.println(F("Root parser fail!"));
    return false;
  }
  updateInterval = atol(node_settings["updateInterval"]);

  Serial.print(F("Update interval:"));
  Serial.println(updateInterval);
  return true;
}

int httpRequest(char method[5], char path[50], char data[100]){
  Serial1.print(F("at+cipstart=\"TCP\",\""));
  Serial1.print(host);
  Serial1.print(F("\","));
  Serial1.print(port);
  sendAT("", 8000, DEBUG);

  Serial.println(path);
  sendAT("AT+CIPSEND", 3000, DEBUG);
  // GET or POST
  Serial1.print(method);
  Serial1.print(F(" "));
  Serial1.print(path);
  Serial1.print(F(" HTTP/1.0\r\n"));
  Serial1.print(F("Host: "));
  Serial1.print(host);
  Serial1.print(F("\r\n"));
  //Serial1.print(F("Connection: close \r\n"));
  if(method == "POST") {
    Serial1.print(F("Content-Type: application/x-www-form-urlencoded\r\n"));
    Serial1.print(F("Content-Length: "));
    Serial1.print(strlen(data) + 10);
    Serial1.print(F("\r\n"));
    Serial1.print(F("\r\n"));
    Serial1.print(F("gps_event="));
    Serial1.print(data);
  }
  Serial1.print(F("\r\n"));
  Serial1.write(0x1A);
  sendAT("", 6000, DEBUG);
  //Serial.println(response);
  if (strstr(response, "200 OK")) {
    // request is successfull
    Serial.println(F("HTTP request successfull"));
    // parse response
    
    int i = 0;
    char * current = response;
    memset(httpBuffer, 0, sizeof(httpBuffer));
    strcpy(httpBuffer, "\0");

    current = strtok(response, "\n");
    while(current != NULL) {
      current = strtok(NULL, "\n");
      //Serial.print("Current");
      //Serial.println(strlen(current));
      //Serial.println(current);

      if (i >= 3) {
        strcat(httpBuffer, current);
      }
      if (strlen(current) == 1) {
        i++;
      }
    }
    Serial.print(F("HTTP response: "));
    Serial.println(httpBuffer);
    memset(response, 0, sizeof(response));
    strncpy(response, httpBuffer, strlen(httpBuffer));
    memset(httpBuffer, 0, sizeof(httpBuffer));
    Serial1.print(F("AT+CIPCLOSE"));
    delay(1000);
    serialFlush();

  }

}

void sendAT(String command, const int timeout, boolean debug)
{
  //serialFlush();
  memset(response, 0, sizeof(response));
  strcpy(response, "\0");
  if (debug) Serial.println(command);
  //if (debug) Serial.println(strlen(response));
  int i = 0;
  int linecout = 0;
  Serial1.print(command);
  Serial1.println(F(""));
  long int time = millis();   
  while((time+timeout) > millis()){
    while(Serial1.available()){
      char currChar = Serial1.read();
      if (linecout >= 4){
        // skip couple of lines from the response
        // A7 module echoes the sent commands back, we want only the response
        if (strlen(response) < sizeof(response)-5) {
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

void serialFlush(){
  while(Serial1.available() > 0) {
    char t = Serial1.read();
  }
}