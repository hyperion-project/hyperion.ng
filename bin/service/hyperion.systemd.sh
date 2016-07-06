[Unit]
Description=Hyperion ambient light systemd service

[Service]
Type=simple
User=hyperion
Group=hyperion
ExecStart=/usr/bin/hyperiond /etc/hyperion/hyperion.config.json
ExecReload=/bin/kill -HUP $MAINPID
TimeoutStopSec=2
Restart=always
RestartSec=2
 
[Install]
WantedBy=multi-user.target
