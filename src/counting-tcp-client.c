#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <time.h>

#define NO_NAGLE (0)
#define QUICKACK (0)

#define TCP_PORT (5127)
#define RX_BUFF_SIZE (1525)
#define TX_BUFF_SIZE (1525)

#define SLEEP_USECS (250000)

#define END_LINK (9)

int main(int argc, char *argv[]) {
	int sockfd = 0;
	char receiveBuff[RX_BUFF_SIZE];
	char sendBuff[TX_BUFF_SIZE];
	struct sockaddr_in serv_addr;
	int tmp = 0;
	int cnt = 0;
	int done = 0;
	time_t ticks;
#if (QUICKACK ==1) || (NO_NAGLE ==1)
	int one = 1;
#endif

	// verify args or usage
	if (argc != 2) {
		printf("\n Usage: %s <ip of server> \n", argv[0]);
		return 1;
	}

	// initialize receive buffer and open a socket
	memset(receiveBuff, 0, RX_BUFF_SIZE);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("\n Error : Could not create socket \n");
		return 1;
	}

#if (NO_NAGLE ==1)
	// disable Nagle algorithm on socket to chunk small data
	tmp = setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
	if (tmp < 0) {
		printf("\n Error : Could not set socket option \n");
		return 1;
	}
#endif

	// initialize server context
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(TCP_PORT);

	// use argument 1 as the IP address to connect to
	tmp = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
	if (tmp <= 0) {
		printf("\n Error : converting ip address \n");
		return 1;
	}

	// start the connection
	tmp = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	if (tmp != 0) {
		printf("\n Error : Connect Failed \n");
		return 1;
	}

#if (QUICKACK ==1)
	// don't wait to ack
	tmp = setsockopt(sockfd, SOL_TCP, TCP_QUICKACK, &one, sizeof(one));
	if (tmp < 0) {
		printf("\n Error : Could not set socket option \n");
		return 1;
	}
#endif

	// receive time as initial exchange (hard-coded order is a problem)
	tmp = read(sockfd, receiveBuff, RX_BUFF_SIZE);
	if (tmp < 0) {
		printf("\n Error : Read error \n");
		return 1;
	}

	// print out received time
	receiveBuff[tmp] = 0;
	if (fputs(receiveBuff, stdout) == EOF) {
		printf("\n Error : fputs error\n");
	}

	//
	cnt = 1;
	while (!done) {

		// send count and time
		memset(sendBuff, 0, TX_BUFF_SIZE);
		ticks = time(NULL);
		snprintf(sendBuff, TX_BUFF_SIZE, "Sending %d at %.24s\n", cnt,
				ctime(&ticks));
		tmp = write(sockfd, sendBuff, TX_BUFF_SIZE);
		if (tmp != TX_BUFF_SIZE) {
			perror("\n Error : Unable to write all bytes or write error \n");
		}
		printf("%s", sendBuff);

		cnt++;

		// wait a little before sending again
		usleep(SLEEP_USECS);

		// check if we are done
		if (cnt > END_LINK) {
			done = 1;
		}

	}

	usleep(SLEEP_USECS);

	// receive time as final exchange (hard-coded order is a problem)
	memset(receiveBuff, 0, RX_BUFF_SIZE);
	tmp = read(sockfd, receiveBuff, RX_BUFF_SIZE);
	if (tmp < 0) {
		printf("\n Error : Read error \n");
		return 1;
	}

	// print out received time
	receiveBuff[tmp] = 0;
	if (fputs(receiveBuff, stdout) == EOF) {
		printf("\n Error : fputs error \n");
	}

	// close it out gracefully
	tmp = close(sockfd);
	if (tmp < 0) {
		printf("\n Error : Problem closing socket \n");
	}

	return 0;
}
