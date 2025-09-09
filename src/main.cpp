#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>

// BLE-MIDI設定
BLEMIDI_CREATE_INSTANCE("KantanPartChanger", MIDI);

// 音階マッピング（C5からA5）
const uint8_t midiNotes[6] = {60, 62, 64, 65, 67, 69}; // C5, D5, E5, F5, G5, A5

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
const uint32_t debounceMs = 50;

// 汎用LED切り替え関数（MIDI送信なし）
void toggleLED(uint8_t index) {
  if (!ledStates[index]) {
    // ランダムカラー点灯
    uint8_t r = random(0, 256);
    uint8_t g = random(0, 256);
    uint8_t b = random(0, 256);
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
  // ボタンピン設定（配列化）
  for (int i = 0; i < 6; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLDOWN);
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
  Serial.println("[BLE-MIDI] Initialized - Device: KantanPartChanger");
  
  Serial.println("[DEBUG] Setup complete");
  // ピン情報表示（配列化）
  for (int i = 0; i < 6; i++) {
    Serial.print("LED"); Serial.print(i + 1); Serial.print("_PIN: "); Serial.println(LED_PINS[i]);
    Serial.print("BUTTON"); Serial.print(i + 1); Serial.print("_PIN: "); Serial.println(BUTTON_PINS[i]);
  }
}

void loop() {
  // BLE-MIDI処理
  MIDI.read();
  
  uint32_t now = millis();
  
  // 全ボタンを配列で処理
  for (int i = 0; i < 6; i++) {
    uint8_t reading = digitalRead(BUTTON_PINS[i]);
    
    if (reading != lastButtonStates[i]) {
      lastDebounceTimes[i] = now;
      Serial.print("[DEBUG] Button"); Serial.print(i + 1); Serial.println(" state changed");
      
      if (reading == HIGH && lastButtonStates[i] == LOW) {
        // ボタン押下時: MIDI Note ON (LOW→HIGH)
        MIDI.sendNoteOn(midiNotes[i], 127, 1);
        Serial.print("[BLE-MIDI] Note ON: "); Serial.print(NOTE_NAMES[i]); 
        Serial.print(" ("); Serial.print(midiNotes[i]); Serial.println(")");
        Serial.print("[DEBUG] Button"); Serial.print(i + 1); Serial.println(" press detected!");
        
        // ボタンが押されたときのみLED切り替え（Note ONとは独立）
        if (!buttonPressed[i]) {
          toggleLED(i);
          buttonPressed[i] = true;
        }
        
      } else if (reading == LOW && lastButtonStates[i] == HIGH) {
        // ボタン離した時: MIDI Note OFF (HIGH→LOW)
        MIDI.sendNoteOff(midiNotes[i], 0, 1);
        Serial.print("[BLE-MIDI] Note OFF: "); Serial.print(NOTE_NAMES[i]); 
        Serial.print(" ("); Serial.print(midiNotes[i]); Serial.println(")");
        Serial.print("[DEBUG] Button"); Serial.print(i + 1); Serial.println(" release detected!");
        
        buttonPressed[i] = false; // ボタン押下状態リセット
      }
      
      lastButtonStates[i] = reading;
    }
  }

  delay(50);
}

