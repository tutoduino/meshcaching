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
  // The "transport" modes add 4 bytes of transport code
  if (out.routeType == kRouteTransportFlood ||
      out.routeType == kRouteTransportDirect) {
    offset += 4;
  }
  if (offset >= len) {
    return false;
  }

  // path_length byte: bits 0-5 = hop count, bits 6-7 = (hash size - 1)
  uint8_t pathLengthByte = raw[offset];
  out.hopCount = pathLengthByte & 0x3F;
  out.hashSize = ((pathLengthByte >> 6) & 0x03) + 1;
  offset += 1;

  out.path = raw + offset;
  offset += (size_t)out.hopCount * out.hashSize;
  if (offset > len) {
    return false;  // inconsistent packet
  }

  out.payload = raw + offset;
  out.payloadLen = len - offset;
  return true;
}

bool lastHopId(const PacketView &pkt, const uint8_t *&id, size_t &idLen) {
  // A TRACE packet starts with a random tag, not with a hash: it can
  // only be identified by tag matching (see traceTag()).
  if (pkt.payloadType == kPayloadTrace) {
    return false;
  }
  if (pkt.hopCount >= 1) {
    // Normal case: the last hash of the path is the last node traversed
    id = pkt.path + (size_t)(pkt.hopCount - 1) * pkt.hashSize;
    idLen = pkt.hashSize;
    return true;
  }
  if (pkt.payloadType == kPayloadAdvert && pkt.payloadLen >= kAdvertPubkeyLen) {
    // Zero-hop advert: the sender's full public key opens the payload
    id = pkt.payload;
    idLen = kAdvertPubkeyLen;
    return true;
  }
  // Other zero-hop packets: the start of the payload does NOT identify
  // the sender (recipient hash for a direct message, channel hash for a
  // group message, CRC for an ACK...). The old "first byte = source
  // hash" heuristic caused false detections: sender undeterminable, so
  // no match.
  return false;
}

bool traceTag(const PacketView &pkt, uint32_t &tag) {
  if (pkt.payloadType != kPayloadTrace || pkt.payloadLen < sizeof(tag)) {
    return false;
  }
  memcpy(&tag, pkt.payload, sizeof(tag));
  return true;
}

size_t buildTracePing(uint8_t *out, uint32_t tag,
                      const uint8_t *targetHash, uint8_t hashSize) {
  uint8_t flags;
  switch (hashSize) {
    case 1: flags = 0x00; break;
    case 2: flags = 0x01; break;
    case 4: flags = 0x02; break;
    case 8: flags = 0x03; break;
    default: return 0;  // invalid hash size
  }

  size_t offset = 0;
  // Header: TRACE payload + DIRECT route
  out[offset++] = (kPayloadTrace << 2) | kRouteDirect;
  // path_length (routing level): 0 = zero-hop, the packet is sent only
  // once and is not relayed any further (and no path bytes)
  out[offset++] = 0x00;
  memcpy(out + offset, &tag, sizeof(tag));
  offset += sizeof(tag);
  uint32_t authCode = 0;  // no specific authentication code
  memcpy(out + offset, &authCode, sizeof(authCode));
  offset += sizeof(authCode);
  out[offset++] = flags;  // bits 0-1 = hash size of the traced path
  // List of nodes to trace: a single one, the target
  memcpy(out + offset, targetHash, hashSize);
  offset += hashSize;
  return offset;
}

}  // namespace meshcore
