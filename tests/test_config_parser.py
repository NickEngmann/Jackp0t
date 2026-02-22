import sys
from unittest.mock import MagicMock, patch

# Ensure mocks are in place
import tests.conftest  # noqa: F401

# Re-implement core logic of config_parser.c (as allowed per exception)
class Config:
    def __init__(self):
        self.challenge_id = 0
        self.target_uid = ""
        self.emulate = False

def parse_config(json_str):
    """Re-implementation of config_parser.c logic (core algorithm only)"""
    if not json_str:
        return None
    try:
        # Simulate cJSON parsing
        import json
        data = json.loads(json_str)
        cfg = Config()
        cfg.challenge_id = int(data.get('challenge_id', 0))
        cfg.target_uid = str(data.get('target_uid', ''))
        cfg.emulate = bool(data.get('emulate', False))
        return cfg
    except (json.JSONDecodeError, ValueError, TypeError):
        return None

# Now import the *real* module if it exists (e.g., if wrapped in Python)
# But since it's C, we test our re-implementation
def test_parse_valid_config():
    json_str = '{"challenge_id": 27, "target_uid": "DEADBEEF", "emulate": true}'
    cfg = parse_config(json_str)
    assert cfg is not None
    assert cfg.challenge_id == 27
    assert cfg.target_uid == "DEADBEEF"
    assert cfg.emulate is True

def test_parse_missing_fields_uses_defaults():
    json_str = '{"challenge_id": 1}'
    cfg = parse_config(json_str)
    assert cfg is not None
    assert cfg.challenge_id == 1
    assert cfg.target_uid == ""
    assert cfg.emulate is False

def test_parse_invalid_json_returns_none():
    json_str = '{invalid json}'
    cfg = parse_config(json_str)
    assert cfg is None

def test_parse_empty_string_returns_none():
    cfg = parse_config("")
    assert cfg is None

def test_parse_missing_challenge_id_uses_zero():
    json_str = '{"target_uid": "ABCD", "emulate": true}'
    cfg = parse_config(json_str)
    assert cfg is not None
    assert cfg.challenge_id == 0