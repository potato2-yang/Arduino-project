import cv2
import paho.mqtt.client as mqtt
import time
from ultralytics import YOLO

# MQTT 設定
mqttClient = mqtt.Client()
mqttServer = "mqttgo.io"
mqttTopicPic = "potato/class9/pic"
mqttTopicCnt = "potato/class9/cnt"
mqttClient.connect(mqttServer)

# YOLO 模型設定
model = YOLO('yolov8m.pt')
names = model.names
print(names)

cv2.namedWindow('YOLOv8', cv2.WINDOW_NORMAL)

# 發佈間隔設定
last_publish_time = [time.time()]
publish_interval = 2  # 每隔 2 秒發佈一次人數

# MQTT 接收圖片處理函數
def on_message(client, userdata, message):
    fileName="test.jpg"
    f=open(fileName, "wb")
    f.write(message.payload)
    f.close()
    frame = cv2.imread(fileName)
    results = model(frame,verbose = False)
    #自己畫上物件外框
    #對所有畫面上的物件外框
    cnt=0
    for box in results[0].boxes.data:
        x1 = int(box[0]) #左
        y1 = int(box[1]) #上
        x2 = int(box[2]) #右
        y2 = int(box[3]) #下
        r = round(float(box[4]),2) #信任度
        n = names[int(box[5])] #物件名稱
        # 自己畫上外框並寫上名稱
        if n in ["person"]:
            #   畫矩形    影像    左上     右下   顏色(b,g,r)  寬度
            cv2.rectangle(frame, (x1,y1), (x2,y2), (0,255,0),  2)
            # 寫物件名  影像   內容  座標     字形                   大小  顏色    寬度
            cv2.putText(frame, n, (x1,y1), cv2.FONT_HERSHEY_SIMPLEX,0.7 , (0,0,255),2)
            # 計算人數
            cnt +=1 

    # 在左上角寫上人數
    cv2.putText(frame,"PERSON=" + str(cnt), (20,100), cv2.FONT_HERSHEY_SIMPLEX,1,(0,0,255),2 )
    
    # 每隔 publish_interval 秒傳送一次人數
    if time.time() - last_publish_time[0] >= publish_interval:
        mqttClient.publish(mqttTopicCnt, cnt)
        last_publish_time[0] = time.time()

    cv2.imshow("yolov8",frame )
    key = cv2.waitKey(1)
    if key==27:
        exit()

mqttClient.subscribe(mqttTopicPic)
mqttClient.on_message = on_message
mqttClient.loop_forever()
