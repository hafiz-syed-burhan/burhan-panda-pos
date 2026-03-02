# GCC image (C compiler) use karo
FROM gcc:latest

# Container ke andar ek folder banao
WORKDIR /usr/src/app

# Aapka saara code (including vip_pos.c) yahan copy hoga
COPY . .

# Compile karo (vip_pos.c ko use karke executable banao)
RUN make

# Jab container chale, to ye app start ho jaye
CMD ["./vip_pos"]
