// http://librarymanager/All#M5StickC https://github.com/m5stack/M5StickC
#include <M5StickCPlus.h>
#include <WiFi.h>

// Download these from here
// https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering/tree/master/ArduinoLibs
#include <SkaarhojPgmspace.h>
#include <ATEMbase.h>
#include <ATEMstd.h>

// Define the IP address of your ATEM switcher - Change IP
IPAddress switcherIp(192, 168, 1, 240);

// Put your wifi SSID and password here
const char* ssid = "Authentic City Church";
const char* password = "ThisHouseIsHoly";

// Set this to 1 if you want the orientation to update automatically
#define AUTOUPDATE_ORIENTATION 0

// You can customize the red/green/grey if you want
// http://www.barth-dev.de/online/rgb565-color-picker/
#define GRAY  0x0841 //   8   8  8
#define GREEN 0x0400 //   0 128  0
#define RED   0xF800 // 255   0  0



/////////////////////////////////////////////////////////////
// You probably don't need to change things below this line
#define LED_PIN 10

ATEMstd AtemSwitcher;

int orientation = 0;
int orientationPrevious = 0;
int orientationMillisPrevious = millis();
int buttonBMillis = 0;

int cameraNumber = 1;

int previewTallyPrevious = 1;
int programTallyPrevious = 1;
int cameraNumberPrevious = cameraNumber;

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
   M5.lcd.print("Connected to the WiFi network");
  Serial.println("\nWiFi Connected!");
  Serial.println("WiFi Connect To: ");
  Serial.println(WiFi.SSID());  //Output Network name. 
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); //Output IP Address.

  //
  M5.begin();
  M5.IMU.Init();
  M5.Lcd.setRotation(orientation);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  AtemSwitcher.begin(switcherIp);
  AtemSwitcher.serialOutput(0x80);
  AtemSwitcher.connect();

  // GPIO初期化
  pinMode(26, INPUT); // PIN  (INPUT, OUTPUT, ANALOG)無線利用時にはANALOG利用不可, DAC出力可
  pinMode(36, INPUT); // PIN  (INPUT,       , ANALOG)入力専用、INPUT_PULLUP等も不可
  pinMode( 0, INPUT); // PIN  (INPUT, OUTPUT,       )外部回路でプルアップ済み
  pinMode(32, INPUT); // GROVE(INPUT, OUTPUT, ANALOG)
  pinMode(33, INPUT); // GROVE(INPUT, OUTPUT, ANALOG)
}

void setOrientation() {
  float accX = 0, accY = 0, accZ = 0;
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  //Serial.printf("%.2f   %.2f   %.2f \n",accX * 1000, accY * 1000, accZ * 1000);

  if (accZ < .9) {
    if (accX > .6) {
      orientation = 1;
    } else if (accX < .4 && accX > -.5) {
      if (accY > 0) {
        orientation = 0;
      } else {
        orientation = 2;
      }
    } else {
      orientation = 3;
    }
  }

  if (orientation != orientationPrevious) {
    Serial.printf("Orientation changed to %d\n", orientation);
    M5.Lcd.setRotation(orientation);
  }
}


void loop() {
  // ボタンの状態更新
  M5.update();

  if (AUTOUPDATE_ORIENTATION) {
    if (orientationMillisPrevious + 500 < millis()) {
      setOrientation();
      orientationMillisPrevious = millis();
    }
  }

  if (M5.BtnA.wasPressed()) {
    AtemSwitcher.changeProgramInput(cameraNumber);
  }
  if (M5.BtnB.wasPressed()) {
    setOrientation();
    buttonBMillis = millis();
  }

  if (M5.BtnB.isPressed() && buttonBMillis != 0 && buttonBMillis < millis() - 500) {
    Serial.println("Changing camera number");
    cameraNumber = (cameraNumber % 4) + 1;
    Serial.printf("New camera number: %d\n", cameraNumber);

    buttonBMillis = 0;
  }

  // Check for packets, respond to them etc. Keeping the connection alive!
  AtemSwitcher.runLoop();

  int programTally = AtemSwitcher.getProgramTally(cameraNumber);
  int previewTally = AtemSwitcher.getPreviewTally(cameraNumber);

  if ((orientation != orientationPrevious) || (cameraNumber != cameraNumberPrevious) || (programTallyPrevious != programTally) || (previewTallyPrevious != previewTally)) { // changed?
    if (programTally && !previewTally) { // only program
      drawLabel(RED, BLACK, LOW);
    } else if (programTally && previewTally) { // program AND preview
      drawLabel(RED, GREEN, LOW);
    } else if (previewTally && !programTally) { // only preview
      drawLabel(GREEN, BLACK, HIGH);
    } else if (!previewTally || !programTally) { // neither
      drawLabel(BLACK, GRAY, HIGH);
    }
  }

  programTallyPrevious = programTally;
  previewTallyPrevious = previewTally;
  cameraNumberPrevious = cameraNumber;
  orientationPrevious  = orientation;
}

void drawLabel(unsigned long int screenColor, unsigned long int labelColor, bool ledValue) {
  digitalWrite(LED_PIN, ledValue);
  M5.Lcd.fillScreen(screenColor);
  M5.Lcd.setTextColor(labelColor, screenColor);
  drawStringInCenter(String(cameraNumber), 8);
}

void drawStringInCenter(String input, int font) {
  int datumPrevious = M5.Lcd.getTextDatum();
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.drawString(input, M5.Lcd.width() / 2, M5.Lcd.height() / 2, font);
  M5.Lcd.setTextDatum(datumPrevious);
}
