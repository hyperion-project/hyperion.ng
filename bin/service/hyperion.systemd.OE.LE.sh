[Unit]
Description=Hyperion ambient light systemd service for OpenELEC/LibreELEC/RasPlex
After=graphical.target
After=network.target

[Service]
WorkingDirectory=/storage/hyperion/bin/
Environment=LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
ExecStart=./hyperiond /storage/.config/hyperion/hyperion.config.json
TimeoutStopSec=5
KillMode=mixed
Restart=always
RestartSec=2

[Install]
WantedBy=default.target
