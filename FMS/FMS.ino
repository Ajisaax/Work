#include <SoftwareSerial.h>
#include <ArduinoJson.h>
SoftwareSerial mySerial(3, 2); // RX=3, TX=2
String resultMessage = ""; //result message
int messageEnd = 0; //message number
int lastMessage = 0; //last message status
int capStatus = 0; //fuel tank cap status
int pinCap = 4; // proximity pin for detect the status of fuel tank caps
int fuelData = 1314; // fuel data variable
unsigned long sendInterval = 5000; // set interval time for send the data to wemos
unsigned lastSend = millis(); // set lastSend variable to save the last time the message was send
unsigned nowSend; // set the time now variable

String getValue(String data, char separator, int index); //get value from separated string


void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600); 
  while (!Serial) {
    ; // wait for serial port to connect. 
  }
  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  pinMode(pinCap,INPUT); //set pinCap pin as input pin
}

void loop() // run over and over
{
  nowSend = millis(); // get time now
  capStatus = digitalRead(pinCap); // get status of pinCap.
  while(mySerial.available()>0) {// Listen UL212 From Serial
    char c = mySerial.read(); //variable to read serial from UL212
    if(c == '#'){ //detect the last char from UL212
    messageEnd++; //if last char detected increase the messageEnd value.
    }    
    resultMessage += c; //join Character from UL212
  }
 if (lastMessage != messageEnd){ // filtering message from UL212
 resultMessage = resultMessage.substring(1,resultMessage.length()-1); // get last message without open and close char.
 resultMessage = getValue(resultMessage,',',3); //get value from message that contain separated string from UL212. 3rd order from the message are the fuel height
 fuelData = resultMessage.toInt();
 lastMessage = messageEnd; //set messageEnd value as lastMessage value
 resultMessage = ""; //reset Result message value
 }
 if(nowSend - lastSend >= sendInterval){ //detect the last time send (hyperthreading)
 StaticJsonDocument<100> msgSend ; //initialize variable json to send in WEMOS 
 msgSend["fuel"] = fuelData; // key 'fuel' for UL 212 Height
 msgSend["cap"] = capStatus; // key 'cap' for cap status from proximity inductive
 serializeJson(msgSend,Serial); // send serial to Wemos
 lastSend = nowSend; // set time now as last time send. 
 }

}




String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
