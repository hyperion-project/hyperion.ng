[Unit]
Description=Hyperion ambient light systemd service for OpenELEC/LibreELEC/RasPlex
After=graphical.target

[Service]
ExecStart=/bin/sh -c "exec sh /storage/hyperion/bin/hyperiond.sh /storage/.config/hyperion/hyperion.config.json"
TimeoutStopSec=2
Restart=always
RestartSec=2

[Install]
WantedBy=default.target
