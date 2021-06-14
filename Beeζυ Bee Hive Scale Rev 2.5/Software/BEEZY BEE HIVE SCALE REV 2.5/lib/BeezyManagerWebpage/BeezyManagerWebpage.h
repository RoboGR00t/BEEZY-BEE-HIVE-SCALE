/* LIBRARIES --> */
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
/* <-- LIBRARIES */

/* GLOBAL --> */
bool SETUP_MESSAGE = false;
String WEB_SERVER_DATA = "";
/* <-- GLOBAL */

/* ASYNC WEB SERVER OBJECT INITIALIZATION ON PORT 80 --> */
AsyncWebServer server(80);
/* <-- ASYNC WEB SERVER OBJECT INITIALIZATION ON PORT 80 */

/* ASYNC WEB SOCKET OBJECT INITIALIZATION --> */
AsyncWebSocket ws("/ws");
/* <-- ASYNC WEB SOCKET OBJECT INITIALIZATION */

/* FUNCTIONS DECLARATION --> */
void initWebSocket();
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,void *arg, uint8_t *data, size_t len);
String Beezy_Web_Server(const char* ssid, const char* passwd);
/* <-- FUNCTIONS DECLARATION*/

/* WEB SERVER HANDLER --> */
String Beezy_Web_Server(const char* ssid, const char* passwd){

  /* SET ESP32 AS A WIFI ACCESS POINT --> */
  WiFi.mode(WIFI_AP);
  /* <-- SET ESP32 AS A WIFI ACCESS POINT */

  /* SET THE SSID AND PASSWORD FOR THE ACCESS POINT --> */
  WiFi.softAP(ssid, passwd);
  /* <-- SET THE SSID AND PASSWORD FOR THE ACCESS POINT */

  /* INITIALIZING WEB SOCKET --> */
  initWebSocket();
  /* <-- INITIALIZING WEB SOCKET */

  /* GET WEB PAGE FILES FOR THE WEB SERVER FROM SPIFFS --> */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html","text/html");
  });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/styles.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/javascript");
  });
  server.on("/beezy.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/beezy.png", "image/png");
  });
  /* <-- GET WEB PAGE FILES FOR THE WEB SERVER FROM SPIFFS */

  /* START WEB SERVER --> */
  server.begin();
  /* <-- START WEB SERVER */

  /* WAIT FOR THE USER TO SEND CONFIGURATION DATA & BLINK THE INDICATION LED ON THE BOARD --> */
  while (SETUP_MESSAGE == false){
    delay(500);
    digitalWrite(12, HIGH);
    delay(500);
    digitalWrite(12, LOW);
    Serial.println("WAIT FOR SETUP TO FINISH");
  }
  /* <-- WAIT FOR THE USER TO SEND CONFIGURATION DATA & BLINK THE INDICATION LED ON THE BOARD */

  /* STOP WEB SERVER --> */
  server.end();
  /* <-- STOP WEB SERVER */

  /* STOP WIFI ACCESS POINT FROM ESP32 --> */
  WiFi.softAPdisconnect(true);
  /* <-- STOP WIFI ACCESS POINT FROM ESP32 */

  /* RETURN THE CONFIGURATION DATA --> */
  return WEB_SERVER_DATA;
  /* <-- RETURN THE CONFIGURATION DATA */

}
/* <-- WEB SERVER HANDLER */

/* IMPLEMENT THE WEB SOCKET EVENT --> */
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {

   String msg = (const char*)  data; /* DATA SENDEND FROM THE USER VIA THE WEB PAGE */
   String write_file = msg;
   Serial.println(write_file);

   String location = "";
   String thingspeak = "";
   String phone = "";
   String messages = "";
   String apn = "";
   String prefix = "";

   /* SEPARATE DATA TO MULTIPLE VARIABLES --> */
   byte separator = msg.indexOf('|');
   location = msg.substring(0,separator);
   msg = msg.substring(separator+1);
   separator = msg.indexOf('|');
   thingspeak = msg.substring(0,separator);
   msg = msg.substring(separator+1);
   separator = msg.indexOf('|');
   phone = msg.substring(0,separator);
   msg = msg.substring(separator+1);
   separator = msg.indexOf('|');
   messages = msg.substring(0,separator);
   msg = msg.substring(separator+1);
   separator = msg.indexOf('|');
   apn = msg.substring(0,separator);
   msg = msg.substring(separator+1);
   separator = msg.indexOf('|');
   prefix = msg.substring(0,separator);
   /* <-- SEPARATE DATA TO MULTIPLE VARIABLES */

   /* CHECK IF DATA WHERE SEND CORRECTLY AND WE HAVE NOT DATA LOSES --> */
   if(location != "" && thingspeak != "" && phone != "" && messages != ""){
     WEB_SERVER_DATA = location+"|"+thingspeak+"|"+phone+"|"+messages+"|"+apn+"|"+prefix+"|";
     SETUP_MESSAGE = true;
   }
   /* <-- CHECK IF DATA WHERE SEND CORRECTLY AND WE HAVE NOT DATA LOSES */

}
/* <-- IMPLEMENT THE WEB SOCKET EVENT */

/* INITIALIZING WEB SOCKETS AND WEB SERVER --> */
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
/* <-- INITIALIZING WEB SOCKETS AND WEB SERVER */
