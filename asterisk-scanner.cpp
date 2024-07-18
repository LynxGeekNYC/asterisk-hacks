#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

bool is_asterisk_server(const std::string& ip, int port = 5060) {
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[1024];
    std::string message = "OPTIONS sip:100@" + ip + " SIP/2.0\r\nVia: SIP/2.0/TCP " + ip + "\r\nFrom: <sip:100@" + ip + ">;tag=100\r\nTo: <sip:100@" + ip + ">\r\nCall-ID: 100\r\nCSeq: 1 OPTIONS\r\nContent-Length: 0\r\n\r\n";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return false;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) <= 0) {
        close(sockfd);
        return false;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        close(sockfd);
        return false;
    }

    send(sockfd, message.c_str(), message.size(), 0);
    int len = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    close(sockfd);

    if (len > 0) {
        buffer[len] = '\0';
        if (strstr(buffer, "Asterisk") != nullptr) {
            return true;
        }
    }

    return false;
}

std::vector<std::string> generate_ip_range(const std::string& start_ip, const std::string& end_ip) {
    std::vector<std::string> ip_range;
    struct in_addr start, end, current;

    inet_pton(AF_INET, start_ip.c_str(), &start);
    inet_pton(AF_INET, end_ip.c_str(), &end);

    for (current.s_addr = ntohl(start.s_addr); current.s_addr <= ntohl(end.s_addr); ++current.s_addr) {
        struct in_addr temp;
        temp.s_addr = htonl(current.s_addr);
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &temp, ip_str, INET_ADDRSTRLEN);
        ip_range.push_back(std::string(ip_str));
    }

    return ip_range;
}

void* scan_ip(void* arg) {
    std::string ip = *reinterpret_cast<std::string*>(arg);
    if (is_asterisk_server(ip)) {
        std::cout << "Asterisk server found at " << ip << std::endl;
    } else {
        std::cout << "No Asterisk server at " << ip << std::endl;
    }
    return nullptr;
}

void scan_ip_range(const std::string& start_ip, const std::string& end_ip) {
    std::vector<std::string> ip_range = generate_ip_range(start_ip, end_ip);
    std::vector<pthread_t> threads(ip_range.size());

    for (size_t i = 0; i < ip_range.size(); ++i) {
        pthread_create(&threads[i], nullptr, scan_ip, &ip_range[i]);
    }

    for (size_t i = 0; i < ip_range.size(); ++i) {
        pthread_join(threads[i], nullptr);
    }
}

int main() {
    // Example usage: scan IP range from 192.168.1.1 to 192.168.1.10
    scan_ip_range("192.168.1.1", "192.168.1.10");
    return 0;
}
