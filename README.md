# KantanPartChanger

## ハードウェア要件

- [M5Stack StampS3](https://www.switch-science.com/products/10377)（メインボード）
- 6 x [Grove Mech Keycap](https://wiki.seeedstudio.com/Grove-Mech_Keycap/)（ボタンとLED一体モジュール）
- [説軸、バッテリー管理モジュール](https://www.switch-science.com/products/9689)（接続・バッテリー管理） 
**この機器はKANTAN_Play（通称かんぷれ）専用で、独自ファームウェアが必要です。**

## 概要

KantanPartChangerは、物理ボタンまたはMIDI Control Changeメッセージを使って6つのパート（チャンネル）をオン/オフ切り替えできるBLE-MIDIコントローラーです。各パートはNeoPixel LEDで表現され、状態とバッテリー残量に基づいて色が変わります。

**かんぷれ用独自ファームウェアは以下に存在します:**  
https://github.com/nobynobynoby/KANTAN_Play_core-nori-fork/tree/feature/kantanpartchanger-support

![全体像](img/IMG_20250912_203441_1.jpg)

## 機能

- **BLE-MIDI対応**: Bluetooth Low EnergyでワイヤレスMIDI制御。
- **6パート制御**: ボタンまたはMIDI CCで各パートをオン/オフ。
- **LEDフィードバック**: NeoPixel LEDで視覚表示。
  - パート1-3: オン時に緑
  - パート4-6: オン時に紫
  - バッテリーインジケーター: 20%以下で黄、5%以下で赤
- **バッテリー監視**: 移動平均によるリアルタイムバッテリー電圧監視。
- **MIDI CC処理**: チャンネル1のCC16でパート切り替えに対応。

## ソフトウェア要件

- PlatformIO
- ESP32用Arduinoフレームワーク
- ライブラリ:
  - Adafruit NeoPixel
  - BLE-MIDI（BLEMIDI_Transport経由）

## 使用方法

1. デバイスを電源オン。
2. DAWやMIDIアプリからBLE-MIDIで接続。
3. ボタンでパートを切り替え、またはチャンネル1のCC16でパート状態をビットマスクで送信。


### ノートオン/オフとパートの対応

ボタン操作時、またはパート状態変更時に以下のノートが送信されます（チャンネル1）:

- **パートオン時**: ノートオン (C5=60, D5=62, E5=64, F5=66, G5=68, A5=70)
- **パートオフ時**: ノートオフ (C#5=61, D#5=63, F5=65, F#5=67, G#5=69, A#5=71)

パートの有効/無効状態がノートオン/オフに対応しています。  
**かんぷれ側の設定で、これらのノートをパートオン/オフに割り当ててください。**

<video src="img/VID_20250912_174708.mp4" controls></video>

## ライセンス

このプロジェクトはMITライセンスの下で公開されています - 詳細はLICENSEファイルを参照してください。