#!/usr/bin/env bash

set -eux
DOCKER_TAG="${1:-ubuntu2204}"
DOCKER_ROOT="${2:-no}"
DISK_PERCENT=$(df / | grep -oP '(?<=)[0-9]+(?=%\s/)')

if [ "${DISK_PERCENT}" -gt 95 ]; then
  docker image prune --force --filter "until=3h"
  docker builder prune --force --filter "until=3h"
fi

time docker build -t "tmp-${DOCKER_TAG}" -f "Dockerfile.${DOCKER_TAG}" .

if [ "${DOCKER_ROOT}" == "yes" ]; then
  DOCKER_OPTS="--user $(id -u):$(id -g)"
else
  DOCKER_OPTS=""
fi

time docker run ${DOCKER_OPTS} --rm -it \
  -v $PWD/videos/source:/source \
  -v $PWD/videos/dist:/dist \
  -v $PWD/modules/join_logo_scp_trial/JL:/join_logo_scp_trial/JL \
  -v $PWD/modules/join_logo_scp_trial/logo:/join_logo_scp_trial/logo \
  -v $PWD/modules/join_logo_scp_trial/result:/join_logo_scp_trial/result \
  -v $PWD/modules/join_logo_scp_trial/setting:/join_logo_scp_trial/setting \
  -v $PWD/modules/join_logo_scp_trial/src:/join_logo_scp_trial/src \
  "tmp-${DOCKER_TAG}" bash
