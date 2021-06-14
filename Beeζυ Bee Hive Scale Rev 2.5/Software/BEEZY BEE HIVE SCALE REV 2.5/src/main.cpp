/*
*
*            ╔╗ ┌─┐┌─┐┌─┐┬ ┬
*           ╠╩╗├┤ ├┤ ┌─┘└┬┘
*          ╚═╝└─┘└─┘└─┘ ┴
*    ╔╗ ┌─┐┌─┐  ╦ ╦┬┬  ┬┌─┐  ╔═╗┌─┐┌─┐┬  ┌─┐
*   ╠╩╗├┤ ├┤   ╠═╣│└┐┌┘├┤   ╚═╗│  ├─┤│  ├┤
*  ╚═╝└─┘└─┘  ╩ ╩┴ └┘ └─┘  ╚═╝└─┘┴ ┴┴─┘└─┘
*
* @author : Georgios Roumeliotis
* @version : 2.5.0
*
*/

/* DEFINES && CONST --> */

/* 1 MINUTE IN MICROSECONDS --> */
#define MINUTES 60000000
/* <-- 1 MINUTE IN MICROSECONDS */


/* ESP32 PINS --> */
#define LOADCELL_DOUT_PIN  14
#define LOADCELL_SCK_PIN  25
#define SETUP_PIN 0
#define INDICATOR 12
#define BATTERY_PIN 33
#define DHTPIN 15
#define DHTTYPE DHT22
/* <-- ESP32 PINS */

/* Configure TinyGSM library */
#define TINY_GSM_MODEM_SIM800      /* Modem is SIM800 */
#define TINY_GSM_RX_BUFFER   1024  /* Set RX buffer to 1Kb */
/**/


/* TTGO T-Call pins */
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
/**/

/* Set serial for debug console (to Serial Monitor, default speed 115200) */
#define SerialMon Serial
/* Set serial for AT commands (to SIM800 module) */
#define SerialAT  Serial1


#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

/* This value is obtained using the SparkFun_HX711_Calibration sketch */
#define calibration_factor  -5290
/* This large value is obtained using the SparkFun_HX711_Calibration sketch */
#define zero_factor 8315300

const char* ssid = "Beezy";
const char* password = "B3EZY!";
/* <-- DEFINES && CONST */

/* LIBRARIES --> */
#include <Arduino.h>
#include <HX711.h>
#include <soc/rtc.h>
#include <Wire.h>
#include <TinyGsmClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
/* <-- LIBRARIES */

/* MY LIBRARIES & HEADERS */
#include <BeezyManagerWebpage.h>
/* <-- MY LIBRARIES & HEADERS */

/* GLOBAL --> */
int SETUP_MODE = false;

String LOCATION = "";
String THINGSPEAK = "";
String PHONE = "";
String MESSAGES = "";
String APN = "";
String PREFIX = "";

int DEEP_SLEEP_MINS = 1;

float SCALE_WEIGHT = 0.0;
float HUMIDITY = 0.0;
float TEMPERATURE = 0.0;
float BATTERY_PER = 0.0;
/* <-- GLOBAL*/

/* OBJECTS DECLARATION --> */
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HX711 scale;
DHT dht = DHT(DHTPIN, DHTTYPE);
/* <-- OBJECTS DECLARATION */

/* FUNCTIONS DECLARATION --> */
void indicator(int pin,int tempo);
bool setup_button(int pin, size_t time_to_wait);
bool save_data_to_file(String data, String file_path);
String get_data_from_file(String file_path);
void parse_data_to_global(String data);
void print_global(void);
bool iniialize_sim800l(String apn);
String sim800l_get_time(void);
bool setPowerBoostKeepOn(int en);
int next_message_wakeup(int messages, String time);
void deepsleep_handler(uint next_wakeup, uint messages);
bool send_sms(String target, String message);
String build_message(String name, int add_weight, int weight, float temperature, float humidity, float batterty_per );
bool upload_data_to_thingspeak(String api_key, int weight, float temperature, float humidity, float battery);
float last_scale_mesure(float w);
/* <-- FUNCTIONS DECLARATION */

