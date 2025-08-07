#!/bin/bash

set -x

#SETUP tools
if [[ ! -d "tools/" ]]; then
	mkdir -p tools
	cd tools

	curl -L https://github.com/arduino/arduino-cli/releases/download/v1.1.1/arduino-cli_1.1.1_Windows_64bit.zip > arduino-cli.zip
	unzip arduino-cli.zip arduino-cli.exe
	rm arduino-cli.zip

	./arduino-cli core install esp32:esp32 --config-file ../.cli-config.yml

	cd ..
fi

#SETUP Clone git libraries
mkdir -p libraries
cd libraries

REPO_URL="https://github.com/adafruit/Adafruit_BusIO.git"
DEST_DIR="$(basename "$REPO_URL" .git)"
echo "$REPO_URL"
if [ -d "$DEST_DIR" ]; then
	(cd "$DEST_DIR" && git pull)
else
	(git clone "$REPO_URL")
fi

REPO_URL="https://github.com/adafruit/Adafruit_NeoPixel.git"
DEST_DIR="$(basename "$REPO_URL" .git)"
echo "$REPO_URL"
if [ -d "$DEST_DIR" ]; then
	(cd "$DEST_DIR" && git pull)
else
	(git clone "$REPO_URL")
fi

cd ..
