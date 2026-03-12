# Variables
CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0` -lsqlite3
TARGET = vip_pos
SRC = vip_pos.c

# Default rule: Jab aap sirf 'make' likhenge
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

# Clean rule: Purani compiled file delete karne ke liye
clean:
	rm -f $(TARGET)



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



# Purani build commands ke niche ye add karein:

#run:
#	@echo "Stopping old container..."
#	-docker stop panda-desktop
#	-docker rm panda-desktop
#	@echo "Cleaning up unused images to save storage..."
#	-docker image prune -f
#	@echo "Starting Burhan Panda POS with Persistent Data..."
#	docker run -d \
#		--net=host \
#		-e DISPLAY=$(DISPLAY) \
#		-v /tmp/.X11-unix:/tmp/.X11-unix \
#		-v $(PWD)/burhan_panda.db:/usr/src/app/burhan_panda.db \
#		--name panda-desktop \
#		burhan-panda-app:latest



#run:
#	@echo "Stopping old container..."
#	-docker stop panda-desktop
#	-docker rm panda-desktop
#	@echo "Starting Burhan Panda POS on Native Display..."
#	docker run -d \
#	    --net=host \
#	    -e DISPLAY=$(DISPLAY) \
#	    -v /tmp/.X11-unix:/tmp/.X11-unix \
#	    --name panda-desktop \
#	    burhan-panda-app:latest

