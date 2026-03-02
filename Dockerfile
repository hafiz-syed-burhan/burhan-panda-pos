FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Sabse zaroori: libsqlite3-dev ko add kiya hai compilation ke liye
RUN apt-get update && apt-get install -y \
    build-essential \
    libgtk-3-dev \
    pkg-config \
    make \
    libsqlite3-dev \
    sqlite3 \
    fonts-liberation \
    fonts-dejavu-core \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app
COPY . .

# Ab 'make' error nahi dega kyunke sqlite3 library mil jayegi
RUN make

CMD ["./vip_pos"]