/* SETUP FUNCTION --> */
void setup(){

  /* INITIALIAZE UART SERIAL COMMUNICATION WITH 115200 BAUDRATE -->*/
  Serial.begin(115200);
  /* <-- INITIALIAZE UART SERIAL COMMUNICATION WITH 115200 BAUDRATE */

  /* CONFIGURE PINS STATE --> */
  pinMode(SETUP_PIN,INPUT_PULLDOWN);
  pinMode(INDICATOR,OUTPUT);
  /* <-- CONFIGURE PINS STATE */

  /* BLINK LED ONCE FOR ONE SECOND --> */
  indicator(INDICATOR,1000);
  /* <-- BLINK LED ONCE FOR ONE SECOND */

  /* WAIT FOR UART COMMUNICATION TO ESTABLISHED --> */
  while(!Serial) { }
  /* <-- WAIT FOR UART COMMUNICATION TO ESTABLISHED */

  Serial.println("");
  Serial.println("\n[+] --> JUST WAKE UP FROM DEEPSLEEP ");
  Serial.println("\n[+]--> SPIFFS INITIALIZE");


  /* INITIALIZE SPI FlASH FILE SYSTEM --> */
  if(!SPIFFS.begin(true)){
    Serial.println("AN ERROR HAS OCCURED WHILE MOUNTING SPIFFS");
  }
  /* <-- INITIALIZE SPI FlASH FILE SYSTEM */

  /* WAIT ONE SECOND --> */
  delay(1000);
  /* <-- WAIT ONE SECOND */

  /* CHECK UP TO 5 SECONDS IF USER PRESS THE SET UP BUTTON --> */
  SETUP_MODE = setup_button(SETUP_PIN,5);
  /* <-- CHECK UP TO 5 SECONDS IF USER PRESS THE SET UP BUTTON */


  Serial.println(String("[+]--> SETUP_BUTTON : ") + (SETUP_MODE? "TRUE" : "FALSE"));
  Serial.println("\n[+]--> PARSE DATA FROM FILE");


  /* GET CONFIGURATION DATA FROM SPIFFS AND PARSE THEM TO THE GLOBAL VARIABLES --> */
  parse_data_to_global(get_data_from_file("/data.txt"));
  /* <-- GET CONFIGURATION DATA FROM SPIFFS AND PARSE THEM TO THE GLOBAL VARIABLES */

  /* IF SETUP BUTTON IS NOT PRESSED START SIM800L AND DEEPSLEEP HANDLER ELSE START CONFIGURATION WEB SERVER --> */
  if(!SETUP_MODE){


    Serial.println("\n[+] --> WAIT SIM800L TO CONNECT \n");


    /* INITIALIZE SIM800L AND WAIT TO CONNECT TO THE NETWORK --> */
    iniialize_sim800l(APN);
    /* <-- INITIALIZE SIM800L AND WAIT TO CONNECT TO THE NETWORK */

    /**/
    Serial.println("\n[+]--> DEEP SLEEP HANDLER");

    /* GET MESSAGES NUMBER FROM THE CONFIGURATION FILE - GET TIME FROM THE SIM800L - CALCULATE WHEN THE NEXT MESSAGE
    IS GOING TO SEND AND PARSE THAT IN THE DEEPSLEEP HANDLER.IF IS NOT THE TIME FOR THE MESSAGE TO BE SEND DEEPSLEEP
    FOR THE REMAINING IF IT IS LESS THAN A HOUR ELSE DEEPSLEEP FOR ONE HOUR --> */
    deepsleep_handler(next_message_wakeup(MESSAGES.toInt(),sim800l_get_time()),MESSAGES.toInt());
    /* <-- GET MESSAGES NUMBER FROM THE CONFIGURATION FILE - GET TIME FROM THE SIM800L - CALCULATE WHEN THE NEXT MESSAGE
    IS GOING TO SEND AND PARSE THAT IN THE DEEPSLEEP HANDLER.IF IS NOT THE TIME FOR THE MESSAGE TO BE SEND DEEPSLEEP
    FOR THE REMAINING IF IT IS LESS THAN A HOUR ELSE DEEPSLEEP FOR ONE HOUR */
  }
  else{

    Serial.println("[+] --> START SET UP WEB SERVER ");


    /* START CONFIGURATION WEB SERVER AND SAVE CONFIGURATION DATA TO SPIFFS AND RESET LAST MEASURE VALUE --> */
    save_data_to_file(Beezy_Web_Server(ssid,password),"/data.txt");
    save_data_to_file("0.0", "/last_mesure.txt");
    /* <-- START CONFIGURATION WEB SERVER AND SAVE CONFIGURATION DATA TO SPIFFS AND RESET LAST MEASURE VALUE */

    /* SERIAL PRINTS FOR DEBBUGING REASONS --> */
    Serial.println("[+] --> CLOSE SET UP WEB SERVER ");
    /* <-- SERIAL PRINTS FOR DEBBUGING REASONS */

  }
  /* <-- IF SETUP BUTTON IS NOT PRESSED START SIM800L AND DEEPSLEEP HANDLER ELSE START CONFIGURATION WEB SERVER */

  /* SLOW DOWN CPU CLOCK FREQUENCY TO 80 MHZ FOR BETTER READING FROM THE LOADCELL --> */
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
  /* <-- SLOW DOWN CPU CLOCK FREQUENCY TO 80 MHZ FOR BETTER READING FROM THE LOADCELL */

  /* SERIAL PRINTS FOR  REASONS --> */
  Serial.println("\n[+]--> SENSORS INITIALIZE");
  Serial.println("[+]--> INITIALIZING DHT22");
  /* <-- SERIAL PRINTS FOR DEBBUGING REASONS */

  /* INITIALIZING DHT22 TEMPERATURE AND HUMIDITY SENSOR --> */
  dht.begin();
  /* INITIALIZING DHT22 TEMPERATURE AND HUMIDITY SENSOR --> */

  /* INITIALIZING HX711 SENSOR --> */

  Serial.println("[+]--> INITIALIZING SCALE");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Serial.println("[+]--> INITIALIZING SET SCALE");

  /*  THESE VALUES ARE OBTAINED USING THE SPARKFUN_HX711_CALIBRATION SKETCH
  (https://github.com/sparkfun/HX711-Load-Cell-Amplifier/blob/master/firmware/SparkFun_HX711_Calibration/SparkFun_HX711_Calibration.ino) --> */
  scale.set_scale(calibration_factor);
  Serial.println("[+]--> INITIALIZING SET OFFSET");
  scale.set_offset(zero_factor); /* ZERO OUT THE SCALE USING PREVIOUSLY KNOWN ZERO FACTOR */
  /* <-- THESE VALUES ARE USING THE SPARKFUN_HX711_CALIBRATION SKETCH
  (https://github.com/sparkfun/HX711-Load-Cell-Amplifier/blob/master/firmware/SparkFun_HX711_Calibration/SparkFun_HX711_Calibration.ino) */

  /* <-- INITIALIZING HX711 SENSOR */

  /* GET CONFIGURATION DATA FROM SPIFFS AND PARSE THEM TO THE GLOBAL VARIABLES --> */
  parse_data_to_global(get_data_from_file("/data.txt"));
  /* <-- GET CONFIGURATION DATA FROM SPIFFS AND PARSE THEM TO THE GLOBAL VARIABLES */

  /* PRINT GLOBAL VARIABLES FOR DEBUGGING --> */
  print_global();
  /* <-- PRINT GLOBAL VARIABLES FOR DEBUGGING */

  Serial.println("\n[+] --> GET READINGS FROM SENSORS  ");

  /* GATHER ALL DATA FROM THE SENSORS --> */
  HUMIDITY = dht.readHumidity();
  TEMPERATURE = dht.readTemperature();
  BATTERY_PER = map(analogRead(BATTERY_PIN), 0.0f, 4095.0f, 0, 100);
  SCALE_WEIGHT = scale.get_units()*453.592; /* TURN LIBS  TO KG */
  /* GATHER ALL DATA FROM THE SENSORS --> */

  /* CALCULATE HOW MUCH WEIGHT IS ADDED SINCE LAST MEASURE --> */
  float SCALE_ADD_WEIGHT = last_scale_mesure(SCALE_WEIGHT);
  /* <-- CALCULATE HOW MUCH WEIGHT IS ADDED SINCE LAST MEASURE */

  Serial.println("\n[+] --> WAIT SIM800L TO CONNECT \n");

  /* INITIALIZE SIM800L AND WAIT TO CONNECT TO THE NETWORK --> */
  iniialize_sim800l(APN);
  /* <-- INITIALIZE SIM800L AND WAIT TO CONNECT TO THE NETWORK */

  /* SEND MESSAGE VIA SMS OR / AND THINGSPEAK DEPENDING ON PREFIX VALUE --> */
  if(PREFIX == "sms" || PREFIX == "sms_thingspeak"){
    while(!send_sms("+30" + PHONE,build_message(LOCATION,SCALE_ADD_WEIGHT,SCALE_WEIGHT,TEMPERATURE,HUMIDITY,BATTERY_PER))){
      Serial.println("\n[+] --> SENDING SMS ...");
      delay(500);
    }
    Serial.println("\n[+] --> SMS SEND");
    delay(1000);
  }

  if(PREFIX == "thingspeak" || PREFIX == "sms_thingspeak"){
    while(!upload_data_to_thingspeak(THINGSPEAK,SCALE_WEIGHT,TEMPERATURE,HUMIDITY,BATTERY_PER)){
      Serial.println("\n[+] --> SENDING DATA TO THINGSPEAK ... \n");
      delay(500);
    }
    iniialize_sim800l(APN);
    Serial.println("\n[+] --> DATA TO THINGSPEAK SEND \n");
  }
  /* <-- SEND MESSAGE VIA SMS OR / AND THINGSPEAK DEPENDING ON PREFIX VALUE --> */

  /* CALCULATE NEXT WAKEUP IN MINUTES --> */
  int mins = next_message_wakeup(MESSAGES.toInt(),sim800l_get_time());
  /* <--  CALCULATE NEXT WAKEUP IN MINUTES */

  /* IF NEXT WAKEUP IS MORE THAN 60 MINUTES DEEPSLEEP FOR 60 MINUTES ELSE DEEPSLEEP FOR THE NUMBER OF MINUTES --> */
  if(mins > 60){
    esp_sleep_enable_timer_wakeup(3600000000);
    Serial.println("\n\n[+] --> DEEPSLEEP FOR 60 MINUTES");
  }
  else{
    esp_sleep_enable_timer_wakeup(mins*60000000);
    Serial.println("\n\n[+] --> DEEPSLEEP FOR "+String(mins)+" MINUTES");
  }
  esp_deep_sleep_start();
  /* <-- IF NEXT WAKEUP IS MORE THAN 60 MINUTES DEEPSLEEP FOR 60 MINUTES ELSE DEEPSLEEP FOR THE NUMBER OF MINUTES */

}
/* <-- SETUP FUNCTION */

