[Unit]
Description=Hyperion ambient light systemd service
After=network.target

[Service]
ExecStart=/usr/bin/hyperiond /etc/hyperion/hyperion.config.json
WorkingDirectory=/usr/share/hyperion/bin
TimeoutStopSec=5
KillMode=mixed
Restart=always
RestartSec=2

[Install]
WantedBy=multi-user.target
