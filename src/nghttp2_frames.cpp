#include <iostream>
#include <cstring>
#include <nghttp2/nghttp2.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>

#define PORT 8080
using namespace std;

ssize_t send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data) {
    int fd = *(int*)user_data;
    ssize_t sent = send(fd, data, length, 0);
    if (sent == -1) {
        perror("send");
    }
    return sent;
}

int on_frame_send_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
                cout << "Headers frame for response sent\n";
            }
            break;
        case NGHTTP2_DATA:
            cout << "Data frame sent\n";
            break;
        case NGHTTP2_SETTINGS:
            cout << "Settings frame sent\n";
            break;
        case NGHTTP2_RST_STREAM:
            cout << "RST_STREAM frame sent\n";
            break;
        case NGHTTP2_GOAWAY:
            cout << "GOAWAY frame sent\n";
            break;
        default:
            cout << "Other frame sent, type: " << frame->hd.type << endl;
    }
    return 0;
}

int on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    if (frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_REQUEST) {
        std::string response_body = "Hello";
        nghttp2_nv hdrs[] = {
            { (uint8_t *)":status", (uint8_t *)"200", 7, 3, NGHTTP2_NV_FLAG_NONE },
            { (uint8_t *)"content-length", (uint8_t *)"5", 14, 1, NGHTTP2_NV_FLAG_NONE }, // Make sure this is accurate
            { (uint8_t *)"content-type", (uint8_t *)"text/plain", 12, 10, NGHTTP2_NV_FLAG_NONE }
        };

        nghttp2_data_provider data_prd;
        data_prd.source.ptr = &response_body;
        
        data_prd.read_callback = [](nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
                            uint32_t *data_flags, nghttp2_data_source *source, void *user_data) -> ssize_t {
            std::string *str = static_cast<std::string*>(source->ptr);
            if (!str || str->empty()) {
                *data_flags |= NGHTTP2_DATA_FLAG_EOF;
                return 0;  // No data to send or invalid string pointer
            }

            size_t data_length = str->size();
            size_t copy_len = std::min(data_length, length);  // Ensure we do not exceed string size or buffer length

            if (copy_len > 0) {
                memcpy(buf, str->c_str(), copy_len);  // Copy data to buffer
                str->erase(0, copy_len);  // Erase the data that has been sent
            }

            // Check if all data has been sent
            if (str->empty()) {
                *data_flags |= NGHTTP2_DATA_FLAG_EOF;  // Mark the end of data
            }

            return copy_len;  // Return the number of bytes copied
        };



        nghttp2_submit_response(session, frame->hd.stream_id, hdrs, sizeof(hdrs) / sizeof(hdrs[0]), &data_prd);
    }
    return 0;
}
int main_test() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Server started\n";

    while (true) {  // Keep server running indefinitely
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;  // Continue accepting new connections even if one accept fails
        }

        nghttp2_session *session;
        nghttp2_session_callbacks *callbacks;

        nghttp2_session_callbacks_new(&callbacks);
        nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
        nghttp2_session_callbacks_set_on_frame_send_callback(callbacks, on_frame_send_callback);
        nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);

        nghttp2_session_server_new(&session, callbacks, &client_fd);

        nghttp2_settings_entry iv[] = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};
        nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, iv, 1);

        while (true) {  // Handle HTTP/2 communication in a loop
            char buffer[4096];
            ssize_t read_len = read(client_fd, buffer, sizeof(buffer));

            if (read_len == 0) {  // Check for EOF
                cout << "Client disconnected\n";
                break;
            } else if (read_len < 0) {
                perror("read");
                break;
            }

            ssize_t processed = nghttp2_session_mem_recv(session, (const uint8_t *)buffer, read_len);
            if (processed < 0) {
                cerr << "Error processing HTTP/2 data: " << nghttp2_strerror(processed) << endl;
                break;
            }

            int rv = nghttp2_session_send(session);
            if (rv != 0) {
                cerr << "Error sending HTTP/2 frames: " << nghttp2_strerror(rv) << endl;
                break;
            }
        }

        nghttp2_session_del(session);
        nghttp2_session_callbacks_del(callbacks);
        close(client_fd);  // Close the client connection only after the loop ends
    }

    close(server_fd);
    cout << "Server stopped\n";
    return 0;
}