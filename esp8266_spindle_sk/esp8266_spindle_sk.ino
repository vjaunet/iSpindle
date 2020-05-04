/* =========================================================================
//based on  Basic ESP8266 MQTT example
//https://www.hackster.io/chris-topher-slater/brew-home-hydrometer-622973


==========================================================================*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "MPU6050.h"

//--- esp8266 Deep Sleep parameters
#define DEEP_SLEEP_
#define sleepTimeSec  900

//Declare MPU650
MPU6050 mpu;

// PubSub mqtt data
const char* IOTdevice = "HYDRO";
const char* mqttTopic1  = "Hydro/TILT"  ;    //send angle
const char* mqttTopic2  = "Hydro/TEMP" ;     //send temperature


// Update these with values suitable for your network.
#define _WiFiTrials_ 6

const char* ssid = "EetV-WiFi-RdC";
const char* password = "***************";
const char* mqtt_server = "192.168.1.55";// no need to put ":1883"

const char* ssidFB = "E&V-WiFi";
const char* ssidFB2 = "E&V-WiFi_2GEXT";

WiFiClient espClient;
PubSubClient client(espClient);

/* useful wait without delay */
void wait_ms_(uint32_t delay){
  uint32_t start_time=millis();
  while(millis()-start_time < delay);
  return;
}


void tryConnectWifi(const char *ssid, const char *pass){

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //Try connection on main WiFi
  WiFi.begin(ssid, password);
  int WiFitrial=0;
  while (WiFi.status() != WL_CONNECTED &&
	 WiFitrial < _WiFiTrials_ ) {
    delay(2000);
    Serial.print(".");
    WiFitrial++;
  }

}


/* WiFi conection */
void setup_wifi() {

  wait_ms_(10);

  //WiFi.persistent( false );

  tryConnectWifi(ssid,password);
  if (WiFi.status() != WL_CONNECTED)
    tryConnectWifi(ssidFB,password);
  if (WiFi.status() != WL_CONNECTED)
    tryConnectWifi(ssidFB2,password);

  if (WiFi.status() != WL_CONNECTED){
#ifdef DEEP_SLEEP_

    Serial.println("");
    Serial.println("Could not connect");
    Serial.println("Deep Sleep....");

    //redo the WiFi config
    wifi_station_set_auto_connect(false);

    // ESP8266 GO TO DEEPSLEEP
    ESP.deepSleep( sleepTimeSec * 1e6, WAKE_RF_DEFAULT );
#endif
  }



  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //save the WiFi config
  wifi_station_set_auto_connect(true);

}


/* Connection to the mqtt server */
void reconnect() {

  uint8_t N_times=0;

  // Loop until we're reconnected
  while (!client.connected() &&
	 N_times < _WiFiTrials_) {
    Serial.print("Attempting MQTT connection...");

    // Create a client ID
    String clientId = IOTdevice;

    // Attempt to connect
    if (client.connect(clientId.c_str())) {

      Serial.println("connected");

    } else {

      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");

      // Wait 1 seconds before retrying
      delay(2000);
      yield();
      N_times++;
    }
  }

#ifdef DEEP_SLEEP_
  if (N_times >= _WiFiTrials_){
    //redo the WiFi config
    wifi_station_set_auto_connect(false);

    // Esp8266 GO TO DEEPSLEEP
    Serial.println("Going to sleep - Mqtt not Connected...");
    ESP.deepSleep( sleepTimeSec * 1e6, WAKE_RF_DEFAULT );
  }
#endif

}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// SET-UP
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void setup() {

  Serial.begin(9600);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);

  //initializing the MPU6050 -------
  Wire.begin(0,2);
  Wire.setClock(400000);
  mpu.initialize();

}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// LOOP
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
static uint32_t lastMsg=0;

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  // wake the MPU up  and get data -----------
  mpu.setSleepEnabled(false);
  wait_ms_(50);//wait a bit for the device to wake up

  //temperature in degrees C from datasheet
  float Temp = getTempAvg()/340.00 + 36.2;

  float Tilt = getTiltAvg();
  Tilt = asin(Tilt/16384.)*180./3.1416;

  //MPU go to sleep again
  mpu.setSleepEnabled(true);

  // send the messages to the mqtt server----
  char msg[50];
  snprintf (msg, 50, "%f", Tilt);
  Serial.print("Tilt message: ");
  Serial.print(mqttTopic1);
  Serial.println(msg);
  client.publish(mqttTopic1, msg);     // Publish Tilt

  client.loop(); // keep the mqtt client connected and flush msgs
  wait_ms_(300);

  Serial.print("Temp message: ");
  snprintf (msg, 50, "%5.2f", Temp);
  Serial.print(mqttTopic2);
  Serial.println(msg);
  client.publish(mqttTopic2, msg);    // Publish Temp

  client.loop(); // keep the mqtt client connected and flush msgs
  wait_ms_(300);

#ifdef DEEP_SLEEP_

  // Esp8266 GO TO DEEPSLEEP
  Serial.println("Going to sleep Now...");
  ESP.deepSleep( sleepTimeSec * 1e6, WAKE_RF_DEFAULT );

#else

  //wait idle -- We should go to sleep instead --
  uint32_t now = millis();
  while (now - lastMsg < 2000){
    return;
  }
  lastMsg = now;

#endif


}

//============== Get average Acx values=========== ==============
//
//===============================================================
#define Nsamp 200
float getTiltAvg(){

  float meanTilt = 0;

  for (uint8_t i=0;i < Nsamp; i++){
    meanTilt += (float) mpu.getAccelerationX();
    wait_ms_(10); //wait for new data to come in
  }

  return meanTilt /= (float) Nsamp;

}

#define Nsamp_temp 100
float getTempAvg(){

  float meanTemp = 0;

  for (uint8_t i=0;i < Nsamp_temp; i++){
    meanTemp += ((float) mpu.getTemperature());
    wait_ms_(10); //wait for new data to come in
  }

  return meanTemp /= (float) Nsamp_temp;

}


//============== Call back function for suscribers ==============
//        called when a mqtt message arrives
//===============================================================
//void callback(String topic, byte* payload, unsigned int length) {

//   Serial.print("Message arrived [");
//   Serial.print(topic);
//   Serial.print("] ");

//   String keeper;
//   for (int i = 0; i < length; i++) { // Used to concatanate the message
//     keeper += (char)payload[i];
//   }

//   Serial.println(keeper); //debug check for data captured

//   if (topic == subBATT) {  // Battery Saver data
//     KeepTime = keeper.toInt();
//     KeepTime = KeepTime * 3600000; // scale hours to milliseconds

//     Serial.print("                                               KeepTime  =  ");
//     Serial.println(KeepTime);
//   }

//}
