mkdir -p data
chmod 0777 data

rm ~/.local/share/testolmaccount -rf
docker run -v `pwd`/data:/data --rm \
    -e SYNAPSE_SERVER_NAME=localhost -e SYNAPSE_REPORT_STATS=no matrixdotorg/synapse:latest generate
pushd data
. ../autotests/adjust-config.sh
popd
docker run -d \
    --name synapse \
    -p 1234:8008 \
    -p 8448:8008 \
    -p 8008:8008 \
    -v `pwd`/data:/data matrixdotorg/synapse:latest
trap "rm -rf ./data/*; docker rm -f synapse 2>&1 >/dev/null; trap - EXIT" EXIT

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

GTEST_COLOR=1 ctest --verbose "$@"

