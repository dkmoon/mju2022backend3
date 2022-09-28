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

    // 길이 정보를 받는다.
    // 길이 정보가 가변길이 문자열에 끝에 \n 가 붙는 형태이므로
    // \n 를 만날 때까지 1바이트씩 읽어들인다.
    // binary 가 아니므로 ntohl() 은 사용하지 않는다.
    std::cout << "Receiving length info" << std::endl;
    char lengthBuf[64];
    int offset = 0;
    while (offset < sizeof(lengthBuf)) {
        r = recv(activeSock, lengthBuf + offset, 1, 0);
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
        if (lengthBuf[offset] == '\n') {
            break;
        }
        else {
            offset += 1;
        }
    }

    // \n 를 못찾아서 길이 정보를 다 읽지 못해 길이 정보 버퍼를 다 써서 loop 을 종료했을 수도 있다.
    // 따라서 그것을 체크한다.
    if (lengthBuf[offset] != '\n') {
        std::cerr << "Failed to read length info" << std::endl;
        return 1;
    }

    // 설령 문자열을 읽은 것이라도 recv() 는 binary 로 있는 그대로를 전송 받으므로 끝에 \0 가 없다.
    // 마지막에 받은 \n 은 더 이상 필요없으니 이를 \0 으로 바꿔주고 null 로 끝나는 문자열을 만든다.
    lengthBuf[offset] = '\0';

    // sscanf() 는 buf 로부터 scanf 를 할 수 있다.
    // 그리고 parsing 한 필드 숫자를 반환한다. 따라서 반환값이 반드시 1이어야 한다.
    int dataLen = 0;
    int numParsed = sscanf_s(lengthBuf, "%d", &dataLen);
    if (numParsed != 1) {
        std::cerr << "Failed to parse length info" << std::endl;
        return 1;
    }
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
