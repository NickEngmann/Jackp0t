import pytest

# === STANDALONE BADGE EMULATION STATE MACHINE (copied & simplified) ===
class BadgeEmulator:
    """Minimal emulation state machine for badge challenges."""
    def __init__(self):
        self.reset()

    def reset(self):
        self.state = "IDLE"
        self.challenge_id = None
        self.attempts = 0
        self.max_attempts = 3
        self.emulated_type = None

    def start_emulation(self, badge_type: str):
        if badge_type not in ["original", "emulated", "clone"]:
            return False
        self.emulated_type = badge_type
        self.state = "EMULATING"
        return True

    def receive_challenge(self, challenge_id: str):
        if self.state != "EMULATING":
            return False
        self.challenge_id = challenge_id
        self.state = "PROCESSING"
        return True

    def submit_solution(self, solution: str):
        if self.state != "PROCESSING":
            return False
        # Simulate correct solution for demo
        if solution == "DEFCON27":
            self.state = "COMPLETED"
            return True
        else:
            self.attempts += 1
            if self.attempts >= self.max_attempts:
                self.state = "FAILED"
                return False
            else:
                self.state = "EMULATING"
                return False

    def get_state(self):
        return self.state


# === TESTS ===
def test_emulator_initial_state():
    emu = BadgeEmulator()
    assert emu.state == "IDLE"
    assert emu.emulated_type is None

def test_start_emulation_success():
    emu = BadgeEmulator()
    assert emu.start_emulation("emulated") is True
    assert emu.state == "EMULATING"
    assert emu.emulated_type == "emulated"

def test_start_emulation_invalid_type():
    emu = BadgeEmulator()
    assert emu.start_emulation("unknown") is False
    assert emu.state == "IDLE"

def test_receive_challenge():
    emu = BadgeEmulator()
    emu.start_emulation("clone")
    assert emu.receive_challenge("test_challenge") is True
    assert emu.challenge_id == "test_challenge"
    assert emu.state == "PROCESSING"

def test_receive_challenge_wrong_state():
    emu = BadgeEmulator()
    assert emu.receive_challenge("test_challenge") is False

def test_submit_solution_success():
    emu = BadgeEmulator()
    emu.start_emulation("emulated")
    emu.receive_challenge("challenge1")
    assert emu.submit_solution("DEFCON27") is True
    assert emu.state == "COMPLETED"

def test_submit_solution_failure():
    emu = BadgeEmulator()
    emu.start_emulation("emulated")
    emu.receive_challenge("challenge1")
    assert emu.submit_solution("wrong") is False
    assert emu.attempts == 1
    assert emu.state == "EMULATING"

def test_submit_solution_max_attempts():
    emu = BadgeEmulator()
    emu.start_emulation("emulated")
    emu.receive_challenge("challenge1")
    
    # First wrong attempt
    assert emu.submit_solution("wrong") is False
    assert emu.attempts == 1
    assert emu.state == "EMULATING"
    
    # Reset to PROCESSING state for second attempt
    emu.receive_challenge("challenge1")
    
    # Second wrong attempt
    assert emu.submit_solution("wrong") is False
    assert emu.attempts == 2
    assert emu.state == "EMULATING"
    
    # Reset to PROCESSING state for third attempt
    emu.receive_challenge("challenge1")
    
    # Third wrong attempt - should fail
    assert emu.submit_solution("wrong") is False
    assert emu.attempts == 3
    assert emu.state == "FAILED"

def test_submit_solution_wrong_state():
    emu = BadgeEmulator()
    assert emu.submit_solution("DEFCON27") is False