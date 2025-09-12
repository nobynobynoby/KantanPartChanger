// インクルードはここにまとめる
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
// --- バッテリー電圧取得・移動平均用 ---
#define BATTERY_ADC_PIN 8
#define BATTERY_SAMPLES 10           // 移動平均のサンプル数
#define BATTERY_UPDATE_INTERVAL 30000 // LED更新間隔（30秒）
static float batteryVoltageSamples[BATTERY_SAMPLES] = {0.0};
static int batterySampleIndex = 0;
static unsigned long lastBatteryLEDUpdate = 0;
static float displayedBatteryVoltage = 4.2; // 表示用バッテリー電圧
static bool batteryInitialized = false; // 初期化フラグ

// バッテリー電圧取得＆移動平均＆LED色制御（仮）
void updateBatteryLEDs() {
  int batteryValue = analogRead(BATTERY_ADC_PIN);
  float Vfs_cal = 2.3096859379622803; // キャリブ済み定数

  // ADC値から電圧へ変換
  float voltage = (batteryValue / 4095.0) * Vfs_cal / (51.0 / (100.0 + 51.0)); // V

  // 初回は全サンプルを現在の電圧で初期化
  if (!batteryInitialized) {
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      batteryVoltageSamples[i] = voltage;
    }
    batteryInitialized = true;
    displayedBatteryVoltage = voltage;
    lastBatteryLEDUpdate = millis();
  } else {
    // 移動平均に追加
    batteryVoltageSamples[batterySampleIndex] = voltage;
    batterySampleIndex = (batterySampleIndex + 1) % BATTERY_SAMPLES;

    // 移動平均を計算
    float sum = 0.0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      sum += batteryVoltageSamples[i];
    }
    float averageVoltage = sum / BATTERY_SAMPLES;

    // 30秒間隔でLED表示を更新
    unsigned long currentTime = millis();
    if (currentTime - lastBatteryLEDUpdate >= BATTERY_UPDATE_INTERVAL) {
      displayedBatteryVoltage = averageVoltage;
      lastBatteryLEDUpdate = currentTime;
    }
  }

  // displayedBatteryVoltage に応じてLEDを点灯（仮: 今は20%以上扱い）
  // 例: ここでbatteryPercentを計算し、toggleLED等で色を変える
  // float batteryPercent = ...;
  // if (batteryPercent <= 5) { ... } else if (batteryPercent <= 20) { ... } else { ... }
}

// BLE-MIDI設定
BLEMIDI_CREATE_INSTANCE("KantanPartChanger", MIDI);

// プロトタイプ宣言
void handleMidiCC(uint8_t channel, uint8_t controller, uint8_t value);
// ...existing code...

// ...existing code...

// 音階マッピング（LEDの状態に応じて切り替え）
const uint8_t midiNotesOn[6] = {60, 62, 64, 66, 68, 70};  // LED点灯時のノート
const uint8_t midiNotesOff[6] = {61, 63, 65, 67, 69, 71}; // LED消灯時のノート

// ★設定★（配列化）
const uint8_t LED_PINS[6] = {13, 5, 2, 7, 11, 3};    // LED ピン番号
const uint8_t BUTTON_PINS[6] = {15, 6, 1, 9, 10, 4}; // ボタン ピン番号
const char* NOTE_NAMES[6] = {"C5", "D5", "E5", "F5", "G5", "A5"}; // 音階名

