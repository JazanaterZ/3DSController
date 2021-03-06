#include <stddef.h>

#include "general.h"

#include "settings.h"
#include "screenshot.h"

#include "wireless.h"

SOCKET listener;
SOCKET client;

struct sockaddr_in client_in;

int sockaddr_in_sizePtr = (int)sizeof(struct sockaddr_in);

struct packet buffer;
char hostName[80];

void initNetwork(void) {
	WSADATA wsaData;
	
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	if(gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) {
		error("gethostname()");
	}
}

void printIPs(void) {
	struct hostent *phe = gethostbyname(hostName);
    if(phe == 0) {
       error("gethostbyname()");
    }
	
	int i;
    for(i = 0; phe->h_addr_list[i] != 0; i++) {
        struct in_addr addr;
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
        printf("%s\n", inet_ntoa(addr));
    }
	
	if(i) {
		printf("Usually you want the first one.\n");
	}
}

void startListening(void) {
	int nret;
	
	listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if(listener == INVALID_SOCKET) {
		error("socket()");
	}
	
	SOCKADDR_IN serverInfo;
	
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = IP;
	serverInfo.sin_port = htons(settings.port);
	
	u_long one = 1;
	ioctlsocket(listener, FIONBIO, &one);
	
	nret = bind(listener, (LPSOCKADDR)&serverInfo, sizeof(struct sockaddr));
	
	if(nret == SOCKET_ERROR) {
		error("bind()");
	}
}

void sendBuffer(int length) {
	if(sendto(listener, (char *)&buffer, length, 0, (struct sockaddr *)&client_in, sizeof(struct sockaddr_in)) != length) {
		error("sendto");
	}
}

int receiveBuffer(int length) {
	return recvfrom(listener, (char *)&buffer, length, 0, (struct sockaddr *)&client_in, &sockaddr_in_sizePtr);
}

void sendScreenshot(void) {
	FILE *f = fopen(SCREENSHOT_NAME, "rb");
	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	unsigned char *screenshotData = malloc(len);
	rewind(f);
	fread(screenshotData, len, 1, f);
	fclose(f);
	
	buffer.command = SCREENSHOT;
	
	while(1) {
		int tl = len - buffer.offset > SCREENSHOT_CHUNK ? SCREENSHOT_CHUNK : len - buffer.offset;
		memcpy(buffer.data, screenshotData + buffer.offset, tl);
		sendBuffer(tl + offsetof(struct packet, screenshotPacket));
		if(tl < SCREENSHOT_CHUNK) break;
	}
	
	free(screenshotData);
}
