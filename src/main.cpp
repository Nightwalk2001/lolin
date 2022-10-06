#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <CheapStepper.h>
#include <TaskScheduler.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#define DEVICE_ID "esp01"

const char *SSID = "mqtt";
const char *PASSPHRASE = "12345678";

const size_t CAPACITY = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;

const char *BROKER = "81.71.138.9";
const int PORT = 1883;
const char *USER = "admin";
const char *PASSWORD = "public";
const char *SUB_TOPIC = "feeding-times";
const char *PUB_TOPIC = "feeding-res";
const char *ERROR_TOPIC = "error-occur";

#define BTN D5 // 按钮引脚
#define LIGHT A0 // 光强传感器引脚
#define LIGHT_LED D6 // 光强传感器LED引脚
#define POWER_LED D4 // 电源LED引脚
#define IN1 13 // 电机IN1引脚
#define IN2 4 // 电机IN2引脚
#define IN3 15 // 电机IN3引脚
#define IN4 5 // 电机IN4引脚

const unsigned long PRESS_DURATION = 100; // 单击最小时间间隔100ms
const unsigned long LONG_PRESS_DURATION = 3000; // 长按最小时间间隔3000ms
const unsigned long LIGHT_INTENSITY_THRESHOLD = 1034;

unsigned long counter = 0; // 时间间隔计时器
int prevState = HIGH;
int currentState; // 按钮的前一时刻// 状态和当前时刻状态
unsigned int lightIntensity = 1024; // 光照强度模拟引脚输入值
unsigned int amount = 0; // 检测到的光强小于阈值的次数

WiFiClient wifi;
PubSubClient pubsub(wifi);
CheapStepper stepper(IN1, IN2, IN3, IN4); // 电机控制对象
Scheduler ts; // 任务调度器对象

struct FeedingConfig {
    String time;
    int count;
};

void setupWifi() {
    WiFi.begin(SSID, PASSPHRASE);
    while (WiFi.status() != WL_CONNECTED) {
        delay(3000);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the WiFi network");
}

void btnHandler();

void stepperLoop();

void feed(int c);

void inspect();

void cbk();

void listen() {
    pubsub.loop();
}

// 按钮loop
Task listenBtn(0, TASK_FOREVER, &btnHandler, &ts, true);
// 电机loop 如果电机不转动，把下面那行解除注释
Task stepperTask(0, TASK_FOREVER, &stepperLoop, &ts, true);

Task pubsubTask(0, TASK_FOREVER, &listen, &ts, true);

Task l(0, TASK_ONCE, &cbk);
Task i(10, 200, &inspect);

// 重置函数，真正的代码在loop函数下方，被注释掉了
// 功能是删除保存在闪存里的饲喂时间、数量信息
void reset() {
//    LittleFS.remove("./conf.json");
    pubsub.publish(PUB_TOPIC, "恢复出厂设置");
}

// 处理按钮事件
void btnHandler() {
    currentState = digitalRead(BTN);
    if (currentState != prevState) {
        if (currentState == LOW) counter = millis();
        else {
            unsigned long diff = millis() - counter;
            if (diff < LONG_PRESS_DURATION && diff >= PRESS_DURATION) feed(1);  // 单击按钮
            else if (diff >= LONG_PRESS_DURATION) reset(); // 长按按钮
        }
        prevState = currentState;
    }
}

void stepperLoop() {
    stepper.run();
}

void cbk() {
    digitalWrite(LIGHT_LED, LOW);

    DynamicJsonDocument doc(1024);
    doc["id"] = DEVICE_ID;
    doc["type"] = "manual";
    doc["amount"] = amount;
    String out;
    serializeJson(doc, out);
    pubsub.publish(PUB_TOPIC, out.c_str());
    amount = 0;
}

//void step() {
//
//}

void feed(int c) {
    digitalWrite(LIGHT_LED, HIGH);
    stepper.moveCW(2048 * c);

    ts.addTask(i);
    i.enable();
}

// 监测饲料是否成功掉下
void inspect() {
    if (i.isLastIteration()) {
        ts.addTask(l);
        l.enable();
    }
    lightIntensity = analogRead(LIGHT);
    if (lightIntensity < LIGHT_INTENSITY_THRESHOLD) amount++;
}

void callback(char *topic, byte *payload, unsigned int length) {
    DynamicJsonDocument doc(1024);

    DeserializationError error = deserializeJson(doc, payload);

    File f = LittleFS.open("./conf.json", "w");
    serializeJson(doc, f);
    f.close();

//    int count = doc["count"];
//    feed(count);
}

void setupMqtt() {
    pubsub.setServer(BROKER, PORT);
    pubsub.setCallback(callback);
    while (!pubsub.connected()) {
        Serial.printf("%s connects to mqtt broker\n", DEVICE_ID);

        if (pubsub.connect(DEVICE_ID, USER, PASSWORD))
            Serial.println("mqtt broker connected");
        else delay(3000);
    }

    pubsub.subscribe(SUB_TOPIC);
}

void error(const char *message) {
    DynamicJsonDocument doc(1024);
    doc["message"] = message;
    doc["id"] = DEVICE_ID;
    String out;
    serializeJson(doc, out);
    pubsub.publish(ERROR_TOPIC, out.c_str());
}

void setupFs() {
    bool opened = LittleFS.begin();
    if (!opened) Serial.println("An Error has occurred while mounting LittleFS");

    File file = LittleFS.open("./conf.json", "r");

    DynamicJsonDocument config(1024);

//    DynamicJsonDocument array(CAPACITY);

    if (!file || file.isDirectory()) return;
    else deserializeJson(config, file);
    file.close();

    String out;
    serializeJson(config, out);
    pubsub.publish(PUB_TOPIC, out.c_str());
//
//    auto array = config.as<JsonArray>();
//
//    for (JsonObject i: array) {
//        int count = i.getMember("count");
//        Serial.println(count);
//        const char *time = i.getMember("time");
//        pubsub.publish(PUB_TOPIC, time);
//    }

}

void save() {

}

void setup() {
    Serial.begin(9600);

    // 设置按钮、光强传感器、光强传感器LED、电源LED引脚IO模式
    pinMode(BTN, INPUT);
    pinMode(LIGHT, INPUT);
    pinMode(LIGHT_LED, OUTPUT);
    pinMode(POWER_LED, OUTPUT);

    setupWifi();
    setupMqtt();
    setupFs();

    stepper.setRpm(12);
}

void loop() {
    ts.execute();
}
