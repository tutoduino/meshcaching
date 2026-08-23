#pragma once
#include <stddef.h>
#include <stdint.h>

// =====================================================================
// Format de paquet MeshCore (doc officielle meshcore-dev/MeshCore) :
//   [header 1 octet] [transport_codes 0 ou 4 octets] [path_length 1 octet]
//   [path 0-64 octets] [payload 0-184 octets]
// header = VV PPPP RR (version / payload type / route type)
//
// Module purement protocolaire : aucune dépendance matérielle, aucune
// politique applicative (le choix de "qui est le répéteur cible" reste
// dans l'application).
// =====================================================================
namespace meshcore {

constexpr uint8_t kRouteTransportFlood = 0x00;
constexpr uint8_t kRouteFlood = 0x01;
constexpr uint8_t kRouteDirect = 0x02;
constexpr uint8_t kRouteTransportDirect = 0x03;

constexpr uint8_t kPayloadAdvert = 0x04;
constexpr uint8_t kPayloadTrace = 0x09;  // "trace un chemin" : le ping natif de MeshCore

constexpr size_t kMaxPacketLen = 256;
constexpr size_t kAdvertPubkeyLen = 32;  // une annonce embarque la clé publique complète
constexpr size_t kTracePingLen = 12;     // header + path_len + tag + auth + flags + 1 hash

// Vue décodée d'un paquet : les pointeurs désignent le tampon d'origine,
// rien n'est copié.
struct PacketView {
  uint8_t routeType;
  uint8_t payloadType;
  uint8_t hopCount;
  uint8_t hashSize;
  const uint8_t *path;
  const uint8_t *payload;
  size_t payloadLen;
};

// Découpe un paquet brut. Renvoie false si le paquet est incohérent.
bool parse(const uint8_t *raw, size_t len, PacketView &out);

// Identifiant du dernier nœud ayant émis ce paquet : dernier hash du
// chemin, ou l'émetteur lui-même pour un paquet zéro-hop. Renvoie false
// si indéterminable — c'est le cas des paquets TRACE, qui n'ont pas de
// hash en tête de payload (cf. traceTag()).
bool lastHopId(const PacketView &pkt, const uint8_t *&id, size_t &idLen);

// Tag d'un paquet TRACE (false si le paquet n'en est pas un, ou est
// trop court). C'est ce tag, renvoyé tel quel par le nœud tracé, qui
// permet d'apparier une réponse à notre ping.
bool traceTag(const PacketView &pkt, uint32_t &tag);

// Construit un ping TRACE zéro-hop vers un seul nœud, identifié par le
// premier octet du hash de sa clé publique. out doit faire au moins
// kTracePingLen octets ; renvoie la taille écrite.
// Format du payload (wiki MeshCore, "Companion Radio Protocol") :
//   [tag 4o][auth_code 4o][flags 1o][liste de hash à tracer]
size_t buildTracePing(uint8_t *out, uint32_t tag, uint8_t targetHash);

}  // namespace meshcore
