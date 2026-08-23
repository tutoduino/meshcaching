#include "Protocol.h"

#include <string.h>

namespace meshcore {

bool parse(const uint8_t *raw, size_t len, PacketView &out) {
  if (len < 2) {
    return false;
  }
  uint8_t header = raw[0];
  out.routeType = header & 0x03;
  out.payloadType = (header >> 2) & 0x0F;

  size_t offset = 1;
  // Les modes "transport" ajoutent 4 octets de code de transport
  if (out.routeType == kRouteTransportFlood ||
      out.routeType == kRouteTransportDirect) {
    offset += 4;
  }
  if (offset >= len) {
    return false;
  }

  // Octet path_length : bits 0-5 = nombre de sauts, bits 6-7 = (taille du hash - 1)
  uint8_t pathLengthByte = raw[offset];
  out.hopCount = pathLengthByte & 0x3F;
  out.hashSize = ((pathLengthByte >> 6) & 0x03) + 1;
  offset += 1;

  out.path = raw + offset;
  offset += (size_t)out.hopCount * out.hashSize;
  if (offset > len) {
    return false;  // paquet incohérent
  }

  out.payload = raw + offset;
  out.payloadLen = len - offset;
  return true;
}

bool lastHopId(const PacketView &pkt, const uint8_t *&id, size_t &idLen) {
  // Un paquet TRACE commence par un tag aléatoire, pas par un hash :
  // il ne s'identifie que par appariement de tag (cf. traceTag()).
  if (pkt.payloadType == kPayloadTrace) {
    return false;
  }
  if (pkt.hopCount >= 1) {
    // Cas normal : le dernier hash du chemin est le dernier nœud traversé
    id = pkt.path + (size_t)(pkt.hopCount - 1) * pkt.hashSize;
    idLen = pkt.hashSize;
    return true;
  }
  if (pkt.payloadType == kPayloadAdvert && pkt.payloadLen >= kAdvertPubkeyLen) {
    // Annonce zéro-hop : la clé publique complète de l'émetteur ouvre le payload
    id = pkt.payload;
    idLen = kAdvertPubkeyLen;
    return true;
  }
  // Autres paquets zéro-hop : le début du payload n'identifie PAS
  // l'émetteur (hash du destinataire pour un message direct, hash de
  // canal pour un message de groupe, CRC pour un ACK...). L'ancienne
  // heuristique "premier octet = hash source" provoquait de fausses
  // détections : émetteur indéterminable, donc pas de correspondance.
  return false;
}

bool traceTag(const PacketView &pkt, uint32_t &tag) {
  if (pkt.payloadType != kPayloadTrace || pkt.payloadLen < sizeof(tag)) {
    return false;
  }
  memcpy(&tag, pkt.payload, sizeof(tag));
  return true;
}

size_t buildTracePing(uint8_t *out, uint32_t tag, uint8_t targetHash) {
  size_t offset = 0;
  // Header : payload TRACE + route DIRECT
  out[offset++] = (kPayloadTrace << 2) | kRouteDirect;
  // path_length (niveau routage) : 0 = zéro-hop, le paquet est émis une
  // seule fois sans être relayé plus loin (et pas d'octets de chemin)
  out[offset++] = 0x00;
  memcpy(out + offset, &tag, sizeof(tag));
  offset += sizeof(tag);
  uint32_t authCode = 0;  // pas de code d'authentification particulier
  memcpy(out + offset, &authCode, sizeof(authCode));
  offset += sizeof(authCode);
  out[offset++] = 0x00;  // flags, réservé pour l'instant
  // Liste des nœuds à tracer : un seul, la cible
  out[offset++] = targetHash;
  return offset;
}

}  // namespace meshcore
