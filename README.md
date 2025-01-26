# TempoBW
Lolin D32 (D32 architecture) outdoor meteo-thermometer (B)T + (W)IFI

Materials used:

- OBO electrician's box
- SSD1306 display 128x32
- SHT41 humidity sensor
- BME280 barometric pressure sensor
- DS3231 clock Module
- DS18b20 (Dallas) temperature sensor

Project compiled in Arduino IDE
Setting board -> No OTA (large APP)

After compilation, it is possible to connect in two ways:
- using WIFI (mobile data must be turned off), after connecting, open the browser and enter the IP address 192.168.4.1
- using bluetooth, for example via the Serial Bluetooth Terminal program, help will be displayed after connection

