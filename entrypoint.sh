#!/bin/bash

# 1. Environment Setup
export USER=root
export HOME=/root
export DISPLAY=:1

# 2. Purani locks saaf karna
rm -rf /tmp/.X1-lock /tmp/.X11-unix/X1

# 3. VNC Server setup
mkdir -p ~/.vnc
echo "burhan" | vncpasswd -f > ~/.vnc/passwd
chmod 600 ~/.vnc/passwd

# 4. VNC Server ko Start karna (Pehle server chalna chahiye)
tightvncserver :1 -geometry 1850x750 -depth 24

# 5. NoVNC Proxy start karna (Background mein &)
/usr/share/novnc/utils/launch.sh --vnc localhost:5901 --listen 6080 &

# Display ready honay ka intezar
sleep 5

# 6. AB LAGAYEIN LOOP (Taake app crash ho to restart ho)
echo "Starting BurhanPanda App with Auto-Restart..."
while true; do
    ./vip_pos
    echo "App (vip_pos) crashed or closed. Restarting in 2 seconds..."
    sleep 2
done
