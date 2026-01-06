#define M5STACK_MPU6886

#include <M5Stack.h>
#include "freertos/FreeRTOS.h"
#include "imu_filter.h"
#include "MadgwickAHRS.h"
#include "bala.h"
#include "pid.h"
#include "calibration.h"

// ===== BLE（Central）追加 =====
#include <BLEDevice.h>

// ===== BLE UUID（JOY-C側と一致）=====
static BLEUUID SERVICE_UUID("12345678-1234-1234-1234-1234567890ab");
static BLEUUID CHAR_UUID   ("abcdefab-1234-5678-1234-abcdefabcdef");
static const char* TARGET_NAME = "JOY-C";

extern uint8_t bala_img[41056];
static void PIDTask(void *arg);
static void draw_waveform();

static float angle_point = -1.5f;
static volatile float target_speed = 0.0f;   // encoder差分ベースの速度目標（PIDTaskが読む）

float kp = 24.0f, ki = 0.0f, kd = 90.0f;
float s_kp = 15.0f, s_ki = 0.075f, s_kd = 0.0f;

bool calibration_mode = false;

Bala bala;

PID pid(angle_point, kp, ki, kd);
PID speed_pid(0, s_kp, s_ki, s_kd);

// ===== BLE受信値 =====
static volatile int16_t g_cmd_y0 = 0;
static volatile uint32_t g_last_rx_ms = 0;
static bool g_ble_ready = false;
static bool g_ble_connected = false;

// JOY-C生値の中心（最初は仮。必要なら調整）
static int16_t g_center_y0 = 0;     // 自動推定したいなら別途処理
static const int16_t DEAD_BAND = 15; // 生値の小さい揺れを無視

// 速度指令の最大値（要調整：大きいほど速いが倒れやすい）
static const float CMD_SPEED_MAX = 12.0f;

// 生値を target_speed に変換する関数
static float mapJoyToTargetSpeed(int16_t y0_raw) {
  // ここで「前に倒すと前進」か「逆」かは機体により変わるので、
  // 逆なら符号を反転してください（return -cmd;）
  int16_t v = y0_raw - g_center_y0;

  if (abs(v) <= DEAD_BAND) return 0.0f;

  // ここはJOY-Cのレンジが不明なので、まずは「±100」で正規化する想定（要調整）
  // もし y0 が 0..255 のようなら center=128, range=127 が目安。
  const float RANGE = 100.0f;
  float n = (float)v / RANGE;
  if (n > 1.0f) n = 1.0f;
  if (n < -1.0f) n = -1.0f;

  // return n * CMD_SPEED_MAX;

  float cmd = n * CMD_SPEED_MAX;
  if (fabs(cmd) < 0.5f) cmd = 0.0f;     // 最終的に小さい速度指令は切る
  return cmd;
}

// ===== センターを平均で更新する関数 =====
static void recenterJoyY0(uint32_t ms = 800) {
  uint32_t t0 = millis();
  int32_t sum = 0;
  int cnt = 0;

  while (millis() - t0 < ms) {
    sum += g_cmd_y0;
    cnt++;
    delay(5);
  }
  if (cnt > 0) g_center_y0 = (int16_t)(sum / cnt);
}

// ===== Notifyコールバック =====
static void notifyCallback(
  BLERemoteCharacteristic*,
  uint8_t* data,
  size_t length,
  bool
) {
  if (length < 2) return;
  int16_t y0 = (int16_t)((data[1] << 8) | data[0]); // little endian
  g_cmd_y0 = y0;
  g_last_rx_ms = millis();
}

// ===== BLE接続（setupで1回呼ぶ）=====
static void bleConnectJoyC() {
  BLEDevice::init("");

  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("BLE scan...");

  BLEScanResults results = scan->start(5, false);

  BLEAdvertisedDevice found;
  bool ok = false;

  for (int i = 0; i < results.getCount(); i++) {
    BLEAdvertisedDevice dev = results.getDevice(i);

    // 名前一致 or サービスUUID一致で捕まえる（どちらかでOK）
    bool nameOk = dev.haveName() && (dev.getName() == std::string(TARGET_NAME));
    bool svcOk  = dev.haveServiceUUID() && dev.isAdvertisingService(SERVICE_UUID);

    if (nameOk || svcOk) {
      found = dev;
      ok = true;
      break;
    }
  }

  if (!ok) {
    M5.Lcd.println("JOY-C not found");
    g_ble_ready = false;
    return;
  }

  BLEClient* client = BLEDevice::createClient();
  M5.Lcd.println("BLE connect...");
  if (!client->connect(&found)) {
    M5.Lcd.println("connect failed");
    g_ble_ready = false;
    return;
  }

  BLERemoteService* service = client->getService(SERVICE_UUID);
  if (!service) {
    M5.Lcd.println("service not found");
    g_ble_ready = false;
    return;
  }

  BLERemoteCharacteristic* ch = service->getCharacteristic(CHAR_UUID);
  if (!ch) {
    M5.Lcd.println("char not found");
    g_ble_ready = false;
    return;
  }

  if (ch->canNotify()) {
    ch->registerForNotify(notifyCallback);
  }

  // 簡易センター推定：接続直後の値を中心にしてみる（必要なら後で改善）
  g_center_y0 = g_cmd_y0;

  g_ble_ready = true;
  g_ble_connected = true;
  g_last_rx_ms = millis();
  M5.Lcd.println("BLE ready!");
}

