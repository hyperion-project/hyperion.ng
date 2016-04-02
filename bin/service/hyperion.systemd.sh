[Unit]
Description=Hyperion Systemd service

[Service]
Type=simple
User=root
Group=root
UMask=007
ExecStart=/opt/hyperion/bin/hyperiond /etc/hyperion.config.json
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
TimeoutStopSec=10
 
[Install]
WantedBy=multi-user.target
