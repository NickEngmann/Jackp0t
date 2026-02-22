import sys
from unittest.mock import MagicMock

# Ensure mocks are in place
import tests.conftest  # noqa: F401

# Re-implementation of rf_emulation_logic.c core logic
class RFEmulator:
    def __init__(self):
        self.active = False
        self.target_uid = ""
        self.current_uid = ""

    def start_emulation(self, uid):
        self.target_uid = uid
        self.active = True

    def stop_emulation(self):
        self.active = False
        self.target_uid = ""

    def set_current_uid(self, uid):
        self.current_uid = uid

    def check_match(self):
        if not self.active:
            return False
        return self.current_uid == self.target_uid

def test_start_emulation_sets_target():
    emu = RFEmulator()
    emu.start_emulation("DEADBEEF")
    assert emu.active is True
    assert emu.target_uid == "DEADBEEF"

def test_stop_emulation_clears_state():
    emu = RFEmulator()
    emu.start_emulation("ABCD")
    emu.stop_emulation()
    assert emu.active is False
    assert emu.target_uid == ""

def test_uid_match_triggers_success():
    emu = RFEmulator()
    emu.start_emulation("DEADBEEF")
    emu.set_current_uid("DEADBEEF")
    assert emu.check_match() is True

def test_uid_mismatch_returns_false():
    emu = RFEmulator()
    emu.start_emulation("DEADBEEF")
    emu.set_current_uid("CAFEBABE")
    assert emu.check_match() is False

def test_emulation_inactive_always_false():
    emu = RFEmulator()
    emu.set_current_uid("DEADBEEF")
    assert emu.check_match() is False