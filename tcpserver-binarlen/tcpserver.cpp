#include <cstring>
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

    // TCP socket 을 만든다.
    SOCKET passiveSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (passiveSock == INVALID_SOCKET) {
        std::cerr << "socket failed with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // socket 을 특정 주소, 포트에 바인딩 한다.
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    r = bind(passiveSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (r == SOCKET_ERROR) {
        std::cerr << "bind failed with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // TCP 는 연결을 받는 passive socket 과 실제 통신을 할 수 있는 active socket 으로 구분된다.
    // passive socket 은 socket() 뒤에 listen() 을 호출함으로써 만들어진다.
    // active socket 은 passive socket 을 이용해 accept() 를 호출함으로써 만들어진다.
    r = listen(passiveSock, 10);
    if (r == SOCKET_ERROR) {
        std::cerr << "listen faijled with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // passive socket 을 이용해 accept() 로 연결을 기다린다.
    // 연결이 완료되고 만들어지는 소켓은 active socket 이다.
    std::cout << "Waiting for a connection" << std::endl;
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    SOCKET activeSock = accept(passiveSock, (sockaddr*)&clientAddr, &clientAddrSize);
    if (activeSock == INVALID_SOCKET) {
        std::cerr << "accept failed with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // 길이 정보를 받기 위해서 4바이트를 읽는다.
    // network byte order 로 전성되기 때문에 ntohl() 을 호출한다.
    std::cout << "Receiving length info" << std::endl;
    int dataLenNetByteOrder;
    int offset = 0;
    while (offset < 4) {
        r = recv(activeSock, ((char*)&dataLenNetByteOrder) + offset, 4 - offset, 0);
        if (r == SOCKET_ERROR) {
            std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
            return 1;
        }
        else if (r == 0) {
            // 메뉴얼을 보면 recv() 는 소켓이 닫힌 경우 0 을 반환함을 알 수 있다.
            // 따라서 r == 0 인 경우도 loop 을 탈출하게 해야된다.
            std::cerr << "socket closed while reading length" << std::endl;
            return 1;
        }
        offset += r;
    }
    int dataLen = ntohl(dataLenNetByteOrder);
    std::cout << "Received length info: " << dataLen << std::endl;

    // socket 으로부터 데이터를 받는다.
    // TCP 는 연결 기반이므로 누가 보냈는지는 accept 시 결정되고 그 뒤로는 send/recv 만 호출한다.
    std::cout << "Receiving stream" << std::endl;
    char buf[65536 * 2];
    offset = 0;
    while (offset < dataLen) {
        r = recv(activeSock, buf + offset, dataLen - offset, 0);
        if (r == SOCKET_ERROR) {
            std::cerr << "recv failed with error " << WSAGetLastError() << std::endl;
            return 1;
        }
        else if (r == 0) {
            // 메뉴얼을 보면 recv() 는 소켓이 닫힌 경우 0 을 반환함을 알 수 있다.
            // 따라서 r == 0 인 경우도 loop 을 탈출하게 해야된다.
            break;
        }
        std::cout << "Received " << r << " bytes" << std::endl;
        offset += r;
    }

    // 이 특정 연결의 activeSock 을 닫는다.
    r = closesocket(activeSock);
    if (r == SOCKET_ERROR) {
        std::cerr << "closesocket(acitve) failed with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // 연결을 기다리는 passive socket 을 닫는다.
    r = closesocket(passiveSock);
    if (r == SOCKET_ERROR) {
        std::cerr << "closesocket(passive) failed with error " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Winsock 을 정리한다.
    WSACleanup();
    return 0;
}
