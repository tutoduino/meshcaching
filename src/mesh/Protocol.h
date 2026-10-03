#pragma once
#include <stddef.h>
#include <stdint.h>

// =====================================================================
// MeshCore packet format (official meshcore-dev/MeshCore doc):
//   [header 1 byte] [transport_codes 0 or 4 bytes] [path_length 1 byte]
//   [path 0-64 bytes] [payload 0-184 bytes]
// header = VV PPPP RR (version / payload type / route type)
//
// Pure protocol module: no hardware dependency, no application policy
// (picking "which repeater is the target" stays in the application).
// =====================================================================
namespace meshcore {

constexpr uint8_t kRouteTransportFlood = 0x00;
constexpr uint8_t kRouteFlood = 0x01;
constexpr uint8_t kRouteDirect = 0x02;
constexpr uint8_t kRouteTransportDirect = 0x03;

constexpr uint8_t kPayloadAdvert = 0x04;
constexpr uint8_t kPayloadTrace = 0x09;  // "trace a path": the native MeshCore ping

constexpr size_t kMaxPacketLen = 256;
constexpr size_t kAdvertPubkeyLen = 32;  // an advert carries the full public key

// TRACE ping size: fixed part + one hash of 1, 2, 4 or 8 bytes
constexpr size_t kTraceHeaderLen = 11;   // header + path_len + tag + auth + flags
constexpr size_t kMaxTraceHashSize = 8;
constexpr size_t kTracePingLen = kTraceHeaderLen + kMaxTraceHashSize;  // max buffer size


// Decoded view of a packet: the pointers reference the original buffer,
// nothing is copied.
struct PacketView {
  uint8_t routeType;
  uint8_t payloadType;
  uint8_t hopCount;
  uint8_t hashSize;
  const uint8_t *path;
  const uint8_t *payload;
  size_t payloadLen;
};

// Parses a raw packet. Returns false if the packet is inconsistent.
bool parse(const uint8_t *raw, size_t len, PacketView &out);

// Identifier of the last node that sent this packet: last hash of the
// path or, for a zero-hop advert, the full public key of the sender.
// Returns false when undeterminable: TRACE packets (no hash at the head
// of the payload, see traceTag()) and every other zero-hop packet -
// their payload identifies the recipient or the channel, never the
// sender.
bool lastHopId(const PacketView &pkt, const uint8_t *&id, size_t &idLen);

// Tag of a TRACE packet (false if the packet is not one, or is too
// short). This tag, echoed as-is by the traced node, is what lets a
// reply be matched to our ping.
bool traceTag(const PacketView &pkt, uint32_t &tag);

// Builds a zero-hop TRACE ping to a single node, identified by the
// first hashSize bytes of its public key. hashSize must be 1, 2, 4 or 8
// (it is encoded in the flags byte: 0, 1, 2 or 3). out must be at least
// kTracePingLen bytes long; returns the size written, or 0 if hashSize
// is invalid.
// Payload format (MeshCore wiki, "Companion Radio Protocol"):
//   [tag 4B][auth_code 4B][flags 1B][list of hashes to trace]
size_t buildTracePing(uint8_t *out, uint32_t tag,
                      const uint8_t *targetHash, uint8_t hashSize);

}  // namespace meshcore
