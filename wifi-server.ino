/*
  Sketch: 
  RFID TAG Reader WIFI Server
  
  Description: 
  This is a simple WIFI server used to capture data from  an RFID tag reader for the 
  purpose of monitoring employees' attendance.  The registration and verification of 
  the RFID UUID's is handled using a NestJs Microservices API and a NOSQL Database.
  <link to nestjs microservices api>   
  
  Circuit:
  ESP8266 WIFI Module
  MRFC522 RFID 13.56 MHz Tag Reader
  <link to circuit diagram>

  Created 02/01/20
  By Leon Lafayette
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266HTTPClient.h>

const int NOT_FOUND_LED = 2;
const int HEALTHY_LED = 16;
const int SS_PIN = 4; 
const int RST_PIN = 5;
const int BAUD = 9600;
const int PORT = 200;
const String TEST_UUID = "8B 00 42 0E";
const String SSID = "your ssid here";
const String PASSWORD = "your password here";

ESP8266WebServer server(PORT);
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup(void) {
  pinMode(HEALTHY_LED, OUTPUT);
  pinMode(NOT_FOUND_LED, OUTPUT);
  digitalWrite(HEALTHY_LED, HIGH);
  digitalWrite(NOT_FOUND_LED, LOW);
  Serial.begin(BAUD);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");
  SPI.begin(); 

  mfrc522.PCD_Init(); 
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleHealthCheck);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  monitorMDNS();
  monitorRFID();
}

void monitorMDNS() {
  server.handleClient();
  MDNS.update();
}

void handleHealthCheck() {
  digitalWrite(HEALTHY_LED, HIGH);
  digitalWrite(NOT_FOUND_LED, LOW);
  server.send(200, "text/plain",  "RFID WIFI Server OK");
}

void handleNotFound() {
  digitalWrite(HEALTHY_LED, LOW);
  String message = "Page Not Found\n\n";
  server.send(404, "text/plain", message);
  digitalWrite(NOT_FOUND_LED, HIGH);
}

void monitorRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
 
  if (!mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
 
  Serial.println();
  Serial.print(" UID tag :");
  
  String data= "";
  
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     data.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     data.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  data.toUpperCase();
  Serial.println();
  
  String scannedUUID = data.substring(1);
  
  if (scannedUUID == TEST_UUID)
  {
    Serial.println(" Access Granted ");
    Serial.println();
    delay(3000);
    HTTPClient http;
 
    http.begin("http://06df8392.ngrok.io"); 
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String body = "uuid="+scannedUUID;  //Body to send to API
    int httpCode = http.POST(body);     //Send the request
    String payload = http.getString();  //Get the response payload
 
    Serial.println(httpCode);           //Print HTTP return code
    Serial.println(payload);            //Print request response payload
 
    http.end();                         //Close connection
  }
  
  else   {
    Serial.println(" Access Denied ");
    delay(3000);
  }
}
