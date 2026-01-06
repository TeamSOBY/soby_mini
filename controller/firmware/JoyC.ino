#include <M5StickCPlus.h>
#include "Hat_JoyC.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ===== BLE UUID（FIRE側と一致させる）=====
static const char* BLE_DEVICE_NAME = "JOY-C";
static BLEUUID SERVICE_UUID("12345678-1234-1234-1234-1234567890ab");
static BLEUUID CHAR_UUID   ("abcdefab-1234-5678-1234-abcdefabcdef");

TFT_eSprite canvas = TFT_eSprite(&M5.Lcd);
JoyC joyc;

BLECharacteristic *pChar = nullptr;
volatile bool bleConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override { bleConnected = true; }
  void onDisconnect(BLEServer* pServer) override {
    bleConnected = false;
    // 再アドバタイズ
    BLEDevice::startAdvertising();
  }
};

char info[50];

void setup() {
  M5.begin();
  Serial.begin(115200);

  M5.Lcd.setRotation(1);
  canvas.createSprite(160, 80);
  joyc.begin();

  // ===== BLE Peripheral 開始 =====
  BLEDevice::init(BLE_DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pChar = pService->createCharacteristic(
    CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
  );
  pChar->addDescriptor(new BLE2902());  // Notifyに必要

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  canvas.fillSprite(BLACK);
  canvas.setCursor(0, 10);
  canvas.println("JOY-C BLE TX");
  canvas.pushSprite(10, 0);
}

void loop() {
  joyc.update();

  // 送信データ：左スティックY（y0）を int16 で送る
  int16_t y0 = (int16_t)joyc.y0;

  // Notify（接続中のみ）
  if (bleConnected && pChar) {
    uint8_t payload[2];
    payload[0] = (uint8_t)(y0 & 0xFF);         // little endian
    payload[1] = (uint8_t)((y0 >> 8) & 0xFF);
    pChar->setValue(payload, sizeof(payload));
    pChar->notify();
  }

  // 画面表示（既存に近い形）
  canvas.fillSprite(BLACK);
  canvas.setCursor(0, 10);
  canvas.println("JOY-C BLE TX");
  sprintf(info, "YL(y0): %d", y0);
  canvas.println(info);
  canvas.println(bleConnected ? "BLE: Connected" : "BLE: Advertising");
  canvas.pushSprite(10, 0);

  // LED表示（既存）
  if (joyc.btn0 && joyc.btn1)      joyc.setLEDColor(0x00ffe8);
  else if (joyc.btn0)             joyc.setLEDColor(0xff0000);
  else if (joyc.btn1)             joyc.setLEDColor(0x0000ff);
  else                            joyc.setLEDColor(0x00ff00);

  delay(20); // 50Hz送信
}