/* LOOP FUNCTION --> */
void loop(){}
/* <-- LOOP FUNCTION */



/********************************************** CUSTOM FUNCTIONS ****************************************************/


/* TOGGLE A PIN FOR A SPECIFIC TIME --> */
void indicator(int pin,int tempo){
  delay(tempo);
  digitalWrite(pin, HIGH);
  delay(tempo);
  digitalWrite(pin, LOW);
}
/* TOGGLE A PIN FOR A SPECIFIC TIME --> */

/* WAIT UP TO (time_to_wait) SECONDS FOR PIN TO GET HIGH (RETURNS PIN STATUS) --> */
bool setup_button(int pin, size_t time_to_wait){
  int status = 0;
  for (size_t i = 0; i < time_to_wait; i++) {
    status = digitalRead(pin);
    delay(10);
    if(status == 1){
      return status;
    }
    delay(990);
  }

  return status;
}
/* <-- WAIT UP TO (time_to_wait) SECONDS FOR PIN TO GET HIGH (RETURNS PIN STATUS) */

/* SAVE DATA TO A SPECIFIED FILE IN SPIFFS --> */
bool save_data_to_file(String data, String file_path){
  File write = SPIFFS.open(file_path, "w");
  write.print(data);
  write.close();
  return true;
}
/* <-- SAVE DATA TO A SPECIFIED FILE IN SPIFFS */

