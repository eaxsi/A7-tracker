// A7-tracker
// GPRS enabled GPS-tracker

// Eero Silfverberg 2018

#include <ArduinoJson.h>

// p_buffer settings to store constant data
char p_buffer[100];
#define P(str) (strcpy_P(p_buffer, PSTR(str)), p_buffer)

const char * const server_addr PROGMEM   = "http://dev.eero.tech";

char response[300];
char imei[20];

void setup()
{
  delay(2000);
    // init serial
	Serial.begin(115200);
	Serial1.begin(115200);
  Serial.println("Hello!");
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
	Serial.println("A7 powered");

  getIMEI();
  waitUntilConnected();
  initModem();
  getSettings();
}

void loop()
{
  delay(1000);
  Serial.println("Loop!");
}
void getIMEI(){
  sendAT("AT+CGSN", 1000, true);
  delay(1000);
}

void initModem(){
  sendAT("AT+CGATT=1", 5000, true);
  //sendAT("AT+CGDCONT=1,\"IP\",\"cmnet\"",3000,true);
  sendAT("AT+CGACT=1,1",8000,true);
  //sendAT("AT+CSTT=\"internet\"", 1000, true);
  //sendAT("AT+CIICR", 5000, true);
  //sendAT("AT+CIPSTATUS", 5000, true);
}

void waitUntilConnected(){
  while (!strstr(response, "+CREG: 1,1\r")){
    Serial.println("Searching for network...");
    sendAT("AT+CREG?", 1000, true);
    delay(1000);
  }
  Serial.println("Registered to network!");
  sendAT("AT+CSQ", 1000, true);
}

int getSettings(){
  sendAT("at+cipstart=\"TCP\",\"eero.tech\",8080",8000, true);

  sendAT("AT+CIPSEND", 3000, true);
  Serial1.print("GET /api/test HTTP/1.0\r\n");
  delay(500);
  Serial1.print("\r\n");
  delay(500);
  Serial1.write(0x1A);
  delay(500);
  sendAT("", 5000, true);

  sendAT("AT+CIPSHUT", 1000, true);
  return 0;
}

int sendData(){
  return 0;
}

void sendAT(String command, const int timeout, boolean debug)
{
  memset(response, 0, sizeof(response));
  Serial.println(command);
  int i = 0;
  int linecout = 0;
  Serial1.print(command);
  Serial1.println("");
  long int time = millis();   
  while((time+timeout) > millis()){
    while(Serial1.available()){
      char currChar = Serial1.read();
      if (linecout >= 4){
        // skip couple of lines from the response
        // A7 module echoes the sent commands back, we want only the response
        response[i] = currChar;
        i++;
      }
      if (currChar == '\r' || currChar == '\n') {
        linecout++;
      }
    }
  }
  response[i] = '\0'; // end response correctly
  Serial.print("RESP:");
  Serial.print(response);
  Serial.print('\n');
}