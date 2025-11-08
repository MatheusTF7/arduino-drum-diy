// YARG Drum Pads - Arduino Leonardo (versão analógica com piezo)
// Usa leitura analógica para detecção de batidas e envia teclas HID via Keyboard.
// Envia teclas HID para YARG

#include <Keyboard.h>
#include <math.h>

// ----------------------
// CONFIGURAÇÕES GERAIS
// ----------------------
const bool IS_USE_LOG_CALIBRATION = false;
const bool IS_USE_LOG_HITS = false;
const bool IS_USE_LOG_RAW = false;

const bool IS_USE_HIT_PAUSE = true;
const int  HITS_BEFORE_PAUSE = 50;
const unsigned long PAUSE_DURATION_MS = 3000;
const unsigned long DEBOUNCE_MS = 50;
const int HIT_DELAY = 3;

// ----------------------
// MAPEAMENTO DE HARDWARE
// ----------------------
const int pedal1ButtonPin = 2; // Kick
const int pedal2ButtonPin = 3; // Hi-Hat

const int N_PADS = 4;
const uint8_t padPins[N_PADS] = {A0, A1, A2, A3};
const char    keyMap[N_PADS]  = {'z', 'x', 'c', 'v'};
bool padEnabled[N_PADS]       = {true, true, true, true};

// ----------------------
// CALIBRAÇÃO / SENSIBILIDADE
// ----------------------
const int BASELINE_SAMPLES = 200;
const int MIN_THRESHOLD = 50;
const int MULT = 6;
int baseline[N_PADS];
int thresh[N_PADS];

// ----------------------
// VARIÁVEIS GLOBAIS
// ----------------------
unsigned long lastHitTime[N_PADS];
unsigned long hitCount = 0;

// ----------------------
// SETUP
// ----------------------
void setup() {
  if (IS_USE_LOG_CALIBRATION) {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("Calibrando baseline dos pads...");
  }

  pinMode(pedal1ButtonPin, INPUT_PULLUP);
  pinMode(pedal2ButtonPin, INPUT_PULLUP);
  Keyboard.begin();

  // Calibração dos pads
  for (int i = 0; i < N_PADS; i++) {
    if (!padEnabled[i]) continue;
    long sum = 0, sumSq = 0;
    for (int s = 0; s < BASELINE_SAMPLES; s++) {
      int v = analogRead(padPins[i]);
      sum += v;
      sumSq += (long)v * v;
      delay(2);
    }
    float avg = (float)sum / BASELINE_SAMPLES;
    float var = ((float)sumSq / BASELINE_SAMPLES) - (avg * avg);
    float stdv = sqrt(max(0.0, var));
    baseline[i] = (int)round(avg);
    thresh[i] = max(MIN_THRESHOLD, (int)round(stdv * MULT));
    lastHitTime[i] = 0;

    if (IS_USE_LOG_CALIBRATION) {
      Serial.print("Pad "); Serial.print(i);
      Serial.print(": baseline="); Serial.print(baseline[i]);
      Serial.print(" thresh="); Serial.println(thresh[i]);
    }
  }

  if (IS_USE_LOG_CALIBRATION) Serial.println("Calibração concluída.");
  Serial.println("YARG Drum (Analógico) iniciado!");
}

// ----------------------
// LOOP PRINCIPAL
// ----------------------
void loop() {
  unsigned long now = millis();

  // --- LEITURA DOS PADS ---
  for (int i = 0; i < N_PADS; i++) {
    if (!padEnabled[i]) continue;

    int raw = analogRead(padPins[i]);
    int delta = raw - baseline[i];
    if (delta < 0) delta = 0;

    if (delta >= thresh[i] && (now - lastHitTime[i] > DEBOUNCE_MS)) {
      sendKeyHit(i);
      lastHitTime[i] = now;

      if (IS_USE_LOG_RAW) {
        Serial.print("Raw pad "); Serial.print(i);
        Serial.print(" -> "); Serial.println(raw);
      }
    }
  }

  // --- LEITURA DOS PEDAIS ---
  checkKickPedal();
  checkHiHatPedal();

  delay(2);
}

// ----------------------
// FUNÇÕES AUXILIARES
// ----------------------
void sendKeyHit(int padIndex) {
  char k = keyMap[padIndex];
  Keyboard.press(k);
  delay(HIT_DELAY);
  Keyboard.release(k);

  handleHitEvent(String("Pad ") + String(padIndex) + " (" + String(k) + ")");
}

void checkKickPedal() {
  static bool lastKickState = HIGH;
  int currentKickState = digitalRead(pedal1ButtonPin);

  if (currentKickState == LOW && lastKickState == HIGH) {
    Keyboard.press(' ');
    delay(HIT_DELAY);
    Keyboard.release(' ');
    handleHitEvent("Kick");
  }

  lastKickState = currentKickState;
}

void checkHiHatPedal() {
  static bool lastHiHatState = HIGH;
  int currentHiHatState = digitalRead(pedal2ButtonPin);

  if (currentHiHatState == LOW && lastHiHatState == HIGH) {
    Keyboard.press('b'); // tecla de Hi-Hat (ajustável)
    delay(HIT_DELAY);
    Keyboard.release('b');
    handleHitEvent("HiHat");
  }

  lastHiHatState = currentHiHatState;
}

void handleHitEvent(String origin) {
  hitCount++;

  if (IS_USE_LOG_HITS) {
    Serial.print("Hit #");
    Serial.print(hitCount);
    Serial.print(" - ");
    Serial.println(origin);
  }

  if (IS_USE_HIT_PAUSE && hitCount % HITS_BEFORE_PAUSE == 0) {
    if (IS_USE_LOG_HITS) {
      Serial.print("Pausa automática de ");
      Serial.print(PAUSE_DURATION_MS / 1000);
      Serial.println(" segundos...");
    }
    delay(PAUSE_DURATION_MS);
  }
}
