FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

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

RUN make

EXPOSE 6080

# Xvfb: Virtual Display banata hai
# x11vnc: Us display ko share karta hai (Port 5900)
# launch.sh: Browser ke liye proxy chalata hai (Port 6080)
CMD Xvfb :99 -screen 0 1024x768x16 & \
    sleep 2 && \
    x11vnc -display :99 -forever -nopw -listen localhost -shared & \
    sleep 2 && \
    DISPLAY=:99 ./vip_pos & \
    /usr/share/novnc/utils/launch.sh --vnc localhost:5900 --listen 6080
