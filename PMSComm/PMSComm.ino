#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include<espnow.h>

#define MY_NAME         "PMS_NODE" // FMS NODE name
#define MY_ROLE         ESP_NOW_ROLE_COMBO   // set the node as tranceiver     
#define RECEIVER_ROLE   ESP_NOW_ROLE_SLAVE              
#define WIFI_CHANNEL    1 // set the wifi channel
#define NODE_ID         2
#define SERIAL_ID       "917556753837551"
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};   // set the receiver address to broadcast address
String val="";
float latValue = 0; // latValue from arduino
float lonValue = 0; // lonValue from arduino
int drivingBehavior = 0; //drivingBehavior from arduino
int panicButtonVal = 0; //panicButtonVal from arduino
int gxFilter = 0; // gxFilter from arduino
int gyFilter = 0; // gyFilter from arduino
int gzFilter = 0; // gzFilter from arduino 
int engineStatus = 0; // engineStatus from arduino

struct __attribute__((packed)) dataSend { // create construct variable for send data to blackbox
 int id;
 String serialId;
 String message;
};

struct __attribute((packed)) dataRecv{ // create construct variable for receive
  String macVerif;
  int engState;
};

void OnDataRecv(uint8_t * mac, uint8_t * data, uint8_t len){ //set callback function
  dataRecv Rx; //create variable Rx as struct variable
  memcpy(&Rx,data,sizeof(data)); // read the incoming data and stor to Rx
  if(Rx.macVerif == WiFi.macAddress()){ // check the verification mac address
     StaticJsonDocument<100> msg; //set the msg variable for json
     msg["eng"] = Rx.engState; // set the value from key eng
     serializeJson(msg,Serial); // send data to arduino.
  }
}

void setup() {
  Serial.begin(9600); // begin serial connection with baudrate 9600;
  WiFi.mode(WIFI_STA); // set wifi mode as station
  WiFi.disconnect(); // disconnect wifi from all connection 
  if(esp_now_init()!=0){ // init espnow protocol
    return;
  }
  esp_now_set_self_role(MY_ROLE); // set Wemos Role
  esp_now_add_peer(receiverAddress, MY_ROLE,WIFI_CHANNEL,NULL,0); // set transceiver address as peer 
  
}

void loop() {
if(Serial.available() >0 ){ // check serial message
  StaticJsonDocument<300> recv; // Set recv as json document variable
  DeserializationError err = deserializeJson(recv,Serial); //deserialize json message from serial
  if(err == DeserializationError::Ok){ // if there is no error in deserialization process
    latValue = recv["lat"].as<float>(); // read json latValue from arduino
    lonValue = recv["lon"].as<float>(); // read json lonValue from arduino
    drivingBehavior = recv["dri"].as<int>(); // read driving behavior from arduino
    panicButtonVal = recv["pb"].as<int>(); // read panic button value from arduino
    gxFilter = recv["gx"].as<int>(); // read gx filter value from arduino
    gyFilter = recv["gy"].as<int>(); //read gy filter value from arduino
    gzFilter = recv["gz"].as<int>(); // read gz filter value from arduino
    engineStatus = recv["eng"].as<int>(); // read engine status from arduino
    dataSend messageSend; // set messageSend variable as struct message
    val += String(latValue); // add latValue to variable  
    val += ","; // add separator
    val += String(lonValue); // add lonValue to val variable 
    val += ",";// add separator
    val += String(drivingBehavior); // add drivingBehavior to variable
    val += ",";// add separator
    val += String(panicButtonVal); // add panicButtonVal to variable
    val += ",";// add separator
    val += String(gxFilter); // add gxFilter to variable
    val += ",";// add separator
    val += String(gyFilter); // add gyFilter to variable
    val += ",";// add separator
    val += String(gzFilter); // add gzFilter to variable
    val += ",";// add separator
    val += String(engineStatus); // add engineStatus to variable
    messageSend.id = NODE_ID; //set node id = 2 (PMS)
    messageSend.serialId = SERIAL_ID; // set serial id
    messageSend.message = val; //set message = val variable.
    esp_now_send(receiverAddress,(uint8_t *) &messageSend, sizeof(messageSend)); // send message with espnow
    val = ""; //reset variable val
  }
  else{
    while(Serial.available()>0) Serial.read(); // if error and there is still a message read it and flush it
  }  
}
}
