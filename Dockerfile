FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Desktop tools aur VNC server install karein
RUN apt-get update && apt-get install -y \
    xfce4 xfce4-goodies \
    tightvncserver \
    novnc \
    python3-websockify \
    build-essential \
    libgtk-3-dev \
    pkg-config \
    make \
    libsqlite3-dev \
    sqlite3 \
    fonts-liberation \
    fonts-dejavu-core \
    fonts-noto-color-emoji \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app

# Saara code aur images copy karein
COPY . .

# App compile karein
RUN make clean && make

# entrypoint.sh ko executable banayein
RUN chmod +x entrypoint.sh

# NoVNC ka port (6080) expose karein
EXPOSE 6080

# Seedha app chalane ke bajaye startup script chalaein
CMD ["./entrypoint.sh"]
