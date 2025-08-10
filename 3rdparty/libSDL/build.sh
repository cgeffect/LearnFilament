#!/bin/bash

workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=SDL

# 版本 https://github.com/libsdl-org/SDL/archive/refs/tags/preview-3.1.6.tar.gz
rm -rf "$workdir/deploy" && rm -rf "$workdir/${src_dir_inside}" && \
cd "$workdir" && tar -zxvf "$workdir/source/${src_dir_inside}.tar.gz" && \
cd "$workdir/${src_dir_inside}/" && mkdir "$workdir/${src_dir_inside}/build" && cd "$workdir/${src_dir_inside}/build"


cmake -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DSDL_TEST_LIBRARY=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_INSTALL_PREFIX="${workdir}/deploy" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="14.0" .. && \
make -j12 && make install

if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build SDL"
    exit -1
fi

echo "success!"


