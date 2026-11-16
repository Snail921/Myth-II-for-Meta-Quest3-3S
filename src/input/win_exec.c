#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
int main(int argc,char**argv){if(argc!=2||strlen(argv[1])>200)return 2;unsigned char p[256]={2};uint32_t n=strlen(argv[1]);memcpy(p+1,&n,4);memcpy(p+5,argv[1],n);int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return 3;struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(7946);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);int ok=sendto(fd,p,sizeof(p),0,(struct sockaddr*)&a,sizeof(a))==sizeof(p);close(fd);return ok?0:4;}
