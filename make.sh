#!/bin/bash

# Setup verbose output and fail on errors
export PS4="\e[34m>> \e[37m"; set -x; set -e

# Cleanup
function cleanup_and_exit() {
	local exit_code=$?

	set +e;
	rm a.* 2>/dev/null

	if [ "$exit_code" -eq 0 ]; then
		echo -e "\e[32m\nSUCCESS!\e[0m"
	else
		echo -e "\e[31m\nERROR!\e[0m"
	fi
}

# Trap exit
trap cleanup_and_exit EXIT

# Compile / run
gcc *.c -Wall -Wextra -g -std=c89 -pedantic
./a

# Get misra
MISRA_REPO='https://github.com/furdog/MISRA.git'
if cd misra; then git pull; cd ..; else git clone "$MISRA_REPO"; fi

# Check all .h files
misra/misra.sh setup
for file in *.h; do
	misra/misra.sh check "$file"
done
