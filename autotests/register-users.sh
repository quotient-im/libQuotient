#!/bin/bash

set -e

if [ ! -x register_new_matrix_user ]; then
    echo "This script is meant to be executed in a Synapse container"
fi

echo Register alice
for i in 1 2 3 4 5 6 7 8 9; do
    register_new_matrix_user --admin -u alice$i -p secret -c /data/homeserver.yaml https://localhost:8008
done
echo Register bob
for i in 1 2 3; do
    register_new_matrix_user --admin -u bob$i -p secret -c /data/homeserver.yaml https://localhost:8008
done
echo Register carl
register_new_matrix_user --admin -u carl -p secret -c /data/homeserver.yaml https://localhost:8008

