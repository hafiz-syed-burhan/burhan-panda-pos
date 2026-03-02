# Ubuntu base image
FROM ubuntu:22.04

# Interactive mode band
ENV DEBIAN_FRONTEND=noninteractive

# GTK, SQLite, aur Browser support (VNC) install karein
RUN apt-get update && apt-get install -y \
    build-essential \
    libgtk-3-dev \
    pkg-config \
    make \
    libsqlite3-dev \
    sqlite3 \
    xvfb x11vnc novnc websockify \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app
COPY . .

# Compile karein
RUN make

# Browser port
EXPOSE 6080

# Script jo VNC aur App dono chalaye gi
CMD Xvfb :99 -screen 0 1024x768x16 & \
    DISPLAY=:99 ./vip_pos & \
    /usr/share/novnc/utils/launch.sh --vnc localhost:5900 --listen 6080
