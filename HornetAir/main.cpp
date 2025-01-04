#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>

#define PORT 8080

int main() {
    // Create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed." << std::endl;
        return -1;
    }

    // Configure the server address
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    address.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed." << std::endl;
        close(server_fd);
        return -1;
    }

    // Start listening for connections
    if (listen(server_fd, 1) < 0) {
        std::cerr << "Listen failed." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    // Accept a client connection
    int addrlen = sizeof(address);
    int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (client_fd < 0) {
        std::cerr << "Connection failed." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Client connected." << std::endl;

    // Capture video from the camera
    cv::VideoCapture cap(0); // Use the default camera
    if (!cap.isOpened()) {
        std::cerr << "Failed to open the camera." << std::endl;
        close(client_fd);
        close(server_fd);
        return -1;
    }

    // Variables for video frames and buffer
    cv::Mat frame;
    std::vector<uchar> buffer;

    while (true) {
        cap >> frame; // Capture a frame
        if (frame.empty()) {
            std::cerr << "Empty frame captured." << std::endl;
            break;
        }

        // Compress the frame as a JPEG
        cv::imencode(".jpg", frame, buffer);

        // Dynamically check and ensure buffer size matches encoded frame size
        size_t buffer_size = buffer.size(); // Actual size of the encoded JPEG
        if (buffer_size == 0) {
            std::cerr << "Failed to encode frame." << std::endl;
            break;
        }

        // Step 1: Send the frame size (4 bytes in network byte order)
        int frame_size = htonl(static_cast<int>(buffer_size));
        if (send(client_fd, &frame_size, sizeof(frame_size), 0) != sizeof(frame_size)) {
            std::cerr << "Failed to send frame size." << std::endl;
            break;
        }

        // Step 2: Send the frame data
        ssize_t sent_bytes = send(client_fd, buffer.data(), buffer_size, 0);
        if (sent_bytes < 0) {
            std::cerr << "Failed to send frame." << std::endl;
            break;
        } else if ((size_t)sent_bytes != buffer_size) {
            std::cerr << "Partial frame sent: " << sent_bytes << " of " << buffer_size << " bytes." << std::endl;
            break;
        }

        buffer.clear(); // Clear the buffer for the next frame

        // Add a delay to control frame rate (e.g., ~30 FPS)
        if (cv::waitKey(33) == 'q') { // ~33 ms delay
            break;
        }
    }

    // Cleanup
    close(client_fd);
    close(server_fd);
    return 0;
}

