#include <nghttp2/asio_http2_server.h>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::server;
using namespace std;

bool file_exists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}

int main(int argc, char *argv[]) {
    try {
        // Setup SSL/TLS context
        string server_key = "/install/github.com/http2-sandbox/build/server.key";
        string server_cert = "/install/github.com/http2-sandbox/build/server.crt";

        if (!file_exists(server_key) || !file_exists(server_cert)) {
            cerr << "Private key or certificate file does not exist" << endl;
            return 1;
        }

        boost::asio::io_context io_context;
        boost::asio::ssl::context tls(boost::asio::ssl::context::tlsv12);

        tls.set_options(boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::single_dh_use);

        tls.use_private_key_file(server_key, boost::asio::ssl::context::pem);
        tls.use_certificate_chain_file(server_cert);

        http2 server;

        server.handle("/", [](const request& req, const response& res) {
            cout << "Received request" << endl;
            header_map headers;
            headers.emplace("content-type", header_value{"text/plain", false});
            res.write_head(200, headers);
            res.end("Hello, HTTP/2!");
        });

        boost::system::error_code ec;
        // if (server.listen_and_serve(ec, tls, "localhost", "8080", true)) {
        if (server.listen_and_serve(ec, tls, "localhost", "8080")) {
            cerr << "Failed to start server: " << ec.message() << endl;
            return 1;
        }

        io_context.run();
    } catch (const std::exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
