#include <lwip/sockets.h>
#include "wifi_stream.h"

// 初始化udp设置
#define DEST_IP "192.168.1.100" // 目标设备IP
#define DEST_PORT 8888          // 目标端口
static int udp_socket = -1;
struct sockaddr_in dest_addr;

// 初始化UDP Socket
void udp_init() {
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    assert(udp_socket >= 0);

    // 设置目标地址
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PORT);
    inet_pton(AF_INET, DEST_IP, &dest_addr.sin_addr.s_addr);

    // 设置发送缓冲区（可选）
    int send_buf_size = 65535;
    setsockopt(udp_socket, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size));
}