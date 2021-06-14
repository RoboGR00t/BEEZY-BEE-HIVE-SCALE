#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHT_PIN 13
#define DHTTYPE DHT22   // DHT 22  (AM2302)

// Initialize DHT sensor for normal 16mhz Arduino:
DHT dht = DHT(DHT_PIN, DHTTYPE);

void dht22_init(void){
  dht.begin();
  delay(10000);
}

float get_temperature(void){
  delay(2000);
  return dht.readTemperature();
}

float get_humidity(void){
  delay(2000);
  return dht.readHumidity();
}
