// YARG Drum Pads - Arduino Leonardo (versão digital TTL LM393)
// Cada pad/prato conectado a uma entrada digital (0 ou 1)
// Envia teclas HID para YARG

#include <Keyboard.h>

// ----------------------
// CONFIGURAÇÕES GERAIS
// ----------------------
const bool IS_USE_LOG_HITS = false;
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

const int N_PADS = 7;
const uint8_t padPins[N_PADS] = {4, 5, 6, 7, 8, 9, 10};
const char    keyMap[N_PADS]  = {'z', 'x', 'c', 'v', 's', 'd', 'f'};

// true = conectado, false = ignorar
bool padEnabled[N_PADS] = {true, true, false, false, false, false, false};

// ----------------------
// VARIÁVEIS GLOBAIS
// ----------------------
unsigned long lastHitTime[N_PADS];
bool lastState[N_PADS];
unsigned long hitCount = 0;

// ----------------------
// SETUP
// ----------------------
void setup() {
  Serial.begin(115200);
  Keyboard.begin();

  for (int i = 0; i < N_PADS; i++) {
    pinMode(padPins[i], INPUT);
    lastState[i] = HIGH;
    lastHitTime[i] = 0;
  }

  pinMode(pedal1ButtonPin, INPUT_PULLUP);
  pinMode(pedal2ButtonPin, INPUT_PULLUP);

  Serial.println("YARG Drum TTL (Digital) iniciado!");
}

// ----------------------
// LOOP PRINCIPAL
// ----------------------
void loop() {
  unsigned long now = millis();

  // --- LEITURA DOS PADS ---
  for (int i = 0; i < N_PADS; i++) {
    if (!padEnabled[i]) continue;
    int state = digitalRead(padPins[i]);

    if (state == LOW && lastState[i] == HIGH) {
      if (now - lastHitTime[i] > DEBOUNCE_MS) {
        sendKeyHit(i);
        lastHitTime[i] = now;
      }
    }
    lastState[i] = state;
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
    Keyboard.press('b'); // tecla do hi-hat (ajuste conforme o YARG)
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
