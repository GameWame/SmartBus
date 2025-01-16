#include <WiFiNINA.h>
#include <PubSubClient.h>

#define IR_SENSOR_RIGHT 12
#define IR_SENSOR_LEFT 11

int vspeed = 110;    // Velocità di base per muoversi in avanti
int tspeed = 255;    // Velocità massima per girare

const char* ssid = "WiFi-LabIoT";
const char* password = "s1jzsjkw5b";

// MQTT Broker Configuration:
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "numpass";
const char *partenza_topic = "tpartenza";
const char *start_topic = "startbus";
const char *pos_topic = "posbus";
int percentuale = 0;
const int mqtt_port = 1883;

// Definizione pin
const int trigPin1 = 13;
const int echoPin1 = 2;
const int trigPin2 = 6;
const int echoPin2 = 5;

unsigned long lastUltrasonicCheck = 0;
unsigned long ultrasonicInterval = 100; // Intervallo di 100 ms

// Variabili
long duration;
int distance1;
int distance2;

int pullmanArrived = 0;

// Pin motore destro
int enableRightMotor = 10;
int rightMotorPin1 = 3; // out1
int rightMotorPin2 = 4; // out2
 
// Pin motore sinistro
int enableLeftMotor = 9;
int leftMotorPin1 = 8;  // out4
int leftMotorPin2 = 7;  // out3

int canStart = 0;

WiFiClient rev2Client;            // WiFi client
PubSubClient client(rev2Client);  // MQTT client

int calculateDistanceSalita() {
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);

  duration = pulseIn(echoPin1, HIGH);
  return duration * 0.034 / 2;
}

int calculateDistanceDiscesa() {
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  duration = pulseIn(echoPin2, HIGH);
  return duration * 0.034 / 2;
}

void setup() {
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  //right motor
  pinMode(enableRightMotor, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  
  //left motor
  pinMode(enableLeftMotor, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);

  pinMode(IR_SENSOR_RIGHT, INPUT);
  pinMode(IR_SENSOR_LEFT, INPUT);
  Serial.begin(9600);

  // Connessione WiFi
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to the WiFi network.");
  Serial.print("ARDUINO IP_Address: ");
  Serial.println(WiFi.localIP());

  // Connessione MQTT
  client.setServer(mqtt_broker, mqtt_port);
  client.subscribe(start_topic);
  client.setCallback(callback);

  Serial.print("Subscribed to the topic: ");
  Serial.println(start_topic);
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  if(pullmanArrived){
    if (millis() - lastUltrasonicCheck >= ultrasonicInterval){
      distance1 = calculateDistanceSalita();
      if (distance1 <= 2) {
        client.publish(topic, "increase");
        Serial.println("INCREASE");
      }

    distance2 = calculateDistanceDiscesa();
    if (distance2 <= 2) {
      client.publish(topic, "decrease");
      Serial.println("DECREASE");
      }
    }
    delay(500);
  }else{
    int rightIRSensorValue = digitalRead(IR_SENSOR_RIGHT);
    int leftIRSensorValue = digitalRead(IR_SENSOR_LEFT);

    //If none of the sensors detects black line, then go straight
    if (rightIRSensorValue == LOW && leftIRSensorValue == LOW)
    {

      Serial.println("Avanti");
      moveForward(vspeed);
    }
    //If right sensor detects black line, then turn left
    else if (rightIRSensorValue == HIGH && leftIRSensorValue == LOW )
    {
        Serial.println("Destra");
        turnLeft(tspeed);
    }
    //If left sensor detects black line, then turn left  
    else if (rightIRSensorValue == LOW && leftIRSensorValue == HIGH )
    {
        Serial.println("Sinistra");
        turnRight(tspeed);
    }

    else
    {
      percentuale += 1;
      if(percentuale == 1){
        client.publish(pos_topic, "25");
        delay(70);
      }
      if(percentuale == 2){
        client.publish(pos_topic, "50");
        delay(80);
      }
      if(percentuale == 3){
        client.publish(pos_topic, "75");
        delay(80);
      }
      if(percentuale == 4){
        client.publish(pos_topic, "100");
        Serial.println("STOP");
        pullmanArrived = 1;
        client.publish(partenza_topic, "start");
        stopMotors();
        delay(20);
        percentuale = 0;
      }
    }
  }
  client.loop();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("REV2")) {
      Serial.println("Connected to the MQTT Broker.");
      client.subscribe(start_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Function to process received messages
void callback(char* start_topic, byte* message, unsigned int length) {
  //Serial.print("Message arrived on topic: ");
  //Serial.println(topic);
  Serial.print(" - Message : ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  pullmanArrived = 0;
  moveForward(tspeed);
}

void moveForward(int speed) {
    analogWrite(enableRightMotor, speed);
    analogWrite(enableLeftMotor, speed);

    digitalWrite(rightMotorPin1, HIGH);
    digitalWrite(rightMotorPin2, LOW);
    digitalWrite(leftMotorPin1, HIGH);
    digitalWrite(leftMotorPin2, LOW);
}

void turnLeft(int speed) {
    analogWrite(enableRightMotor, speed);
    analogWrite(enableLeftMotor, speed / 3);

    digitalWrite(rightMotorPin1,HIGH);
    digitalWrite(rightMotorPin2,LOW);
    digitalWrite(leftMotorPin1,HIGH);
    digitalWrite(leftMotorPin2,LOW);
}

void turnRight(int speed) {
    analogWrite(enableRightMotor, speed / 3);
    analogWrite(enableLeftMotor, speed);

    digitalWrite(rightMotorPin1,LOW);
    digitalWrite(rightMotorPin2,HIGH);
    digitalWrite(leftMotorPin1,HIGH);
    digitalWrite(leftMotorPin2,LOW);

}

void stopMotors() {
    analogWrite(enableRightMotor, 0);
    analogWrite(enableLeftMotor, 0);

    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, LOW);
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, LOW);
}