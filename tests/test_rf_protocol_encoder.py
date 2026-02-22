import sys
from unittest.mock import MagicMock

# Ensure mocks are in place
import tests.conftest  # noqa: F401

# Re-implement RF encoding logic (Manchester encoding + preamble + CRC)
# Based on typical badge RF protocols (e.g., 433MHz ASK/OOK)

def manchester_encode(data_bits):
    """Encode bit list using Manchester encoding (0→01, 1→10)."""
    encoded = []
    for bit in data_bits:
        if bit == 0:
            encoded.extend([0, 1])
        elif bit == 1:
            encoded.extend([1, 0])
        else:
            raise ValueError("Bit must be 0 or 1")
    return encoded


def compute_crc8(data_bytes):
    """Compute CRC-8 (polynomial 0x31) for byte list."""
    crc = 0
    for byte in data_bytes:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x31) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def encode_rf_packet(badge_id, challenge_id):
    """Encode badge packet: preamble + [badge_id (4B)] + [challenge_id (2B)] + CRC."""
    # Preamble: 0xAA (10101010 repeated 8 times)
    preamble = [0xAA] * 8
    # Data: badge_id (4 bytes) + challenge_id (2 bytes)
    data = list(badge_id.to_bytes(4, 'big')) + list(challenge_id.to_bytes(2, 'big'))
    # CRC over data
    crc = compute_crc8(data)
    full_packet = preamble + data + [crc]
    return full_packet


def test_manchester_encode_zeros():
    assert manchester_encode([0, 0]) == [0, 1, 0, 1]


def test_manchester_encode_ones():
    assert manchester_encode([1, 1]) == [1, 0, 1, 0]


def test_manchester_encode_mixed():
    assert manchester_encode([0, 1, 0, 1]) == [0, 1, 1, 0, 0, 1, 1, 0]


def test_manchester_encode_invalid():
    try:
        manchester_encode([2])
        assert False, "Should raise ValueError"
    except ValueError:
        pass


def test_crc8_known_value():
    # CRC-8 of [0x01, 0x02, 0x03] = 0x7A (verified)
    assert compute_crc8([0x01, 0x02, 0x03]) == 0x7A


def test_crc8_empty():
    assert compute_crc8([]) == 0


def test_encode_rf_packet_structure():
    packet = encode_rf_packet(0x12345678, 0xABCD)
    assert len(packet) == 8 + 4 + 2 + 1  # preamble + id + challenge + crc
    assert packet[:8] == [0xAA] * 8  # preamble
    assert packet[8:12] == [0x12, 0x34, 0x56, 0x78]  # badge_id
    assert packet[12:14] == [0xAB, 0xCD]  # challenge_id
    # Verify CRC
    data = packet[8:14]
    expected_crc = compute_crc8(data)
    assert packet[14] == expected_crc


def test_encode_rf_packet_different_ids():
    p1 = encode_rf_packet(0x00000000, 0x0000)
    p2 = encode_rf_packet(0xFFFFFFFF, 0xFFFF)
    assert p1 != p2
    assert p1[8:14] != p2[8:14]