/*  RETURNS THE DATA OF A SPECIFIED FILE --> */
String get_data_from_file(String file_path){
  File read = SPIFFS.open(file_path, "r");
  String FILE_STRING = "";
  while (read.available()) {
    FILE_STRING +=  read.readString();
  }

  read.close();

  return FILE_STRING;
}
/* <--  RETURNS THE DATA OF A SPECIFIED FILE */

/* PARSE DATA TO THE GLOBAL VARIABLES --> */
void parse_data_to_global(String data){
  String msg = data;
  byte separator = msg.indexOf('|');
  LOCATION = msg.substring(0,separator);
  msg = msg.substring(separator+1);
  separator = msg.indexOf('|');
  THINGSPEAK = msg.substring(0,separator);
  msg = msg.substring(separator+1);
  separator = msg.indexOf('|');
  PHONE = msg.substring(0,separator);
  msg = msg.substring(separator+1);
  separator = msg.indexOf('|');
  MESSAGES = msg.substring(0,separator);
  msg = msg.substring(separator+1);
  separator = msg.indexOf('|');
  APN = msg.substring(0,separator);
  msg = msg.substring(separator+1);
  separator = msg.indexOf('|');
  PREFIX = msg.substring(0,separator);
}
/* <-- PARSE DATA TO THE GLOBAL VARIABLES */

