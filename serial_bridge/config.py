"""Settings of the bridge, from serial_bridge/.env (never committed) or the environment."""

import os
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import urlparse

from dotenv import load_dotenv

BAUD_RATE = 115200
# the backend ingests at most one weight every 10 s (AGENTS.md, bridge rule 2)
MIN_SEND_INTERVAL_SECONDS = 10
MAX_QUEUE = 1000
_LOCAL_HOSTS = {"localhost", "127.0.0.1", "::1"}


class ConfigError(ValueError):
    """A setting is missing or not valid; the message says which and why."""


@dataclass(frozen=True)
class Settings:
    serial_port: str
    backend_url: str
    api_key: str
    cage_id: str
    send_interval_seconds: float = MIN_SEND_INTERVAL_SECONDS

    def __post_init__(self) -> None:
        if not self.serial_port:
            raise ConfigError("SERIAL_PORT is required (COM3 on Windows, /dev/ttyUSB0 on Linux)")
        if not self.api_key or self.api_key == "replace-with-your-backend-api-key":
            raise ConfigError("API_KEY is required: the same API_KEY as the backend")
        if not self.cage_id or len(self.cage_id) > 50:
            raise ConfigError("CAGE_ID is required, at most 50 characters (e.g. cage-1)")
        url = urlparse(self.backend_url)
        # always HTTPS to the backend; plain HTTP only to try it against a backend on this machine
        if url.scheme != "https" and not (url.scheme == "http" and url.hostname in _LOCAL_HOSTS):
            raise ConfigError("BACKEND_URL must be https:// (http:// only for localhost)")
        if self.send_interval_seconds < MIN_SEND_INTERVAL_SECONDS:
            raise ConfigError(f"SEND_INTERVAL_SECONDS can't be less than {MIN_SEND_INTERVAL_SECONDS}")


def load_settings(env_file: Path | None = None) -> Settings:
    """Reads serial_bridge/.env next to this file; real environment variables win over it."""
    load_dotenv(env_file or Path(__file__).with_name(".env"), override=False)
    try:
        interval = float(os.getenv("SEND_INTERVAL_SECONDS", MIN_SEND_INTERVAL_SECONDS))
    except ValueError:
        raise ConfigError("SEND_INTERVAL_SECONDS must be a number of seconds") from None
    return Settings(
        serial_port=os.getenv("SERIAL_PORT", "").strip(),
        backend_url=os.getenv("BACKEND_URL", "").strip().rstrip("/"),
        api_key=os.getenv("API_KEY", "").strip(),
        cage_id=os.getenv("CAGE_ID", "cage-1").strip(),
        send_interval_seconds=interval,
    )