// the setup routine runs once when M5Stack starts up
void setup() {
  M5.begin(true, false, false, false);

  Serial.begin(115200);
  M5.IMU.Init();

  int16_t x_offset, y_offset, z_offset;
  float angle_center;
  calibrationInit();

  if (M5.BtnB.isPressed()) {
    calibrationGryo();
    calibration_mode = true;
  }

  if (M5.BtnC.isPressed()) {
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Charge mode");
    while (1) {
      if (M5.Power.isCharging()) {
        M5.Lcd.println("Start charging...");
        while (1) {
          if (M5.Power.isChargeFull())
            M5.Lcd.println("Charge completed!");
          delay(5000);
        }
      }
      delay(500);
    }
  }

  calibrationGet(&x_offset, &y_offset, &z_offset, &angle_center);
  Serial.printf("x: %d, y: %d, z:%d, angle: %.2f", x_offset, y_offset, z_offset, angle_center);

  angle_point = angle_center;
  pid.SetPoint(angle_point);

  SemaphoreHandle_t i2c_mutex;
  i2c_mutex = xSemaphoreCreateMutex();
  bala.SetMutex(&i2c_mutex);
  ImuTaskStart(x_offset, y_offset, z_offset, &i2c_mutex);

  // BLE接続（ここでJOY-Cに接続してNotify購読）
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  bleConnectJoyC();

  xTaskCreatePinnedToCore(PIDTask, "pid_task", 4 * 1024, NULL, 4, NULL, 1);

  M5.Lcd.drawJpg(bala_img, 41056);
  if (calibration_mode) {
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("calibration mode");
  }
}

// the loop routine runs over and over again forever
void loop() {
  static uint32_t next_show_time = 0;
  vTaskDelay(pdMS_TO_TICKS(5));

  if (millis() > next_show_time) {
    draw_waveform();
    next_show_time = millis() + 10;
  }

  M5.update();
  if (M5.BtnA.wasPressed()) {
    angle_point += 0.25;
    pid.SetPoint(angle_point);
  }

  if (M5.BtnB.wasPressed()) {
    recenterJoyY0(800);  // 0.8秒平均
  }

  if (M5.BtnC.wasPressed()) {
    angle_point -= 0.25;
    pid.SetPoint(angle_point);
  }
}

static void PIDTask(void *arg) {
  float bala_angle;
  float motor_speed = 0;

  int16_t pwm_speed;
  int16_t pwm_output;
  int16_t pwm_angle;

  int32_t encoder = 0;
  int32_t last_encoder = 0;
  uint32_t last_ticks = 0;

  pid.SetOutputLimits(1023, -1023);
  pid.SetDirection(-1);

  speed_pid.SetIntegralLimits(40, -40);
  speed_pid.SetOutputLimits(1023, -1023);
  speed_pid.SetDirection(1);

  for (;;) {
    vTaskDelayUntil(&last_ticks, pdMS_TO_TICKS(5));

    // ===== BLE受信→ target_speed に反映 =====
    // フェイルセーフ：200ms以上受信がなければ止める
    uint32_t age = millis() - g_last_rx_ms;
    if (!g_ble_ready || age > 200) {
      target_speed = 0.0f;
    } else {
      int16_t y0 = g_cmd_y0;
      target_speed = mapJoyToTargetSpeed(y0);
    }
    speed_pid.SetPoint(target_speed);

    if (fabs(target_speed) < 0.01f) {
      speed_pid.SetIntegral(0);
    }

    bala_angle = getAngle();
    bala.UpdateEncoder();

    encoder = bala.wheel_left_encoder + bala.wheel_right_encoder;

    // motor_speed filter（encoder差分/5ms）
    motor_speed = 0.8f * motor_speed + 0.2f * (float)(encoder - last_encoder);
    last_encoder = encoder;

    if (fabs(bala_angle) < 70) {
      pwm_angle = (int16_t)pid.Update(bala_angle);
      pwm_speed = (int16_t)speed_pid.Update(motor_speed);
      pwm_output = pwm_speed + pwm_angle;
      pwm_output = constrain(pwm_output, -1023, 1023);
      bala.SetSpeed(pwm_output, pwm_output);
    } else {
      bala.SetSpeed(0, 0);
      bala.SetEncoder(0, 0);
      speed_pid.SetIntegral(0);
    }
  }
}

static void draw_waveform() {
#define MAX_LEN 120
#define X_OFFSET 100
#define Y_OFFSET 95
#define X_SCALE 3
  static int16_t val_buf[MAX_LEN] = {0};
  static int16_t pt = MAX_LEN - 1;
  val_buf[pt] = constrain((int16_t)(getAngle() * X_SCALE), -50, 50);

  if (--pt < 0) {
    pt = MAX_LEN - 1;
  }

  for (int i = 1; i < (MAX_LEN); i++) {
    uint16_t now_pt = (pt + i) % (MAX_LEN);
    M5.Lcd.drawLine(i + X_OFFSET, val_buf[(now_pt + 1) % MAX_LEN] + Y_OFFSET, i + 1 + X_OFFSET,
                    val_buf[(now_pt + 2) % MAX_LEN] + Y_OFFSET, TFT_BLACK);
    if (i < MAX_LEN - 1) {
      M5.Lcd.drawLine(i + X_OFFSET, val_buf[now_pt] + Y_OFFSET, i + 1 + X_OFFSET,
                      val_buf[(now_pt + 1) % MAX_LEN] + Y_OFFSET, TFT_GREEN);
    }
  }
}