/* PRINT GLOBAL VARIAL ON THE SERIAL MONITOR --> */
void print_global(void){
  Serial.print("LOCATION : ");
  Serial.println(LOCATION +"|");
  Serial.print("THINGSPEAK : ");
  Serial.println(THINGSPEAK +"|");
  Serial.print("PHONE : ");
  Serial.println(PHONE +"|");
  Serial.print("MESSAGES : ");
  Serial.println(MESSAGES +"|");
  Serial.print("APN : ");
  Serial.println(APN +"|");
  Serial.print("PREFIX : ");
  Serial.println(PREFIX +"|");
}
/* <-- PRINT GLOBAL VARIAL ON THE SERIAL MONITOR */

/* SET POWER  BOOST KEEP ON (ESP32 TTGO CALL USES THE IP5306 BATTERY MANAGER) --> */
bool setPowerBoostKeepOn(int en){
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if (en) {
    Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    Wire.write(0x35); // 0x37 is default reg value
  }
  return Wire.endTransmission() == 0;
}
/* <-- SET POWER  BOOST KEEP ON (ESP32 TTGO CALL USES THE IP5306 BATTERY MANAGER) */

/* INITIALIZE SIM800L AND START GPRS CONNECTION IF A APN CODE IS PROVIDED --> */
bool iniialize_sim800l(String apn = ""){
  /* INITIALIZE I2C COMMUNICATION FOR THE IP5306 --> */
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(500);
  /* <-- INITIALIZE I2C COMMUNICATION FOR THE IP5306 */

  bool isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KEEP ON ") + (isOk ? "OK" : "FAIL"));

  /* SET MODEM RESET , ENSABLE AND POWER PINS --> */
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
  /* <-- SET MODEM RESET , ENSABLE AND POWER PINS */

  /* SET GSM MODULE BAUDERATE AND UART PINS --> */
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  /* <-- SET GSM MODULE BAUDERATE AND UART PINS */

  /* INITIALIZE MODEM --> */
  modem.init();
  /* <-- INITIALIZE MODEM */

  /* RETURN THE STATUS IF GPRS IS CONNECTED --> */
  if (!modem.gprsConnect(apn.c_str())) {
    return false;
  }
  else {
    return true;
  }
  /* <-- RETURN THE STATUS IF GPRS IS CONNECTED */

}
/* <-- INITIALIZE SIM800L AND START GPRS CONNECTION IF A APN CODE IS PROVIDED */

