#!/bin/bash
# 1. VNC Server setup
mkdir -p ~/.vnc
echo "burhan" | vncpasswd -f > ~/.vnc/passwd
chmod 600 ~/.vnc/passwd
tightvncserver :1 -geometry 1280x800 -depth 24

# 2. NoVNC Proxy start (Web browser ke liye)
/usr/share/novnc/utils/launch.sh --vnc localhost:5901 --listen 6080 &

# 3. App ko virtual screen par chalao
sleep 5
export DISPLAY=:1
./vip_pos
