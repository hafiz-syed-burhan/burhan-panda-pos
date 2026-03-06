FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Emojis ke liye fonts-noto-color-emoji ko add kiya gaya hai
RUN apt-get update && apt-get install -y \
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
COPY . .

RUN make

CMD ["./vip_pos"]