/* RETURN TIME FROM SIM800L --> */
String sim800l_get_time(void){
  String time = modem.getGSMDateTime(DATE_TIME);
  time = time.substring(0,8);
  return time;
}
/* <-- RETURN TIME FROM SIM800L */

/* RETURNS HOW MUCH TIME IS LEFT FOR THE NEXT MESSAGE TO BE SEND --> */
int next_message_wakeup(int messages, String time){

  int next_message = 0;

  int hour = 0;
  int minutes = 0;


  hour = time.substring(0,2).toInt();
  minutes = time.substring(3,5).toInt();
  Serial.print("HOUR : ");
  Serial.println(time.substring(0,2));

  Serial.print("MINUTES : ");
  Serial.println(time.substring(3,5));


  if(messages == 1){
    /* 21:00:00   24h */

    if(hour >= 21){
      next_message = ((24-hour+21)*60) - minutes;
    }
    else{
      next_message = (21*60) - ((hour*60)+minutes);
    }

  }
  else if(messages == 2){
    /* 09:00:00 - 21:00:00   12h */

    int next_09 = 0;
    int next_21 = 0;

    if(hour >= 9){
      next_09 = ((24-hour+9)*60) - minutes;
    }
    else{
      next_09 = (9*60) - ((hour*60)+minutes);
    }

    if(hour >= 21){
      next_21 = ((24-hour+21)*60) - minutes;
    }
    else{
      next_21 = (21*60) - ((hour*60)+minutes);
    }

    if(next_09 < next_21){
      next_message = next_09;
    }
    else{
      next_message = next_21;
    }

  }
  else if(messages == 3){

    int next_05 = 0;
    int next_21 = 0;
    int next_14 = 0;


    if(hour >= 5){
      next_05 = ((24-hour+5)*60) - minutes;
    }
    else{
      next_05 = (5*60) - ((hour*60)+minutes);
    }

    if(hour >= 21){
      next_21 = ((24-hour+21)*60) - minutes;
    }
    else{
      next_21 = (21*60) - ((hour*60)+minutes);
    }


    if(hour >= 14){
      next_14 = ((24-hour+14)*60) - minutes;
    }
    else{
      next_14 = (14*60) - ((hour*60)+minutes);
    }


    if(next_05 < next_21){
      next_message = next_05;
    }
    else{
      next_message = next_21;
    }


    if(next_14 < next_message){
      next_message = next_14;
    }

  }
  else if(messages == 4){

    int next_03 = 0;
    int next_15 = 0;
    int next_09 = 0;
    int next_21 = 0;


    if(hour >= 3){
      next_03 = ((24-hour+3)*60) - minutes;
    }
    else{
      next_03 = (3*60) - ((hour*60)+minutes);
    }

    if(hour >= 21){
      next_21 = ((24-hour+21)*60) - minutes;
    }
    else{
      next_21 = (21*60) - ((hour*60)+minutes);
    }

    if(hour >= 15){
      next_15 = ((24-hour+15)*60) - minutes;
    }
    else{
      next_15 = (15*60) - ((hour*60)+minutes);
    }

    if(hour >= 9){
      next_09 = ((24-hour+9)*60) - minutes;
    }
    else{
      next_09 = (9*60) - ((hour*60)+minutes);
    }


    if(next_03 < next_09){
      next_message = next_03;
    }
    else{
      next_message = next_09;
    }


    if(next_15 < next_message){
      next_message = next_15;
    }

    if(next_21 < next_message){
      next_message = next_21;
    }

  }


  Serial.print("NEXT MESSAGE : ");
  Serial.println(next_message);

  return next_message;

}
/* <-- RETURNS HOW MUCH TIME IS LEFT FOR THE NEXT MESSAGE TO BE SEND */

