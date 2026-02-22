import pytest

# === STANDALONE CONFIG PARSER LOGIC (copied & simplified from C) ===
def parse_config_line(line: str):
    """Parse a single config line: 'KEY = VALUE' or 'KEY=VALUE' or comment/empty."""
    line = line.strip()
    if not line or line.startswith('#'):
        return None, None
    if '=' not in line:
        return None, None
    # Split on first '=' only
    key, value = line.split('=', 1)
    key = key.strip()
    # If key is empty after stripping, it's invalid
    if not key:
        return None, None
    value = value.strip()
    # Trim quotes if present
    if value.startswith('"') and value.endswith('"'):
        value = value[1:-1]
    return key, value

def parse_config_string(config_str: str) -> dict:
    """Parse multi-line config string into dict."""
    config = {}
    for line in config_str.strip().splitlines():
        key, value = parse_config_line(line)
        if key:
            config[key] = value
    return config

def validate_config(config: dict) -> tuple[bool, list[str]]:
    """Validate required keys and value constraints."""
    errors = []
    required_keys = ['badge_type', 'challenge_timeout_ms', 'rf_channel']
    for key in required_keys:
        if key not in config:
            errors.append(f"Missing required key: {key}")
    if 'badge_type' in config and config['badge_type'] not in ['original', 'emulated', 'clone']:
        errors.append("badge_type must be 'original', 'emulated', or 'clone'")
    if 'challenge_timeout_ms' in config:
        try:
            val = int(config['challenge_timeout_ms'])
            if val <= 0 or val > 60000:
                errors.append("challenge_timeout_ms must be 1–60000")
        except ValueError:
            errors.append("challenge_timeout_ms must be an integer")
    return len(errors) == 0, errors


# === TESTS ===
def test_parse_config_line_valid():
    assert parse_config_line("badge_type = original") == ("badge_type", "original")
    assert parse_config_line("badge_type=emulated") == ("badge_type", "emulated")
    assert parse_config_line('rf_channel = "11"') == ("rf_channel", "11")

def test_parse_config_line_empty():
    assert parse_config_line("") == (None, None)
    assert parse_config_line("   ") == (None, None)

def test_parse_config_line_comment():
    assert parse_config_line("# This is a comment") == (None, None)

def test_parse_config_line_no_equals():
    assert parse_config_line("invalid line") == (None, None)

def test_parse_config_line_invalid():
    assert parse_config_line("= no_key") == (None, None)
    assert parse_config_line("= ") == (None, None)
    assert parse_config_line("key = ") == ("key", "")

def test_parse_config_string():
    config_str = """
    badge_type = original
    # This is a comment
    rf_channel = "11"
    """
    result = parse_config_string(config_str)
    assert result == {"badge_type": "original", "rf_channel": "11"}

def test_validate_config_success():
    config = {
        "badge_type": "original",
        "challenge_timeout_ms": "5000",
        "rf_channel": "11"
    }
    valid, errors = validate_config(config)
    assert valid is True
    assert errors == []

def test_validate_config_missing_key():
    config = {"badge_type": "original"}
    valid, errors = validate_config(config)
    assert valid is False
    assert "Missing required key: challenge_timeout_ms" in errors

def test_validate_config_invalid_type():
    config = {
        "badge_type": "invalid_type",
        "challenge_timeout_ms": "5000",
        "rf_channel": "11"
    }
    valid, errors = validate_config(config)
    assert valid is False
    assert "badge_type must be 'original', 'emulated', or 'clone'" in errors

def test_validate_config_timeout_range():
    config = {
        "badge_type": "original",
        "challenge_timeout_ms": "70000",
        "rf_channel": "11"
    }
    valid, errors = validate_config(config)
    assert valid is False
    assert "challenge_timeout_ms must be 1–60000" in errors

def test_validate_config_timeout_non_integer():
    config = {
        "badge_type": "original",
        "challenge_timeout_ms": "abc",
        "rf_channel": "11"
    }
    valid, errors = validate_config(config)
    assert valid is False
    assert "challenge_timeout_ms must be an integer" in errors