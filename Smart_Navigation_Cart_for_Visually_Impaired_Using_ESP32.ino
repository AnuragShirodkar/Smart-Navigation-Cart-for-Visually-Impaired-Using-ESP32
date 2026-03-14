/*
 Smart Navigation Cart for Visually Impaired Using ESP32
*/

#include <SPI.h>
#include <MFRC522.h>
#include <NewPing.h>
#include "BluetoothSerial.h"
#include "DFRobotDFPlayerMini.h"

// --- ROBOT STATES ---
enum RobotState { WAITING, TRAVELING, ARRIVED };
RobotState robotState = WAITING;

// --- BLUETOOTH ---
BluetoothSerial SerialBT;
String targetDestination = "";

// --- VALID ITEMS ---
String validItems[] = {
  "BOO",
  "CLE",
  "VEG",
  "CHE",
  "GRO",
  "TOY",
  "BEV",
  "SNA",
  "DAI",
  "ELE"
};
const int TOTAL_VALID_ITEMS = 10;

// --- MOTOR PINS ---
#define ENA 14
#define IN1 12
#define IN2 13
#define ENB 32
#define IN3 33
#define IN4 25

// --- SENSOR PINS ---
#define SENSOR_LEFT    34
#define SENSOR_CENTER  35
#define SENSOR_RIGHT   26
#define TRIG_PIN       27
#define ECHO_PIN       22
#define MAX_DISTANCE   30

// --- RFID ---
#define SS_PIN  5
#define RST_PIN 2
#define DEST_BLOCK 4

// --- DFPLAYER ---
#define DFPLAYER_ESP_RX 4
#define DFPLAYER_ESP_TX 21

// --- AUDIO TRACKS ---
const int SND_OBSTACLE = 1;
const int SND_ARRIVED  = 2;
const int SND_LEFT     = 3;
const int SND_RIGHT    = 4;
const int SND_START    = 5;
const int SND_INVALID  = 6;

// --- AUDIO DURATIONS ---
const unsigned long soundDurations[] = {
  0,1800,2000,1000,1000,1500,2000
};

// --- COMPONENT OBJECTS ---
MFRC522 mfrc522(SS_PIN, RST_PIN);
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
MFRC522::MIFARE_Key key;
HardwareSerial dfPlayerSerial(1);
DFRobotDFPlayerMini myDFPlayer;

// --- SPEED PARAMETERS ---
int forwardSpd = 180;
int turnSpdLow = 80;
int turnSpdHigh = 220;
unsigned long junctionTurnTime = 600;

// --- PWM SETTINGS ---
const int PWM_FREQ = 5000;
const int PWM_RES  = 8;

// --- FUNCTION DECLARATIONS ---
void stopMotors();
void moveForward(int speed);
void turnLeft(int lowSpeed, int highSpeed);
void turnRight(int lowSpeed, int highSpeed);
String readDataFromBlock(byte blockAddr);
void playAudio(int trackNumber, bool waitForCompletion = true, unsigned long timeoutMs = 5000);
int safePingCm();
bool checkInventory(String item);

void setup() {

  Serial.begin(115200);

  // Initialize Bluetooth
  SerialBT.begin("SmartNavigationCart");

  // Initialize DFPlayer
  dfPlayerSerial.begin(9600, SERIAL_8N1, DFPLAYER_ESP_RX, DFPLAYER_ESP_TX);

  if (myDFPlayer.begin(dfPlayerSerial)) {
    myDFPlayer.volume(25);
    myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
    myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  }

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);

  // Line sensors
  pinMode(SENSOR_LEFT, INPUT);
  pinMode(SENSOR_CENTER, INPUT);
  pinMode(SENSOR_RIGHT, INPUT);

  // RFID initialization
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  stopMotors();
}