/* IF IT IS NOT THE TIME TO SEND THE MESSAGE IT STARTS DEEPSLEEP FOR THE REMAINING TIME MAX DEEPSLEEP TIME IS 60 MINUTES --> */
void deepsleep_handler(uint next_wakeup, uint messages){

  if(messages == 1 && (next_wakeup <= 1440 && next_wakeup >= 1435)){
    return;
  }

  if(messages == 2 && (next_wakeup <= 720 && next_wakeup >= 715)){
    return;
  }

  if(messages == 3 && (next_wakeup <= 480 && next_wakeup >= 475)){
    return;
  }

  if(messages == 4 && (next_wakeup <= 360 && next_wakeup >= 355)){
    return;
  }

  if(next_wakeup >= 60){
    esp_sleep_enable_timer_wakeup(3600000000);
    Serial.println("\n\n[+] --> DEEPSLEEP FOR 1 HOUR");
    esp_deep_sleep_start();

  }
  else if(next_wakeup < 60 && next_wakeup != 0){
    Serial.println("\n[+] --> DEEPSLEEP FOR " + String(next_wakeup) + " MINUTES");
    esp_sleep_enable_timer_wakeup(next_wakeup*60000000);
    esp_deep_sleep_start();
  }
  else{
    return;
  }

}
/* <-- IF IT IS NOT THE TIME TO SEND THE MESSAGE IT STARTS DEEPSLEEP FOR THE REMAINING TIME MAX DEEPSLEEP TIME IS 60 MINUTES */

/* SEND MESSAGE VIA SMS --> */
bool send_sms(String target, String message){
  if(modem.sendSMS(target, message)){
    return true;
  }
  else{
    return false;
  }
}
/* <-- SEND MESSAGE VIA SMS */

/* BUILD THE SMS MESSAGE TO BE SEND --> */
String build_message(String name, int add_weight, int weight, float temperature, float humidity, float batterty_per ){
  String BEEZY_SMS = "NAME : "+ name +
                     "\nADD WEIGHT : " + add_weight +" gr" +
                     "\nTOTAL WEIGHT : " + weight +" gr" +
                     "\nTEMPERATURE : " + temperature + " oC" +
                     "\nHUMIDITY : " + humidity + " %" +
                     "\nBATTERY : " + batterty_per + " %";
  return BEEZY_SMS;
}
/* <-- BUILD THE SMS MESSAGE TO BE SEND */

/* SEND DATA TO THINGSPEAK VIA GET REQUEST --> */
bool upload_data_to_thingspeak(String api_key, int weight, float temperature, float humidity, float battery){

  if (!client.connect("api.thingspeak.com", 80)) {
    return false;
  }

  String httpRequestData = "api_key=" + api_key + "&field1="+ String(weight) +
                                                  "&field2="+ String(temperature) +
                                                  "&field3="+ String(humidity) +
                                                  "&field4="+ String(battery);



  client.print(String("GET ") + "/update" + " HTTP/1.1\r\n");
  client.print(String("Host: ") + "api.thingspeak.com" + "\r\n");
  client.println("Connection: close");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(httpRequestData.length());
  client.println();
  client.println(httpRequestData);

  return true;
}
/* <-- SEND DATA TO THINGSPEAK VIA GET REQUEST */

/* RETURNS TO DIFFERENCE BETWEEN THE LAST WEIGHTING MEASUREMENT AND STORES LAST MEASURE TO SPIFFS --> */
float last_scale_mesure(float w){

  File read = SPIFFS.open("/last_mesure.txt","r");
  String weight = "";
  while (read.available()) {
    weight +=  read.readString();
  }
  read.close();

  float add_w = w - weight.toFloat();

  File write = SPIFFS.open("/last_mesure.txt","w");
  write.print(w);
  write.close();

  return add_w;
}
/* <-- RETURNS TO DIFFERENCE BETWEEN THE LAST WEIGHTING MEASUREMENT AND STORES LAST MEASURE TO SPIFFS */
