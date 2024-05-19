# traffic-blaster



## Description

Researching different http2 protocol implementations

## Build from source


#### Install Dependencies


```bash
# get the package manager
cd /opt
git clone git@github.com:microsoft/vcpkg.git
cd vcpkg
./bootstrap.sh # linux, macos


```

#### Compile

```bash
mkdir build
cd build
cmake ../ "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build . --parallel 2
```


