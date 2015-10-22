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
#include <rsa_encrypt.h>

void doprocessing (int sock);
int verifyConnection(int sock);
float calculateKey(float r);
void incrementConnectCount(int sock);
void decrementConnectCount(int sock);
int verifyString(char s1[]);
void noticfyCountChange(int sock);

static int *connection_count;
pthread_mutex_t lock;
static char publicKey[]="-----BEGIN PUBLIC KEY-----\n"\
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2aZ7C2byljaEW963DR99\n"\
"DBaRFMCms4FS+CfQSVnsPIR1FZ9c2zAKuxaKr606BuHfgRMa0m1kFDSuXg8oeg/8\n"\
"7xwOuyUpkpjKbcJBIceZ1quqbab3fLjxEu2/eHxX2fOOSwIC4t4KKoGhEohb7m0O\n"\
"stHbkjJyX3BNIqBTLMbAfLs2H5UZQ5ihwXCs8Jt7lyHZYOmmz4j1EXwACz5BKJa/\n"\
"yQ6O6DzhWhw7mfX7oeHLx7y0lP7mAnvjsPkuSDHWg8761c3PNV0KAxlOmXWV+46m\n"\
"o2ajly528gHtlcG8jyXngKu9zSNPtXbyL0tqK8Ia0XG3bTcbXobrLx5EEvLF17bO\n"\
"cQIDAQAB\n"\
"-----END PUBLIC KEY-----\n";

