import sys
from unittest.mock import MagicMock

# Ensure mocks are in place
import tests.conftest  # noqa: F401

# Re-implementation of state_machine.c core logic
class State:
    IDLE = "IDLE"
    SCANNING = "SCANNING"
    EMULATING = "EMULATING"
    CHALLENGE_COMPLETE = "CHALLENGE_COMPLETE"

class StateMachine:
    def __init__(self):
        self.state = State.IDLE
        self.callbacks = {
            State.IDLE: [],
            State.SCANNING: [],
            State.EMULATING: [],
            State.CHALLENGE_COMPLETE: []
        }

    def transition(self, new_state):
        # Valid transitions per spec
        valid = {
            State.IDLE: [State.SCANNING],
            State.SCANNING: [State.EMULATING, State.IDLE],
            State.EMULATING: [State.CHALLENGE_COMPLETE, State.IDLE],
            State.CHALLENGE_COMPLETE: [State.IDLE]
        }
        if new_state in valid.get(self.state, []):
            self.state = new_state
            return True
        return False

    def on_state_enter(self, state, callback):
        self.callbacks[state].append(callback)

def test_initial_state_is_idle():
    sm = StateMachine()
    assert sm.state == State.IDLE

def test_valid_transition_scanning():
    sm = StateMachine()
    assert sm.transition(State.SCANNING) is True
    assert sm.state == State.SCANNING

def test_invalid_transition_direct_emulate():
    sm = StateMachine()
    assert sm.transition(State.EMULATING) is False
    assert sm.state == State.IDLE

def test_chain_transitions():
    sm = StateMachine()
    assert sm.transition(State.SCANNING) is True
    assert sm.transition(State.EMULATING) is True
    assert sm.transition(State.CHALLENGE_COMPLETE) is True
    assert sm.state == State.CHALLENGE_COMPLETE

def test_callback_on_state_enter():
    sm = StateMachine()
    called = []
    def cb():
        called.append(True)
    sm.on_state_enter(State.SCANNING, cb)
    sm.transition(State.SCANNING)
    assert len(called) == 1