// == SERVER SIDE ==
#include <cstdio>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <time.h>
#include <ctime>
#include <chrono>
#include <sys/types.h> // for data types in unix sys-calls 
#include <sys/socket.h> // for structs that define sockets
#include <netinet/in.h> // constants and structs for internet domain addresses
// sockaddr_in is a struct containing an internet address
/*
struct sockaddr_in
{
  short   sin_family; // must be AF_INET
  u_short sin_port;
  struct  in_addr sin_addr;
  char    sin_zero[8]; // Not used, must be zero 
};
*/
int bytes_read, bytes_written;

void send_and_recieve(int sockfd); 

void error(const char* message)
{
  // used when a system call fails
  std::perror(message);
  exit(1);
}

int main(int argc, char* argv[])
{
  // checks if the user entered the right number of arguments in cli
  if(argc != 2)
  {
    std::cerr << "ERROR: No port provided";
    exit(1);
  }
  int sockfd, newsockfd, portno, pid;
  socklen_t clilen; // size of the address of the client (needed to accept sys call)
  char buffer[256]; // server will read chars into this buffer from the socket connection
  struct sockaddr_in serv_addr, cli_addr; // structs containing internet address
  int n;

  // create and open socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  std::cout << "socket: " << sockfd << '\n';
  if(sockfd < 0)
    error("ERROR: Error opening socket");
  
  // set fields in serv_addr
  bzero((char*) &serv_addr, sizeof(serv_addr)); // initialize serv_addr buffer
  portno = std::stoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY; // ip address of server
  serv_addr.sin_port = htons(portno); // port number (in network byte order)
  
  if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR: Error on binding socket to local address"); // bind socket to local address

  listen(sockfd, 5); // listen on socket for connections
  clilen = sizeof(cli_addr); // allocate memory for new address to connect with client

  // calculate time
  std::time_t now = std::time(0);
  char* dt = std::ctime(&now);

  while(1)
  {
    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
    if(newsockfd < 0)
      error("ERROR: Error accepting request from client");
    std::cout << "Client successfully connected \n";

    pid = fork();
    if(pid < 0)
      error("ERROR: Error on fork");
    if(pid == 0)
    {
      close(sockfd);
      send_and_recieve(newsockfd);
      std::cout << "***** Session *****"
        << "\nBytes read: " << bytes_read << "B \n";
      std::cout << dt << '\n';
      exit(0);
    }
    else close(newsockfd);
  }
  close(sockfd);
  return 0;
}

// recieve and send automatic response from server
void send_and_recieve(int sockfd)
{
  char buffer[256];
  while(1)
  {
    // after client successfully connects to the server...
    bzero(buffer,256); // we initialize buffer
    //n = read(sockfd, buffer, 255); // read from the new file descriptor
    bytes_read += recv(sockfd, (char*)&buffer, sizeof(buffer), 0);
    if(!strcmp(buffer,"exit") || !strcmp(buffer,"Exit"))
    {
      std::cout << "Client has left the chat. \n";
    }
    if(bytes_read < 0)
      error("ERROR: Error reading from socket");
    std::cout << "Message: " << buffer << '\n';

    bzero(buffer,256);
    bytes_written = send(sockfd, (char*)&buffer , sizeof(buffer), 0);
    //n = write(sockfd, "Server: Got your message", 24);
    if(bytes_written < 0)
      error("ERROR: Error writing to socket");
  }
}