static char privateKey[]="-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEogIBAAKCAQEA2aZ7C2byljaEW963DR99DBaRFMCms4FS+CfQSVnsPIR1FZ9c\n"\
"2zAKuxaKr606BuHfgRMa0m1kFDSuXg8oeg/87xwOuyUpkpjKbcJBIceZ1quqbab3\n"\
"fLjxEu2/eHxX2fOOSwIC4t4KKoGhEohb7m0OstHbkjJyX3BNIqBTLMbAfLs2H5UZ\n"\
"Q5ihwXCs8Jt7lyHZYOmmz4j1EXwACz5BKJa/yQ6O6DzhWhw7mfX7oeHLx7y0lP7m\n"\
"AnvjsPkuSDHWg8761c3PNV0KAxlOmXWV+46mo2ajly528gHtlcG8jyXngKu9zSNP\n"\
"tXbyL0tqK8Ia0XG3bTcbXobrLx5EEvLF17bOcQIDAQABAoIBAFb3wdTVhvtkv1Ci\n"\
"VoyE14ecAM7FCBdUBp4n8n9M0iuAVfSU4BqpQLBvGm2GD1iuqos94grRMRstzbrB\n"\
"3c+gUdYNZQl8mPb93G0tIK9pvVvc7lWwUNXiZFnG7CotrUDmpCXuoM6cIH3JnF32\n"\
"ZJ2JsETdvBnX1IG0Mu4yF+odZM6xlrl2s763dM0A1W2+V8+kinuMk6Ij/w8f6qwO\n"\
"xWNv9T9iIug/ZqYxl2Z6AC15lt1plWqmpR+/C/vZAycO/4ba23yA/T93ppI6Prls\n"\
"SrRRQOuiSAxOiXnJeJrjIR3LCLxA/FaZCVHn+5nP4whDNWSwD3cW4V42fT0AaP39\n"\
"H02O6P0CgYEA+u+uRpnoz6QbE6DLK9WXxRa2TzEBwCjuXeCP1az5l1T8rbXY2wdB\n"\
"0jG5Cypidm2K5VgUTtcpL2OEtYpO0yGR18STHp8ivjMq/zwcbQNXMjQQoS+MpY4C\n"\
"JB9D/Sa+rIlp3TlQFR0O0WpFkxPlisMuwgMC5mHaU+1YGOZWfHJfVnMCgYEA3grY\n"\
"12ZoULANxYMyoa1+1ffMpS79SYFDDjgoawL3gmrJ77hRFwPVKoS9kD+P8vLcCbb6\n"\
"GOGNibhTE84zgHUSt10BxvqT5rEtimd51GSQDQvz6XN8BtJ8e9lT27CqMI/A4RSA\n"\
"ORclyL/7xcnblqfJZ4HbrCdT3GMLczpksEosKosCgYATP31LtxSPkNPu3n+XHiwl\n"\
"Fwfm7ShoN/uIwefmiP0gKYm0PWWj71uSJvQ9sgOZd++WhfoQzho/o0+TqUdAlKue\n"\
"NFrgl25Pzpb2uwKnLgFrPWklJAOS02DAqpFJgsZPb77qFDJbXD49u/wYOwd5bF1S\n"\
"zm5Tg5/+ng8K4egyZYeAlwKBgDvrxXTrp/8OY8kHjORgw7fOdaWmNqhdf9EYip/G\n"\
"XzEMzI4quYdye1ZMGDQTUy+HJqZggMGyLNYbrh/MJ83kyjY9nTsraDp7WgTdw9zz\n"\
"foRm014O0kutBfcOQTkqReNxB+Y1+BxwCLxSQwmvXdkcUpfiTXv56QGyoOZpvb5t\n"\
"ZdndAoGAbREkiXUYwxspb2cUDziiKzMTKSqfagPf3y8SwoHOfRy3iAMtvwVOsxHL\n"\
"z14rg+V8Y9BH8eKFKs7IhUmLuDByc2N38Wa1ncKQAxEDBWIzxqi9f2uLsRIyo1Sa\n"\
"wfc1y9ldw8hdR5GuaNgH4WmM/xFYwQ9eY37XpEJogiBbXolScsw=\n"\
"-----END RSA PRIVATE KEY-----\n";

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
      incrementConnectCount(newsockfd);
      printf("Current number of connected clients: %d\n", *connection_count);
      doprocessing(newsockfd);
      decrementConnectCount(newsockfd);
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
   size_t keysize = sizeof(publicKey);
   size_t intsize = sizeof(int);
   int encrypted_sie;
   unsigned char encrypted[4098];
   unsigned char decrypted[4098] = {};
   
   //send public key size
   n = send(sock, &keysize, intsize, 0);
   if (n < 0)
   {
     perror("ERROR sending public key");
     exit(1);
   }

   //send public key
   n = send(sock, publicKey, keysize, 0);
   if (n < 0)
   {
      perror("ERROR sending public key");
      exit(1);
   }

   //recieve encryted length
   n = read(sock, &encrypted_sie, intsize);
   if (n < 0)
   {
      perror("ERROR reading encrypted message");
      exit(1);
   }

   //receive encrypted msg
   n = read(sock,encrypted,4098);
   if (n < 0)
   {
      perror("ERROR reading encrypted message");
      exit(1);
   }

   //decrypt msg
   int decrypted_length = private_decrypt(encrypted,encrypted_sie,privateKey, decrypted);
   if(decrypted_length == -1)
   {
       printLastError("Private Decrypt failed ");
       exit(0);
   }

   if (verifyString(decrypted)) {
      printf("Connection successfully verified\n");
      return 0;
   }
   printf("Connection verification failed\n");
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
         // if (n == -1) {
         //    decrementConnectCount(sock);
         //    exit(0);
         // }
         perror("ERROR reading from socket");
         //exit(1);
         break;
      }
      
      printf("Here is the message: %s\n",buffer);
      
      n = write(sock,"I got your message",18);
      if (n < 0)
      {
         perror("ERROR writing to socket");
         //exit(1);
         break;
      }
   }
}

float calculateKey(float r) {
   return r*3;
}

void incrementConnectCount(int sock) {
   pthread_mutex_lock(&lock);
   *connection_count += 1;
   pthread_mutex_unlock(&lock);
   noticfyCountChange(sock);
}

void decrementConnectCount(int sock) {
   printf("decrement called\n");
   pthread_mutex_lock(&lock);
   *connection_count -= 1;
   pthread_mutex_unlock(&lock);
   //noticfyCountChange(sock);
}

int verifyString(char s1[]){
   char s2[] = "Activity Heat Map";
   int len = strlen(s1);
   int i;

   if (len != strlen(s2))
   return 0; // They must be different
   for (i=0 ; i < len; i++)
   {
      if (s1[i] != s2[i])
      return 0;  // They are different
   }
   return 1;  // They must be the same
}

void noticfyCountChange(int sock){
   int n;
   //send count
   n = send(sock, connection_count, sizeof(int), 0);
   if (n < 0)
   {
     perror("ERROR sending count change");
     exit(1);
   }
   printf("sent\n");
}