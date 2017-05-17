#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>   
#include <time.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "Actors.h"
#include "BlockingQueue.h"

void* handle_client_actor(void *arg);

struct connection_data{
	int fd;
	struct sockaddr_in addr;
	unsigned int addr_len;
};

