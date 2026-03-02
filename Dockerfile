FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Sirf zaruri libraries
RUN apt-get update && apt-get install -y \
    build-essential \
    libgtk-3-dev \
    fonts-liberation \
    fonts-dejavu-core \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app
COPY . .
RUN make

# Seedha app chalayega
CMD ["./vip_pos"]
