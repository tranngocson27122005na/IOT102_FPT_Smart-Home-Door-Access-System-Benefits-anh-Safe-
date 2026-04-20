#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebServer.h>

// ================= WIFI + WEB =================
const char* ssid = "SmartDoor_WIFI";
const char* pass = "12345678";

WebServer server(80);

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= RFID =================
#define SS_PIN 5
#define RST_PIN 17

MFRC522 rfid(SS_PIN, RST_PIN);
String masterUID = "FABA145";

// ================= PIN =================
const int led = 32;
const int trigPin = 25;
const int echoPin = 26;
const int servoPin = 27;

// ================= SERVO =================
Servo doorServo;

// ================= STATE =================
bool doorOpen = false;
bool carDetected = false;
float distance = 0;

// =========================================
//                WEB PAGE
// =========================================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>DOOR Control</title>
</head>
<body>
    <h1> SMART HOME DOOR </h1>

    <input type="password" id="pin" placeholder="Enter PIN">
    <button onclick="login()">LOGIN</button>

    <br><br>

    <div id="doorControl" style="display:none;">
        <button onclick="doorOn()">DOOR ON</button>
        <br><br>
        <button onclick="doorOff()">DOOR OFF</button>
    </div>

    <p id="message"></p>

<script>
    const correctPIN = "1234";
    let isLoggedIn = false;

    function login() {
        let pin = document.getElementById("pin").value;
        let msg = document.getElementById("message");

        if (pin === correctPIN) {
            isLoggedIn = true;
            document.getElementById("doorControl").style.display = "block";
            msg.innerText = "Login success";
        } else {
            msg.innerText = "Wrong PIN";
        }
    }

    function doorOn() {
        if (!isLoggedIn) {
            alert("Please login first");
            return;
        }

        fetch("/on");
    }

    function doorOff() {
        if (!isLoggedIn) {
            alert("Please login first");
            return;
        }

        fetch("/off");
    }
</script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// =========================================
//            DISTANCE SENSOR
// =========================================
float readDistance() {
  long duration;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  return duration * 0.034 / 2;
}

// =========================================
//              DOOR CONTROL
// =========================================
void moCua() {
  // Nếu cửa đang mở thì không làm gì
  if (doorOpen) {
    Serial.println("Door already OPEN");
    return;
  }

  digitalWrite(led, LOW);
  Serial.println("Opening door...");

  for (int angle = 0; angle <= 180; angle++) {
    doorServo.write(angle);
    delay(10);
  }

  doorOpen = true;
  digitalWrite(led, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door OPEN");
}

void dongCua() {
  digitalWrite(led, LOW);
  delay(2000);
  // Nếu cửa đang đóng thì không làm gì
  if (!doorOpen) {
    Serial.println("Door already CLOSED");
    return;
  }
  Serial.println("Closing door...");

  for (int angle = 180; angle >= 0; angle--) {
    doorServo.write(angle);
    delay(10);
  }

  doorOpen = false;
  digitalWrite(led, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door CLOSED");
}

// =========================================
//                RFID
// =========================================
String readCardUID() {
  if (!rfid.PICC_IsNewCardPresent()) return "";
  if (!rfid.PICC_ReadCardSerial()) return "";

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();
  return uid;
}

bool checkCard(String uid) {
  return uid == masterUID;
}

void handleRFID() {
  String uid = readCardUID();

  if (uid == "") return;

  Serial.print("UID: ");
  Serial.println(uid);

//Neu UID dung he thong cho phep truy cap
  if (checkCard(uid)) {
    // Quét lần 1 neu UID dung -> mở
    if (!doorOpen) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access OK");
      lcd.setCursor(0, 1);
      lcd.print("Opening...");
      moCua();
    } else { // Quét lần 2 -> đóng
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Scan Again");
      lcd.setCursor(0, 1);
      lcd.print("Closing...");
      dongCua();
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wrong Card");
    Serial.println("Wrong");
  }

  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan card...");
}

// =========================================
//                SETUP
// =========================================
void setup() {
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  doorServo.attach(servoPin);
  doorServo.write(0);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Scan card...");

  SPI.begin();
  rfid.PCD_Init();

  WiFi.softAP(ssid, pass);
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/on", moCua);
  server.on("/off", dongCua);
  server.begin();
}

// =========================================
//                 LOOP
// =========================================
void loop() {
  server.handleClient();

  distance = readDistance();

  Serial.print("Distance: ");
  Serial.println(distance);

  // Xe tới -> mở
  if (distance < 10) {
    carDetected = true;

    if (!doorOpen) {
      moCua();
    }
  }

  // Xe đi qua -> đóng
  if (distance >= 12 && doorOpen && carDetected) {
    dongCua();
    carDetected = false;
  }

  handleRFID();

  delay(200);
}
