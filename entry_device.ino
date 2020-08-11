/*
  version 1.1 - previous bool값 변경 지점 수정.
              - 부팅 직후를 분기하는 부분 삭제.
*/
#include <Arduino.h>
#include <FTP_Client.h>
#include <ESP8266WiFi.h>

#ifndef STASSID
#define STASSID "jesusschool"
#define STAPSK  "****"
#endif
/*#ifndef TESTSSID
#define TESTSSID  "****"
#define TESTPSK   "****"
#endif*/

#define serialDBG


const char*     ssid     = STASSID;
const char*     password = STAPSK;
const char*     host = "192.168.0.35"; //192.168.0.1 ftp 서버 주소
const uint16_t  port = 20;            //20 ftp 서버 포트 번호

const char      carNumber[128] = "01";

FTP_Client      ftp;

int isJesusschool = 0;
int isTestAP = 0;
int jesusschool = 0;
int testAP = 0;
bool signal_IO = false;
bool previous = false;


void connectWiFi(const char* _ssid, const char* _passwd) {
  #ifdef serialDBG
  Serial.print("Connecting");
  #endif
  unsigned long _m = millis();
  WiFi.disconnect();
  WiFi.begin(_ssid, _passwd);
  while (WiFi.status() != WL_CONNECTED && millis() - _m < 30000) {
    delay(500);
    Serial.print(".");
  }
  #ifdef serialDBG
  Serial.println("");
  Serial.print("WiFi connected ");
  Serial.println((WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "failed");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("\nMax Free Heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.println("");
  #endif
}

void scanWiFiList() {
  //Search target WiFi AP
  Serial.println();
  Serial.println("Searching AP List..");
  int num_Of_Networks = WiFi.scanNetworks();
  isJesusschool = 0;
  isTestAP = 0;
  for (int i = 0; i < num_Of_Networks; i++) {
    if (WiFi.SSID(i).equals(STASSID)) {
      Serial.println("StaAP Founded--");
      jesusschool = WiFi.RSSI(i);
      isJesusschool = 1;
      signal_IO = true;
    }
    #ifdef TESTSSID
    else if (WiFi.SSID(i).equals(TESTSSID)) {
      Serial.println("TestAP Founded--");
      testAP = WiFi.RSSI(i);
      isTestAP = 1;
    }
    #endif
  }
  if (jesusschool<-80){
    signal_IO = false;
  }
  if (isJesusschool == 0 || jesusschool < -90 ) {
    jesusschool = 0;
  }
  if (isTestAP == 0 || testAP < -90) {
    testAP = -100;
  }

  if (WiFi.status() != WL_CONNECTED && (isJesusschool || isTestAP)) {
    if (jesusschool != 0 && jesusschool > testAP) {
      #ifdef serialDBG
      Serial.println();
      Serial.print("connect ");
      Serial.print(STASSID);
      Serial.println("...");
      #endif
      connectWiFi(STASSID, STAPSK);
    }
    #ifdef TESTSSID
    else if (testAP>-100) {      //testAP != -100 || jesusschool<=testAP (if jesusschool==0, jesusschool>testAP)
      #ifdef serialDBG
      Serial.println();
      Serial.println("connect TestAP...");
      #endif
      connectWiFi(TESTSSID, TESTPSK);
    }
    #endif
  }
}

bool ftp_sendStatus(bool sig){
  if (ftp.openConnection()) {
    delay(2000);
    #ifdef serialDBG
    Serial.println("----create File to server----");
    #endif
    if(ftp.initFile(FTP_FileType_ASCII)){
      char filename[128];
      snprintf(filename,128, "[%s]", carNumber);
      if(!ftp.deleteFile(filename)){ //파일이 존재하지 않을 경우 오류발생, 연결 재시도
        #ifdef serialDBG
        Serial.println("--deleting Failed.--");
        #endif
        if(!ftp.openConnection()){ 
          #ifdef serialDBG
          Serial.println("----closing connection----");
          #endif
          ftp.closeConnection();
          #ifdef serialDBG
          Serial.println("--deleting Failed.--");
          #endif
          return false;
        }
        ftp.initFile(FTP_FileType_ASCII);
      }
      if(ftp.newFile(filename)) {
        char message[128];
        if(sig){
          snprintf(message, 128, "IN\tRSSI: %d\nbooted %dms ago ", WiFi.RSSI(), millis());
        } else{
          sprintf(message, "OUT");
        }
        ftp.write(message);
        #ifdef serialDBG
        Serial.println("----data written to server----");
        #endif
        ftp.closeFile();
        #ifdef serialDBG
        Serial.println("----File closed----");
        #endif
        return true;
      }
      return false;
    }
    
    // Close the connection
    #ifdef serialDBG
    Serial.println();
    Serial.println("----closing connection----");
    Serial.println();
    #endif
    ftp.closeConnection();
    return true;
  }
  return false;
}

bool isChanged(){
  if(previous^signal_IO){
    //previous = signal_IO;
    return true;
  }
  return false;
}

void setup() {
  #ifdef serialDBG
  Serial.begin(115200);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  #endif
  jesusschool = 0;
  testAP = 0;
  signal_IO = false;
  previous = false;
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  ftp.begin(host, port, "guest", "a", 15000);
  connectWiFi(STASSID, STAPSK);
}

void loop() {
  scanWiFiList();
  //Serial.print("RSSI: ");
  //Serial.println(WiFi.RSSI());
  //ftp_contentList();
  unsigned long current=0;
  
  if(isChanged() && WiFi.status() == WL_CONNECTED){
    //if(millis() < 100000){
      for(current = millis(); millis() - current < 20000;){
        if(ftp_sendStatus(signal_IO)){
          previous = signal_IO;
          break;
        }
        delay(1000);
      }
    /*}else{
      for(current = millis(); millis() - current < 20000;){
        if(ftp_sendStatus(signal_IO)){
          previous = signal_IO
          break;
        }
        delay(1000);
      }
    }*/
  }
  
  #ifdef serialDBG
  Serial.println("Delay 3 seconds");
  #endif
  delay(3000); // execute once every 3 seconds, don't flood remote service
}
