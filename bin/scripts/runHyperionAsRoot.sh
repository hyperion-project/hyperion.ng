#!/bin/sh

if [ "`id -u`" -ne 0 ]; then
    echo "Please run this script as root. E.g., sudo $0"
	exit 99
fi

prompt () {
	while true; do
		read -p "$1 " yn
		case "${yn:-Y}" in
			[Yes]* ) return 0;;
			[No]* ) return 1;;
			* ) echo "Please answer Yes or No.";;
		esac
	done
}

echo "The script takes care that Hyperion is executed with root privileges. This poses a security risk!"
echo "It is recommended not to do so unless there are key reasons (e.g. WS281x usage)."

if prompt "Are you sure you want to run Hyperion under root? [Yes/No]"; then
    echo "Restarting Hyperion Service as root..."
	systemctl is-active --quiet hyperion@pi.service && systemctl disable --quiet hyperion@pi.service --now >/dev/null 2>&1
	systemctl enable --quiet hyperion@root.service --now >/dev/null 2>&1
	sed -i 's/pi.service/root.service/' /etc/update-motd.d/10-hyperbian >/dev/null 2>&1
else
	exit 99
fi
