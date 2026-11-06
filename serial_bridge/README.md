# serial_bridge

Lee la balanza del Arduino por USB y manda las lecturas estables al backend como eventos `WEIGHT`.

```bash
pip install -r requirements.txt
cp config.example.env .env     # SERIAL_PORT, BACKEND_URL, API_KEY, CAGE_ID (el .env nunca se sube)
python bridge.py               # Ctrl+C para detenerlo
```

| Qué hace | Detalle |
|---|---|
| Lee el puerto a 115200 baudios | Ignora líneas vacías, las que empiezan con `#` y el JSON roto, sin caerse |
| Manda solo lecturas estables | Como máximo una cada `SEND_INTERVAL_SECONDS` (10 s o más) |
| Si el backend no responde o da 5xx | Guarda en una cola de hasta 1000 eventos (descarta los más viejos) y reintenta con espera de 1, 2, 4… hasta 60 s, con el mismo `eventId` |
| Si el backend responde 4xx | Lo registra y lo descarta: mandarlo otra vez daría lo mismo |
| Si se desconecta el USB | Vuelve a abrir el puerto cada 5 s, sin cerrarse |
| Se detiene | Con Ctrl+C o SIGTERM (systemd, NSSM, Docker), cerrando limpio |

| Módulo | Responsabilidad |
|---|---|
| `config.py` | Lee y valida la configuración (solo HTTPS; HTTP únicamente para `localhost`) |
| `readings.py` | Convierte una línea del firmware en lectura y decide cuáles se mandan |
| `events.py` | Arma el sobre del contrato del backend (`type: WEIGHT`, `source: arduino`) |
| `delivery.py` | Manda al backend y guarda en la cola lo que no se pudo mandar |
| `serial_source.py` | Entrega las líneas del USB y lo reabre si se desconecta |
| `bridge.py` | Conecta todo |

## Pruebas

```bash
pip install -r requirements-dev.txt
pytest
ruff check .
```

Para probarlo sin Arduino en Linux, un par de puertos virtuales hace de cable:

```bash
socat -d pty,raw,echo=0,link=/tmp/ttyARDUINO pty,raw,echo=0,link=/tmp/ttyBRIDGE &
echo '{"grams": 1000.0, "stable": true}' > /tmp/ttyARDUINO
SERIAL_PORT=/tmp/ttyBRIDGE BACKEND_URL=http://localhost:8080 API_KEY=dev-key python bridge.py
```
