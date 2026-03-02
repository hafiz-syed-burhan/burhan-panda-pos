# Ubuntu base image use karein (Graphical apps ke liye best hai)
FROM ubuntu:22.04

# Interactive mode band karne ke liye (taake installation na ruke)
ENV DEBIAN_FRONTEND=noninteractive

# Build essentials, GTK aur sari libraries install karein
RUN apt-get update && apt-get install -y \
    build-essential \
    libgtk-3-dev \
    pkg-config \
    make \
    libcanberra-gtk-module \
    libcanberra-gtk3-module \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app
COPY . .

# Compile karein
RUN make

# Runtime par libraries ka rasta dikhane ke liye
ENV LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

# Aapka executable
CMD ["./vip_pos"]
