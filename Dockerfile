#- -------------------------------------------------------------------------------------------------
#- Global
#-
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS=yes
ARG PYTHONUNBUFFERED=1
ARG TZ=Asia/Tokyo

# NodeJs version
ARG NODE_VERSION=18

#- -------------------------------------------------------------------------------------------------
#- Builder
#-
FROM ubuntu:jammy as builder
# ARG は、利用する各ステージごとに宣言する必要がある。
ARG DEBIAN_FRONTEND
ENV DEBIAN_FRONTEND=${DEBIAN_FRONTEND}
ARG DEBCONF_NOWARNINGS
ENV DEBCONF_NOWARNINGS=${DEBCONF_NOWARNINGS}
ARG PYTHONUNBUFFERED
ENV PYTHONUNBUFFERED=${PYTHONUNBUFFERED}
ARG TZ
ENV TZ=${TZ}

ARG NODE_VERSION
ENV NODE_VERSION=${NODE_VERSION}

# VMAF
## Ref: https://github.com/Netflix/vmaf/releases
ARG VMAF_VERSION=3.0.0
ARG VMAF_URL="https://github.com/Netflix/vmaf/archive/refs/tags/v${VMAF_VERSION}.tar.gz"
ARG VMAF_SHA256=7178c4833639e6b989ecae73131d02f70735fdb3fc2c7d84bc36c9c3461d93b1

# AviSynthPlus
## Ref: https://github.com/AviSynth/AviSynthPlus/releases
ARG AVISYNTHPLUS_VERSION=v3.7.3

# l-smash-source
## Ref: https://github.com/HomeOfAviSynthPlusEvolution/L-SMASH-Works/releases
ARG LSMASHSOURCE_VERSION=1194.0.0.0

# FFmpeg
## Ref: https://ffmpeg.org/releases/
ARG FFMPEG_VERSION=7.0
ARG FFMPEG_URL="https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.bz2"

# retry dns and some http codes that might be transient errors
ARG WGET_OPTS="--retry-on-host-error --retry-on-http-error=429,500,502,503"

# Build Dependencies
SHELL ["/bin/bash", "-c"]
RUN set -eux && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    apt-file \
    autoconf \
    automake \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git-core \
    gnupg \
    libass-dev \
    libfreetype6-dev \
    libgnutls28-dev \
    libmp3lame-dev \
    libtool \
    libunistring-dev \
    libvorbis-dev \
    meson \
    ninja-build \
    pkg-config \
    texinfo \
    wget \
    yasm \
    zlib1g-dev
# Do not apt clean to resolve dependencies

## Create Working directory
RUN mkdir -p ~/ffmpeg_sources

## VMAF
RUN apt-get -y install --no-install-recommends doxygen xxd

## NASM
RUN apt-get -y install --no-install-recommends nasm

## libx264
RUN apt-get -y install --no-install-recommends libx264-dev

## libx265
RUN apt-get -y install --no-install-recommends libx265-dev libnuma-dev

## libvpx
RUN apt-get -y install --no-install-recommends libvpx-dev

## libfdk-aac
RUN apt-get -y install --no-install-recommends libfdk-aac-dev

## libopus
RUN apt-get -y install --no-install-recommends libopus-dev

## libaom
RUN apt-get -y install --no-install-recommends libaom-dev libdav1d-dev

## libaribb24
RUN apt-get -y install --no-install-recommends libaribb24-dev

## OpenCL
RUN apt-get -y install --no-install-recommends ocl-icd-opencl-dev opencl-headers

## avisynth+
RUN apt-get -y install --no-install-recommends checkinstall

## join_logo_scp_trial
RUN apt-get -y install --no-install-recommends curl git make gcc g++ cmake libboost-all-dev

## Intel Media SDK
RUN sed -i -e's/ main/ main contrib non-free/g' /etc/apt/sources.list && \
    set -eux && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    libmfx-dev \
    libva-dev \
    intel-media-va-driver-non-free

