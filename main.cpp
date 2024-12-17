#include <opencv2/opencv.hpp>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>

#define SERVER_IP "192.168.1.100" // Replace with the IP of the receiver
#define SERVER_PORT 8080          // Port number for the connection
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480

using namespace cv;
using namespace std;

// Function to create and connect a socket
int createSocket(const char* server_ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error: Cannot create socket" << endl;
        return -1;
    }

    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error: Cannot connect to server" << endl;
        close(sock);
        return -1;
    }

    cout << "Connected to server: " << server_ip << ":" << port << endl;
    return sock;
}

int main() {
    // Open the Arducam Video Stream
    VideoCapture cap(0); // Camera index 0
    if (!cap.isOpened()) {
        cerr << "Error: Cannot open camera" << endl;
        return -1;
    }

    cap.set(CAP_PROP_FRAME_WIDTH, VIDEO_WIDTH);
    cap.set(CAP_PROP_FRAME_HEIGHT, VIDEO_HEIGHT);

    // Socket setup
    int sock = createSocket(SERVER_IP, SERVER_PORT);
    if (sock < 0) return -1;

    Mat frame;
    vector<uchar> encoded;
    vector<int> encode_param = {IMWRITE_JPEG_QUALITY, 80}; // Set JPEG quality to 80

    while (true) {
        cap >> frame; // Capture a frame
        if (frame.empty()) {
            cerr << "Error: Blank frame grabbed" << endl;
            break;
        }

        // Encode the frame as JPEG
        imencode(".jpg", frame, encoded, encode_param);

        // Send the size of the encoded frame first
        int size = encoded.size();
        if (send(sock, &size, sizeof(size), 0) < 0) {
            cerr << "Error: Failed to send frame size" << endl;
            break;
        }

        // Send the encoded frame
        if (send(sock, encoded.data(), size, 0) < 0) {
            cerr << "Error: Failed to send frame data" << endl;
            break;
        }

        cout << "Sent frame of size: " << size << " bytes" << endl;

        // Optional: Add a small delay
        usleep(10000); // 10ms
    }

    close(sock);
    cap.release();
    cout << "Disconnected and camera released" << endl;
    return 0;
}
