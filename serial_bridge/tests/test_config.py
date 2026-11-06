import pytest

from config import ConfigError, Settings, load_settings

VALID = {
    "serial_port": "COM3",
    "backend_url": "https://cuymonitor.duckdns.org",
    "api_key": "key",
    "cage_id": "cage-1",
}


def test_valid_settings_are_accepted():
    assert Settings(**VALID).send_interval_seconds == 10


@pytest.mark.parametrize(
    ("change", "message"),
    [
        ({"serial_port": ""}, "SERIAL_PORT"),
        ({"api_key": ""}, "API_KEY"),
        ({"api_key": "replace-with-your-backend-api-key"}, "API_KEY"),
        ({"cage_id": "x" * 51}, "CAGE_ID"),
        ({"backend_url": "http://cuymonitor.duckdns.org"}, "https"),
        ({"backend_url": "ftp://localhost"}, "https"),
        ({"send_interval_seconds": 5}, "SEND_INTERVAL_SECONDS"),
    ],
)
def test_invalid_settings_say_what_is_wrong(change, message):
    with pytest.raises(ConfigError, match=message):
        Settings(**{**VALID, **change})


def test_plain_http_is_allowed_only_for_a_backend_on_this_machine():
    assert Settings(**{**VALID, "backend_url": "http://localhost:8080"})


def test_settings_are_read_from_the_env_file(tmp_path, monkeypatch):
    for name in ("SERIAL_PORT", "BACKEND_URL", "API_KEY", "CAGE_ID", "SEND_INTERVAL_SECONDS"):
        monkeypatch.delenv(name, raising=False)
    env = tmp_path / ".env"
    env.write_text(
        "SERIAL_PORT=/dev/ttyACM0\nBACKEND_URL=https://cuymonitor.duckdns.org/\nAPI_KEY=abc\n"
        "CAGE_ID=cage-2\nSEND_INTERVAL_SECONDS=15\n"
    )

    settings = load_settings(env)

    assert settings == Settings("/dev/ttyACM0", "https://cuymonitor.duckdns.org", "abc", "cage-2", 15.0)