## libvmaf
RUN set -eux && \
    cd ~/ffmpeg_sources && \
    wget ${WGET_OPTS} -O vmaf.tar.gz "${VMAF_URL}" && \
    echo "${VMAF_SHA256}  vmaf.tar.gz" | sha256sum --status -c - && \
    tar xf vmaf.tar.gz && \
    cd vmaf-*/libvmaf && \
    meson build --buildtype=release \
    -Dbuilt_in_models=true \
    -Ddefault_library=static \
    -Denable_avx512=true \
    -Denable_docs=false \
    -Denable_float=true \
    -Denable_tests=false && \
    ninja -vC build && \
    ninja -vC build install

## AviSynth+
RUN set -eux && \
    cd ~/ffmpeg_sources && \
    git clone --depth 1 -b ${AVISYNTHPLUS_VERSION} https://github.com/AviSynth/AviSynthPlus.git && \
    cd AviSynthPlus && \
    mkdir avisynth-build && \
    cd avisynth-build && \
    cmake ../ -G Ninja && \
    ninja && \
    checkinstall --pkgname=avisynth --pkgversion="$(grep -r \
    Version avs_core/avisynth.pc | cut -f2 -d " ")-$(date --rfc-3339=date | \
    sed 's/-//g')-git" --backup=no --deldoc=yes --delspec=yes --deldesc=yes \
    --strip=yes --stripso=yes --addso=yes --fstrans=no --default ninja install && \
    ldconfig

## FFmpeg
RUN set -eux && \
    cd ~/ffmpeg_sources && \
    wget ${WGET_OPTS} -O ffmpeg.tar.bz2     "${FFMPEG_URL}" && \
    wget ${WGET_OPTS} -O ffmpeg.tar.bz2.asc "${FFMPEG_URL}.asc" && \
    curl -sfSL https://ffmpeg.org/ffmpeg-devel.asc | gpg --import && \
    gpg --verify ffmpeg.tar.bz2.asc ffmpeg.tar.bz2 && \
    \
    tar xf ffmpeg.tar.bz2 && \
    cd ffmpeg-* && \
    ./configure \
    --disable-debug \
    --disable-doc \
    --disable-ffplay \
    --extra-libs="-lpthread -lm" \
    \
    --ld="g++" \
    --enable-small \
    --pkg-config-flags="--static" \
    --extra-cflags="-fopenmp" \
    --extra-ldflags="-fopenmp -Wl,-z,stack-size=2097152" \
    --toolchain=hardened \
    --enable-static \
    --disable-shared \
    \
    --enable-avisynth \
    --enable-gpl \
    --enable-libaom \
    --enable-libaribb24 \
    --enable-libass \
    --enable-libdav1d \
    --enable-libfdk-aac \
    --enable-libfreetype \
    --enable-libmfx \
    --enable-libmp3lame \
    --enable-libopus \
    --enable-libvmaf \
    --enable-libvorbis \
    --enable-libvpx \
    --enable-libx264 \
    --enable-libx265 \
    --enable-nonfree \
    --enable-opencl \
    --enable-vaapi \
    --enable-version3 && \
    make -j$(nproc) install && \
    hash -r

## Install obuparse
RUN set -eux && \
    cd ~/ffmpeg_sources && \
    git clone --depth 1 https://github.com/dwbuiten/obuparse.git && \
    cd obuparse && \
    make -j$(nproc) && \
    make install -j$(nproc)

## l-smash
RUN set -eux && \
    cd ~/ffmpeg_sources && \
    git clone --depth 1 https://github.com/vimeo/l-smash.git && \
    cd l-smash && \
    ./configure \
    --prefix="/usr/local" \
    --enable-shared \
    && \
    make -j$(nproc) lib && \
    make -j$(nproc) install && \
    ldconfig

