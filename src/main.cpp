/**
 * @file    main.cpp
 * @brief   Géolocalisation d'un répéteur Meshcore
 *
 * @details Affiche le RSSI (niveau de signal) et le temps écoulé depuis la
 *          dernière réception d'un paquet provenant d'un répéteur Meshcore
 *          spécifique, sur un Seeed Wio Tracker L1 Pro.
 *
 * @author  Tutoduino
 * @see     https://tutoduino.fr/menu-sdr/geolocalisation-repeteur-meshcore/
 * @note    Adaptation Wio Tracker L1 Pro (nRF52840 + SX1262 + OLED SH1106).
 */
#include <Arduino.h>
#include <avr/dtostrf.h>  // dtostrf() : le %f de printf n'est pas fiable sur nRF52
#include <SPI.h>
#include <RadioLib.h>
#include <Adafruit_SH110X.h>
// =====================================================================
// 1) BROCHAGE - Seeed Wio Tracker L1 Pro (nRF52840 + SX1262)
// =====================================================================
// Les broches (P_LORA_*, SX126X_*, PIN_BUTTON1, SPI, I2C) sont definies
// dans variants/Seeed_Wio_Tracker_L1/variant.h, repris du firmware
// MeshCore. SPI.begin() et Wire.begin() s'utilisent donc sans arguments.
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDR DISPLAY_ADDRESS  // 0x3D : adresse du SH1106 (cf. variant.h)
#define BUTTON_PIN PIN_BUTTON1  // bouton utilisateur, utilise ici pour declencher un ping
// =====================================================================
// 2) PARAMETRES RADIO - Meshcore Ile de France
// =====================================================================
#define LORA_FREQ_MHZ 869.618
#define LORA_BW_KHZ 62.5
#define LORA_SF 8
#define LORA_CR 8
#define LORA_TX_POWER 14
// !! A ADAPTER : prefixe de la cle publique du repeteur MeshCore vise.
const uint8_t TARGET_PUBKEY_PREFIX[] = { 0x57, 0xDB };
#define TARGET_PUBKEY_PREFIX_LEN (sizeof(TARGET_PUBKEY_PREFIX))
// =====================================================================
// 3) FORMAT DE PAQUET MESHCORE (doc officielle meshcore-dev/MeshCore)
//    header (1 octet) = VV PPPP RR  (version / payload type / route type)
// =====================================================================
#define ROUTE_TYPE_TRANSPORT_FLOOD 0x00
#define ROUTE_TYPE_FLOOD 0x01
#define ROUTE_TYPE_DIRECT 0x02
#define ROUTE_TYPE_TRANSPORT_DIRECT 0x03
#define PAYLOAD_TYPE_ADVERT 0x04
#define PAYLOAD_TYPE_TRACE 0x09  // "trace un chemin" : c'est le ping natif de MeshCore
#define MAX_PACKET_LEN 256
#define ADVERT_PUBKEY_LEN 32  // une annonce contient la cle publique complete (32 octets)
// =====================================================================
// 4) ETAT GLOBAL DE L'APPLICATION
// =====================================================================
// On regroupe tout ce qui concerne "le dernier paquet vu du repeteur cible"
// dans une seule structure : c'est plus simple à lire qu'une liste de
// variables globales éparpillées.
struct RepeaterStatus {
  bool hasPacket = false;      // avons-nous deja recu un paquet du repeteur cible ?
  unsigned long lastSeenMs = 0;
  float rssi = 0;
  float snr = 0;
};
RepeaterStatus targetStatus;
// Tag de la derniere requete TRACE envoyee, et instant d'envoi : permet de
// reconnaitre la reponse (qui renvoie ce meme tag) sans avoir a en decoder
// un hash, puisque le format du payload TRACE est different des messages.
uint32_t lastSentTag = 0;
unsigned long lastPingMs = 0;
#define TRACE_REPLY_TIMEOUT_MS 10000UL  // on n'accepte une reponse que dans les 10s suivant le ping
SX1262 lora = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// Ce drapeau est mis a "true" par l'interruption radio des qu'un paquet arrive.
// On le traite ensuite tranquillement dans loop(), jamais dans l'interruption
// elle-meme (regle de base avec RadioLib / ESP32).
volatile bool packetReceived = false;
void onPacketReceivedISR() {
  packetReceived = true;
}
// =====================================================================
// 5) AFFICHAGE OLED
// =====================================================================
void showMessage(const char *ligne1, const char *ligne2 = "") {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(ligne1);
  display.println(ligne2);
  display.display();
}
void drawStatusScreen() {
  display.clearDisplay();
  if (!targetStatus.hasPacket) {
    showMessage("En attente de", "paquets MeshCore...");
    return;
  }
  // --- Titre : identifiant du repeteur surveille ---
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("REPETEUR %02X%02X", TARGET_PUBKEY_PREFIX[0], TARGET_PUBKEY_PREFIX[1]);
  // --- RSSI en grand, centre horizontalement ---
  display.setTextSize(3);
  char rssiStr[10];
  dtostrf(targetStatus.rssi, 0, 0, rssiStr);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(rssiStr, 0, 0, &x1, &y1, &w, &h);
  int16_t xCentre = (SCREEN_WIDTH - w) / 2;
  display.setCursor(xCentre, 26);
  display.print(rssiStr);
  display.setTextSize(1);
  display.print(" dBm");
  // --- SNR et temps ecoule depuis le dernier paquet ---
  unsigned long secondesEcoulees = (millis() - targetStatus.lastSeenMs) / 1000;
  display.setCursor(0, 54);
  char snrStr[10];
  dtostrf(targetStatus.snr, 0, 1, snrStr);
  display.printf("SNR:%sdB", snrStr);
  display.setCursor(90, 54);
  display.printf("%lus", secondesEcoulees);
  display.display();
}
// =====================================================================
// 6) INITIALISATION MATERIELLE
// =====================================================================
void initOled() {
  // Pas de rail Vext ni de reset dedie sur le Wio Tracker L1 : l'ecran
  // SH1106 est alimente en permanence, il suffit de demarrer le bus I2C
  // (broches fixees par la variante).
  Wire.begin();
  Wire.setClock(100000);
  Serial.println(F("Initialisation OLED..."));
  // Adresse nominale 0x3D (cf. variant.h) ; certains modules SH1106
  // repondent en 0x3C, on tente les deux avant d'abandonner.
  if (!display.begin(OLED_I2C_ADDR) && !display.begin(0x3C)) {
    Serial.println(F("Echec init SH1106"));
    while (true) {}  // on bloque ici : sans ecran, pas la peine de continuer
  }
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  display.display();
  delay(100);
}
void initRadio() {
  Serial.println(F("Initialisation LoRa..."));
  SPI.begin();  // broches SPI fixees par la variante
  // Le SX1262 du Wio Tracker L1 a un TCXO 1.8V sur DIO3 : sans cette
  // declaration, l'init de la radio echoue (le quartz ne demarre jamais).
  int state = lora.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                         RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER,
                         8, SX126X_DIO3_TCXO_VOLTAGE);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Erreur LoRa : "));
    Serial.println(state);
    showMessage("Erreur LoRa", String(state).c_str());
    while (true) {}
  }
  // Chaine RF de la carte : DIO2 pilote le switch d'antenne, et RadioLib
  // doit gerer la broche RXEN (valeurs reprises du firmware MeshCore).
  lora.setDio2AsRfSwitch(true);
  lora.setRfSwitchPins(SX126X_RXEN, SX126X_TXEN);
  lora.setCurrentLimit(140);        // limite de courant du PA
  lora.setRxBoostedGainMode(true);  // meilleure sensibilite en reception
  lora.setDio1Action(onPacketReceivedISR);
  state = lora.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Erreur startReceive : "));
    Serial.println(state);
  }
}
// =====================================================================
// 7) ENVOI D'UN "PING" (paquet TRACE) VERS LE REPETEUR CIBLE
// =====================================================================
// MeshCore a une fonctionnalite native pour ca : le payload TRACE (0x09),
// concu pour tracer un chemin et collecter le SNR de chaque saut. Avec un
// seul noeud dans la liste, c'est l'equivalent MeshCore d'un ping.
// Format confirme par la doc officielle (wiki MeshCore, "Companion Radio
// Protocol") : [tag 4o][auth_code 4o][flags 1o][liste de hash a tracer].
//
// Ici on vise un seul repeteur, en "zero-hop" (path_len=0 au niveau du
// paquet) : le paquet est emis une seule fois, sans etre relaye plus loin.
//
// Equivalent nRF52 de esp_random() : lecture de 4 octets du generateur
// aleatoire materiel. L'acces direct au peripherique RNG est permis tant
// que la pile BLE (SoftDevice) n'est pas activee — c'est le cas ici.
static uint32_t hwRandom32() {
  uint32_t value = 0;
  NRF_RNG->TASKS_START = 1;
  for (int i = 0; i < 4; i++) {
    NRF_RNG->EVENTS_VALRDY = 0;
    while (NRF_RNG->EVENTS_VALRDY == 0) {}
    value = (value << 8) | (uint8_t)(NRF_RNG->VALUE);
  }
  NRF_RNG->TASKS_STOP = 1;
  return value;
}
void sendTracePing() {
  uint8_t buf[16];
  size_t offset = 0;
  // --- Header du paquet : payload TRACE + route DIRECT ---
  buf[offset++] = (PAYLOAD_TYPE_TRACE << 2) | ROUTE_TYPE_DIRECT;
  // --- path_length du paquet (niveau routage) : 0 = zero-hop ---
  buf[offset++] = 0x00;
  // (pas d'octets de chemin a la suite puisqu'il n'y a aucun saut)
  // --- Payload TRACE ---
  uint32_t tag = hwRandom32();  // identifiant aleatoire de cette requete
  memcpy(buf + offset, &tag, sizeof(tag));
  offset += sizeof(tag);
  lastSentTag = tag;   // on retiendra ce tag pour reconnaitre la reponse
  lastPingMs = millis();
  uint32_t authCode = 0;  // pas de code d'authentification particulier
  memcpy(buf + offset, &authCode, sizeof(authCode));
  offset += sizeof(authCode);
  buf[offset++] = 0x00;  // flags, reserve pour l'instant
  // Liste des noeuds a tracer : un seul, le repeteur cible
  buf[offset++] = TARGET_PUBKEY_PREFIX[0];
  Serial.printf("Envoi TRACE (tag=%08lX) vers REPETEUR %02X...\n",
                (unsigned long)tag, TARGET_PUBKEY_PREFIX[0]);
  int state = lora.transmit(buf, offset);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Erreur d'emission : "));
    Serial.println(state);
  }
  // transmit() declenche aussi une interruption DIO1 de "fin d'emission",
  // qui peut avoir arme packetReceived a tort : on l'ignore avant de
  // repasser en ecoute, pour ne pas traiter du vide comme un paquet recu.
  packetReceived = false;
  lora.startReceive();
}
// =====================================================================
// 8) DECODAGE DU PAQUET MESHCORE
// =====================================================================
// Cette fonction cherche l'identifiant (hash de cle publique) du DERNIER
// noeud qui a retransmis le paquet, et le compare au prefixe du repeteur
// que l'on surveille. Elle renvoie true si ca correspond.
//
// Rappel du format d'un paquet MeshCore :
//   [header 1 octet] [transport_codes 0 ou 4 octets] [path_length 1 octet]
//   [path 0-64 octets] [payload 0-184 octets]
bool packetComesFromTarget(const uint8_t *packet, size_t len) {
  if (len < 2) return false;
  uint8_t header = packet[0];
  uint8_t routeType = header & 0x03;
  uint8_t payloadType = (header >> 2) & 0x0F;
  size_t offset = 1;
  // Les modes "transport" ajoutent 4 octets de code de transport
  if (routeType == ROUTE_TYPE_TRANSPORT_FLOOD || routeType == ROUTE_TYPE_TRANSPORT_DIRECT) {
    offset += 4;
  }
  if (offset >= len) return false;
  // Octet path_length : bits 0-5 = nombre de sauts, bits 6-7 = (taille du hash - 1)
  uint8_t pathLengthByte = packet[offset];
  uint8_t hopCount = pathLengthByte & 0x3F;
  uint8_t hashSize = ((pathLengthByte >> 6) & 0x03) + 1;
  offset += 1;
  const uint8_t *pathBytes = packet + offset;
  offset += (size_t)hopCount * hashSize;
  if (offset > len) return false;  // paquet incoherent, on l'ignore
  const uint8_t *payload = packet + offset;
  size_t payloadLen = len - offset;
  // Cas particulier : un paquet TRACE ne contient PAS de hash en tete de
  // payload (contrairement aux messages REQ/RESPONSE/TXT). Il commence par
  // un tag aleatoire de 4 octets. On ne peut donc pas le reconnaitre par
  // hash : on compare son tag a celui de notre dernier ping envoye.
  if (payloadType == PAYLOAD_TYPE_TRACE) {
    if (payloadLen < 4 || lastSentTag == 0) return false;
    if (millis() - lastPingMs > TRACE_REPLY_TIMEOUT_MS) return false;
    uint32_t receivedTag;
    memcpy(&receivedTag, payload, sizeof(receivedTag));
    return receivedTag == lastSentTag;
  }
  // Identifiant du dernier "sauteur" a comparer au repeteur cible
  const uint8_t *lastHopId = nullptr;
  size_t lastHopIdLen = hashSize;
  if (hopCount >= 1) {
    // Cas normal : le dernier hash du chemin est le dernier repeteur traverse
    lastHopId = pathBytes + (size_t)(hopCount - 1) * hashSize;
  } else if (payloadType == PAYLOAD_TYPE_ADVERT && payloadLen >= ADVERT_PUBKEY_LEN) {
    // Annonce sans saut : la cle publique complete de l'emetteur est en tete du payload
    lastHopId = payload;
    lastHopIdLen = ADVERT_PUBKEY_LEN;
  } else if (payloadLen >= 1) {
    // Paquet direct sans saut : le premier octet du payload est le hash source
    lastHopId = payload;
    lastHopIdLen = 1;
  }
  if (lastHopId == nullptr) return false;
  size_t compareLen = min(lastHopIdLen, (size_t)TARGET_PUBKEY_PREFIX_LEN);
  return memcmp(lastHopId, TARGET_PUBKEY_PREFIX, compareLen) == 0;
}
// =====================================================================
// 9) TRAITEMENT D'UN PAQUET RECU
// =====================================================================
void handleIncomingPacket() {
  uint8_t buf[MAX_PACKET_LEN];
  size_t len = lora.getPacketLength();
  if (len == 0 || len > sizeof(buf)) {
    lora.startReceive();
    return;
  }
  int state = lora.readData(buf, len);
  float rssi = lora.getRSSI();
  float snr = lora.getSNR();
  // On ignore les paquets illisibles OU dont le CRC est invalide :
  // un paquet corrompu ne doit jamais etre interprete comme venant du
  // repeteur cible (risque de fausse detection).
  if (state != RADIOLIB_ERR_NONE) {
    if (state != RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.print(F("Erreur de reception : "));
      Serial.println(state);
    }
    lora.startReceive();
    return;
  }
  bool isTarget = packetComesFromTarget(buf, len);
  char rssiStr[12], snrStr[12];
  dtostrf(rssi, 0, 1, rssiStr);
  dtostrf(snr, 0, 1, snrStr);
  Serial.printf("Paquet recu : len=%u RSSI=%s dBm SNR=%s dB %s\n",
                (unsigned)len, rssiStr, snrStr, isTarget ? "[REPETEUR CIBLE]" : "");
  if (isTarget) {
    targetStatus.hasPacket = true;
    targetStatus.lastSeenMs = millis();
    targetStatus.rssi = rssi;
    targetStatus.snr = snr;
    drawStatusScreen();  // mise a jour immediate de l'ecran
  }
  lora.startReceive();
}
// =====================================================================
// 10) SETUP / LOOP
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // bouton utilisateur, relie a la masse quand presse
  initOled();
  showMessage("Initialisation...");
  initRadio();
  Serial.println(F("En attente de paquets MeshCore..."));
  showMessage("En attente de", "paquets MeshCore...");
}
void loop() {
  // Rafraichit l'ecran une fois par seconde (pour le compteur "temps ecoule")
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate >= 1000) {
    lastDisplayUpdate = millis();
    drawStatusScreen();
  }
  // Appui sur le bouton utilisateur : on force un ping TRACE vers le repeteur cible,
  // au lieu d'attendre passivement son prochain paquet.
  static unsigned long lastButtonPress = 0;
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastButtonPress > 500) {
    lastButtonPress = millis();
    sendTracePing();
  }
  if (packetReceived) {
    packetReceived = false;
    handleIncomingPacket();
  }
}
