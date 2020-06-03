/*hello_server.c*/
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

#define BUF_SIZE 4096


struct node 
  {
     int file_descriptor;
     int port_number;
     char* client_name;
     struct node *next;
  };

typedef struct _rwlock_t {
    sem_t lock; // binary semaphore (basic lock)
    sem_t writelock; // used to allow ONE writer or MANY readers
    int readers; // count of readers reading in critical section
} rwlock_t;


  struct node *head = NULL;
  rwlock_t mutex;
  

void rwlock_init(rwlock_t *rw) {
    rw->readers = 0;
    sem_init(&rw->lock, 0,1); 
    sem_init(&rw->writelock, 0, 1); 
}

void rwlock_acquire_readlock(rwlock_t *rw) {
    
    sem_wait(&rw->lock);
    rw->readers++;
    if (rw->readers == 1)
    sem_wait(&rw->writelock);
    sem_post(&rw->lock);
    
    return;
}

void rwlock_release_readlock(rwlock_t *rw) {
    sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)
    sem_post(&rw->writelock);
    sem_post(&rw->lock);
    return;
}

void rwlock_acquire_writelock(rwlock_t *rw) {
    sem_wait(&rw->writelock);
    return;
}

void rwlock_release_writelock(rwlock_t *rw) {
    sem_post(&rw->writelock);
    return;
}
   struct node* Add(int file_descriptor, int port_number, char client_name[]) 
  {
    rwlock_acquire_writelock(&mutex);
    struct node *temp = (struct node*) malloc(sizeof(struct node));
    temp->file_descriptor = file_descriptor;
     temp->port_number = port_number;
     printf("%s GOT THIS\n", client_name);
     char* name_of_client =  ( char *) malloc (strlen(client_name));
     strcpy(name_of_client,client_name);
     temp->client_name = name_of_client;
     temp->next = head;
     head = temp;
     rwlock_release_writelock(&mutex);
     return temp;
  }
  

  struct node* Delete(int file_descriptor) 
  {
    rwlock_acquire_writelock(&mutex);
     struct node* previous = NULL;
     struct node* current = head;
     if(head == NULL) 
     {
        rwlock_release_writelock(&mutex);
        return NULL;
     }
  
     while(current->file_descriptor != file_descriptor) 
     {
        if(current->next == NULL) 
        {
           rwlock_release_writelock(&mutex);
           return NULL;
        } 
        else 
        {
           previous = current;
           current = current->next;
        }
     }
     if(current == head) 
     {
        head = head->next;
     } else 
     {
        previous->next = current->next;
     }    
    rwlock_release_writelock(&mutex);
     return current;
  }
  

  void printList() {
     struct node *ptr = head;
     // rwlock_acquire_readlock(&mutex);
     printf("\n[ ");
    
     //start from the beginning
     while(ptr != NULL) {
        printf("(%d,%d,%s) ",ptr->file_descriptor,ptr->port_number,(ptr->client_name));
        ptr = ptr->next;
     }
    // rwlock_release_readlock(&mutex);
     printf(" ]");
  }


char* lst() 
  { 
    char* lst_clients = (char*) malloc(BUF_SIZE); 
    struct node *ptr = head;  
    rwlock_acquire_readlock(&mutex);
    while(ptr!= NULL) 
    { 
      strcat(lst_clients,ptr->client_name); 
      strcat(lst_clients," ");  
      // printf("%s\n",lst_clients );  
      ptr = ptr->next;  
    };  
    strcat(lst_clients,"\n");
    rwlock_release_readlock(&mutex);
    return lst_clients; 
  }


