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
INCLUDEDIRS= -I common -I client -I server

# Common code source & header files
GENERIC_SOURCES = common/common.cpp common/message.cpp
GENERIC_HEADERS = common/common.h   common/message.h   common/errors.h

# Server files
SERVER_SOURCES = server/server.cpp server/sessionclient.cpp server/main.cpp
SERVER_HEADERS = server/server.h   server/sessionclient.h

# Client files
CLIENT_SOURCES = client/client.cpp client/main.cpp
CLIENT_HEADERS = client/client.h

# Libraries to include in the binaries
LIBRARIES = -lncurses -lpthread

# Default target: compiles the executable files
all: .generic server #client

# Clean up all temporary and debug files and all executables, to force the compiler to rebuild the project from scratch
clean:
	rm -f build/*


# Intermediate step, compile the common source files into an object file, ready to be included in a binary
.generic: $(GENERIC_SOURCES) $(GENERIC_HEADERS)
	g++ $(DEBUG) -c -o build/common.o $(GENERIC_SOURCES) $(INCLUDEDIRS)


# Server
server: .generic $(SERVER_SOURCES) $(SERVER_HEADERS)
	g++ $(DEBUG) -o build/lanmessenger_server build/common.o $(SERVER_SOURCES) $(INCLUDEDIRS) $(LIBRARIES)

# Client
client: .generic $(CLIENT_SOURCES) $(CLIENT_HEADERS)
	g++ $(DEBUG) -o build/lanmessenger build/common.o $(CLIENT_SOURCES) $(INCLUDEDIRS) $(LIBRARIES)