## l-smash-source
RUN set -eux && \
    cd ~/ffmpeg_sources && \
    git clone --depth 1 -b ${LSMASHSOURCE_VERSION} https://github.com/HomeOfAviSynthPlusEvolution/L-SMASH-Works.git && \
    cd L-SMASH-Works/AviSynth && \
    LDFLAGS="-Wl,-Bsymbolic" meson build && \
    cd build && \
    ninja -v && \
    ninja install && \
    ldconfig

## FFmpeg runtime package list saved
RUN set -eux && \
    apt-file update && \
    ldd /usr/local/bin/ffmpeg /usr/local/bin/ffprobe /usr/local/lib/avisynth/liblsmashsource.so | \
    awk -F ' => ' '{print $2}' | awk -F' ' '{print $1}' | \
    tr -d '\t' | sort | uniq | xargs -I'{}' apt-file search '{}' | \
    awk -F':' '{print $1}' | sort | uniq > /ffmpeg_runtime.txt

RUN set -eux && \
    ls -lah /usr/local/bin && \
    ffmpeg -version

RUN set -eux && \
    ffmpeg -hide_banner -hwaccels && \
    ffmpeg -hide_banner -buildconf && \
    for i in decoders encoders; do echo ${i}:; ffmpeg -hide_banner -${i} | \
    egrep -i "[x|h]264|[x|h]265|av1|cuvid|hevc|libmfx|nv[dec|enc]|qsv|vaapi|vp9"; done

## Node setup
RUN set -eux && \
    cd /tmp && \
    mkdir -p /etc/apt/keyrings && \
    curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg && \
    echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_${NODE_VERSION}.x nodistro main" | tee /etc/apt/sources.list.d/nodesource.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    nodejs && \
    \
    # Cleanup \
    apt -y autoremove && \
    apt -y clean && \
    rm -rf /var/lib/apt/lists/*

## join_logo_scp_trial build
COPY .    /tmp/JoinLogoScpTrialSetLinux
RUN set -eux && \
    cd /tmp/JoinLogoScpTrialSetLinux/modules/chapter_exe/src && \
    make -j$(nproc) && \
    cp -av chapter_exe /tmp/JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial/bin/ && \
    \
    cd /tmp/JoinLogoScpTrialSetLinux/modules/logoframe/src && \
    make -j$(nproc) && \
    cp -av logoframe /tmp/JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial/bin/ && \
    \
    cd /tmp/JoinLogoScpTrialSetLinux/modules/join_logo_scp/src && \
    make -j$(nproc) && \
    cp -av join_logo_scp /tmp/JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial/bin/ && \
    \
    cd /tmp/JoinLogoScpTrialSetLinux/modules/tsdivider/ && \
    mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc) && \
    cp -av tsdivider /tmp/JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial/bin/ && \
    \
    cd /tmp/JoinLogoScpTrialSetLinux/modules/delogo/src && \
    make -j$(nproc) && \
    cp -av libdelogo.so /tmp/JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial && \
    \
    mv -v /tmp/JoinLogoScpTrialSetLinux/modules/join_logo_scp_trial /tmp/join_logo_scp_trial && \
    \
    cd /tmp/join_logo_scp_trial && \
    nodejs && \
    node -v && \
    npm --version && \
    npm install --no-save --loglevel=info --production


#- -------------------------------------------------------------------------------------------------
#- Runner-base
#- A stage to copy software such as FFmpeg from the builder.
#-
FROM ubuntu:jammy as runner-base
# ARG は、利用する各ステージごとに宣言する必要がある。
ARG DEBIAN_FRONTEND
ENV DEBIAN_FRONTEND=${DEBIAN_FRONTEND}
ARG DEBCONF_NOWARNINGS
ENV DEBCONF_NOWARNINGS=${DEBCONF_NOWARNINGS}
ARG PYTHONUNBUFFERED
ENV PYTHONUNBUFFERED=${PYTHONUNBUFFERED}
ARG TZ
ENV TZ=${TZ}

