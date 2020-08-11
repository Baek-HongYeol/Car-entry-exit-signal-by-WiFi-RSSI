/*
  version 1.1 - previous bool값 변경 지점 수정.
              - 부팅 직후를 분기하는 부분 삭제.
              
  version 2.0 - FTP 제거,
              - TESTSSID 제거.
              - Firebase RealTime Database 적용.
              - 연결 상태에는 정보를 계속 업로드.
  
*/
#include <Arduino.h>
#include <FirebaseArduino.h>
#include <ESP8266WiFi.h>

#ifndef STASSID
#define STASSID "jesusschool"
#define STAPSK  "****"
#endif

#define serialDBG


const char*     ssid     = STASSID;
const char*     password = STAPSK;
const char*     host = "192.168.0.35"; //192.168.0.1 ftp 서버 주소
const uint16_t  port = 20;            //20 ftp 서버 포트 번호

const String carNumber = "01";


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
  Firebase.begin("mytestproject-905ac.firebaseio.com", "***************************");
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
      Serial.print("RSSI: ");
      jesusschool = WiFi.RSSI(i);
      Serial.println(jesusschool);
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
  }
}

String path = "EntryInfo/"+carNumber;
bool sendIn(){
  long atime = millis();
  //IO 변경
  Firebase.setBool(path+"/IO", true);
  if (Firebase.failed()) {
      Serial.print("updating /IO failed:");
      Serial.println(Firebase.error());
      return false;
  }
  Serial.println("update /IO");
  //RSSI 변경
  int rss = WiFi.RSSI();
  Firebase.setInt(path+"/RSSI", rss);
  if (Firebase.failed()) {
      Serial.print("updating /RSSI failed:");
      Serial.println(Firebase.error());
      return false;
  }
  Serial.println("update /RSSI");
  //bootedTime 변경
  long bootedTime = millis();
  String text = String(bootedTime);
  Firebase.setString(path+"/bootedTime", text+"ms");
  if (Firebase.failed()) {
      Serial.print("updating /bootedTime failed:");
      Serial.println(Firebase.error());
      return false;
  }
  Serial.println("update /bootedTime");
   Serial.print("function spends ");
  Serial.print(millis()-atime);
  Serial.println("ms.");
  return true;
}
bool sendOut(){
  long atime = millis();
  // IO 변경
  Firebase.setBool(path+"/IO", false);
  if (Firebase.failed()) {
      Serial.print("updating /IO failed:");
      Serial.println(Firebase.error());
      return false;
  }
  // RSSI 변경
  int rss = WiFi.RSSI();
  Firebase.setInt(path+"/RSSI", rss);
  if (Firebase.failed()) {
      Serial.print("updating /RSSI failed:");
      Serial.println(Firebase.error());
      return false;
  }
  // bootedTime 변경
  long bootedTime = millis();
  String text = String(bootedTime);
  Firebase.setString(path+"/bootedTime", text+"ms");
  if (Firebase.failed()) {
      Serial.print("updating /bootedTime failed:");
      Serial.println(Firebase.error());
      return false;
  }
  Serial.print("function spends ");
  Serial.print(millis()-atime);
  Serial.println("ms.");
  return true;
}

bool sendStatus(bool sig){
  if(sig){
    return sendIn();
  }
  else{
    return sendOut();
  }
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
}
unsigned long startTime = 0;
void loop() {
  scanWiFiList();
  //Serial.print("RSSI: ");
  //Serial.println(WiFi.RSSI());
  //delay(500);
  unsigned long current=0;

  //when IO state changed, or every 10 seconds when it have In state, sendStatus.
  if((isChanged() || (millis()-startTime >10000 && signal_IO))&& WiFi.status() == WL_CONNECTED){
    startTime = millis();
    for(current = startTime; millis() - current < 10000;){
      if(sendStatus(signal_IO)){
        previous = signal_IO;
        break;
      }
      delay(500);
    }
  }
  
  #ifdef serialDBG
  Serial.println("Delay 3 seconds");
  #endif
  delay(3000); // execute once every 3 seconds, don't flood remote service
}
