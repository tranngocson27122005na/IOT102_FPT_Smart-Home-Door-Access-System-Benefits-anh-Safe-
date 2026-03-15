  #include <ESP32Servo.h>
  #include <Wire.h>               //I2C
  #include <LiquidCrystal_I2C.h>  //LCD controll
  #include <SPI.h>
  #include <MFRC522.h>
  //==========WEB SERVER INIT========
  //Thư viện điều khiển wifi và serverweb
  #include <WiFi.h>
  #include <WebServer.h>

  const char* ssid = "SmartGara_WIFI";
  const char* pass = "12345678";

  //Mở cổng port mặc định của http là 80;
  WebServer server(80);
  //=======END WEB SERVER INIT=======

  //=======GIAO DIEN VA XU LY WEB SERVER=====
  void handleRoot() {

    String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
  <title>DOOR Control</title>
  </head>

  <body>

  <h1>GARA DOOR MONITOR</h1>

  <button onclick="doorOn()">DOOR ON</button>
  <br><br>
  <button onclick="doorOff()">DOOR OFF</button>

  <script>

  function doorOn(){
    fetch("/on");
  }

  function doorOff(){
    fetch("/off");
  }

  </script>

  </body>
  </html>
  )rawliteral";

    /*Gửi phản hồi lên trình duyệt
    200: httpStatus
    kiểu trả về: text/html*/
    server.send(200, "text/html", html);
  }


  LiquidCrystal_I2C lcd(0x27, 16, 2);  //Init LCD, 0x27 is address I2C

  //======Khai bao chan RFID
  #define SS_PIN 5
  #define RST_PIN 17

  MFRC522 rfid(SS_PIN, RST_PIN);

  String masterUID = "FABA145";

  //=====Khai bao chan ultra, servo
  const int trigPin = 25;
  const int echoPin = 26;
  const int servoPin = 27;

  Servo doorServo;

  float distance;
  bool doorOpen = false;
  bool carDetected = false;  //Phan hien thay doi nhanh
  float readDistance() {

    long duration;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);

    float dis = duration * 0.034 / 2;

    return dis;
  }

  void moCua() {

    Serial.println("Mo cua");

    for (int angle = 0; angle <= 180; angle++) {
      doorServo.write(angle);
      delay(10);  // càng lớn càng chậm
    }

    doorOpen = true;
  }

  void dongCua() {

    Serial.println("Dong cua sau 3s");

    delay(3000);

    for (int angle = 180; angle >= 0; angle--) {
      doorServo.write(angle);
      delay(10);
    }

    doorOpen = false;
  }


  //=======HANDLE RFID===========
  //=============================
  //========== DOC UID ==========
  String readCardUID() {

    if (!rfid.PICC_IsNewCardPresent())
      return "";

    if (!rfid.PICC_ReadCardSerial())
      return "";

    String uid = "";

    for (byte i = 0; i < rfid.uid.size; i++) {
      uid += String(rfid.uid.uidByte[i], HEX);
    }

    uid.toUpperCase();

    return uid;
  }

  //========== KIEM TRA ==========
  bool checkCard(String uid) {

    if (uid == masterUID) {
      return true;
    }

    return false;
  }

  //========== XU LY RFID ==========
  void handleRFID() {

    String uid = readCardUID();

    if (uid == "") return;

    Serial.print("UID: ");
    Serial.println(uid);

    if (checkCard(uid) && doorOpen == false) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wellcome bro");
      lcd.setCursor(0, 1);
      lcd.print("Corrected");
      moCua();
      delay(3000);
      lcd.setCursor(0, 1);
      lcd.print("Scan card...");
      Serial.println("OK");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Wrong");
      delay(3000);
      lcd.setCursor(0, 1);
      lcd.print("Scan card...");
    }

    delay(1500);
  }

  void setup() {
    Serial.begin(115200);

    //=======ULtrasonic set up=====
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    //====== SERVO setUP=========
    doorServo.attach(servoPin);
    doorServo.write(0);  // cửa đóng

    //=========SET UP RFID
    SPI.begin();
    rfid.PCD_Init();
    lcd.setCursor(0, 1);
    Serial.println("Scan card...");

    //=====LCD Set Up=======
    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("Wellcome bro");

    //======SETUP WEB=======
    WiFi.softAP(ssid, pass);
    Serial.println(WiFi.softAPIP());  //lấy IP của trang

    server.on("/", handleRoot);  //Khi truy cập đúng IP; thì gọi handleRoot. gọi trang
    server.on("/on", moCua);
    server.on("/off", dongCua);
    //khởi tạo server
    server.begin();
  }

  void loop() {
    //=======WEB HANDLE====
    server.handleClient();
    //=======DI TU TRONG NHA RA============
    distance = readDistance();
    Serial.print("Distance: ");
    Serial.println(distance);

    // phát hiện xe tới cửa
    if (distance < 10) {
      carDetected = true;

      if (doorOpen == false) {
        moCua();
      }
    }

    // xe đã đi qua cảm biến
    if (distance >= 12 && doorOpen == true && carDetected == true) {
      dongCua();
      carDetected = false;
    }
    //===========KET THUC DI TU TRONG NHA RA
    //====================================//
    //===========DI TU NGOAI VAO BANG RFID==========
    handleRFID();

    delay(200);
  }