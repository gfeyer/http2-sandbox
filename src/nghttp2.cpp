#include <iostream>
#include <boost/asio.hpp>
#include <nghttp2/nghttp2.h>
#include <iomanip>  // Include this for std::setfill and std::setw

using boost::asio::ip::tcp;
namespace asio = boost::asio;
using namespace std;

ssize_t recv_callback(nghttp2_session *session, uint8_t *data, size_t length, int flags, void *user_data) {
    cout << "recv_callback()" << endl;
}

ssize_t send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data) {
    // Log or dump data
    std::cout << "Sending data: " << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        std::cout << std::setw(2) << static_cast<unsigned>(data[i]);
        if (i + 1 != length) std::cout << " ";
    }
    std::cout << std::endl;

    // Actual send operation would happen here
    return length; // Assume all data is sent successfully for debug purposes
}

void setup_nghttp2_session(nghttp2_session *&session, tcp::socket &socket) {
    nghttp2_session_callbacks *callbacks;
    nghttp2_session_callbacks_new(&callbacks);

    // Make sure to set necessary callbacks here
    nghttp2_session_callbacks_set_send_callback2(callbacks, send_callback);
    nghttp2_session_callbacks_set_recv_callback2(callbacks, recv_callback);
    // Set other callbacks as necessary

    int rv = nghttp2_session_server_new(&session, callbacks, &socket);
    if (rv != 0) {
        cerr << "Failed to create nghttp2 session: " << nghttp2_strerror(rv) << endl;
        // Handle error appropriately
    }

    nghttp2_session_callbacks_del(callbacks);
}

void send_settings_frame(nghttp2_session *session) {
    // Prepare the SETTINGS frame - no settings means default values are used
    nghttp2_settings_entry iv[1] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
    };
    int rv = nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, iv, 1);
    if (rv != 0) {
        cerr << "Error submitting SETTINGS: " << nghttp2_strerror(rv) << endl;
    }
    // Now tell nghttp2 to send the queued frames
    if (!session) {
        std::cerr << "Session is null or not properly initialized." << std::endl;
        return;
    }
    cout << "Sending SETTING frame with NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS: " << 100 << endl;

    rv = nghttp2_session_send(session);
    if (rv != 0) {
        std::cerr << "Failed to send frames: " << nghttp2_strerror(rv) << std::endl;
    }
}

// Handle client connection
void handle_client(tcp::socket socket) {
    cout << "handle_client()" << endl;

    try {
        nghttp2_session *session;
        setup_nghttp2_session(session, socket);

        // Main loop to handle HTTP/2 communication
        while (socket.is_open()) {
            vector<uint8_t> buffer(4096);
            size_t n = socket.read_some(asio::buffer(buffer));

            if (n > 0) {
                // Check for the HTTP/2 client connection preface
                if (n >= 24 && memcmp(buffer.data(), "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24) == 0) {
                    cout << "Received HTTP/2 connection preface" << endl;
                    send_settings_frame(session);  // Send SETTINGS frame
                }

                ssize_t processed = nghttp2_session_mem_recv2(session, buffer.data(), n);
                if (processed < 0) {
                    cerr << "nghttp2 processing error: " << nghttp2_strerror(processed) << endl;
                    break;
                }
                nghttp2_session_send(session);
            }
        }
        nghttp2_session_del(session);
    } catch (exception &e) {
        cerr << "Exception: " << e.what() << endl;
    }
}

int main() {
    try {
        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::thread(&handle_client, std::move(socket)).detach();
        }
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