char** parsed(char* str_input, char * delim, int* howManyParsed)
{
  char* forParsedLength;
  *howManyParsed = 0;
  char str_input_copy[strlen(str_input)];
  strcpy(str_input_copy, str_input);
  char* parsedWord;
  forParsedLength = strtok(str_input_copy, delim);

  while( forParsedLength != NULL ) //calculating the length of the str_input
  {
      (*howManyParsed)++;
      forParsedLength = strtok(NULL, delim);
   }
   
   parsedWord = strtok(str_input, delim);
   char** parsedList = (char **)malloc((*howManyParsed) * sizeof(char*));   //using that calcuated length to make dynamic array of char
   for(int i=0; i< *howManyParsed; i++)
   {
    parsedList[i] = parsedWord;
    parsedWord = strtok(NULL, delim);
    
   }
   return parsedList;
}


void * after_accept(void* args)
{
  struct node * connection =  (struct node *) args;
  int client_sock = connection->file_descriptor;
  char response[BUF_SIZE];           //what to send to the client
  fd_set activefds, readfds;
  FD_ZERO(&activefds);
  FD_SET(client_sock, &activefds);
  int i,n;

  while(1)
  {
      //update the set of selectable file descriptors
    readfds = activefds;

      for(i=0; i < FD_SETSIZE; i++){

        //was the file descriptor i set?
        if(FD_ISSET(i, &readfds)){

          if(i == client_sock){ //activity on server socket, incoming connection

            //read from client and echo back
            n = read(client_sock, response, BUF_SIZE-1); 
            response[n] ='\0'; //removing old buffer values
             
            printf("Sending: %s\n",response); 

            if(n <= 0){ //closed or error on socket

              //close client sockt
              close(client_sock);

              //remove file descriptor from set
              FD_CLR(client_sock, &activefds);

            }
            else{ //client sent a message

              response[n] = '\0'; //NULL terminate
              char ** spaced_response;
              int * parsedsize = (int *) malloc(sizeof(int));
              spaced_response = parsed(response," ",parsedsize);
              char message_for_client[BUF_SIZE];
              int message_descriptor = 0;
              char client_name_msg[BUF_SIZE];

              if(strcmp("/msg",spaced_response[0])==0)
              {
                if(*parsedsize < 3)
                {
                  strcat(message_for_client,"missing destination or message\n");
                  write(client_sock,message_for_client,strlen(message_for_client));
                  continue;
                }
                struct node *ptr = head;
                int client_found =0;
                while(ptr!=NULL)
                {
                  if(strcmp(ptr->client_name,spaced_response[1])==0)
                  {
                    client_found = 1;
                    message_descriptor = ptr->file_descriptor;
                    break;
                  }
                  ptr = ptr->next;
                }

                if(client_found == 0)
                {
                  strcat(message_for_client,"client with this name does not exist\n");
                  write(client_sock,message_for_client,strlen(message_for_client));
                  continue;
                }

                struct node* ptr1 = head;
                while (ptr1!= NULL)
                {
                  if(ptr1->file_descriptor == client_sock)
                  {
                    strcpy(client_name_msg,ptr1->client_name);
                    break;
                  }
                  ptr1 = ptr1 ->next;
                }
                strcat(message_for_client,client_name_msg);
                strcat(message_for_client," sent this message: ");

                i =2;
                while (i< *parsedsize)
                {
                  strcat(message_for_client,spaced_response[i]);
                  strcat(message_for_client," ");
                  i++;
                }
                write(message_descriptor,message_for_client,strlen(message_for_client));
              }
              
              else if (strcmp(spaced_response[0],"/list\n") == 0)
              {
                // char lst_list[BUF_SIZE] =lst();
                // lst_list[strlen(lst_list)] = '\0';
                write(client_sock,lst(),strlen(lst()));
              }

              else if(strcmp(spaced_response[0],"/quit\n")==0)
              {
                char quit_message[100];
                strcat(quit_message,"client have quitted \n");
                Delete(client_sock);
                printList();
                write(client_sock,quit_message,strlen(quit_message));
                close(client_sock);
                FD_CLR(client_sock, &activefds);

              }  

              else
              {
                write(client_sock, response, n);

              }
              // //echo messget to client
              // write(client_sock, response, n);

              // printf("Received From: %s:%d (%d): %s",         //LOG
              //        inet_ntoa(client_saddr_in.sin_addr), 
              //        ntohs(client_saddr_in.sin_port), 
              //        client_sock, response);
            }

          }

        }

    }
  }

}



