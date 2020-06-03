/*get_usna.c*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/select.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>


void * after_write(void * args)
{

    int sock = *((int *) args);
    char *buffer;
    size_t bufsize = 32;
    buffer = (char *)malloc(bufsize * sizeof(char));

    while(1)
    {
        if (getline(&buffer,&bufsize,stdin) != 0)
        {
            if(write(sock,buffer,strlen(buffer)) < 0)
            {
                perror("send");
                break;
            }
        }   
    }
    free(buffer);
}

int main(int argc, char * argv[]){

  if(argc < 4)
  {
    perror("too less arguements passed");
    return 0;
  }
  
  char * hostname = argv[1];    //the hostname we are looking up
  short port = atoi(argv[2]);
  char * name = argv[3];                 //the port we are connecting on

  pthread_t thread; //for multi-threading

  struct addrinfo *result;       //to store results
  struct addrinfo hints;         //to indicate information we want

  struct sockaddr_in *saddr_in;  //socket interent address

  int s,n;                       //for error checking

  int sock;                      //socket file descriptors

  char response[4096];           //read in 4096 byte chunks

  //setup our hints
  memset(&hints,0,sizeof(struct addrinfo));  //zero out hints
  hints.ai_family = AF_INET; //we only want IPv4 addresses

  //Convert the hostname to an address
  if( (s = getaddrinfo(hostname, NULL, &hints, &result)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(s));
    exit(1);
  }

  //convert generic socket address to inet socket address
  saddr_in = (struct sockaddr_in *) result->ai_addr;

  //set the port in network byte order
  saddr_in->sin_port = htons(port);

  //open a socket
  if( (sock = socket(AF_INET, SOCK_STREAM, 0))  < 0){
    perror("socket");
    exit(1);
  }

  //connect to the server
  if(connect(sock, (struct sockaddr *) saddr_in, sizeof(*saddr_in)) < 0){
    perror("connect");
    exit(1);
  }

  if(write(sock,name,strlen(name)) < 0){
            perror("send");
    }

  pthread_create(&thread,NULL,after_write,&sock);

  // char *buffer;
  // size_t bufsize = 32;
  // buffer = (char *)malloc(bufsize * sizeof(char));
  // write(sock, name, strlen(name));
  
  //send the request
  while(1)
  {
  //   if (getline(&buffer,&bufsize,stdin) != 0)
  //   {
  //     if(write(sock,buffer,strlen(buffer)) < 0){
  //       perror("send");
  //   }
  //   free(buffer);
  // }
  // if(write(sock,request,strlen(request)) < 0){
  //   perror("send");

    //read the response until EOF
    n = read(sock, response, 4096);
    if (n > 0){
      // printf("%s: ", name);
      //write response to stdout
      if(write(1, response, n) < 0){
        perror("write");
        exit(1);
      }
      printf("\n");


    }
      if (n<=0){
      perror("read");
      return;
      }
  }

  //close the socket
  close(sock);
  //kill the thread
  return 0; //success
}
