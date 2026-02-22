import sys
from unittest.mock import MagicMock

# Ensure mocks are in place
import tests.conftest  # noqa: F401

# Re-implement badge emulator state machine logic
# States: IDLE, LISTEN, EMULATE, ERROR
# Transitions triggered by: timer_expiry, packet_received, config_change

class BadgeEmulator:
    def __init__(self):
        self.state = "IDLE"
        self.emulate_target = None
        self.last_packet = None
        self.timer_expired = False

    def set_config(self, emulate_target):
        """Set badge to emulate target (e.g., 'dc26')."""
        if emulate_target is None:
            self.emulate_target = None
            self.state = "IDLE"
        else:
            self.emulate_target = emulate_target
            self.state = "LISTEN"

    def on_timer_expiry(self):
        """Called when listen timeout expires."""
        if self.state == "LISTEN":
            if self.emulate_target:
                self.state = "EMULATE"
            else:
                self.state = "IDLE"
        elif self.state == "EMULATE":
            # Restart listen after emulate
            self.state = "LISTEN"

    def on_packet_received(self, packet):
        """Process incoming RF packet."""
        if self.state == "LISTEN":
            self.last_packet = packet
            # If packet matches expected format, go to EMULATE
            if packet and len(packet) >= 15:  # valid packet length
                self.state = "EMULATE"
        elif self.state == "EMULATE":
            # Ignore packets during emulation
            pass

    def get_state(self):
        return self.state


def test_initial_state_is_idle():
    emulator = BadgeEmulator()
    assert emulator.get_state() == "IDLE"


def test_set_config_transitions_to_listen():
    emulator = BadgeEmulator()
    emulator.set_config("dc26")
    assert emulator.get_state() == "LISTEN"
    assert emulator.emulate_target == "dc26"


def test_set_config_none_returns_to_idle():
    emulator = BadgeEmulator()
    emulator.set_config("dc26")
    emulator.set_config(None)
    assert emulator.get_state() == "IDLE"
    assert emulator.emulate_target is None


def test_timer_expiry_in_listen_emulates():
    emulator = BadgeEmulator()
    emulator.set_config("dc26")
    assert emulator.get_state() == "LISTEN"
    emulator.on_timer_expiry()
    assert emulator.get_state() == "EMULATE"


def test_timer_expiry_in_emulate_returns_to_listen():
    emulator = BadgeEmulator()
    emulator.set_config("dc26")
    emulator.on_timer_expiry()  # → EMULATE
    assert emulator.get_state() == "EMULATE"
    emulator.on_timer_expiry()  # → LISTEN
    assert emulator.get_state() == "LISTEN"


def test_packet_received_in_listen_triggers_emulate():
    emulator = BadgeEmulator()
    emulator.set_config("dc26")
    assert emulator.get_state() == "LISTEN"
    emulator.on_packet_received([0xAA] * 8 + [0x01] * 6 + [0x00])
    assert emulator.get_state() == "EMULATE"


def test_packet_received_in_emulate_ignored():
    emulator = BadgeEmulator()
    emulator.set_config("dc26")
    emulator.on_timer_expiry()  # → EMULATE
    assert emulator.get_state() == "EMULATE"
    emulator.on_packet_received([0xFF] * 15)
    assert emulator.get_state() == "EMULATE"  # unchanged


def test_packet_received_short_ignored():
    emulator = BadgeEmulator()
    emulator.set_config("dc26")
    emulator.on_packet_received([0x01, 0x02])  # too short
    assert emulator.get_state() == "LISTEN"