int main(int argc, char * argv[]){

  char hostname[]="127.0.0.1";   //localhost ip address to bind to
  if(argc < 2)
  {
    perror("too less arguements passed");
    return 0;
  }
  short port=atoi(argv[1]);               //the port we are to bind to
  char* name_of_client =  ( char *) malloc (BUF_SIZE);
  int len_of_name;

  pthread_t thread; //for multi-threading

  struct sockaddr_in saddr_in;  //socket interent address of server
  struct sockaddr_in client_saddr_in;  //socket interent address of client

  socklen_t saddr_len = sizeof(struct sockaddr_in); //length of address

  int server_sock, client_sock;         //socket file descriptor


  char response[BUF_SIZE];           //what to send to the client
  int n;                             //length measure

  //set up the address information
  saddr_in.sin_family = AF_INET;
  inet_aton (hostname, &saddr_in.sin_addr);
  saddr_in.sin_port = htons(port);

  rwlock_init(&mutex);
  
  //open a socket
  if( (server_sock = socket(AF_INET, SOCK_STREAM, 0))  < 0){
    perror("socket");
    exit(1);
  }

  //bind the socket
  if(bind(server_sock, (struct sockaddr *) &saddr_in, saddr_len) < 0){
    perror("bind");
    exit(1);
  }

  //ready to listen, queue up to 5 pending connectinos
  if(listen(server_sock, 5)  < 0){
    perror("listen");
    exit(1);
  }


  saddr_len = sizeof(struct sockaddr_in); //length of address

  
  printf("Listening On: %s:%d\n", inet_ntoa(saddr_in.sin_addr), ntohs(saddr_in.sin_port));
/* echo_server_select.c*/
  fd_set activefds, readfds;

  FD_ZERO(&activefds); //clear the set
  FD_SET(server_sock, &activefds); //add fd to the set
//add other file descriptors

//select at most FD_SETSIZE file descriptor from set that are ready for an action
  

  //server setup and etc.


  while(1){ //loop


    //update the set of selectable file descriptors
    readfds = activefds;

    //Perform a select
    if( select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0){
      perror("select");
      exit(1);
    }

    //check for activity on all file descriptors
    int i = 0;
    int flag = 0; // to not run instructions if scoket isnot vallid
    for(i=0; i < FD_SETSIZE; i++){

      //was the file descriptor i set?
      if(FD_ISSET(i, &readfds)){

        if(i == server_sock){ //activity on server socket, incoming connection

          //accept incoming connections = NON BLOCKING
          client_sock = accept(server_sock, (struct sockaddr *) &client_saddr_in, &saddr_len);

          /* ADD CONDITON FOR ERROR: CLIENT_SOCKET <0 */

          read(client_sock,name_of_client,4096);

          //Generate error message & close the socket if client already in use.
          rwlock_acquire_readlock(&mutex);
          struct node *ptr = head;
          while(ptr!=NULL)
          {
            if(strcmp(ptr->client_name,name_of_client)==0)
            {
              printf("Client already in use\n");
              close(client_sock);
              flag = 1;
              break;
            }
            ptr = ptr->next;
          }
          rwlock_release_readlock(&mutex);

          if (flag == 0)
          {
            struct node * connection = Add(client_sock,client_saddr_in.sin_port,name_of_client);
            printList();

            printf("Connection From: %s:%d (%d)\n", inet_ntoa(client_saddr_in.sin_addr), 
                   ntohs(client_saddr_in.sin_port), client_sock);

            //add socket file descriptor to set
            //FD_SET(client_sock, &activefds);

            if (pthread_create(&thread, NULL, after_accept,connection)!=0)
            {
              printf("thread failure");
            }
          }
        }

      }

    }

  }
}