# GCC image use karo build ke liye
FROM gcc:latest

# App directory banao
WORKDIR /usr/src/app

# Saara code copy karo
COPY . .

# C code ko compile karo
RUN gcc -o burhan-panda-app main.c

# App chalao
CMD ["./burhan-panda-app"]
