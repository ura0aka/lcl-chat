// == CLIENT SIDE ==
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
// host computer on the internet
/*
struct  hostent
{
  char    *h_name;        // official name of host 
  char    **h_aliases;    // alias list 
  int     h_addrtype;     // host address type (*always AF_INET*)
  int     h_length;       // length of address (in bytes)
  char    **h_addr_list;  // list of addresses from name server 
  #define h_addr  h_addr_list[0]  // addresses for the host, returned in network byte order 
};
*/
void error(const char* message)
{
  std::perror(message);
  exit(1);
}

int socket_setup(char* args[])
{
  int sockfd, portno;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  
    // create and open socket
  portno = std::stoi(args[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    error("ERROR: Error opening socket \n");

  server = gethostbyname(args[1]);
  if(server == NULL)
  {
    std::cerr << "ERROR: No such host found \n";
    exit(0);
  }

  // set fields in serv_addr
  bzero((char*) &serv_addr, sizeof(serv_addr)); // initialize serv_addr buffer
  serv_addr.sin_family = AF_INET;
  bcopy((char*) server->h_addr,(char*) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  // for client to establish connection to the server (fd, host address, address sz)
  if(connect(sockfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR: Error connecting to server");
  std::cout << "Connected successfully to the server \n";

  return sockfd;
}

void send_message(int& sockfd)
{
  while(1)
  {
    int n;
    char buffer[256];
    // reads message from stdin, writes message to socket, reads reply from socket
    std::cout << "> Enter your message: ";
    std::string data{};
    std::getline(std::cin, data);
    memset(&buffer, 0, sizeof(buffer)); // clear buffer, initialize
    strcpy(buffer, data.c_str());
    if(data == "Exit" || data == "exit")
    {
      write(sockfd,buffer,strlen(buffer));
      break;
    }

    n = write(sockfd,buffer,strlen(buffer));
    if(n < 0)
      error("ERROR: Error writing to socket");
  }

}

int main(int argc, char* argv[])
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer[256]; // chars to be written into from the socket connection
  if(argc != 3) // we require ip address(hostname), port number in this order
  {
    std::cerr << "Usage: ip_address port" << argv[0] << '\n';
    exit(0);
  }

  sockfd = socket_setup(argv);
  send_message(sockfd);

  close(sockfd);
  return 0;
}
