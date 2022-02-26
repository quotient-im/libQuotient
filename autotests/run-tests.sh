mkdir -p data
chmod 0777 data
rm ~/.local/share/testolmaccount -rf
docker run -v `pwd`/data:/data --rm \
    -e SYNAPSE_SERVER_NAME=localhost -e SYNAPSE_REPORT_STATS=no matrixdotorg/synapse:v1.24.0 generate
./.ci/adjust-config.sh
docker run -d \
    --name synapse \
    -p 1234:8008 \
    -p 8448:8008 \
    -p 8008:8008 \
    -v `pwd`/data:/data matrixdotorg/synapse:v1.24.0
trap "rm -rf ./data/*; docker rm -f synapse 2>&1 >/dev/null; trap - EXIT" EXIT

echo Waiting for synapse to start...
until curl -s -f -k https://localhost:1234/_matrix/client/versions; do echo "Checking ..."; sleep 2; done
echo Register alice
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice1 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice2 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice3 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice4 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice5 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice6 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice7 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice8 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u alice9 -p secret -c /data/homeserver.yaml https://localhost:8008'
echo Register bob
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u bob1 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u bob2 -p secret -c /data/homeserver.yaml https://localhost:8008'
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u bob3 -p secret -c /data/homeserver.yaml https://localhost:8008'
echo Register carl
docker exec synapse /bin/sh -c 'register_new_matrix_user --admin -u carl -p secret -c /data/homeserver.yaml https://localhost:8008'

GTEST_COLOR=1 ctest --verbose "$@"
