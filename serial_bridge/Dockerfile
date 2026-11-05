# Optional image of the bridge for Linux laptops only (Task 6.3). On Windows the bridge runs as a service
# without Docker: USB passthrough there is unreliable and Docker Desktop is too heavy for the farm laptop.
#
#   docker build -t cuy-monitor-serial-bridge:local serial_bridge
#   docker run -d --restart unless-stopped --device /dev/ttyUSB0 --env-file serial_bridge/.env \
#     --name cuy-serial-bridge cuy-monitor-serial-bridge:local
FROM python:3.14-slim

WORKDIR /app
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY config.py readings.py events.py delivery.py serial_source.py bridge.py ./

# not root; the dialout group (gid 20 in Debian) can open the serial port passed with --device
RUN useradd --system --no-create-home --gid dialout bridge
USER bridge
ENV PYTHONUNBUFFERED=1
STOPSIGNAL SIGTERM
CMD ["python", "bridge.py"]
