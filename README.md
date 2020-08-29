# Car-entry-exit-signal-by-WiFi-RSSI
차에 연결하면 와이파이 세기를 측정해 자동차가 일정 범위 안에 있는지 판별하여 서버에 신호를 보내는 장치 (with Arduino)

~~FTP Protocol based on
<https://github.com/nimaltd/FTP_Client> based on <https://github.com/blackcodetavern/ESP32_FTPClient>~~
FTP Library is changed into Firebase RealTimeDB

using Lolin D1 mini board(WeMos D1 mini)

FTP_Client library was modified.
- isConnected variable added from _isConnected in ESP32_FTPClient
- isAvailable() function added from isConnected() in ESP32_FTPClient
- variable port added for other server's port
