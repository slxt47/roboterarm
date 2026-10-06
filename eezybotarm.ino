/*
 * EEZYbotARM MK2 mit Joy-it Gamepad (PS2-Protokoll) am Arduino Uno
 *
 * Bibliothek: "PS2X" von Bill Porter (madsci1016/Arduino-PS2X) –
 * im Bibliotheksverwalter nach "PS2X" suchen oder das ZIP von GitHub einbinden.
 *
 * Gamepad-Empfänger  -> Arduino (wie in der Joy-it Anleitung)
 *   Data      (1)    -> Pin 13
 *   Command   (2)    -> Pin 11
 *   GND       (4)    -> GND
 *   Power     (5)    -> 3.3 V
 *   Attention (6)    -> Pin 10
 *   Clock     (7)    -> Pin 12
 *
 * Servos (Signalleitung)     -> Arduino
 *   Basis-Drehung (MG995/946) -> Pin 3
 *   Hauptarm / Schulter      -> Pin 5
 *   Vorderarm / Ellbogen     -> Pin 6
 *   Greifer (SG90)           -> Pin 9
 *   Servo + und -            -> EXTERNES 5-6 V Netzteil (min. 3 A), GND mit Arduino-GND verbinden!
 *
 * Bedienung:
 *   Linker Stick  X : Basis drehen
 *   Linker Stick  Y : Hauptarm heben/senken
 *   Rechter Stick Y : Vorderarm vor/zurück
 *   R1 / L1         : Greifer schließen / öffnen
 *   START           : Arm in Grundstellung fahren
 */

#include <PS2X_lib.h>
#include <Servo.h>

// --- Gamepad-Pins ---
#define PS2_DAT 13
#define PS2_CMD 11
#define PS2_ATT 10
#define PS2_CLK 12

// --- Servo-Pins ---
#define PIN_BASE     5
#define PIN_SHOULDER 6
#define PIN_ELBOW    3
#define PIN_GRIPPER  9

PS2X ps2x;
Servo servoBase, servoShoulder, servoElbow, servoGripper;

// Aktuelle Positionen (Grad) – Grundstellung
int posBase     = 90;
int posShoulder = 90;
int posElbow    = 90;
int posGripper  = 90;

// Bewegungsgrenzen – nach dem Zusammenbau an den eigenen Arm anpassen!
const int BASE_MIN = 0,   BASE_MAX = 180;
const int SHO_MIN  = 40,  SHO_MAX  = 140;
const int ELB_MIN  = 40,  ELB_MAX  = 140;
const int GRIP_OPEN = 60, GRIP_CLOSED = 120;

const int DEADZONE = 20;   // Stick-Totzone um die Mittelstellung (128)
const int STEP     = 2;    // Grad pro Durchlauf -> Geschwindigkeit
const int LOOP_MS  = 20;   // Zykluszeit

int stickToStep(int value) {
  int delta = value - 128;
  if (abs(delta) < DEADZONE) return 0;
  return (delta > 0) ? STEP : -STEP;
}

void attachAll() {
  servoBase.attach(PIN_BASE);
  servoShoulder.attach(PIN_SHOULDER);
  servoElbow.attach(PIN_ELBOW);
  servoGripper.attach(PIN_GRIPPER);
}

void writeAll() {
  servoBase.write(posBase);
  servoShoulder.write(posShoulder);
  servoElbow.write(posElbow);
  servoGripper.write(posGripper);
}

void setup() {
  Serial.begin(57600);
  attachAll();
  writeAll();

  // Gamepad konfigurieren (clock, command, attention, data, pressures, rumble)
  int error = 1;
  while (error != 0) {
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_ATT, PS2_DAT, false, false);
    if (error != 0) {
      Serial.print("Gamepad nicht gefunden, Fehler ");
      Serial.println(error);
      delay(1000);
    }
  }
  Serial.println("Gamepad bereit.");
}

void loop() {
  ps2x.read_gamepad(false, 0);

  // Sticks -> relative Bewegung
  posBase     += stickToStep(ps2x.Analog(PSS_LX));
  posShoulder -= stickToStep(ps2x.Analog(PSS_LY));   // Stick nach vorne = heben
  posElbow    -= stickToStep(ps2x.Analog(PSS_RY));

  // Greifer
  if (ps2x.Button(PSB_R1)) posGripper += STEP;
  if (ps2x.Button(PSB_L1)) posGripper -= STEP;

  // Grundstellung
  if (ps2x.ButtonPressed(PSB_START)) {
    posBase = 90; posShoulder = 90; posElbow = 90; posGripper = 90;
  }

  // Grenzen einhalten
  posBase     = constrain(posBase,     BASE_MIN, BASE_MAX);
  posShoulder = constrain(posShoulder, SHO_MIN,  SHO_MAX);
  posElbow    = constrain(posElbow,    ELB_MIN,  ELB_MAX);
  posGripper  = constrain(posGripper,  GRIP_OPEN, GRIP_CLOSED);

  writeAll();
  delay(LOOP_MS);
}
