#!/bin/bash

# 1. USER variable set karna zaroori hai VNC ke liye
export USER=root
export HOME=/root

# 2. Purani locks saaf karna (agar container restart ho)
rm -rf /tmp/.X1-lock /tmp/.X11-unix/X1

# 3. VNC Server setup
mkdir -p ~/.vnc
echo "burhan" | vncpasswd -f > ~/.vnc/passwd
chmod 600 ~/.vnc/passwd

# VNC ko forced start karna
tightvncserver :1 -geometry 1280x800 -depth 24

# 4. NoVNC Proxy start (Background mein)
/usr/share/novnc/utils/launch.sh --vnc localhost:5901 --listen 6080 &

# 5. App chalane se pehle thora intezar taake display ready ho jaye
sleep 5
export DISPLAY=:1

# 6. App start karna
./vip_pos
