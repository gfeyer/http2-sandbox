#include <nghttp2/asio_http2_server.h>
#include <iostream>

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::server;

int main(int argc, char *argv[]) {
    boost::system::error_code ec;
    http2 server;

    server.num_threads(1);

    server.handle("/", [](const request &req, const response &res) {
        boost::system::error_code ignored_ec;
        header_map headers;
        headers.emplace("content-type", header_value{"text/plain"});

        res.write_head(200, headers);
        res.end("Hello, HTTP/2!");


        // Optionally close the connection here
        // res.on_close([](uint32_t error_code) { /* handle close */ });
    });

    if (server.listen_and_serve(ec, "localhost", "8080")) {
        std::cerr << "error: " << ec.message() << std::endl;
    }

    return 0;
}
