# Variables
CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0` -lsqlite3
TARGET = vip_pos
SRC = vip_pos.c

# Default rule: Jab Dockerfile 'RUN make' chalayegi, toh sirf compilation hogi
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

# Ye target sirf Jenkins ya Host terminal par chalane ke liye hai
# Dockerfile ke andar isey nahi chalana
run-broadway:
	@echo "Stopping and removing old container..."
	-docker stop burhan-panda-broadway || true
	-docker rm burhan-panda-broadway || true
	
	@echo "Removing unused Docker images to save space..."
	-docker image prune -f
	
	@echo "Starting Burhan Panda on Broadway (Port 8085)..."
	docker run -d \
		-p 8085:8085 \
		-v $(PWD)/burhan_panda.db:/usr/src/app/burhan_panda.db \
		--name burhan-panda-broadway \
		burhan-panda-app:latest

clean:
	rm -f $(TARGET)
