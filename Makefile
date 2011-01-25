#
# LAN Messenger
# Copyright © 2011 Valerio Pilo
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#

# Uncomment to enable debugging symbols
DEBUG = -O0 -g3 -fno-inline -Wall -Woverloaded-virtual -Wsign-compare -Wundef

# Uncomment to enable raw packet dumps of data in the respective program
# SERVER_DEFINES = -DNETWORK_DEBUG
# CLIENT_DEFINES = -DNETWORK_DEBUG

# Uncomment to completely disable debug output
SERVER_DEFINES += -DNODEBUG
CLIENT_DEFINES += -DNODEBUG

# Where to find includes
INCLUDEDIRS= -I common/ -I common/messages/ -I client/ -I server/ -Lbuild/

# Common code source & header files
GENERIC_SOURCES = common/*.cpp common/messages/*.cpp
GENERIC_HEADERS = common/*.h   common/messages/*.h

# Server files
SERVER_SOURCES = server/*.cpp
SERVER_HEADERS = server/*.h

# Client files
CLIENT_SOURCES = client/*.cpp
CLIENT_HEADERS = client/*.h

# Libraries to link in the binaries
LIBRARIES = -lncurses -lpthread

# Default target: compiles the executable files
all: $(GENERIC_SOURCES) $(GENERIC_HEADERS) client server
	@rm -f *.log
	@echo "Done!"

# Clean up all temporary and debug files and all executables, to force the compiler to rebuild the project from scratch
clean:
	@rm -rf build/*
	@rm -f *.log


# Server
server: $(SERVER_SOURCES) $(SERVER_HEADERS)
	@echo "Building the server..."
	g++ $(DEBUG) -o build/lanmessenger_server $(GENERIC_SOURCES) $(SERVER_SOURCES) $(SERVER_DEFINES) $(INCLUDEDIRS) $(LIBRARIES)

# Client
client: $(CLIENT_SOURCES) $(CLIENT_HEADERS)
	@echo "Building the client..."
	g++ $(DEBUG) -o build/lanmessenger $(GENERIC_SOURCES) $(CLIENT_SOURCES) $(CLIENT_DEFINES) $(INCLUDEDIRS) $(LIBRARIES)

