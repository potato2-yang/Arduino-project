#include <WiFi.h>
#include <PubSubClient.h>  //請先安裝PubSubClient程式庫

// ------ 以下修改成你自己的WiFi帳號密碼 ------
char ssid[] = "SSID";
char password[] = "password";


//------ 以下修改成你腳位 ------
#include <ESP32Servo.h>
Servo myServo; //建立一個伺服馬達物件
int pinServo = 4;//伺服馬達腳位

// ------ 以下修改成你MQTT設定 ------
char* MQTTServer = "mqttgo.io";  //免註冊MQTT伺服器
int MQTTPort = 1883;             //MQTT Port
char* MQTTUser = "";             //不須帳密
char* MQTTPassword = "";         //不須帳密
//推播主題1
char* MQTTPubTopic1 = "potato/class9/light";
//訂閱主題1:改變馬達
char* MQTTSubTopic1 = "potato/class9/servo";
char* MQTTSubTopic2 = "potato/class9/cnt";  // 新增人數訂閱主題


long MQTTLastPublishTime;          //此變數用來記錄推播時間
long MQTTPublishInterval = 5000;  //每5秒推撥一次 //使用delay不能超過6秒，會離線
long lightLowStartTime = 0;        // 記錄亮度低於1000的開始時間
long lightLowDuration = 20000;    // 設定亮度低於1000需維持20秒
long lightOnStartTime = 0;         // 記錄亮度高於3500的開始時間
long lightOnDelay = 10000;         // 設定亮度高於3500且人數大於1需延遲10秒開燈
bool isLightOff = false;           // 記錄目前是否已關燈
int roomPeopleCount = 0;  // 新增房間人數變數
bool isPeopleCountReceived = false; 


WiFiClient WifiClient;
PubSubClient MQTTClient(WifiClient);

void setup() {
  Serial.begin(115200);
  myServo.attach(pinServo, 500, 2400);//伺服馬達
  pinMode(36, INPUT);  

  //開始WiFi連線
  WifiConnecte();

  //開始MQTT連線
  MQTTConnecte();
}

void loop() {
  //如果WiFi連線中斷，則重啟WiFi連線
  if (WiFi.status() != WL_CONNECTED) WifiConnecte();

  //如果MQTT連線中斷，則重啟MQTT連線
  if (!MQTTClient.connected()) MQTTConnecte();

  //如果距離上次傳輸已經超過5秒，則Publish亮度
  if ((millis() - MQTTLastPublishTime) >= MQTTPublishInterval) {  //millis:開機時間
    //讀取亮度
    int value=analogRead(36);
    Serial.print("亮度:");
    Serial.print(value);
    MQTTClient.publish(MQTTPubTopic1, String(value).c_str());  //c_str:string=>char[]
    Serial.println("亮度已推播到MQTT Broker");
    MQTTLastPublishTime = millis();  //更新最後傳輸時間
    // 若亮度小於3500，記錄開始時間，若超過20秒則關燈並推播通知
    if (isPeopleCountReceived &&roomPeopleCount == 0 &&value < 3500) {
      if (lightLowStartTime == 0) {
        lightLowStartTime = millis();
      } else if (millis() - lightLowStartTime >= lightLowDuration && !isLightOff) {
        Serial.println("亮度過高已超過20秒，自動關燈");
        myServo.write(38); // 將伺服馬達轉到關燈位置
        MQTTClient.publish(MQTTSubTopic1, "1"); // 推播關燈通知
        isLightOff = true; // 設置狀態為已關燈
        lightLowStartTime = 0; // 重置開始時間
      }
      
    } else {
      lightLowStartTime = 0; // 重置開始時間，亮度恢復正常
      isLightOff = false; // 亮度恢復時重置關燈狀態
    }
     if (value > 3500 && isPeopleCountReceived && roomPeopleCount > 0) {
      if (lightOnStartTime == 0) {
        lightOnStartTime = millis();
      } else if (millis() - lightOnStartTime >= lightOnDelay) {
        Serial.println("亮度過高且房間人數大於1，延遲10秒後自動開燈");
        myServo.write(3); // 將伺服馬達轉到開燈位置
        MQTTClient.publish(MQTTSubTopic1, "0"); // 推播開燈通知
        lightOnStartTime = 0; // 重置開始時間
      }
    } else {
      lightOnStartTime = 0; // 重置開始時間
    }
  }
  MQTTClient.loop();  //更新訂閱狀態
  delay(50);


}


//開始WiFi連線
void WifiConnecte() {
  //開始WiFi連線
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi連線成功");
  Serial.print("IP Address:");
  Serial.println(WiFi.localIP());
}

//開始MQTT連線
void MQTTConnecte() {
  MQTTClient.setServer(MQTTServer, MQTTPort);
  MQTTClient.setCallback(MQTTCallback); //接收訊息事件event
  while (!MQTTClient.connected()) {
    //以亂數為ClietID
    String MQTTClientid = "esp32-" + String(random(1000000, 9999999));
    if (MQTTClient.connect(MQTTClientid.c_str(), MQTTUser, MQTTPassword)) {
      //連結成功，顯示「已連線」。
      Serial.println("MQTT已連線");
      //訂閱SubTopic1主題
      MQTTClient.subscribe(MQTTSubTopic1);
      MQTTClient.subscribe(MQTTSubTopic2);
    } else {
      //若連線不成功，則顯示錯誤訊息，並重新連線
      Serial.print("MQTT連線失敗,狀態碼=");
      Serial.println(MQTTClient.state());
      Serial.println("五秒後重新連線");
      delay(5000);
    }
  }
}

//接收到訂閱時
void MQTTCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print(topic);
  Serial.print("訂閱通知:");
  String payloadString;  //將接收的payload轉成字串
  //顯示訂閱內容
  for (int i = 0; i < length; i++) {
    payloadString = payloadString + (char)payload[i];
  }
  Serial.println(payloadString);

 
  //比對主題是否為訂閱主題1  stremp(A,B)字串比對，不同字數
  if (strcmp(topic, MQTTSubTopic1) == 0) {
    if (payloadString == "1") {//關燈
      Serial.println("改變馬達：" + payloadString);
      myServo.write(38);
    }
    if (payloadString == "0") {//開燈
      Serial.println("改變馬達：" + payloadString);
      myServo.write(3);
    }

  }

   // 比對是否為房間人數主題
  if (strcmp(topic, MQTTSubTopic2) == 0) {
    roomPeopleCount = payloadString.toInt();  // 將接收到的人數字串轉換為整數
    isPeopleCountReceived = true;  // 設置為已接收到人數
    Serial.print("房間人數更新: ");
    Serial.println(roomPeopleCount);
  }
  
}
