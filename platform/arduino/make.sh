#!/bin/bash

###############################################################################
# CONFIGURATION:
###############################################################################
COMPILER="$PWD"/tools/arduino-cli

SERIAL_PORT=COM15
MONITOR_BAUD=115200

export TARGET_NAME="psu_test"
export TARGET=psu_test

#EXTRA_FLAGS="-v"

###############################################################################
# TARGETS AVAILABLE
###############################################################################

if [ "$TARGET" == "psu_test" ]; then
	BOARD=esp32:esp32:esp32c6
	FQBN=:CDCOnBoot=cdc
else
	echo "Bad target!"
	exit 1
fi

###############################################################################
# MAIN
###############################################################################
# Set environment variables
if [ -z "${GIT_REPO_VERSION+x}" ]; then
	export GIT_REPO_VERSION=$(git describe --tags)
fi

compile() {
	# enumerate (and empty) all generated files
	mkdir -p build

	rm build/* 2> /dev/null # Clean build

	echo "Initial setup..."

	# Setup tools and libraries
	./setup.sh
	if [[ $? -ne 0 ]]; then
	    echo "FATAL ERROR: Setup failed."
	    exit 1
	fi

	echo "Testing..."

	
	pushd ../../
	./make.sh
	if [[ $? -ne 0 ]]; then
	    echo "FATAL ERROR: Test failed."
	    exit 1
	fi
	popd


	echo "Copying..."

	# Copy all necessary files into build/
	cp arduino.ino build/build.ino
	cp *.h build/
	cp ../../*.h build/


	echo "Compiling..."

	# Goto build directory with all generated files
	cd build

	echo "PROPS: " ${PROPS}
	echo "FQBN: " ${FQBN}
	while ! ${COMPILER} compile -b ${BOARD}${FQBN} --warnings "all" \
			   ${PROPS} -e --libraries "../libraries/" \
			   ${EXTRA_FLAGS}; do
		read -p "Press any key to continue "
		exit
	done
}

# Function to monitor
monitor() {
	if [ -n "${SERIAL_PORT+x}" ]; then
		while true; do
			${COMPILER} monitor -p ${SERIAL_PORT} \
			      --config baudrate=${MONITOR_BAUD} ${EXTRA_FLAGS};
			sleep 1
		done
	fi
}

upload() {
	if [ -n "${SERIAL_PORT+x}" ]; then
		while ! ${COMPILER} upload -b ${BOARD}${FQBN} \
				   -p ${SERIAL_PORT} ${EXTRA_FLAGS}; do
			sleep 1
		done
	fi
}

# Check if the argument is "monitor"
if [ "$1" == "monitor" ]; then
	monitor
elif [ "$1" == "upload" ]; then
	cd build
	upload
elif [ "$1" == "flash" ]; then
	cd build
	upload
else
	compile
	upload
	monitor
fi

read -p "Press any key to continue "
