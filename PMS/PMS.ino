#include <avr/wdt.h> // library for watchdog timer
#include "Kalman.h" // library for kalman filter
#include "Ublox.h" //library for GPS
#include <Wire.h> // library for i2c
#include <MPU9250_asukiaaa.h> //library for accessing MPU9250
#include <TinyGPS.h> //library for accessing GPS
#include <stdarg.h>
#include <ArduinoJson.h> // library for json processing in arduino
#include <SoftwareSerial.h> // library for serial read and serial write
SoftwareSerial GPSSerial(3,2); // Rx = 3, Tx = 2
MPU9250_asukiaaa imuSensor; // initialize imusensor
Ublox M8_Gps; // initialize gps

unsigned long intervalGyro = 200; // set variable for gyro reading interval 200 ms
unsigned nowGyro ; // set variable for get the current millis to gyro
unsigned lastGyro = millis(); // set variable to store last gyro reading time

unsigned long intervalSerial = 1000; // set variable for gyro reading interval 1000 ms
unsigned nowSerial; // set variable for get the current millis to serial
unsigned lastSerial = millis(); // set variable to store last serial send time

float gx = 0; // define variable to store gyro x value
float gy = 0; // define variable to store gyro y value
float gz = 0; // define variable to store gyro z value

float gxFilter = 0; // define variable to filtering gyro x value
float gyFilter = 0; // define variable to filtering gyro y value
float gzFilter = 0; // define variable to filtering gyro z value

float gxArr[10]; // define variable to store gyro x in array with size = 10
float gyArr[10]; // define variable to store gyro y in array with size = 10
float gzArr[10]; // define variabel to store gyro z in array with size = 10

int resultGx = 0; // define variable to store the result from gyro x calculation
int resultGy = 0; // define variable to store the result from gyro y calculation
int resultGz = 0; // define variable to store the result from gyro z calculation

int itr = 0; // define variable to counting the number of gyro when inserting to array
int gyroFinalResult = 0; // define variable to store all result from gyro result
int drivingBehavior = 0; // define the decision are the driving behavior safe or not (1/0)

float latValue = 0; // define latitude value variable
float lonValue = 0; // define longitude value variable

Kalman myFilter1(0.00125,32,1023,1); // kalman filter for gyro x
Kalman myFilter2(0.00125,32,1023,1); // kalman filter for gyro y
Kalman myFilter3(0.00125,32,1023,1); // kalman filter for gyro z

int panicButtonVal = 0; // define variable to store data reading from panic button

int panicButton = 5; // set pin 5 for panic button
int engineCut = 4; // set pin 4 for engine cut

int engineStatus = 1; // define variable to store engine status

void getGyro(); // initialize function for gyro reading from MPU 9250
void updateGyroArr(); // initialize function for store the gyro into array 
void panicButtonCheck(); // initialize function to check button value
void gpsRead(); // initialize function to read GPS.
void serialTx(); // initialize function to transmit serial to wemos
void serialRx(); // initialize function to receive serial from wemos
void engineState(int val); //initialize function to control the engine 
String getValue(String data, char separator, int index);



void setup() {
  Serial.begin(9600); // begin serial to wemos
  GPSSerial.begin(9600); // begin serial to gps
  imuSensor.beginGyro(); // begin imu sensor
  pinMode(engineCut, OUTPUT); // set engine cut as output
  pinMode(panicButton,INPUT); // set panic button as input
  wdt_enable(WDTO_4S); // set time threshold for watchdog timer 
}

void loop() {
  nowGyro = millis(); // get the current millis for gyro reading process
  nowSerial = millis(); // get the current millis for serial send process
  gpsRead(); // read the GPS longlat value
  serialRx(); // read the serial message from wemos
  if(nowGyro - lastGyro >= intervalGyro){ // check the interval for gyro reading process
    getGyro(); // get gyro value from MPU9250
    updateGyroArr(); // update gyro array filter
    lastGyro = nowGyro; // set last time for reading gyro to nowGyro time. 
  }
  panicButtonCheck(); // check the panic button value
  if(nowSerial - lastSerial >= intervalSerial){// check the interval for gyro reading process
    serialTx(); // send serial to wemos;
    lastSerial = nowSerial; // set the last time to now time for serial transmit process.
  }
  
  wdt_reset(); // reset the watchdog timer 
}

