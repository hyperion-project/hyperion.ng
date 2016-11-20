[Unit]
Description=Hyperion ambient light systemd service
After=network.target

[Service]
ExecStart=/usr/bin/hyperiond /etc/hyperion/hyperion.config.json
TimeoutStopSec=5
KillMode=mixed
Restart=always
RestartSec=2

[Install]
WantedBy=multi-user.target
