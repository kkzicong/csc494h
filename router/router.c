#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void doprocessing (int sock);
int verifyConnection(int sock);
float calculateKey(float r);
void incrementConnectCount();
void decrementConnectCount();
static int *connection_count;
pthread_mutex_t lock;

int main( int argc, char *argv[] )
{
   int sockfd, newsockfd, portno, clilen;
   char buffer[256];
   struct sockaddr_in serv_addr, cli_addr;
   int  n, pid;
   int err;
   pthread_t tid;
   
   connection_count = mmap(NULL, sizeof *connection_count, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

   *connection_count = 0;

   /* First call to socket() function */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0)
      {
      perror("ERROR opening socket");
      exit(1);
      }
   
   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = 5002;
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY; 
   serv_addr.sin_port = htons(portno);
   
   char ip[100];

   //initialize connection count mutex
   if (pthread_mutex_init(&lock, NULL) != 0)
   {
     printf("\n mutex init failed\n");
     return 1;
   }

   /* Now bind the host address using bind() call.*/
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
      {
      perror("ERROR on binding");
      exit(1);
      }
   
   /* Now start listening for the clients, here
   * process will go in sleep mode and will wait
   * for the incoming connection
   */

   listen(sockfd,5);
   clilen = sizeof(cli_addr);
   
   while (1)
   {
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (newsockfd < 0)
         {
         perror("ERROR on accept");
         exit(1);
         }
      
      /* Create child process */
      pid = fork();
      if (pid < 0)
      {
         perror("ERROR on fork");
         exit(1);
      }
      
      if (pid == 0)
      {
      /* This is the client process */
      close(sockfd);
      if (verifyConnection(newsockfd) < 0) {
         exit(0);
      }
      printf("Verify connection successful!\n");
      incrementConnectCount();
      printf("Current number of connected clients: %d\n", *connection_count);
      doprocessing(newsockfd);
      decrementConnectCount();
      exit(0);
      }
      else
      {
         close(newsockfd);
      }
   }
}

int verifyConnection(int sock){
   int n;
   size_t float_size = sizeof(float);
   char buffer[float_size];
   float r_buf;
   float key;

   bzero(buffer,float_size);
   
   srand(time(NULL));
   r_buf = ((float)rand()/(float)(RAND_MAX));

   n = send(sock, &r_buf, float_size, 0);

   if (n < 0)
      {
      perror("ERROR writing to socket");
      exit(1);
      }
   key = calculateKey(r_buf);

   n = read(sock,&r_buf,float_size);
   if (n < 0)
   {
      perror("ERROR writing to socket");
      exit(1);
   }

   printf("%f\n", r_buf);
   if(r_buf == key) {
      return 0;
   }
   return -1;
}

void doprocessing (int sock)
{
   int n;
   char buffer[256];
   
   bzero(buffer,256);
   
   while(1) {
      n = read(sock,buffer,255);
   
      if (n < 0)
         {
         perror("ERROR reading from socket");
         exit(1);
         }
      
      printf("Here is the message: %s\n",buffer);
      n = write(sock,"I got your message",18);
      
      if (n < 0)
         {
         perror("ERROR writing to socket");
         exit(1);
         }
   }
}

float calculateKey(float r) {
   return r*3;
}

void incrementConnectCount() {
   pthread_mutex_lock(&lock);
   *connection_count += 1;
   pthread_mutex_unlock(&lock);
}

void decrementConnectCount() {
   pthread_mutex_lock(&lock);
   *connection_count -= 1;
   pthread_mutex_unlock(&lock);
}