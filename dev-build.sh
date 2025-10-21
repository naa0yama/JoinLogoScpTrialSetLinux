#!/usr/bin/env bash
set -euo pipefail

# Default values
DOCKER_ROOT="no"
DOCKER_SHELL="no"
DOCKER_TAG="ubuntu2404"
DOCKER_TARGET=""
DOCKER_TTY="auto"
GH_ACTION="no"
INPUT=""

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
		--input)
			INPUT="$2"
			shift 2
			;;
		--gh-action)
			GH_ACTION="yes"
			# 複数のタグが改行区切りで渡される場合、最初のタグを使用
			DOCKER_TAG=$(echo "$2" | head -n1)
			shift 2
			;;
		-h|--help)
			echo "Usage: $0 [--tag TAG] [--target TARGET] [--root] [--tty auto|yes|no] [--shell] [--input INPUT] [--gh-action TAG]"
			echo "  --tag:          ubuntu2404 (default: ubuntu2404)"
			echo "  --target:       Docker build target stage (optional)"
			echo "  --root:         run as root user (default: no)"
			echo "  --tty:          auto | yes | no (default: auto)"
			echo "  --shell:        run in shell mode instead of executing default command"
			echo "  --input:        input file path to pass as INPUT environment variable to container"
			echo "  --gh-action:    GitHub Action mode - skip build, run docker execution only with specified tag(s)"
			echo "                  If multiple tags are provided (newline-separated), uses the first one"
			echo "    auto:         Automatically detects the presence of a TTY"
			echo "    yes:          Force use of -it option"
			echo "    no:           Run without a TTY"
			exit 0
			;;
		*)
			echo "Unknown option: $1"
			echo "Use --help for usage information"
			exit 1
			;;
	esac
done

# Skip build process if GitHub Action mode
if [ "${GH_ACTION}" == "yes" ]; then
	echo "#- GitHub Action mode: Skipping build, using existing image ${DOCKER_TAG}"
else
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
	time docker build ${BUILD_TARGET} \
	-t "tmp-${DOCKER_TAG}" -f "Dockerfile.${DOCKER_TAG}" . > /dev/null
	set -u +x
fi

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
sudo rm -rfv modules/join_logo_scp_trial/result/* \
				$PWD/videos/source/*.{lwi,mp4} \
				$PWD/videos/source:/source/*.{lwi,mp4}
# Docker実行コマンドの決定
# GitHub Actionモードでは完全なイメージ名を使用、通常モードではtmp-プレフィックスを付与
if [ "${GH_ACTION}" == "yes" ]; then
	IMAGE_NAME="${DOCKER_TAG}"
else
	IMAGE_NAME="tmp-${DOCKER_TAG}"
fi

# INPUT環境変数の設定
if [ -n "${INPUT}" ]; then
	INPUT_ENV_OPT="-e INPUT=${INPUT}"
else
	INPUT_ENV_OPT=""
fi

# /dev/driデバイスの存在確認
if [ -d "/dev/dri" ]; then
	DRI_DEVICE_OPT="--group-add video --device /dev/dri:/dev/dri"
else
	DRI_DEVICE_OPT=""
	echo "Warning: /dev/dri not found, hardware acceleration will be disabled"
fi

if [ "${DOCKER_SHELL}" == "yes" ]; then
	# シェルモードで実行
	time docker run ${DOCKER_OPTS} ${DOCKER_TTY_OPTS} --rm \
		${INPUT_ENV_OPT} \
		-v $PWD/videos/source:/source \
		-v $PWD/modules/join_logo_scp_trial/logo:/join_logo_scp_trial/logo \
		-v $PWD/modules/join_logo_scp_trial/JL:/join_logo_scp_trial/JL \
		-v $PWD/modules/join_logo_scp_trial/result:/join_logo_scp_trial/result \
		-v $PWD/modules/join_logo_scp_trial/src:/join_logo_scp_trial/src \
		${DRI_DEVICE_OPT} \
		"${IMAGE_NAME}" /bin/bash
else
	# デフォルトコマンドで実行
	time docker run ${DOCKER_OPTS} ${DOCKER_TTY_OPTS} --rm \
		-e LOG_FILE="/source/test_jlse.log" \
		${INPUT_ENV_OPT} \
		-v $PWD/videos/source:/source \
		-v $PWD/modules/join_logo_scp_trial/logo:/join_logo_scp_trial/logo \
		-v $PWD/modules/join_logo_scp_trial/JL:/join_logo_scp_trial/JL \
		-v $PWD/modules/join_logo_scp_trial/result:/join_logo_scp_trial/result \
		-v $PWD/modules/join_logo_scp_trial/src:/join_logo_scp_trial/src \
		${DRI_DEVICE_OPT} \
		"${IMAGE_NAME}" test_jlse
fi
