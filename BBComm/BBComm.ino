#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoJson.h>
#include "AESLib.h"
#define MY_NAME         "BLACKBOX_NODE" // black box node
#define MY_ROLE         ESP_NOW_ROLE_COMBO // set the node as transceiver
#define RECEIVER_ROLE   ESP_NOW_ROLE_SLAVE
#define WIFI_CHANNEL    1 //set wifi channel
#define NODE_ID         4 // set channel 

AESLib aesLib;
byte aes_key[] = { 0x00, 0xEC, 0xFD, 0x5B, 0xA0, 0xD5, 0x64, 0x44, 0x54, 0xB4, 0xEB, 0x1D, 0x38, 0x0C, 0xDA, 0xCE};
byte aes_iv[N_BLOCK] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};   // set the receiver address to broadcast address
String val="";

struct __attribute__((packed)) dataRecv { // create construct variable for send data to blackbox
 int id;
 String serialId;
 String message;
};

struct __attribute((packed)) dataSend{ // create construct variable for receive
  int id;
  String serialId;
  String message;
};

void OnDataRecv(uint8_t * mac, uint8_t * data, uint8_t len){ //set callback function
  dataRecv Rx; //create variable Rx as struct variable
  memcpy(&Rx,data,sizeof(data)); // read the incoming data and stor to Rx
  StaticJsonDocument<300> sendBB; // set json object variable 
  sendBB["id"] = Rx.id; // set value for key id json
  sendBB["serialId"]=Rx.serialId; // set value for serialId key
  sendBB["message"]=Rx.message; // set value for message 
  sendBB["devId"] = mac; //set value for mac 
  serializeJson(sendBB, Serial); //send json to blackbox
  Serial.println(); //add new line for serial json
}

String encrypt(char * msg, uint16_t msgLen, byte iv[]) {
  int cipherlength = aesLib.get_cipher64_length(msgLen);
  char encrypted[cipherlength]; // AHA! needs to be large, 2x is not enough
  aesLib.encrypt64(msg, msgLen, encrypted, aes_key, sizeof(aes_key), iv);
  byte aes_iv[N_BLOCK] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  return String(encrypted);
}


void setup() {
  // put your setup code here, to run once:
 Serial.begin(9600); // begin serial connection with baudrate 9600;
  WiFi.mode(WIFI_STA); // set wifi mode as station
  WiFi.disconnect(); // disconnect wifi from all connection 
  if(esp_now_init()!=0){ // init espnow protocol
    return;
  }
  esp_now_set_self_role(MY_ROLE); // set Wemos Role
  esp_now_add_peer(receiverAddress, MY_ROLE,WIFI_CHANNEL,NULL,0); // set transceiver address as peer 
  aesLib.gen_iv(aes_iv);
  aesLib.set_paddingmode(paddingMode::CMS);
}

void loop() {
  // put your main code here, to run repeatedly:
if(Serial.available()>0){ //check serial message
  StaticJsonDocument<200> recv; // set json object for receiving message from blackbox
  DeserializationError err = deserializeJson(recv,Serial); //deserialize json message from serial
  if(err == DeserializationError::Ok){ // if there is no error in deserialization process
    dataSend messageSend; //set messageSend variable as struct message
    messageSend.id = recv["id"].as<int>(); //set node id to send in PMS
    messageSend.serialId = recv["serialId"].as<String>(); //set serial id to send in PMS
    String engineData = recv["ENG"].as<String>(); //set message to send in PMS
    uint16_t dlen = engineData.length();
    String enc = encrypt(strdup(engineData.c_str()),dlen,aes_iv);
    messageSend.message = enc;
    esp_now_send(receiverAddress,(uint8_t *) &messageSend, sizeof(messageSend)); // send message with espnow
  }
  else{
    while(Serial.available()>0) Serial.read();
  }
  
}
}
