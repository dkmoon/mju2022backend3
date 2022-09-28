#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>

// ws2_32.lib 를 링크한다.
#pragma comment(lib, "Ws2_32.lib")

static unsigned short SERVER_PORT = 27015;

int main()
{
    int r = 0;

    // Winsock 을 초기화한다.
    WSADATA wsaData;
    r = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (r != NO_ERROR) {
        std::cerr << "WSAStartup failed with error " << r << std::endl;
        return 1;
    }

    struct sockaddr_in serverAddr;

    // TCP socket 을 만든다.
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // TCP 는 연결 기반이다. 서버 주소를 정하고 connect() 로 연결한다.
    // connect 후에는 별도로 서버 주소를 기재하지 않고 send/recv 를 한다.
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    r = connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (r == SOCKET_ERROR) {
        std::cerr << "connect failed with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // 길이를 먼저 보낸다.
    // 길이를 문자열로 보내고 마지막에 \n 을 보내는 것으로 한다.
    // binary 가 아니므로 htonl 은 하지 않는다.
    // 4바이트 int 의 최대 숫자를 문자열로 표시해도 64글자가 될 수 없으므로 여기서는 64bytes 만큼의 buf 를 쓴다.
    // sprintf 는 버퍼에 printf 결과물을 저장하고, 몇글자를 썼는지를 반환한다.
    // 따라서 그 반환값만큼 길이 정보를 보내야되므로 기억한다.
    int dataLen = 32768;
    char lengthBuf[64];
    int lengthBytes = sprintf_s(lengthBuf, "%d\n", dataLen);

    int offset = 0;
    while (offset < lengthBytes) {
        r = send(sock, lengthBuf + offset, lengthBytes - offset, 0);
        if (r == SOCKET_ERROR) {
            std::cerr << "failed to send length: " << WSAGetLastError() << std::endl;
            return 1;
        }
        offset += r;
    }
    std::cout << "Sent length info: " << dataLen << std::endl;

    // send 로 데이터를 보낸다. 여기서는 초기화되지 않은 쓰레기 데이터를 보낸다.
    char data[32768];
    offset = 0;
    while (offset < dataLen) {
        r = send(sock, data + offset, dataLen - offset, 0);
        if (r == SOCKET_ERROR) {
            std::cerr << "send failed with error " << WSAGetLastError() << std::endl;
            return 1;
        }
        std::cout << "Sent " << r << " bytes" << std::endl;
        offset += r;
    }

    // Socket 을 닫는다.
    r = closesocket(sock);
    if (r == SOCKET_ERROR) {
        std::cerr << "closesocket failed with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Winsock 을 정리한다.
    WSACleanup();
    return 0;
}