void loop() {

  // --- BLUETOOTH COMMAND ---
  if (SerialBT.available()) {

    String voiceCommand = SerialBT.readString();
    voiceCommand.trim();
    voiceCommand.toUpperCase();

    if (voiceCommand.length() > 0) {

      String shortCode;

      if (voiceCommand.length() > 3)
        shortCode = voiceCommand.substring(0,3);
      else
        shortCode = voiceCommand;

      if (checkInventory(shortCode)) {
        targetDestination = shortCode;
        robotState = TRAVELING;
        playAudio(SND_START,true);
      } 
      else {
        playAudio(SND_INVALID,true);
      }
    }
  }

  // --- STATE MACHINE ---
  switch(robotState) {

    case WAITING:
      stopMotors();
      break;

    case ARRIVED:
      stopMotors();
      break;

    case TRAVELING: {

      // --- OBSTACLE DETECTION ---
      int dist = safePingCm();

      if(dist>0 && dist<=MAX_DISTANCE) {

        stopMotors();
        playAudio(SND_OBSTACLE,true);

        while(true){
          delay(200);
          int d2 = safePingCm();
          if(!(d2>0 && d2<=MAX_DISTANCE)) break;
        }

        delay(300);
        return;
      }

      // --- RFID NAVIGATION ---
      if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()){

        stopMotors();

        String tagData = readDataFromBlock(DEST_BLOCK);
        tagData.trim();
        tagData.toUpperCase();

        if(tagData.startsWith("D_")){

          if(tagData.indexOf(targetDestination)>=0){
            robotState = ARRIVED;
            playAudio(SND_ARRIVED,true);
          }
          else{
            moveForward(forwardSpd);
            delay(600);
          }
        }

        else if(tagData.startsWith("J_")){

          if(tagData.indexOf(targetDestination)>=0){
            playAudio(SND_LEFT,false);
            turnLeft(turnSpdLow,turnSpdHigh);
            delay(junctionTurnTime);
          }
          else{
            playAudio(SND_RIGHT,false);
            turnRight(turnSpdLow,turnSpdHigh);
            delay(junctionTurnTime);
          }
        }

        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
      }

      // --- LINE FOLLOWING ---
      int s1 = digitalRead(SENSOR_LEFT);
      int s3 = digitalRead(SENSOR_CENTER);
      int s5 = digitalRead(SENSOR_RIGHT);

      if(s3==LOW) moveForward(forwardSpd);
      else if(s1==LOW) turnLeft(turnSpdLow,turnSpdHigh);
      else if(s5==LOW) turnRight(turnSpdLow,turnSpdHigh);
      else stopMotors();

      break;
    }
  }
}

// --- INVENTORY CHECK ---
bool checkInventory(String item){

  for(int i=0;i<TOTAL_VALID_ITEMS;i++){
    if(validItems[i]==item) return true;
  }

  return false;
}

// --- AUDIO PLAYER ---
void playAudio(int trackNumber,bool waitForCompletion,unsigned long timeoutMs){

  if(trackNumber<=0) return;

  myDFPlayer.play(trackNumber);

  if(!waitForCompletion) return;

  unsigned long dur=1000;

  if(trackNumber < (sizeof(soundDurations)/sizeof(soundDurations[0])))
    dur = soundDurations[trackNumber];

  unsigned long start=millis();

  while(millis()-start < dur){
    delay(10);
  }
}

// --- RFID DATA READ ---
String readDataFromBlock(byte blockAddr){

  byte buffer[18];
  byte size = sizeof(buffer);

  MFRC522::StatusCode status;

  status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    blockAddr,
    &key,
    &(mfrc522.uid)
  );

  if(status!=MFRC522::STATUS_OK) return "";

  status = mfrc522.MIFARE_Read(blockAddr,buffer,&size);

  if(status!=MFRC522::STATUS_OK) return "";

  String result="";

  for(byte i=0;i<16;i++){
    if(buffer[i]==0) break;
    result += (char)buffer[i];
  }

  return result;
}

// --- MOTOR CONTROL ---
void moveForward(int speed){

  ledcWrite(ENA, constrain(speed,0,255));
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);

  ledcWrite(ENB, constrain(speed,0,255));
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
}

void turnLeft(int lowSpeed,int highSpeed){

  ledcWrite(ENA,constrain(lowSpeed,0,255));
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);

  ledcWrite(ENB,constrain(highSpeed,0,255));
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
}

void turnRight(int lowSpeed,int highSpeed){

  ledcWrite(ENA,constrain(highSpeed,0,255));
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);

  ledcWrite(ENB,constrain(lowSpeed,0,255));
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
}

void stopMotors(){

  ledcWrite(ENA,0);
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);

  ledcWrite(ENB,0);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,LOW);
}

// --- ULTRASONIC SAFE READ ---
int safePingCm(){

  unsigned int r = sonar.ping_cm();
  return (int)r;
}