// NeoPixel配列
Adafruit_NeoPixel strips[6] = {
  Adafruit_NeoPixel(1, LED_PINS[0], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(1, LED_PINS[1], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(1, LED_PINS[2], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(1, LED_PINS[3], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(1, LED_PINS[4], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(1, LED_PINS[5], NEO_GRB + NEO_KHZ800)
};

// 状態管理配列
bool ledStates[6] = {false, false, false, false, false, false};
uint8_t lastButtonStates[6] = {0, 0, 0, 0, 0, 0};
uint32_t lastDebounceTimes[6] = {0, 0, 0, 0, 0, 0};
bool buttonPressed[6] = {false, false, false, false, false, false}; // ボタン押下状態追加
uint8_t currentMidiNote[6] = {0, 0, 0, 0, 0, 0}; // 現在送信中のMIDIノート
const uint32_t debounceMs = 50;

// 汎用LED切り替え関数（MIDI送信なし）
void toggleLED(uint8_t index) {
  // 仮バッテリー残量（今は常に100%扱い）
  int batteryPercent = 100;
  if (!ledStates[index]) {
    uint8_t r = 0, g = 0, b = 0;
    if (batteryPercent <= 5) {
      // 5%以下は赤
      r = 255; g = 0; b = 0;
    } else if (batteryPercent <= 20) {
      // 20%以下は黄
      r = 255; g = 255; b = 0;
    } else {
      // 1,2,3は緑、4,5,6は紫
      if (index < 3) {
        r = 0; g = 255; b = 0; // 緑
      } else {
        r = 128; g = 0; b = 128; // 紫
      }
    }
    strips[index].setPixelColor(0, strips[index].Color(r, g, b));
    strips[index].show();
    ledStates[index] = true;
    Serial.print("[DEBUG] LED"); Serial.print(index + 1); Serial.print(" ON: R="); Serial.print(r);
    Serial.print(" G="); Serial.print(g); Serial.print(" B="); Serial.println(b);
  } else {
    // 消灯
    strips[index].setPixelColor(0, strips[index].Color(0, 0, 0));
    strips[index].show();
    ledStates[index] = false;
    Serial.print("[DEBUG] LED"); Serial.print(index + 1); Serial.println(" OFF");
  }
  Serial.print("[DEBUG] Button"); Serial.print(index + 1); Serial.print(" pressed. LED"); 
  Serial.print(index + 1); Serial.print("On="); Serial.println(ledStates[index]);
}

// BLE接続ハンドラー
void OnConnected() {
  Serial.println("[BLE-MIDI] Device connected");
}

void OnDisconnected() {
  Serial.println("[BLE-MIDI] Device disconnected");
}

void setup() {
  // 主要なMIDIイベントに空ハンドラを登録
  MIDI.setHandleNoteOn([](byte, byte, byte){});
  MIDI.setHandleNoteOff([](byte, byte, byte){});
  MIDI.setHandleProgramChange([](byte, byte){});
  MIDI.setHandlePitchBend([](byte, int){});
  MIDI.setHandleAfterTouchChannel([](byte, byte){});
  MIDI.setHandleAfterTouchPoly([](byte, byte, byte){});
  MIDI.setHandleSystemExclusive([](byte*, unsigned){});
  // ボタンピン設定（配列化）
  for (int i = 0; i < 6; i++) {
    pinMode(BUTTON_PINS[i], INPUT);
    strips[i].begin();
    strips[i].setBrightness(50);
    strips[i].show(); // 消灯
  }
  
  Serial.begin(115200);
  delay(1000);
  
  // BLE-MIDI初期化
  BLEMIDI.setHandleConnected(OnConnected);
  BLEMIDI.setHandleDisconnected(OnDisconnected);

  MIDI.begin();
  // CC受信時のハンドラ登録
  MIDI.setHandleControlChange(handleMidiCC);
  // CC以外もバッファ消化のため空コールバック登録
  MIDI.setHandleNoteOn([](byte, byte, byte){});
  MIDI.setHandleNoteOff([](byte, byte, byte){});
  MIDI.setHandleProgramChange([](byte, byte){});
  MIDI.setHandlePitchBend([](byte, int){});
  MIDI.setHandleAfterTouchChannel([](byte, byte){});
  MIDI.setHandleAfterTouchPoly([](byte, byte, byte){});
  MIDI.setHandleSystemExclusive([](byte*, unsigned){});

  Serial.println("[BLE-MIDI] Initialized - Device: KantanPartChanger");

  Serial.println("[DEBUG] Setup complete");
  // ピン情報表示（配列化）
  for (int i = 0; i < 6; i++) {
    Serial.print("LED"); Serial.print(i + 1); Serial.print("_PIN: "); Serial.println(LED_PINS[i]);
    Serial.print("BUTTON"); Serial.print(i + 1); Serial.print("_PIN: "); Serial.println(BUTTON_PINS[i]);
  }
}

void loop() {
  // BLE-MIDI処理（CC受信はコールバックで処理）
  MIDI.read();
  // バッテリー監視・LED制御
  updateBatteryLEDs();
  
  uint32_t now = millis();
  
  // 全ボタンを配列で処理
  for (int i = 0; i < 6; i++) {
    uint8_t reading = digitalRead(BUTTON_PINS[i]);
    
    if (reading != lastButtonStates[i]) {
      lastDebounceTimes[i] = now;
      Serial.print("[DEBUG] Button"); Serial.print(i + 1); Serial.println(" state changed");
      
      if (reading == HIGH && lastButtonStates[i] == LOW) {
        // ボタン押下時: LEDの状態に応じてMIDI Note ON
        if (!buttonPressed[i]) {
          toggleLED(i); // まずLEDを切り替え
          buttonPressed[i] = true;
          
          // LEDの新しい状態に応じてMIDIノートを決定
          uint8_t noteToSend = ledStates[i] ? midiNotesOn[i] : midiNotesOff[i];
          currentMidiNote[i] = noteToSend; // 送信中ノートを記録
          
          MIDI.sendNoteOn(noteToSend, 127, 1);
          Serial.print("[BLE-MIDI] Note ON: "); Serial.print(noteToSend);
          Serial.print(" (LED "); Serial.print(ledStates[i] ? "ON" : "OFF"); Serial.println(" state)");
          Serial.print("[DEBUG] Button"); Serial.print(i + 1); Serial.println(" press detected!");
        }
        
      } else if (reading == LOW && lastButtonStates[i] == HIGH) {
        // ボタン離した時: 対応するMIDI Note OFF
        if (currentMidiNote[i] != 0) {
          MIDI.sendNoteOff(currentMidiNote[i], 0, 1);
          Serial.print("[BLE-MIDI] Note OFF: "); Serial.print(currentMidiNote[i]); Serial.println();
          Serial.print("[DEBUG] Button"); Serial.print(i + 1); Serial.println(" release detected!");
          
          currentMidiNote[i] = 0; // ノート送信状態リセット
        }
        buttonPressed[i] = false; // ボタン押下状態リセット
      }
      
      lastButtonStates[i] = reading;
    }
  }

  delay(1); // バッファ詰まり対策で短縮
}

// MIDI CC受信処理（コントローラー16, チャンネル1, 1バイトで6パート）
void handleMidiCC(uint8_t channel, uint8_t controller, uint8_t value) {
  if (channel != 1 || controller != 16) return;
  for (int i = 0; i < 6; i++) {
    bool bit = (value >> i) & 0x01;
    if (ledStates[i] != bit) {
      // 状態が変わった場合のみトグル
      toggleLED(i); // LEDとledStates[]がトグルされる
    }
    // 変化なければ何もしない
  }
}