void gpsRead(){ // initialize function to read gps value
  while(GPSSerial.available()>0){ // read gps serial message
    char c = GPSSerial.read(); // store serial message from gps in char c variable
    if(M8_Gps.encode(c)){ // check if the message are encoded
      latValue = M8_Gps.latitude; // store latitude on latValue variable
      lonValue = M8_Gps.longitude; // store longitude on lonValue variable
    }
  }
}


void getGyro(){ //function to get the gyro data from MPU9250
  if (imuSensor.gyroUpdate() == 0){ // update the gyro from MPU9250
  gx = imuSensor.gyroX(); // get gyro x value
  gy = imuSensor.gyroY(); // get gyro y value
  gz = imuSensor.gyroZ(); // get gyro z value
  }
}

void updateGyroArr(){ //function to calculate gyro
  gxFilter = myFilter1.getFilteredValue(gx); // filter gyro x with kalman filter
  gyFilter = myFilter2.getFilteredValue(gy); // filter gyro y with kalman filter
  gzFilter = myFilter3.getFilteredValue(gz); // filter gyro z with kalman filter
  if(itr<=9){ // count the gyro value to store in gyro array
    gxArr[itr] = gxFilter; // insert gyro x to gxArr
    gyArr[itr] = gyFilter; // insert gyro y to gxArr
    gzArr[itr] = gzFilter; // insert gyro z to gxArr
  }
  else{
    itr = 0; // reset the counter 
    gxArr[itr] = gxFilter; // insert gyro x to gxArr
    gyArr[itr] = gyFilter; // insert gyro y to gxArr
    gzArr[itr] = gzFilter; // insert gyro z to gxArr
  }
  itr++; // increase the itr value 
  
  if(gxFilter > 10 || gxFilter <-10){ // check the filtered gyro x value
    resultGx = 1; // if gx filter amplitude less than 10 set the result 1 
  }
  else{
    resultGx = 0; // if gx filter amplitude more than 10 set the result 0
  }

  if(gyFilter > 10 || gyFilter < -10){ // check the filtered gyro y value
    resultGy = 1; // if gy filter amplitude less than 10 set the result 1
  }
  else{
    resultGy = 0; // if gy filter amplitude more than 10 set the result 0
  }

  if(gzFilter > 10 || gzFilter < -10){ // check the filtered gyro z value
    resultGz = 1; // if gz filter amplitude less than 10 set the result 1
  }
  else{
    resultGz = 0; // if gz filter amplitude more than 10 set the result 0
  }

  gyroFinalResult = resultGx+resultGy+resultGz; // get the final result from gyro value
  if(gyroFinalResult >=1) drivingBehavior = 1; // get the driving behavior decision from gyro
  else drivingBehavior = 0;
}

void panicButtonCheck(){ // function to read value from panic button 
  panicButtonVal = digitalRead(panicButton); // read value from panic button
}

void serialRx(){ // function to receive the serial data from wemos 
  if(Serial.available()>0){ // check serial message from wemos
    StaticJsonDocument<100> recv; //define the recv variable json
    DeserializationError err = deserializeJson(recv,Serial); // deserialize json Message
    if(err = DeserializationError::Ok){ // check deserialize json message
      int eng = recv["eng"].as<int>(); // get eng state from wemos 
      engineState(eng); // control engine base on message from wemos
    }
    else{
      while(Serial.available()>0) Serial.read(); // if error and there is still have a message clear it
    }
  }
}

void engineState(int val){ // function to control engine
  digitalWrite(engineCut,val); // set on/off engine
  engineStatus = val; // set status of the engine
}

void serialTx(){ // function to send serial message to wemos with json formatting
  StaticJsonDocument<50> msgSend; // define json variable
  msgSend["lat"] = latValue; // json key and value for latitude
  msgSend["lon"] = lonValue; // json key and value for longitude
  msgSend["dri"] = drivingBehavior; // json key and value for driving behavior
  msgSend["pb"] = panicButtonVal; // json key dan value for panic button value
  msgSend["gx"] = gxFilter; //json key and value for gx filter value
  msgSend["gy"] = gyFilter; // json key and value for gy filter value
  msgSend["gz"] = gzFilter; // json key and value for gz filter value
  msgSend["eng"] = engineStatus; // json key and value for engine status
  serializeJson(msgSend,Serial); // send serial to wemos
}
