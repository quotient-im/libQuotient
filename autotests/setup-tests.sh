#!/bin/bash

# 1.95.1 is the latest release that allows user registration over HTTPS with self-signed certs
# The setup has to be changed to support newer versions
SYNAPSE_REF=${SYNAPSE_REF:-v1.95.1}
SYNAPSE_IMAGE="matrixdotorg/synapse:$SYNAPSE_REF"
SCRIPT_DIR="$PWD/autotests"

if [ ! -f $SCRIPT_DIR/adjust-config.sh ]; then
    echo "This script should be run from the directory above autotests/"
    echo "(i.e. autotests/setup-tests.sh). Other ways of invocation are not supported."
    return 1
fi

DATA_PATH="$SCRIPT_DIR/synapse-data"
if [ ! -d "$DATA_PATH" ]; then
    mkdir -p -- "$DATA_PATH"
    chmod 0777 -- "$DATA_PATH"
else
    rm -rf $DATA_PATH/*
fi

rm -rf ~/.local/share/testolmaccount

echo "Generating the configuration"
docker run -v $DATA_PATH:/data:z --rm \
    -e SYNAPSE_SERVER_NAME=localhost -e SYNAPSE_REPORT_STATS=no $SYNAPSE_IMAGE generate

echo "Adjusting the configuration and preparing the data directory"
(cd "$DATA_PATH" && . "$SCRIPT_DIR/adjust-config.sh")

echo "Starting Synapse"
docker run -d \
    --name synapse \
    -p 1234:8008 \
    -p 8448:8008 \
    -p 8008:8008 \
    -v $DATA_PATH:/data:z $SYNAPSE_IMAGE

if [ -z "$KEEP_SYNAPSE" ]; then
    TRAP_CMD="docker rm -f synapse 2>&1 >/dev/null"
    if [ -z "$KEEP_DATA_PATH" ]; then
        TRAP_CMD="$TRAP_CMD; rm -rf $DATA_PATH"
    fi
    trap "$TRAP_CMD; trap - EXIT" EXIT
fi

printf "Waiting for synapse to start "
until curl -s -f -k https://localhost:1234/_matrix/client/versions; do printf "."; sleep 2; done
echo

if docker exec synapse /bin/sh /data/register-users.sh; then
    echo "You can run ctest with a full set of tests now!"
    echo "If you don't find the synapse container running, make sure to source"
    echo "this script instead of running it in a subshell (the container will be"
    echo "deleted when you exit the shell then), or run it with KEEP_SYNAPSE"
    echo "environment variable set to any value"
fi
