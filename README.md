# A7 Tracker

This tracker is based on Ai-thinker A7 chip which is included in Elecrows 32u4 with A7 GPRS/GSM -board.

## Settings from API

Code is expecting a json string from the API endpoint containing the following fields:
- updateInterval: number of seconds between gps position updates

## Data to API

- id: device id, part of chip IMEI
- lat: latitude in the DDMM.MMMMM format
- lat_d: N/S (North/South) latitude
- lon: longitude in the DDDMM.MMMMM
- lon_d: W/E (West/East) longitude
- alt: altitude
- spd: speed in knots(1kn == 1.852km/h)
- acc: accuracy of postition in meters
