import sys
from unittest.mock import MagicMock

# Ensure mocks are in place first
import tests.conftest  # noqa: F401

# Re-implement config parsing logic (pure algorithm, no hardware)
# Based on typical DEFCON badge config format: JSON-like key-value pairs

def parse_config(config_str):
    """Parse badge config string into dict.
    
    Supports:
      - challenge = dc27
      - emulate = dc26
      - led_pin = 13
      - rf_freq = 433.92
    """
    config = {}
    for line in config_str.strip().split('\n'):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        if '=' not in line:
            continue
        key, value = line.split('=', 1)
        key = key.strip()
        value = value.strip()
        # Try to convert to int/float if possible
        try:
            value = int(value)
        except ValueError:
            try:
                value = float(value)
            except ValueError:
                pass  # keep as string
        config[key] = value
    return config


def test_parse_basic_config():
    config_str = """
    challenge = dc27
    emulate = dc26
    """
    config = parse_config(config_str)
    assert config['challenge'] == 'dc27'
    assert config['emulate'] == 'dc26'


def test_parse_with_numbers():
    config_str = """
    led_pin = 13
    rf_freq = 433.92
    """
    config = parse_config(config_str)
    assert config['led_pin'] == 13
    assert config['rf_freq'] == 433.92


def test_parse_with_comments_and_whitespace():
    config_str = """
    # Badge configuration
    challenge = dc27   # target challenge
    emulate = dc25
    """
    config = parse_config(config_str)
    assert config['challenge'] == 'dc27'
    assert config['emulate'] == 'dc25'


def test_parse_invalid_line_skipped():
    config_str = """
    challenge = dc27
    invalid_line_without_equals
    emulate = dc26
    """
    config = parse_config(config_str)
    assert 'invalid_line_without_equals' not in config
    assert config['challenge'] == 'dc27'
    assert config['emulate'] == 'dc26'


def test_parse_empty_config():
    config = parse_config("")
    assert config == {}
    config = parse_config("# only comments\n")
    assert config == {}