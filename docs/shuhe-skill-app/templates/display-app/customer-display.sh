#!/bin/sh
# Wayland environment wrapper for the customer display app.
UID_W="$(id -u)"

# ST images have used both /run/user/<uid> and /home/weston as the
# Weston runtime directory. Pick the one that actually has a socket.
for dir in "/run/user/$UID_W" "/home/weston"; do
    if [ -e "$dir/wayland-0" ] || [ -e "$dir/wayland-1" ]; then
        export XDG_RUNTIME_DIR="$dir"
        break
    fi
done
[ -n "$XDG_RUNTIME_DIR" ] || export XDG_RUNTIME_DIR="/run/user/$UID_W"

if [ -e "$XDG_RUNTIME_DIR/wayland-1" ]; then
    export WAYLAND_DISPLAY="wayland-1"
else
    export WAYLAND_DISPLAY="wayland-0"
fi

exec /usr/bin/customer-display /opt/logo.bmp
