/*
  Multiprise commandée par bot Telegram — ESP32, 2 relais

  Bibliothèques requises :
    - UniversalTelegramBot
    - ArduinoJson (version 6.x recommandée)

  Les identifiants (WiFi, jeton du bot, identifiant de conversation) sont
  définis dans le fichier "secrets.h", non publié sur GitHub.
  Copier "secrets.h.example" en "secrets.h" puis le compléter.
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "secrets.h"   // WIFI_SSID, WIFI_PASSWORD, BOT_TOKEN, OWNER_CHAT_ID

// --- Broches relais (actifs à LOW) ---
const int RELAY1_PIN = 23;  // Prise 1
const int RELAY2_PIN = 22;  // Prise 2

// --- Etat logique en mémoire ---
bool relay1On = false;
bool relay2On = false;

// --- Telegram bot ---
WiFiClientSecure secureClient;
UniversalTelegramBot bot(BOT_TOKEN, secureClient);

// --- Temporisation polling Telegram ---
unsigned long lastBotCheck = 0;
const unsigned long BOT_POLL_MS = 1200;

// --- Fonctions utilitaires ---
void setRelay(int pin, bool on) {
  // Actif à LOW : on -> LOW, off -> HIGH
  digitalWrite(pin, on ? LOW : HIGH);
}

void sendStatus(const String& chat_id) {
  String s = "🔌 *Etat multiprise*\n";
  s += String("• Prise 1 : ") + (relay1On ? "ON ✅" : "OFF ⛔") + "\n";
  s += String("• Prise 2 : ") + (relay2On ? "ON ✅" : "OFF ⛔") + "\n";
  bot.sendMessage(chat_id, s, "Markdown");
}

void sendHelp(const String& chat_id) {
  String h =
    "🤖 *Commandes disponibles*\n"
    "`ON1` / `OFF1`  → Prise 1\n"
    "`ON2` / `OFF2`  → Prise 2\n"
    "`ALLON`         → Tout ON\n"
    "`ALLOFF`        → Tout OFF\n"
    "`STATUS`        → Etat des prises\n"
    "`/help`         → Aide";
  bot.sendMessage(chat_id, h, "Markdown");
}

void applyCommand(const String& chat_id, String text) {
  // Normaliser
  text.trim();
  text.toUpperCase();

  if (text == "/START") {
    bot.sendMessage(chat_id, "👋 Bienvenue ! Envoie `/help` pour voir les commandes.", "Markdown");
    sendStatus(chat_id);
    return;
  }
  if (text == "/HELP") {
    sendHelp(chat_id);
    return;
  }

  if (text == "ON1")  { relay1On = true;  setRelay(RELAY1_PIN, relay1On); bot.sendMessage(chat_id, "✅ Prise 1 : ON"); return; }
  if (text == "OFF1") { relay1On = false; setRelay(RELAY1_PIN, relay1On); bot.sendMessage(chat_id, "⛔ Prise 1 : OFF"); return; }

  if (text == "ON2")  { relay2On = true;  setRelay(RELAY2_PIN, relay2On); bot.sendMessage(chat_id, "✅ Prise 2 : ON"); return; }
  if (text == "OFF2") { relay2On = false; setRelay(RELAY2_PIN, relay2On); bot.sendMessage(chat_id, "⛔ Prise 2 : OFF"); return; }

  if (text == "ALLON")  {
    relay1On = relay2On = true;
    setRelay(RELAY1_PIN, relay1On);
    setRelay(RELAY2_PIN, relay2On);
    bot.sendMessage(chat_id, "✅ Toutes les prises : ON");
    return;
  }
  if (text == "ALLOFF") {
    relay1On = relay2On = false;
    setRelay(RELAY1_PIN, relay1On);
    setRelay(RELAY2_PIN, relay2On);
    bot.sendMessage(chat_id, "⛔ Toutes les prises : OFF");
    return;
  }

  if (text == "STATUS") { sendStatus(chat_id); return; }

  // Commande inconnue
  bot.sendMessage(chat_id, "❓ Commande inconnue. Envoie `/help`.");
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text    = bot.messages[i].text;
    String from    = bot.messages[i].from_name;

    // Filtrage propriétaire : seule la conversation autorisée peut commander les prises
    if (chat_id != String(OWNER_CHAT_ID)) {
      bot.sendMessage(chat_id, "⛔ Accès refusé.");
      // Notification au propriétaire
      bot.sendMessage(OWNER_CHAT_ID, "⚠️ Tentative d'accès par @" + from + " (chat_id=" + chat_id + ")");
      continue;
    }

    applyCommand(chat_id, text);
  }
}

void connectWiFi() {
  Serial.print("WiFi: connexion à ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - t0 > 20000) break; // 20s timeout
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connecté");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Echec WiFi (reboot dans 5s)...");
    delay(5000);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Broches relais
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  // Etat initial: OFF (HIGH car relais actif à LOW)
  setRelay(RELAY1_PIN, false);
  setRelay(RELAY2_PIN, false);

  connectWiFi();

  // Simplification TLS : le certificat du serveur n'est pas vérifié.
  // Pour un usage durable, configurer le certificat racine de Telegram.
  secureClient.setInsecure();

  bot.sendMessage(OWNER_CHAT_ID, "🚀 Multiprise connectée au WiFi et au bot.");
  sendStatus(OWNER_CHAT_ID);
}

void loop() {
  // Maintenir le WiFi
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Polling des messages Telegram
  if (millis() - lastBotCheck > BOT_POLL_MS) {
    int numNew = bot.getUpdates(bot.last_message_received + 1);
    while (numNew) {
      handleNewMessages(numNew);
      numNew = bot.getUpdates(bot.last_message_received + 1);
    }
    lastBotCheck = millis();
  }
}
