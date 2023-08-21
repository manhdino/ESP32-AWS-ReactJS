#include <Arduino.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#define WIFI_SSID "wifi_network_name"
#define WIFI_PASSWORD "wifi_password"

#define WS_PORT 433 //433 is default port for wss protocol
#define WS_HOST ""
#define WS_URL  ""
#define JSON_DOC_SIZE 2048
#define MSG_SIZE 256
WiFiMulti wifiMulti;
WebSocketsClient wsClient;

void sendErrorMessage(const char *error){
    char msg[MSG_SIZE];
    sprintf(msg,"{\"action\":\"msg\",\"type\":\"error\",\"body\":\"%s\"}",error);
    wsClient.sendTXT(msg);
}

uint8_t toMode(const char *val){
   if(strcmp(val,"output") == 0){
    return OUTPUT;
   }
    if(strcmp(val,"input_pullup") == 0){
    return INPUT_PULLUP;
   }
   return INPUT;
}
void sendACKMessage(){
    wsClient.sendTXT("{\"action\":\"msg\",\"type\":\"status\",\"body\":\"Successfully\"}");
}
void handleMessage(uint8_t * payload){
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    
     // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, payload);

    // Test if parsing succeeds.
    if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    sendErrorMessage(error.c_str());
    return;
   }

   //check type is c string type in json
   if(!doc["type"].is<const char *>()){
      sendErrorMessage("Invalid message type format");
      return;
   }

   if(strcmp(doc["type"],"cmd") == 0){
      if(!doc["body"].is<JsonObject>()){
        sendErrorMessage("Invalid command body");
        return;
      }
      if(strcmp(doc["body"]["type"],"pinMode") == 0){
          pinMode(doc["body"]["pin"],toMode(doc["body"]["mode"]));
          sendACKMessage();
          return;
      }
       if(strcmp(doc["body"]["type"],"digitalWrite") == 0){
          digitalWrite(doc["body"]["pin"],doc["body"]["value"]);
          sendACKMessage();
          return;
      }
       if(strcmp(doc["body"]["type"],"digitalRead") == 0){
          auto value = digitalRead(doc["body"]["pin"]);
           char msg[MSG_SIZE];
          sprintf(msg,"{\"action\":\"msg\",\"type\":\"output\",\"body\":\"%d\"}",value);
          wsClient.sendTXT(msg);
          return;
      }
        sendErrorMessage("Unsupported command type");
   }
      sendErrorMessage("Unsupported message type");
      return;
}

void onWsEvent(WStype_t type, uint8_t * payload, size_t length){
    switch(type)
    { 

    case WStype_DISCONNECTED:
    Serial.println("WS Disconnected");
    break;

    case WStype_CONNECTED:
    Serial.println("WS Connected");
    break;

    case WStype_TEXT:
    Serial.printf("WS Message: %s\n",payload);
    handleMessage(payload);
    break;

    default:
      break;
    }
}

void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  
  Serial.println("Connected");
 
 /*
  Establish a secure connection(SSL) with the websocket server
   the reason is that aws api gateway websockets gives us
   a secure url when once we create the server so we're gonna have
   to establish a secure connection
 */
  wsClient.beginSSL(WS_HOST,WS_PORT,WS_URL,"wss");
  
/*
  To receive any events from this websocket server through this websocket client
    we need to call on event method and provide an event handler
*/
  wsClient.onEvent(onWsEvent);


}

void loop() {
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED);
  wsClient.loop();//Keep connection alive
}