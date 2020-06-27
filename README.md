# A7 Tracker
Tracker based on 
[Elecrow 32u4 with A7 GPRS/GSM board](https://www.elecrow.com/wiki/index.php?title=32u4_with_A7_GPRS/GSM)

## Prerequisites
- [Arduino IDE 1.8.3](https://www.arduino.cc/en/Main/Software) or greater
- [ArduinoJson V5.13.5](https://arduinojson.org/) library (install via Arduinos library manager) 

## Getting Started
- Clone this repository
- Change server addresses in A7.h

## Setup
- modify settings from config.h


## Data structures

### Position update
Tracker sends this message to the server:
```javascript
{"id":"254574","fix":1,"uptime":4115154,"signal":31,"batt":89,"lat":"5106.9792","lat_d":"N","lon":"11402.3003","lon_d":"W","nSat":8,"acc":7}
```

| Key    | Possible values | Comment                                                                         |
|--------|-----------------|---------------------------------------------------------------------------------|
| id     | 000000-999999   | last digits of device IMEI                                                      |
| fix    | 0,1             | availability of GPS signal                                                      |
| uptime | 0-4294967296(~49 days)    | tracker uptime in milliseconds                                                  |
| signal | 0-33, 99        | signal strength, [reference](https://m2msupport.net/m2msupport/signal-quality/) |
| batt   | 0-100           | battery charge level                                                            |
| lat    |                 | latitude, (d)ddmm.mmmm format                                                   |
| lat_d  | N, S            |                                                                                 |
| lon    |                 | longitude, (d)ddmm.mmmm format                                                  |
| lon_d  | E, W            |                                                                                 |
| nSat   | 0-20            | number of satellites the receiver sees                                          |
| acc    | 0-100           | accuracy in meters                                                              |



### Settings update
Tracker expects this response from server:
```javascript
{"loc_interval":"60", "set_interval":"100"}
```
| Key          | Possible values | Comment                               |
|--------------|-----------------|---------------------------------------|
| loc_interval | 0-4294967295    | Location sending interval in seconds  |
| set_interval | 0-4294967295    | Settings fetching interval in seconds |



## Changelog
### V2.0 (Apr 2020)

- new functions
    - send battery level
    - send network signal strength
    - watchdog timer
    - non blocking delay
    - check that modem is alive and connected
    - send message even if no GPS fix
    - switch GPS off when not needed

- enhancements
    - move modem related code to AiA7 class
    - simplify sendAT function

- bugfixes
    - GPGGA parsing bug
    - reboot modem correctly
