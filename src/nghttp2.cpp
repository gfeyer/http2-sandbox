#include <iostream>
#include <boost/asio.hpp>
#include <nghttp2/nghttp2.h>
#include <iomanip> // For std::setw and std::setfill
#include <cstring> // For strlen

using namespace std;
using boost::asio::ip::tcp;
namespace asio = boost::asio;

// Callback to handle sending data
ssize_t send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data) {
    tcp::socket* socket = static_cast<tcp::socket*>(user_data);

    // Print the buffer to cout
    std::cout << "Sending data: ";
    for (size_t i = 0; i < length; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    try {
        asio::write(*socket, asio::buffer(data, length));
        return static_cast<ssize_t>(length);
    } catch (std::exception& e) {
        std::cerr << "Failed to send data: " << e.what() << std::endl;
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

// Callback to handle receiving data
ssize_t recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data) {
    tcp::socket* socket = static_cast<tcp::socket*>(user_data);
    boost::system::error_code ec;
    size_t len = socket->read_some(asio::buffer(buf, length), ec);
    if (ec) {
        std::cerr << "Failed to receive data: " << ec.message() << std::endl;
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    return len;
}

// Callback to handle headers
int on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen,
                       const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data) {
    if (frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_REQUEST) {
        cout << "Received header: ";
        cout.write(reinterpret_cast<const char*>(name), namelen);
        cout << ": ";
        cout.write(reinterpret_cast<const char*>(value), valuelen);
        cout << endl;
    }
    return 0;
}



using namespace std;

// Callback to handle frame reception
int on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    cout << "Received frame: ";

    // Print basic frame information
    switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            cout << "HEADERS";
            break;
        case NGHTTP2_DATA:
            cout << "DATA";
            break;
        case NGHTTP2_PRIORITY:
            cout << "PRIORITY";
            break;
        case NGHTTP2_RST_STREAM:
            cout << "RST_STREAM";
            break;
        case NGHTTP2_SETTINGS:
            cout << "SETTINGS";
            break;
        case NGHTTP2_PUSH_PROMISE:
            cout << "PUSH_PROMISE";
            break;
        case NGHTTP2_PING:
            cout << "PING";
            break;
        case NGHTTP2_GOAWAY:
            cout << "GOAWAY";
            break;
        case NGHTTP2_WINDOW_UPDATE:
            cout << "WINDOW_UPDATE";
            break;
        default:
            cout << "UNKNOWN";
            break;
    }
    cout << " frame, Stream ID: " << frame->hd.stream_id << endl;

    if (frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_REQUEST) {
        cout << "Received HEADERS frame (request)" << endl;

        // Print the headers
        const nghttp2_nv *nva = frame->headers.nva;
        size_t nvlen = frame->headers.nvlen;
        for (size_t i = 0; i < nvlen; ++i) {
            cout << "Header: ";
            cout.write(reinterpret_cast<const char*>(nva[i].name), nva[i].namelen);
            cout << ": ";
            cout.write(reinterpret_cast<const char*>(nva[i].value), nva[i].valuelen);
            cout << endl;
        }

        // Prepare response
        const char* response_body = "Hello HTTP/2";
        nghttp2_data_provider data_prd;
        data_prd.source.ptr = (void*)response_body;
        data_prd.read_callback = [](nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
                                    uint32_t *data_flags, nghttp2_data_source *source, void *user_data) -> ssize_t {
            const char* data = (const char*)source->ptr;
            size_t data_length = strlen(data);
            size_t copy_length = std::min(data_length, length);

            memcpy(buf, data, copy_length);

            if (copy_length < data_length) {
                source->ptr = (void*)(data + copy_length);
            } else {
                *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            }

            return copy_length;
        };

        nghttp2_nv hdrs[] = {
            { (uint8_t*)":status", (uint8_t*)"200", 7, 3, NGHTTP2_NV_FLAG_NONE },
            { (uint8_t*)"content-type", (uint8_t*)"text/plain", 12, 10, NGHTTP2_NV_FLAG_NONE },
            { (uint8_t*)"content-length", (uint8_t*)"12", 14, 2, NGHTTP2_NV_FLAG_NONE }
        };

        int rv = nghttp2_submit_response(session, frame->hd.stream_id, hdrs, sizeof(hdrs) / sizeof(hdrs[0]), &data_prd);
        if (rv != 0) {
            cerr << "Error submitting response: " << nghttp2_strerror(rv) << endl;
        }
    }
    return 0;
}


void setup_nghttp2_session(nghttp2_session *&session, tcp::socket &socket) {
    nghttp2_session_callbacks *callbacks;
    nghttp2_session_callbacks_new(&callbacks);

    nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
    nghttp2_session_callbacks_set_recv_callback(callbacks, recv_callback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);

    int rv = nghttp2_session_server_new(&session, callbacks, &socket);
    if (rv != 0) {
        cerr << "Failed to create nghttp2 session: " << nghttp2_strerror(rv) << endl;
    }

    nghttp2_session_callbacks_del(callbacks);
}

// Function to send the initial SETTINGS frame
void send_settings_frame(nghttp2_session *session) {
    nghttp2_settings_entry iv[1] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
    };
    int rv = nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, iv, 1);
    if (rv != 0) {
        cerr << "Error submitting SETTINGS: " << nghttp2_strerror(rv) << endl;
    }
    rv = nghttp2_session_send(session);
    if (rv != 0) {
        cerr << "Failed to send frames: " << nghttp2_strerror(rv) << std::endl;
    }
}

void handle_client(tcp::socket socket) {
    cout << "handle_client()" << endl;

    try {
        nghttp2_session *session;
        setup_nghttp2_session(session, socket);

        // Main loop to handle HTTP/2 communication
        while (socket.is_open()) {
            vector<uint8_t> buffer(4096);
            boost::system::error_code ec;
            size_t n = socket.read_some(asio::buffer(buffer), ec);

            if (ec) {
                if (ec == boost::asio::error::eof) {
                    // Connection closed cleanly by peer.
                    cout << "Connection closed cleanly by peer" << endl;
                    break;
                } else {
                    // Some other error occurred.
                    cerr << "read_some error: " << ec.message() << endl;
                    break;
                }
            }

            if (n > 0) {
                // Check for the HTTP/2 client connection preface
                if (n >= 24 && memcmp(buffer.data(), "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24) == 0) {
                    cout << "Received HTTP/2 connection preface" << endl;
                    send_settings_frame(session);  // Send SETTINGS frame
                }

                ssize_t processed = nghttp2_session_mem_recv(session, buffer.data(), n);
                if (processed < 0) {
                    cerr << "nghttp2 processing error: " << nghttp2_strerror(processed) << endl;
                    break;
                }
                nghttp2_session_send(session);
            }
        }
        nghttp2_session_del(session);
    } catch (exception &e) {
        cerr << "Exception in handle_client(): " << e.what() << endl;
    }
}

void accept_connections(tcp::acceptor& acceptor, boost::asio::io_context& io_context) {
    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::thread(&handle_client, std::move(socket)).detach();
        }
        accept_connections(acceptor, io_context); // Accept the next connection
    });
}

int main() {
    try {
        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

        accept_connections(acceptor, io_context);

        // Create a pool of threads to run the io_context
        std::vector<std::thread> threads;
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }

        // Wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();
        }


    } catch (std::exception &e) {
        std::cerr << "Exception in main(): " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
