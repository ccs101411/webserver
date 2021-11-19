#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 65536

struct {
    char *ext;
    char *type;
} exts[] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
	{"ico","image/ico"},
	{"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"exe","text/plain" },
    {0,0} };

void get(long taken, int socket_fd,char* buffer){

	char* dir = (char*)0;
	int nowpos=0;
	int file_fd;
	int pointpos;
	long tmp;
	char* fstr = "text/html";
	int buflen = strlen(buffer);
	
	//locate GET dir to char dir
	for(int i=4 ; i < buflen ; i++){
		if(buffer[i] == ' '){
			break;
		}else{
			//dir[nowpos] = buffer[i];
			if(buffer[i] == '.')
				pointpos = nowpos;
			nowpos++;
		}
	}
	//from 4 to nowpos is dir
	dir = malloc(sizeof(char) * nowpos+1);
	dir[0] = '.';
	strncpy(&dir[1], &buffer[4], nowpos);
	
	
	//if don't have dir, give it deafult 
	if(nowpos == 1){
		sprintf(dir,"./deafult.html");
	}
	int len;
	int dirlen = strlen(dir);
	for(int i=0;exts[i].ext!=0;i++){
		len = strlen(exts[i].ext);
		if(!strncmp(&dir[pointpos+2], exts[i].ext, len)){
			fstr = exts[i].type;
			break;
		}
	}
	

	////tell clinet failed
	if((file_fd = open(dir,O_RDONLY)) == -1)
		write(socket_fd, "Failed to open file.",19);
	
	////return file and type
	sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	write(socket_fd,buffer,strlen(buffer));
	
	while((tmp = read(file_fd, buffer, BUFSIZE)) > 0){
		write(socket_fd, buffer, tmp);
	}
	close(file_fd);
	free(dir);
	return;
}
void post(long taken, int socket_fd, char* buffer){
	
	int buflen;
	//check upload file
	char *pos; 
	pos = strstr(buffer,"filename=\"");
	if (pos == 0)
		return;//no upload

	char filename[BUFSIZE+1],dir[BUFSIZE+1];	
	//get file name
	pos+=10;
	long i=9;
	sprintf(dir,"./upload/");

	char *end=strstr(pos,"\""),*j;
	for(j=pos;j!=end;i++,j++){
		dir[i] = *j;
	}//cat filename behind dir
	
	dir[i]=0;

	pos = strstr(pos,"\n");
	pos = strstr(pos+1,"\n");
	pos = strstr(pos+1,"\n");
	pos++;
	//locate position to start 
	end = pos;
	end = strstr(end, "\r\n---------------");
	buflen = strlen(buffer);
	if(end != NULL)	
		*end = '\0';
	else
		end = &buffer[buflen-1];
	//locate end position

	int download_fd = open(dir,O_CREAT|O_WRONLY|O_TRUNC|O_SYNC,S_IRWXO|S_IRWXU|S_IRWXG);
	if(download_fd == -1)
		write(socket_fd,"Faild to download file.\n",19);

	write(download_fd,pos,(end-pos));
	
	while((taken = read(socket_fd, buffer, BUFSIZE)) > 0){
		
		printf("In while\n");
		buflen = strlen(buffer);
		//taken = read(socket_fd, buffer, BUFSIZE);
		end = strstr(end, "---------------");
		if(end != NULL){
			*end = '\0';
			write(download_fd, buffer, (end - buffer));
		}else{
			write(download_fd, buffer, taken);
		}
		if(end != NULL)
			break;
	}
	close(download_fd);
	printf("////close fd////\n");
	
	taken = open("./deafult.html", O_RDONLY);
	sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
	write(socket_fd, buffer, strlen(buffer));

	while((i = read(taken, buffer, BUFSIZE)) > 0){
		
		write(socket_fd, buffer, i);
	}
	close(taken);
	printf("Done");
	return;
}



void dealsocket(int socket_fd){
    
	long i, j, taken;
	int file_fd, buflen, len;
    char * fstr;
    static char buffer[BUFSIZE+1];
	
	//read from browser
    taken = read(socket_fd,buffer,BUFSIZE);
	
	buflen = strlen(buffer);

	//print the buffer on Terminal and remove\r\n	
	if((taken == 0)||(taken == -1))
		{
			perror("read form socket");
			exit(3);
		}
	else
		{
			for(i=0;i<buflen;i++)
			{	
				printf("%c",buffer[i]);
			}
		}

	i = 0;
	//deal GET or POST
	if((strncmp(buffer,"GET ",4) == 0) || (strncmp(buffer,"get ",4) == 0)){
		
		printf("//////////////////////DO GET\n");
		get(taken, socket_fd, buffer);//GET
	}else if((strncmp(buffer,"POST ",5) == 0) || (strncmp(buffer,"post ",5))){
	
		printf("////////////////////DO POST\n");
		post(taken, socket_fd, buffer);//POST
	}
	else{
	
		printf("None Should do\n");//other
	}
	
	return;
}



int main(int argc, char **argv){

    int i, pid, listenfd, socketfd;
    int length;
    int reuse=0;
	static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;
	printf("Start of Server\n");
	
    //work in data
    if(chdir("./data") == -1){ 
        printf("ERROR: Can't Change to directory %s\n",argv[2]);
        exit(4);
    }

    
	//prevent zombie
    signal(SIGCLD, SIG_IGN);

    //open socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd<0)
        exit(3);
	if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) < 0){
		perror("setsockopet error");
	}

	//set net
    serv_addr.sin_family = AF_INET;
    //set IP
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// set port
    serv_addr.sin_port = htons(80);
	
    /* 開啟網路監聽器 */
    if (bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))<0){
		
		perror("bind");
		exit(3);
	}
	
    
	/* 開始監聽網路 */
    if (listen(listenfd,64)<0){
		
		perror("listen");
		exit(3);
	}
    
	while(1){
		
		length = sizeof(cli_addr);
        /* 等待客戶端連線 */
		socketfd = accept(listenfd, (struct sockaddr* )&cli_addr, &length);
        if (socketfd < 0){
			perror("accept");
			exit(3);
		}
		
        /* 分出子行程處理要求 */
        if ((pid = fork()) < 0) {
            perror("fork fail");
			exit(3);
        }else{
        	if (pid == 0) {  /* 子行程 */
            	close(listenfd);
            	dealsocket(socketfd);
            }
    	}
		close(socketfd);
	}

	return 0;
}
