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
DEBUG = -g

# Where to find includes
INCLUDEDIRS= -I common/ -I client/ -I server/ -Lbuild/

# Common code source & header files
GENERIC_SOURCES = common/common.cpp common/message.cpp common/sessionbase.cpp
GENERIC_HEADERS = common/common.h   common/message.h   common/sessionbase.h   common/errors.h

# Server files
SERVER_SOURCES = server/server.cpp server/sessionclient.cpp server/main.cpp
SERVER_HEADERS = server/server.h   server/sessionclient.h

# Client files
CLIENT_SOURCES = client/client.cpp client/sessionserver.cpp client/main.cpp
CLIENT_HEADERS = client/client.h   client/sessionserver.h

# Libraries to include in the binaries
LIBRARIES = -lncurses -lpthread

# Default target: compiles the executable files
all: server client
	@echo "Done!"

# Clean up all temporary and debug files and all executables, to force the compiler to rebuild the project from scratch
clean:
	@rm -rf build/*


# Server
server: $(GENERIC_SOURCES) $(GENERIC_HEADERS) $(SERVER_SOURCES) $(SERVER_HEADERS)
	@echo "Building the server..."
	g++ $(DEBUG) -o build/lanmessenger_server $(GENERIC_SOURCES) $(SERVER_SOURCES) $(INCLUDEDIRS) $(LIBRARIES)

# Client
client: $(GENERIC_SOURCES) $(GENERIC_HEADERS) $(CLIENT_SOURCES) $(CLIENT_HEADERS)
	@echo "Building the client..."
	g++ $(DEBUG) -o build/lanmessenger $(GENERIC_SOURCES) $(CLIENT_SOURCES) $(INCLUDEDIRS) $(LIBRARIES)

