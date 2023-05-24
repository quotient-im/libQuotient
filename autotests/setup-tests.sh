SYNAPSE_IMAGE='matrixdotorg/synapse:v1.61.1'
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
fi

rm -rf ~/.local/share/testolmaccount

echo "Generating the configuration"
docker run -v $DATA_PATH:/data:z --rm \
    -e SYNAPSE_SERVER_NAME=localhost -e SYNAPSE_REPORT_STATS=no $SYNAPSE_IMAGE generate

echo "Adjusting the configuration"
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

echo Waiting for synapse to start...
until curl -s -f -k https://localhost:1234/_matrix/client/versions; do echo "Checking ..."; sleep 2; done
echo Register alice
for i in 1 2 3 4 5 6 7 8 9; do
    docker exec synapse /bin/sh -c "register_new_matrix_user --admin -u alice$i -p secret -c /data/homeserver.yaml https://localhost:8008"
done
echo Register bob
for i in 1 2 3; do
    docker exec synapse /bin/sh -c "register_new_matrix_user --admin -u bob$i -p secret -c /data/homeserver.yaml https://localhost:8008"
done
echo Register carl
docker exec synapse /bin/sh -c "register_new_matrix_user --admin -u carl -p secret -c /data/homeserver.yaml https://localhost:8008"

echo "You can run ctest with a full set of tests now!"
echo "If you don't find the synapse container running, make sure to source"
echo "this script instead of running it in a subshell (the container will be"
echo "deleted when you exit the shell then), or run it with KEEP_SYNAPSE"
echo "environment variable set to any value"
