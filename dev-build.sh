#!/usr/bin/env bash
set -eu

# Default values
DOCKER_TAG="ubuntu2404"
DOCKER_TARGET=""
DOCKER_ROOT="no"
DOCKER_TTY="auto"
DOCKER_SHELL="no"

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --tag)
      DOCKER_TAG="$2"
      shift 2
      ;;
    --target)
      DOCKER_TARGET="$2"
      shift 2
      ;;
    --root)
      DOCKER_ROOT="root"
      shift
      ;;
    --tty)
      DOCKER_TTY="$2"
      shift 2
      ;;
    --shell)
      DOCKER_SHELL="yes"
      shift
      ;;
    -h|--help)
      echo "Usage: $0 [--tag TAG] [--target TARGET] [--root] [--tty auto|yes|no] [--shell]"
      echo "  --tag:    ubuntu2404 | musl-test (default: ubuntu2404)"
      echo "  --target: Docker build target stage (optional)"
      echo "  --root:   run as root user (default: no)"
      echo "  --tty:    auto | yes | no (default: auto)"
      echo "  --shell:  run in shell mode instead of executing default command"
      echo "    auto:   TTYの有無を自動検出"
      echo "    yes:    強制的に-itオプションを使用"
      echo "    no:     TTYなしで実行"
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      echo "Use --help for usage information"
      exit 1
      ;;
  esac
done
DISK_PERCENT=$(df / | grep -oP '(?<=)[0-9]+(?=%\s/)')

echo "#- -----------------------------------------------------------------------------"
echo "#- Disk used ${DISK_PERCENT}%"
echo ""
echo ""

if [ "${DISK_PERCENT}" -gt 95 ]; then
  docker image prune --force --filter "dangling=true, until=3h"
  docker builder prune --force --filter "dangling=true, until=3h"
fi

# Set Docker build target option
if [ -n "${DOCKER_TARGET}" ]; then
  BUILD_TARGET="--target ${DOCKER_TARGET}"
else
  BUILD_TARGET=""
fi

set +u -x
time docker build ${BUILD_TARGET} -t "tmp-${DOCKER_TAG}" -f "Dockerfile.${DOCKER_TAG}" .
set -u +x

if [ "${DOCKER_ROOT}" == "root" ]; then
  DOCKER_OPTS=""
else
  DOCKER_OPTS="--user $(id -u):$(id -g)"
fi

# TTYオプションの設定
if [ "${DOCKER_TTY}" == "no" ]; then
  DOCKER_TTY_OPTS=""
elif [ "${DOCKER_TTY}" == "yes" ]; then
  DOCKER_TTY_OPTS="-it"
else
  # auto: TTYが利用可能かを自動検出
  if [ -t 0 ] && [ -t 1 ]; then
    DOCKER_TTY_OPTS="-it"
  else
    DOCKER_TTY_OPTS=""
  fi
fi

mkdir -p $PWD/modules/join_logo_scp_trial/{JL,logo,result,src}
if [ ! -n "$(ls -A $PWD/videos/source)" ]; then
  echo "source 内に .ts ファイルが存在しません => $PWD/videos/source:/source"
  exit 1
fi
if [ ! -n "$(ls -A $PWD/modules/join_logo_scp_trial/logo)" ]; then
  echo "logo ファイルが存在しません => $PWD/modules/join_logo_scp_trial/logo"
  exit 1
fi

set -x
sudo rm -rfv modules/join_logo_scp_trial/result/*.* \
             $PWD/videos/source/*.{lwi,mp4} \
             $PWD/videos/source:/source/*.{lwi,mp4}
# Docker実行コマンドの決定
if [ "${DOCKER_SHELL}" == "yes" ]; then
  # シェルモードで実行
  time docker run ${DOCKER_OPTS} ${DOCKER_TTY_OPTS} --rm \
    -v $PWD/videos/source:/source \
    -v $PWD/modules/join_logo_scp_trial/logo:/join_logo_scp_trial/logo \
    -v $PWD/modules/join_logo_scp_trial/JL:/join_logo_scp_trial/JL \
    -v $PWD/modules/join_logo_scp_trial/result:/join_logo_scp_trial/result \
    -v $PWD/modules/join_logo_scp_trial/src:/join_logo_scp_trial/src \
    --device "/dev/dri:/dev/dri" \
    "tmp-${DOCKER_TAG}" /bin/bash
else
  # デフォルトコマンドで実行
  time docker run ${DOCKER_OPTS} ${DOCKER_TTY_OPTS} --rm \
    -v $PWD/videos/source:/source \
    -v $PWD/modules/join_logo_scp_trial/logo:/join_logo_scp_trial/logo \
    -v $PWD/modules/join_logo_scp_trial/JL:/join_logo_scp_trial/JL \
    -v $PWD/modules/join_logo_scp_trial/result:/join_logo_scp_trial/result \
    -v $PWD/modules/join_logo_scp_trial/src:/join_logo_scp_trial/src \
    --device "/dev/dri:/dev/dri" \
    "tmp-${DOCKER_TAG}" find /source -type f -name '*.ts' -exec env INPUT="{}" \
  jlse --input "{}" \
  --encode --option ' -vf yadif -map 0:v -aspect 16:9 -c:v libx264 -preset veryfast -movflags faststart -f mp4 -map 0:a -c:a aac -bsf:a aac_adtstoasc' \;
fi
