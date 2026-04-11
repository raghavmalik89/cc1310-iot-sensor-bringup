#ifndef TELEMETRY_PROTOCOL_H
#define TELEMETRY_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define TELEMETRY_PROTOCOL_VERSION                0x01u

#define TELEMETRY_PACKET_TYPE_TELEMETRY          0x10u
#define TELEMETRY_PACKET_TYPE_STATUS             0x20u
#define TELEMETRY_PACKET_TYPE_DEBUG              0x30u
#define TELEMETRY_PACKET_TYPE_RESERVED           0x40u

#define TELEMETRY_PAYLOAD_LENGTH                 12u

#define TELEMETRY_OFFSET_PROTOCOL_VERSION        0u
#define TELEMETRY_OFFSET_PACKET_TYPE             1u
#define TELEMETRY_OFFSET_DEVICE_ID               2u
#define TELEMETRY_OFFSET_STATUS_FLAGS            3u
#define TELEMETRY_OFFSET_PACKET_COUNTER_HI       4u
#define TELEMETRY_OFFSET_PACKET_COUNTER_LO       5u
#define TELEMETRY_OFFSET_TEMPERATURE_HI          6u
#define TELEMETRY_OFFSET_TEMPERATURE_LO          7u
#define TELEMETRY_OFFSET_HUMIDITY_HI             8u
#define TELEMETRY_OFFSET_HUMIDITY_LO             9u
#define TELEMETRY_OFFSET_BATTERY_MV_HI           10u
#define TELEMETRY_OFFSET_BATTERY_MV_LO           11u

#define TELEMETRY_STATUS_SENSOR_DATA_VALID       (1u << 0)
#define TELEMETRY_STATUS_AHT2X_READ_FAULT        (1u << 1)
#define TELEMETRY_STATUS_ENS160_READ_FAULT       (1u << 2)
#define TELEMETRY_STATUS_BATTERY_VALID           (1u << 3)
#define TELEMETRY_STATUS_LOW_BATTERY             (1u << 4)
#define TELEMETRY_STATUS_WATCHDOG_RESET          (1u << 5)

typedef struct
{
    uint8_t protocol_version;
    uint8_t packet_type;
    uint8_t device_id;
    uint8_t status_flags;
    uint16_t packet_counter;
    int16_t temperature_centi_c;
    uint16_t humidity_centi_rh;
    uint16_t battery_mv;
} TelemetryPacket;

static inline void telemetryProtocolEncodeU16Be(uint8_t *payload, uint8_t offset, uint16_t value)
{
    payload[offset] = (uint8_t)(value >> 8);
    payload[offset + 1u] = (uint8_t)(value & 0xFFu);
}

static inline uint16_t telemetryProtocolDecodeU16Be(const uint8_t *payload, uint8_t offset)
{
    return (uint16_t)(((uint16_t)payload[offset] << 8) | (uint16_t)payload[offset + 1u]);
}

static inline void telemetryProtocolEncodeS16Be(uint8_t *payload, uint8_t offset, int16_t value)
{
    telemetryProtocolEncodeU16Be(payload, offset, (uint16_t)value);
}

static inline int16_t telemetryProtocolDecodeS16Be(const uint8_t *payload, uint8_t offset)
{
    return (int16_t)telemetryProtocolDecodeU16Be(payload, offset);
}

static inline bool telemetryProtocolHasSupportedVersion(uint8_t protocol_version)
{
    return (protocol_version == TELEMETRY_PROTOCOL_VERSION);
}

static inline bool telemetryProtocolIsTelemetryType(uint8_t packet_type)
{
    return (packet_type == TELEMETRY_PACKET_TYPE_TELEMETRY);
}

static inline void telemetryProtocolEncodeTelemetry(uint8_t *payload, const TelemetryPacket *packet)
{
    payload[TELEMETRY_OFFSET_PROTOCOL_VERSION] = packet->protocol_version;
    payload[TELEMETRY_OFFSET_PACKET_TYPE] = packet->packet_type;
    payload[TELEMETRY_OFFSET_DEVICE_ID] = packet->device_id;
    payload[TELEMETRY_OFFSET_STATUS_FLAGS] = packet->status_flags;

    telemetryProtocolEncodeU16Be(payload, TELEMETRY_OFFSET_PACKET_COUNTER_HI, packet->packet_counter);
    telemetryProtocolEncodeS16Be(payload, TELEMETRY_OFFSET_TEMPERATURE_HI, packet->temperature_centi_c);
    telemetryProtocolEncodeU16Be(payload, TELEMETRY_OFFSET_HUMIDITY_HI, packet->humidity_centi_rh);
    telemetryProtocolEncodeU16Be(payload, TELEMETRY_OFFSET_BATTERY_MV_HI, packet->battery_mv);
}

static inline bool telemetryProtocolDecodeTelemetry(const uint8_t *payload, uint8_t length, TelemetryPacket *packet)
{
    if ((payload == 0) || (packet == 0) || (length != TELEMETRY_PAYLOAD_LENGTH))
    {
        return false;
    }

    packet->protocol_version = payload[TELEMETRY_OFFSET_PROTOCOL_VERSION];
    packet->packet_type = payload[TELEMETRY_OFFSET_PACKET_TYPE];
    packet->device_id = payload[TELEMETRY_OFFSET_DEVICE_ID];
    packet->status_flags = payload[TELEMETRY_OFFSET_STATUS_FLAGS];
    packet->packet_counter = telemetryProtocolDecodeU16Be(payload, TELEMETRY_OFFSET_PACKET_COUNTER_HI);
    packet->temperature_centi_c = telemetryProtocolDecodeS16Be(payload, TELEMETRY_OFFSET_TEMPERATURE_HI);
    packet->humidity_centi_rh = telemetryProtocolDecodeU16Be(payload, TELEMETRY_OFFSET_HUMIDITY_HI);
    packet->battery_mv = telemetryProtocolDecodeU16Be(payload, TELEMETRY_OFFSET_BATTERY_MV_HI);

    return true;
}

#endif
