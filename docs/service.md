# El bridge como servicio en la laptop del criadero

El bridge tiene que arrancar solo al prender la laptop y seguir corriendo aunque nadie haya iniciado sesión. Se
instala una vez, con el Arduino ya calibrado y conectado por USB.

## 0. Antes de empezar (Windows o Linux)

1. Instala **Python 3.14** y clona el repo (o copia la carpeta `serial_bridge/`).
2. Crea el entorno y las dependencias, dentro de `serial_bridge/`:

   ```bash
   python -m venv .venv
   .venv/bin/pip install -r requirements.txt          # Windows: .venv\Scripts\pip install -r requirements.txt
   ```

3. Copia `config.example.env` a `.env` y llénalo. `API_KEY` es la misma del backend (`infra/.env`) y la pide el
   equipo; **nunca** se sube al repo.
4. Pruébalo a mano: `python bridge.py` debe decir `Serial port ... open` y, cada 10 s con la balanza quieta,
   `Weight ... g sent`. Se detiene con Ctrl+C.

**¿Qué puerto es?** Windows: Administrador de dispositivos → Puertos (COM y LPT) → "Arduino Uno (COM3)".
Linux: `ls /dev/ttyACM* /dev/ttyUSB*` con el Arduino conectado.

## 1. Linux: systemd

```bash
sudo usermod -aG dialout "$USER"                             # permiso para abrir el puerto serial
sudo cp serial_bridge/service/cuy-serial-bridge.service /etc/systemd/system/
sudo nano /etc/systemd/system/cuy-serial-bridge.service       # User= y las rutas, si no son /home/cuy/...
sudo systemctl daemon-reload
sudo systemctl enable --now cuy-serial-bridge
systemctl status cuy-serial-bridge
journalctl -u cuy-serial-bridge -f                            # el log en vivo
```

`Restart=always` lo levanta si se cae; el propio bridge ya reabre el USB si se desconecta.

### Opcional: con Docker (solo Linux, Task 6.3)

```bash
docker build -t cuy-monitor-serial-bridge:local serial_bridge
docker run -d --restart unless-stopped --device /dev/ttyUSB0 --env-file serial_bridge/.env \
  --name cuy-serial-bridge cuy-monitor-serial-bridge:local
docker logs -f cuy-serial-bridge
```

Corre sin root (grupo `dialout`). Si el Arduino aparece como `/dev/ttyACM0`, cambia `--device` y `SERIAL_PORT`.

## 2. Windows: NSSM (recomendado)

[NSSM](https://nssm.cc) convierte un programa en un servicio de Windows. Con `nssm.exe` en la carpeta del repo,
en una consola **como administrador**:

```bat
nssm install CuySerialBridge "C:\cuy\cuy-monitor-arduino\serial_bridge\.venv\Scripts\python.exe" bridge.py
nssm set CuySerialBridge AppDirectory "C:\cuy\cuy-monitor-arduino\serial_bridge"
nssm set CuySerialBridge AppStdout "C:\cuy\logs\bridge.log"
nssm set CuySerialBridge AppStderr "C:\cuy\logs\bridge.log"
nssm set CuySerialBridge AppRotateFiles 1
nssm set CuySerialBridge AppRotateBytes 1048576
nssm set CuySerialBridge Start SERVICE_AUTO_START
nssm start CuySerialBridge
```

Ver si corre: `nssm status CuySerialBridge` (o en `services.msc`). El log queda en `C:\cuy\logs\bridge.log`.

### Alternativa sin instalar nada: Programador de tareas

Programador de tareas → Crear tarea:

- **General:** "Ejecutar tanto si el usuario inició sesión como si no".
- **Desencadenadores:** "Al iniciar el sistema".
- **Acciones:** programa `C:\cuy\cuy-monitor-arduino\serial_bridge\.venv\Scripts\python.exe`, argumentos
  `bridge.py`, "Iniciar en" `C:\cuy\cuy-monitor-arduino\serial_bridge`.
- **Configuración:** "Si la tarea no se está ejecutando, reiniciarla cada 1 minuto" y quitar "Detener la tarea si se
  ejecuta durante más de…".

## 3. Que la laptop no se duerma (Task 6.2)

Si la laptop se suspende, el bridge deja de mandar pesos y el dashboard mostrará la balanza sin datos.

**Windows** (consola como administrador):

```bat
powercfg /change standby-timeout-ac 0
powercfg /change hibernate-timeout-ac 0
powercfg /hibernate off
```

Y en Administrador de dispositivos → Controladoras de bus serie universal → cada "Concentrador raíz USB" →
Administración de energía: quitar "Permitir que el equipo apague este dispositivo para ahorrar energía".

**Linux:**

```bash
sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target
```

Si la laptop tiene tapa, configúrala para que cerrarla no la suspenda (`HandleLidSwitch=ignore` en
`/etc/systemd/logind.conf`).

## 4. Comprobar que llega

- En el log del bridge: `Weight ... g sent` cada 10 s con la balanza quieta.
- En el dashboard: Jaula → **Peso de la jaula** → "Hoy": la gráfica avanza sola.
- Si dice `Backend not reachable`, el bridge guarda las lecturas (hasta 1000) y las manda al volver la conexión.
