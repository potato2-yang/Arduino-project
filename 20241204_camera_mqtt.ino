#include "esp_camera.h"
#include "FS.h" //sd card esp32
#include "SD_MMC.h" //sd card esp32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>//請先安裝PubSubClient程式庫

char* ssid = "Potato";
char* password = "dgdg9064";

// ------ 以下修改成你MQTT設定 ------
char* MQTTServer = "mqttgo.io";//免註冊MQTT伺服器
int MQTTPort = 1883;//MQTT Port
char* MQTTUser = "";//不須帳密
char* MQTTPassword = "";//不須帳密
char* MQTTPubTopic1  = "potato/class9/pic";//推播主題1:即時影像
long MQTTLastPublishTime;//此變數用來記錄推播時間
long MQTTPublishInterval = 1000;//每1秒推撥一次影像
WiFiClient WifiClient;
PubSubClient MQTTClient(WifiClient);

//開始MQTT連線
void MQTTConnecte() {
  MQTTClient.setServer(MQTTServer, MQTTPort);
  //MQTTClient.setCallback(MQTTCallback);
  while (!MQTTClient.connected()) {
    //以亂數為ClientID
    String  MQTTClientid = "esp32-" + String(random(1000000, 9999999));
    if (MQTTClient.connect(MQTTClientid.c_str(), MQTTUser, MQTTPassword)) {
      //連結成功，顯示「已連線」。
      Serial.println("MQTT已連線");
      //訂閱SubTopic1主題
      //MQTTClient.subscribe(MQTTSubTopic1);
    } else {
      //若連線不成功，則顯示錯誤訊息，並重新連線
      Serial.print("MQTT連線失敗,狀態碼=");
      Serial.println(MQTTClient.state());
      Serial.println("五秒後重新連線");
      delay(5000);
    }
  }
}

//拍照傳送到MQTT
String SendImageMQTT() {
  camera_fb_t * fb =  esp_camera_fb_get();
  size_t fbLen = fb->len;
  int ps = 512;
  //開始傳遞影像檔
  MQTTClient.beginPublish(MQTTPubTopic1, fbLen, false);
  uint8_t *fbBuf = fb->buf;
  for (size_t n = 0; n < fbLen; n = n + 2048) {
    if (n + 2048 < fbLen) {
      MQTTClient.write(fbBuf, 2048);
      fbBuf += 2048;
    } else if (fbLen % 2048 > 0) {
      size_t remainder = fbLen % 2048;
      MQTTClient.write(fbBuf, remainder);
    }
  }
  boolean isPublished = MQTTClient.endPublish();
  esp_camera_fb_return(fb);//清除緩衝區
  if (isPublished) {
    return "MQTT傳輸成功";
  }
  else {
    return "MQTT傳輸失敗，請檢查網路設定";
  }
}


void setup() {
  Serial.begin(115200);
  //初始化相機結束
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.fb_count = 2;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 12;
  //設定解析度：FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.frame_size = FRAMESIZE_VGA;//VGA=640*480(VGA格式較穩定)
  //Line notify don't accept bigger than XGA
  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK) {
    Serial.println("鏡頭啟動成功");
    // setup stream ------------------------
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;
    res = s->set_brightness(s, 1); //亮度:(-2~2)
    res = s->set_contrast(s, 1); //對比度:(-2~2)
    res = s->set_saturation(s, 1); //色彩飽和度:(-2~2)
    //res = s->set_special_effect(s, 0);//特殊效果:(0~6)
    //res = s->set_whitebal(s, 1);//啟動白平衡:(0或1)
    //res = s->set_awb_gain(s, 1);//自動白平衡增益:(0或1)
    //res = s->set_wb_mode(s, 0);//白平衡模式:(0~4)
    //res = set_exposure_ctrl(s, 1);;//曝光控制:(0或1)
    //res = set_aec2(s, 0);//自動曝光校正:(0或1)
    //res = set_ae_level(s, 0);//自動曝光校正程度:(-2~2)
    //res = set_aec_value(s, 300);//自動曝光校正值：(0~1200)
    //res = set_gain_ctrl(s, 1);//增益控制:(0或1)
    //res = set_agc_gain(s, 0);//自動增益:(0~30)
    //res = set_gainceiling(s, (gainceiling_t)0); //增益上限:(0~6)
    //res = set_bpc(s, 1);//bpc開啟:(0或1)
    //res = set_wpc(s, 1);//wpc開啟:(0或1)
    //res = set_raw_gma(s, 1);//影像GMA:(0或1)
    //res = s->set_lenc(s, 1);//鏡頭校正:(0或1)
    //res = s->set_hmirror(s, 1);//水平翻轉:(0或1)
    //res = s->set_vflip(s, 1);//垂直翻轉:(0或1)
    //res = set_dcw(s, 1);//dcw開啟:(0或1)
  } else {
    Serial.printf("鏡頭設定失敗，5秒後重新啟動");
    delay(5000);
    ESP.restart();
  }

  //  //設定SD卡
  //  if (!SD_MMC.begin()) {
  //    Serial.println("SD卡讀取失敗，5秒後重新啟動");
  //    delay(5000);
  //    ESP.restart();
  //  } else {
  //    Serial.println("SD卡偵測成功");
  //  }

  //開始網路連線
  Serial.print("連線到WiFi:");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  MQTTConnecte();
}

void loop() {
  //如果MQTT連線中斷，則重啟MQTT連線
  if (!MQTTClient.connected())   MQTTConnecte(); 
  if ((millis() - MQTTLastPublishTime) >= MQTTPublishInterval ) {
    String result = SendImageMQTT();
    Serial.println(result);
    MQTTLastPublishTime = millis(); //更新最後傳輸時間
  }
  delay(100);
}
