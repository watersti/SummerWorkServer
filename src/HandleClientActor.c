//Name: Robert Appelmann
#include "HandleClientActor.h"
#include "BlockingQueue.h"

extern char* quit;

char* chat_file = "chat.txt";

void* handle_client_actor(void *arg){
	//Local Variables	
	//file descriptors for sockets objects
  	int remfd; 
 	int err;
  	
  	//I/O Message Stuff
	char mess_buff[8192];
	char str_buff[256];
	char submit_buff[1024];
	char rec_buff[8192];
 	int mess_len = 0;


	//Random Stuff
	int c;
	char* cptr_start;
	void* conn_ptr;
	void* log_message;
	
	//File stuff
	FILE *sendFile;
	FILE *chatFile;
		
	//Logging Stuff
	char log_buffer[1024];
	int log_len;

	ActorQueues* Q = arg;
	struct connection_data* client;
	
	while(1){
		bzero(&str_buff, 256);
		bzero(&mess_buff, 8192);
		bzero(&rec_buff, 8192);
		
		conn_ptr = BlockingQueue_remove(Q->handle_client_q);
		
		if(((char*)conn_ptr)[0] == 'Q' && ((char*)conn_ptr)[1] == 'U' && ((char*)conn_ptr)[2] == 'I' && ((char*)conn_ptr)[3] == 'T'){
			//Quitting Time!!!
			break;
		}
		
		client = (struct connection_data*)conn_ptr;

		remfd = client->fd;

		sprintf(log_buffer, "HCA: Client <%s> Connected...", inet_ntoa(client->addr.sin_addr));

		mess_len = recv(remfd, rec_buff, 8192, 0);

		if(mess_len < 0){
			sprintf(log_buffer, "%s%s\n", log_buffer, strerror(errno));
			free(conn_ptr);
			continue;
		}
		
		//printf("%s\n", rec_buff);
	
		if(rec_buff[0] == 'G' && rec_buff[1] == 'E' && rec_buff[2] == 'T'){
			sprintf(log_buffer, "%sGET </", log_buffer);
			
			//determine what file they want
			cptr_start = strchr(rec_buff, '/');
			if(!cptr_start){
				continue;
			}
			c = 0;
			while(cptr_start[c] != ' ' && cptr_start[c] != '?' ){
				str_buff[c] = cptr_start[c];
				c++;
			}
			cptr_start = str_buff + 1;
			
			sprintf(log_buffer, "%s%s>...", log_buffer, cptr_start);
			
			if(!strcmp(cptr_start, "")){
				sprintf(mess_buff, "HTTP/1.0 200 OK\r\n\r\n<html><meta http-equiv =\"refresh\" content=\"0; url=/pages/basic.html\"/></html>\r\n");
				err = send(remfd, mess_buff, strlen((char*)mess_buff), 0);
				if(err < 0){
					sprintf(log_buffer, "%s%s\n", log_buffer, strerror(errno));
					free(conn_ptr);
					continue;
				}
				sprintf(log_buffer, "%sRedirected To <pages/basic.html>...", log_buffer);
			}else{
				sendFile = fopen(cptr_start, "r");
				if(sendFile == NULL){
					sprintf(mess_buff, "HTTP/1.0 404 File Not Found\r\n\r\n<html><head>ERROR 404... Page Not Found!!!</head></html>\r\n");
					sprintf(log_buffer, "%sFile Not Found...", log_buffer);

					err = send(remfd, mess_buff, strlen((char*)mess_buff), 0);
					if(err < 0){
						sprintf(log_buffer, "%s%s\n", log_buffer, strerror(errno));
						free(conn_ptr);
						continue;
					}	
				}else{
					sprintf(mess_buff, "HTTP/1.0 200 OK\r\n\r\n");
					sprintf(log_buffer, "%sFile Found..", log_buffer);
					
					err = send(remfd, mess_buff, strlen((char*)mess_buff), 0);
					if(err < 0){
						sprintf(log_buffer, "%s%s\n", log_buffer, strerror(errno));
						free(conn_ptr);
						continue;
					}	
					
					while( !feof(sendFile) ){
						int numread = fread(mess_buff, sizeof(unsigned char), 1000, sendFile);
						if( numread < 1 ) break; // EOF or error
					
						char *mess_buffer_ptr = mess_buff;
						do{
							int numsent = send(remfd, mess_buffer_ptr, numread, 0);
							if( numsent < 1 ){ // 0 if disconnected, otherwise error
								break; // timeout or error
							}
							mess_buffer_ptr += numsent;
							numread -= numsent;
						}while(numread > 0);
					}
					fclose(sendFile);
					
					sprintf(log_buffer, "%sSent...", log_buffer);
				}
				bzero(&str_buff, 256);
			}
		}else if(rec_buff[0] == 'P' && rec_buff[1] == 'O' && rec_buff[2] == 'S' && rec_buff[3] == 'T'){
			c = 0;
			while(c < mess_len - 4 && !( rec_buff[c] == '\r' && rec_buff[c+1] == '\n' && rec_buff[c+2] == '\r' && rec_buff[c+3] == '\n')){
				c++;
			}
			while(rec_buff[c] != '='){
				c++;
			}
			
			cptr_start = &rec_buff[c+1];
			c = 0;
			while(cptr_start[c] != '&'){
				if(cptr_start[c] == '+'){
					submit_buff[c] = ' ';
				}else if(cptr_start[c] == '%'){
					submit_buff[c] = ' ';
					submit_buff[c+1] = ' ';
					submit_buff[c+2] = ' ';
					c+=2;
				}else{
					submit_buff[c] = cptr_start[c];
				}
				c++;
			}
			submit_buff[c] = '\0';
			
			chatFile = fopen(chat_file, "a");
				
			fprintf(chatFile, "%s>: %s\n", inet_ntoa(client->addr.sin_addr), submit_buff);
			
			if(!chatFile){
				printf("Error Opening File!\n");
				continue;
			}
			
			fclose(chatFile);
			
			
			sprintf(mess_buff, "HTTP/1.0 200 OK\r\n\r\n<html><meta http-equiv =\"refresh\" content=\"0; url=/pages/chat.html\"/></html>\r\n");
			err = send(remfd, mess_buff, strlen((char*)mess_buff), 0);
			if(err < 0){
				sprintf(log_buffer, "%s%s\n", log_buffer, strerror(errno));
				free(conn_ptr);
				continue;
			}
			sprintf(log_buffer, "%sRefreshing <chat.html>...", log_buffer);
			
		}else{
		}		
		bzero(&mess_buff, 8192);
		bzero(&rec_buff, 8192);
		close(remfd);

		sprintf(log_buffer, "%sClient Closed\n\0", log_buffer);
		log_len = strlen(log_buffer);
		log_message = malloc(log_len);
		if(log_message){
			memcpy(log_message, log_buffer, log_len);
			BlockingQueue_add(Q->log_q, log_message);
		}

		bzero(&log_buffer, 1024);
		free(conn_ptr);
	}
	printf("Handle Client Thread Done!\n");
	return NULL;
}



