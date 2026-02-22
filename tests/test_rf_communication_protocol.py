import pytest

# === STANDALONE RF PROTOCOL LOGIC (copied & simplified) ===
def encode_rf_packet(frame_type: str, payload: bytes) -> bytes:
    """Encode RF packet: [SYNC:2][TYPE:1][LEN:1][PAYLOAD:LEN][CRC:1]"""
    SYNC = b'\xAA\x55'
    TYPE_MAP = {
        "DATA": b'\x01',
        "ACK": b'\x02',
        "CMD": b'\x03'
    }
    if frame_type not in TYPE_MAP:
        raise ValueError(f"Unknown frame type: {frame_type}")
    type_byte = TYPE_MAP[frame_type]
    length_byte = len(payload).to_bytes(1, 'big')
    crc = (sum(payload) & 0xFF).to_bytes(1, 'big')
    return SYNC + type_byte + length_byte + payload + crc

def decode_rf_packet(data: bytes) -> dict:
    """Decode RF packet. Returns dict or raises ValueError."""
    if len(data) < 5:
        raise ValueError("Packet too short")
    if data[:2] != b'\xAA\x55':
        raise ValueError("Invalid sync bytes")
    frame_type_byte = data[2]
    length = data[3]
    # Check if we have enough bytes for payload + CRC
    if len(data) < 4 + length + 1:
        raise ValueError("Packet too short for payload and CRC")
    payload = data[4:4+length]
    if len(payload) != length:
        raise ValueError("Payload length mismatch")
    crc = data[4+length]
    expected_crc = (sum(payload) & 0xFF)
    if crc != expected_crc:
        raise ValueError(f"CRC mismatch: got {crc}, expected {expected_crc}")
    TYPE_REV = {b'\x01': "DATA", b'\x02': "ACK", b'\x03': "CMD"}
    type_byte = frame_type_byte.to_bytes(1, 'big')
    if type_byte not in TYPE_REV:
        raise ValueError(f"Unknown frame type byte: {frame_type_byte}")
    return {
        "type": TYPE_REV[type_byte],
        "payload": payload,
        "crc": crc
    }

def is_ack_required(frame_type: str) -> bool:
    """Determine if ACK is required for frame type."""
    return frame_type in ["DATA", "CMD"]


# === TESTS ===
def test_encode_rf_packet_data():
    payload = b"HELLO"
    pkt = encode_rf_packet("DATA", payload)
    assert pkt[:2] == b'\xAA\x55'
    assert pkt[2] == 0x01
    assert pkt[3] == len(payload)
    assert pkt[4:4+len(payload)] == payload
    expected_crc = (sum(payload) & 0xFF)
    assert pkt[4+len(payload)] == expected_crc

def test_encode_rf_packet_ack():
    pkt = encode_rf_packet("ACK", b"")
    assert pkt[:2] == b'\xAA\x55'
    assert pkt[2] == 0x02
    assert pkt[3] == 0
    expected_crc = 0  # sum of empty bytes is 0
    assert pkt[4] == expected_crc

def test_decode_rf_packet_success():
    payload = b"TEST"
    pkt = encode_rf_packet("DATA", payload)
    result = decode_rf_packet(pkt)
    assert result["type"] == "DATA"
    assert result["payload"] == payload
    expected_crc = (sum(payload) & 0xFF)
    assert result["crc"] == expected_crc

def test_decode_rf_packet_ack():
    pkt = encode_rf_packet("ACK", b"")
    result = decode_rf_packet(pkt)
    assert result["type"] == "ACK"
    assert result["payload"] == b""
    assert result["crc"] == 0

def test_decode_rf_packet_cmd():
    payload = b"CMD"
    pkt = encode_rf_packet("CMD", payload)
    result = decode_rf_packet(pkt)
    assert result["type"] == "CMD"
    assert result["payload"] == payload

def test_decode_rf_packet_invalid_sync():
    pkt = b'\xFF\xFF\x01\x04TEST\x00'
    with pytest.raises(ValueError, match="Invalid sync bytes"):
        decode_rf_packet(pkt)

def test_decode_rf_packet_short():
    pkt = b'\xAA\x55\x01\x04TE'
    with pytest.raises(ValueError, match="Packet too short"):
        decode_rf_packet(pkt)

def test_decode_rf_packet_crc_mismatch():
    payload = b"TEST"
    pkt = encode_rf_packet("DATA", payload)
    # Corrupt the CRC byte
    pkt = pkt[:-1] + bytes([(pkt[-1] + 1) & 0xFF])
    with pytest.raises(ValueError, match="CRC mismatch"):
        decode_rf_packet(pkt)

def test_is_ack_required():
    assert is_ack_required("DATA") is True
    assert is_ack_required("ACK") is False
    assert is_ack_required("CMD") is True
    assert is_ack_required("UNKNOWN") is False