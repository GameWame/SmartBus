#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <LCD-I2C.h>
#include <Wire.h>
#include <MFRC522.h>

const char* ssid = "WiFi-LabIoT";
const char* password = "s1jzsjkw5b";

const char *mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;
const char *topic = "arrivobus";
const char *ticket_topic = "numticket";

int pullmanArrived = 0;

LCD_I2C lcd(0x27, 20, 4); // Default address of most PCF8574 modules, change according
MFRC522 rfid(10, 9);

WiFiClient rev2Client;            // WiFi client
PubSubClient client(rev2Client);  // MQTT client

byte zero[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
byte one[] = {
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000
};
byte two[] = {
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000
};
byte three[] = {
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100
};
byte four[] = {
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110
};
byte five[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

void setup() {
  
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
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
  client.subscribe(topic);
  client.subscribe(topic_start);
  client.setCallback(callback);
  Serial.print("Subscribed to the topic: ");
  Serial.println(topic);
  reconnect();

  Wire.begin();
  lcd.begin(&Wire);
  lcd.display();
  lcd.backlight();
  lcd.setCursor(0, 1);
  lcd.createChar(0, zero);
  lcd.createChar(1, one);
  lcd.createChar(2, two);
  lcd.createChar(3, three);
  lcd.createChar(4, four);
  lcd.createChar(5, five);
  lcd.print("STATO PULLMAN");
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  if(pullmanArrived){
    clearRow(1);
    pullmanArrived = 0;
  }
  if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
    String uid = getUID();
    Serial.println("RFID ID: " + uid);
    client.publish(ticket_topic, "checked");
  }
  
  delay(50);
  client.loop();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("REV2_station")) {
      Serial.println("Connected to the MQTT Broker.");
      client.subscribe(topic);
      client.setCallback(callback);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  Serial.print(" - Message : ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  Serial.println(messageTemp);
  for(int i = 0; i < messageTemp.toInt(); i++){
    updateProgressBar(i, 100, 1);
  }
}

void updateProgressBar(unsigned long count, unsigned long totalCount, int lineToPrintOn){
    double factor = totalCount/80.0;          
    int percent = (count+1)/factor;
    int number = percent/4;
    int remainder = percent%4;
    if(number > 0)
    {
      lcd.setCursor(number-1,lineToPrintOn);
      lcd.write(5);
    }
   
      lcd.setCursor(number,lineToPrintOn);
      lcd.write(remainder);
      
 }

 void clearRow(byte row){
    lcd.setCursor(0,row);
    lcd.print("                    ");
 }

 String getUID(){
  String uid = "";
  for(int i = 0; i < rfid.uid.size; i++){
    uid += rfid.uid.uidByte[i] < 0x10 ? "0" : "";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA();
  return uid;
}
