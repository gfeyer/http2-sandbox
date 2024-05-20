# https2-sandbox



## Description

Researching different http2 protocol implementations

``` bash
# Client side HTTP/2 GET request 
curl --http2-prior-knowledge -v http://localhost:8080

*   Trying 127.0.0.1:8080...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8080 (#0)
* Using HTTP2, server supports multi-use
* Connection state changed (HTTP/2 confirmed)
* Copying HTTP/2 data in stream buffer to connection buffer after upgrade: len=0
* Using Stream ID: 1 (easy handle 0x5639c35e1650)
> GET / HTTP/2
> Host: localhost:8080
> user-agent: curl/7.68.0
> accept: */*
> 
* Connection state changed (MAX_CONCURRENT_STREAMS == 100)!
< HTTP/2 200 
< content-type: text/plain
< content-length: 12
< 
* Connection #0 to host localhost left intact
Hello HTTP/2%                                       
```

``` bash
# Server side HTTP2 handshake, response and EOF
handle_client()
Received HTTP/2 connection preface
Sending data: 00 00 06 04 00 00 00 00 00 00 03 00 00 00 64 
Received frame: SETTINGS frame, Stream ID: 0
Received frame: WINDOW_UPDATE frame, Stream ID: 0
Received header: :method: GET
Received header: :path: /
Received header: :scheme: http
Received header: :authority: localhost:8080
Received header: user-agent: curl/7.68.0
Received header: accept: */*
Received frame: HEADERS frame, Stream ID: 1
Received HEADERS frame (request)
Sending data: 00 00 00 04 01 00 00 00 00 
Sending data: 00 00 0f 01 04 00 00 00 01 88 5f 87 49 7c a5 8a e8 19 aa 0f 0d 02 31 32 
Sending data: 00 00 0c 00 01 00 00 00 01 48 65 6c 6c 6f 20 48 54 54 50 2f 32 
Received frame: SETTINGS frame, Stream ID: 0
Connection closed cleanly by peer
```

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


