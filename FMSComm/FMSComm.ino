#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include<espnow.h>

#define MY_NAME         "FMS_NODE" // FMS NODE name
#define MY_ROLE         ESP_NOW_ROLE_CONTROLLER   // set the node as controller (Sender)    
#define RECEIVER_ROLE   ESP_NOW_ROLE_SLAVE              
#define WIFI_CHANNEL    1 // set the wifi channel
#define NODE_ID         1
#define SERIAL_ID       "491938086584106"
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};   // set the receiver address to broadcast address
String val="";
int fuelState; // variable for fuel data from arduino
int capState; // variable for cap state data from arduino

struct __attribute__((packed)) dataSend { // create construct variable for send data to blackbox
 int id;
 String serialId;
 String message;
};


void setup() {
  Serial.begin(9600); // begin serial connection with baudrate 9600;
  WiFi.mode(WIFI_STA); // set wifi mode as station
  WiFi.disconnect(); // disconnect wifi from all connection 
  if(esp_now_init()!=0){ // init espnow protocol
    return;
  }
  esp_now_set_self_role(MY_ROLE); // set Wemos Role
  esp_now_add_peer(receiverAddress, RECEIVER_ROLE,WIFI_CHANNEL,NULL,0); // set receiver address as peer 

}

void loop() {
if(Serial.available() >0 ){ // check serial message
  StaticJsonDocument<200> recv; // Set recv as json document variable
  DeserializationError err = deserializeJson(recv,Serial); //deserialize json message from serial
  if(err == DeserializationError::Ok){ // if there is no error in deserialization process
    fuelState = recv["fuel"].as<int>(); //get fuel value from arduino
    capState = recv["cap"].as<int>(); //get cap state from arduino
    dataSend messageSend; // set messageSend variable as struct message
    val += String(fuelState); // add fuelState value to val variable  
    val += ","; // add separator
    val += String(capState); // add capState value to val variable 
    messageSend.id = NODE_ID; //set node id = 1 (FMS)
    messageSend.serialId = SERIAL_ID;
    messageSend.message = val; //set message = val variable.
    esp_now_send(receiverAddress,(uint8_t *) &messageSend, sizeof(messageSend)); // send message with espnow
    val = ""; //reset variable val
  }
  else{
    while(Serial.available()>0) Serial.read(); // if error and there is still a message read it and flush it
  }  
}
}