ARG NODE_VERSION
ENV NODE_VERSION=${NODE_VERSION}

SHELL ["/bin/bash", "-c"]

## Intel Media SDK
# RUN set -eux && \
#     apt-get update && \
#     apt-get install -y --no-install-recommends \
#     intel-media-va-driver-non-free && \
#     apt-get clean && \
#     rm -rf /var/lib/apt/lists/*

# NVENC Video Encoding runtime library
RUN set -eux && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    ca-certificates \
    wget && \
    wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.0-1_all.deb && \
    dpkg -i cuda-keyring_1.0-1_all.deb && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    ocl-icd-opencl-dev && \
    \
    # Cleanup \
    apt -y autoremove && \
    apt -y clean && \
    rm -rf /var/lib/apt/lists/* && \
    rm -f cuda-keyring_1.0-1_all.deb

# FFmpeg runtime
COPY --from=builder /usr/local/bin/ffmpeg       /usr/local/bin/ffmpeg
COPY --from=builder /usr/local/bin/ffprobe      /usr/local/bin/ffprobe
COPY --from=builder /ffmpeg_runtime.txt         /ffmpeg_runtime.txt

RUN set -eux && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    $(cat /ffmpeg_runtime.txt) && \
    \
    # Cleanup \
    apt -y autoremove && \
    apt -y clean && \
    rm -rf /var/lib/apt/lists/*


#- -------------------------------------------------------------------------------------------------
#- Runner
#- The stage where the application runs.
#-
FROM runner-base as runner
# ARG は、利用する各ステージごとに宣言する必要がある。

SHELL ["/bin/bash", "-c"]

# Script dep
RUN set -eux && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    binutils \
    ca-certificates \
    curl \
    gnupg \
    jq \
    tzdata \
    vainfo && \
    \
    # Cleanup \
    apt -y autoremove && \
    apt -y clean && \
    rm -rf /var/lib/apt/lists/*

## Node setup
RUN set -eux && \
    mkdir -p /etc/apt/keyrings && \
    curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg && \
    echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_${NODE_VERSION}.x nodistro main" | tee /etc/apt/sources.list.d/nodesource.list && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    nodejs && \
    \
    # Cleanup \
    apt -y autoremove && \
    apt -y clean && \
    rm -rf /var/lib/apt/lists/* && \
    npm config set cache /tmp --global

## copy only needed files, without copying nvidia dev files
COPY --from=builder /usr/local/bin              /usr/local/bin/
COPY --from=builder /usr/local/share            /usr/local/share/
COPY --from=builder /usr/local/lib              /usr/local/lib/
COPY --from=builder /usr/local/include          /usr/local/include/
COPY --from=builder /tmp/join_logo_scp_trial    /join_logo_scp_trial

## join_logo_scp_trial
WORKDIR /join_logo_scp_trial
RUN set -eux && \
    mv -v libdelogo.so /usr/local/lib/avisynth && \
    ls /usr/local/lib/avisynth && \
    npm link && \
    jlse --help
RUN set -eux && \
    apt-get update && \
    apt-get -y install --no-install-recommends \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libboost-system-dev && \
    \
    # Cleanup \
    apt -y autoremove && \
    apt -y clean && \
    rm -rf /var/lib/apt/lists/* && \
    npm config set cache /tmp --global

## FFmpeg check
RUN set -eux && \
    readelf -d /usr/local/bin/ffmpeg && \
    ffmpeg -hide_banner -hwaccels && \
    ffmpeg -hide_banner -buildconf && \
    for i in decoders encoders; do echo ${i}:; ffmpeg -hide_banner -${i} | \
    egrep -i "[x|h]264|[x|h]265|av1|cuvid|hevc|libmfx|nv[dec|enc]|qsv|vaapi|vp9"; done

ENTRYPOINT ["/bin/bash", "-c"]
CMD ["npm start"]
