#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ThingSpeak.h>
#include <WiFiClient.h>
#define ESP8266BOARD
#define channelID 1539652
#define mqttPort 1883
#define SENSOR D2

//kredensial WiFi
char ssid[] = "Hotspot-laptop";
char pass[] = "bismillahlulus2021";
//char ssid[] = "1245";
//char pass[] = "lalalayeyeye";
WiFiClient client;

//kredensial publish dan subscribe pada ThingSpeak
const char mqttUserName[] = "OTsJBRMIOTUPIi4nIAAqAi4";
const char clientID[] = "OTsJBRMIOTUPIi4nIAAqAi4";
const char mqttPass[] = "zFDQsQQJyIVdZO0AtImu8KCa";

//parameter koneksi 
const char* server = "mqtt3.thingspeak.com";
int status = WL_IDLE_STATUS;
long lastPublishMillis = 0;
int connectionDelay = 1;
int updateInterval = 15;
PubSubClient mqttClient(client);

//parameter waterflow sensor
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRateInLitres;
float flowRateInMilliLitres;
float totalUsageInMilliLitres;
float tarifPenggunaan;
float totalLitres;

//fungsi pulseCounter waterflowsensor
void IRAM_ATTR pulseCounter(){
    pulseCount++;
}

//fungsi callback
void mqttSubscriptionCallback(char* topic, byte* payload, unsigned int length){
    // pesan yang muncul di serial monitor
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i=0; i<length; i++){
        Serial.print((char)payload[i]);
    }
}

//langganan ThingSpeak untuk update
void mqttSubscribe(long subChannelID){
    String myTopic = "channels/"+String(subChannelID)+"/subscribe";
    mqttClient.subscribe(myTopic.c_str());
}

//publish pesan ke channel ThingSpeak
void mqttPublish(long pubChannelID, String message){
    String topicString = "channels/" + String(pubChannelID) + "/publish";
    mqttClient.publish(topicString.c_str(), message.c_str());
}

//terhubung ke wifi
void connectWifi(){
    Serial.print("Connecting to Wifi");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED){
        delay (500);
        Serial.print (WiFi.status());
    }
    Serial.println("Connected to WiFi.");
}

//connecting ke mqtt server
void mqttConnect(){
    while ( !mqttClient.connected() ){
        if (mqttClient.connect(clientID, mqttUserName, mqttPass)){
            Serial.print("MQTT to ");
            Serial.print(server);
            Serial.print(" at port ");
            Serial.print( mqttPort );
            Serial.println(" successful.");
        } else {
            Serial.print("MQTT connection failed, rc = ");
            Serial.print(mqttClient.state());
            Serial.print(" Will try again in view seconds");
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    connectWifi();
    mqttClient.setServer(server, mqttPort);
    mqttClient.setCallback(mqttSubscriptionCallback);
    mqttClient.setBufferSize(2048);

    pinMode(SENSOR, INPUT_PULLUP);
    
    pulseCount = 0;
    flowRateInLitres = 0.0;
    flowRateInMilliLitres = 0;
    attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}

void loop() {
   if (WiFi.status() != WL_CONNECTED) {
        connectWifi();
    }

    if (!mqttClient.connected()){
        mqttConnect();
        mqttSubscribe(channelID);
    }

    mqttClient.loop();

    if (abs(millis() - lastPublishMillis) > updateInterval*1000) {
        
        pulse1Sec = pulseCount;
        pulseCount = 0;

        flowRateInLitres = ((1000.0 / (millis() - lastPublishMillis)) * pulse1Sec) / calibrationFactor; // debit air dalam Liter per menit

        totalLitres += flowRateInLitres;

        flowRateInMilliLitres = (flowRateInLitres / 60) * 1000;

        totalUsageInMilliLitres += flowRateInMilliLitres;

        tarifPenggunaan = totalUsageInMilliLitres * 0.0022;
        
        mqttPublish(channelID, (String("field1=")+String(totalUsageInMilliLitres)));
        mqttPublish(channelID, (String("field2=")+String(tarifPenggunaan)));
        
        lastPublishMillis = millis();
    }

}
