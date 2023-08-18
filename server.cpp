// == SERVER SIDE ==
#include <cstdio>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <complex.h>
#include <pthread.h>
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

#define MAX_CONNECTIONS 10

int bytes_read, bytes_written;
static int client_count = 0;
std::map<std::string,int&> client_fd{};
int status[MAX_CONNECTIONS];
std::map<int,int> client_fd_map {}; // {[sockfd]->[index of array]}


void send_and_recieve(int sockfd, std::string& usr);
std::string client_username_recv(int sockfd);

void clear_extraneous()
{
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
}


void error(const char* message)
{
  // used when a system call fails
  std::perror(message);
  exit(1);
}


int socket_setup(char* args[])
{
  int sockfd, portno;
  struct sockaddr_in serv_addr;
  ++ client_count;

  // create and open socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    error("ERROR: Error opening socket");
  
  // set fields in serv_addr
  bzero((char*) &serv_addr, sizeof(serv_addr)); // initialize serv_addr buffer
  portno = std::stoi(args[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY; // ip address of server
  serv_addr.sin_port = htons(portno); // port number (in network byte order)
  
  if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR: Error on binding socket to local address"); // bind socket to local address

  listen(sockfd, 5); // listen on socket for connections
 
  return sockfd;
}


void* server_query(void* arg)
{
  std::string query{};
  while(1)
  {
    std::getline(std::cin,query);
    // list clients connected to the server
    if(query =="get clients")
    {
      if(client_fd.empty())
        std::cout << "No clients connected \n";
      else
      {
        std::cout << ">> Connected clients: \n";
        for(auto it = client_fd.begin(); it!=client_fd.end(); ++it)
        {
          std::cout << "client: " << it->first << " ";
          if(status[it->second] == -1)
            std::cout << "FREE \n";
          else std::cout << "BUSY with client_" << status[it->second] << '\n'; 
        }        
      }  
    }
    else if(query == "get free clients")
    {
      std::vector<int> free_clients{};
      for(auto it{client_fd_map.begin()}; it != client_fd_map.end(); ++it)
      {
        if(status[it->second] == -1)
          free_clients.push_back(it->first);
      }
      if(free_clients.empty())
        std::cout << ">> no free clients \n";
      else
      {
        for(auto x : free_clients)
          std::cout << "client_" << x << '\n';
      }
    }
    else if(query == "terminate")
    {
      bool quit = false;
      while(!quit)
      {
        char option{};
        std::cout << "Are you sure you want to terminate the program? (y/n): ";
        std::cin >> option;
        if(option == 'y' || option == 'Y')
          exit(0);
        else if(option == 'n' || option == 'N')
          quit = true;
        else
          std::cout << "Error: Invalid option \n";
      }
    }
    else if(query == "kick ")
    {
      std::string temp_user{};
      std::getline(std::cin,temp_user);

      int n;
      for(auto it = client_fd.begin(); it != client_fd.end(); ++it)
      {
        if(temp_user==it->first)
        {
          // testing if we can find the user...
          std::cout << ">> kicking:" << it->first << " ... \n";
          std::string kick_msg{"you are being kicked."};
          int temp_sock = it->second;
          n = send(temp_sock, (char*)&kick_msg, sizeof(kick_msg), 0);
          if(n < 0)
            error("ERROR: Error writing to socket.");
        }
      }
      continue;
    }
    else
    {
      std::cout << "Invalid command. The following are the only valid commands:"
        << "\n>> get clients" << "\n>> terminate" << "\n>> kick <client_name>" << "\n";
    }
  }
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
  
  sockfd = socket_setup(argv);
  clilen = sizeof(cli_addr); // allocate memory for new address to connect with client
  
  pthread_t th_server_query;  
  if( (pthread_create(&th_server_query,NULL,server_query,NULL)) !=0 )
  {
    error("ERROR: Error creating thread for server query.");
    exit(1);
  }

  // calculate time
  std::time_t now = std::time(0);
  char* dt = std::ctime(&now);

  while(1)
  {
    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
    if(newsockfd < 0)
      error("ERROR: Error accepting request from client");
    std::cout << "Client successfully connected \n";
    std::string username = client_username_recv(newsockfd); // get username
    client_fd.insert(std::pair<std::string,int&>(username,sockfd));
  
    for(auto it = client_fd.cbegin(); it != client_fd.cend(); ++it)
    {
      std::cout << ">> client name:<" << it->first 
        << ">|fd number:<" << it->second << ">\n";
    }
    
    pid = fork();
    if(pid < 0)
      error("ERROR: Error on fork");
    if(pid == 0)
    {
      send_and_recieve(newsockfd,username);
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

std::string client_username_recv(int sockfd)
{
  std::string username{};
  int n{};
  char buffer[256];
  bzero(buffer,256);

  n = recv(sockfd, (char*)&buffer, sizeof(buffer), 0);
  if(n < 0)
    error("ERROR: Error reading from socket");
  std::cout << "*Username recieved of new user: " << buffer << '\n';
  username = buffer;

  return username;
}


// recieve and send automatic response from server
void send_and_recieve(int sockfd, std::string& usr)
{ 
  char buffer[256];
  while(1)
  {
    // after client successfully connects to the server...
    bzero(buffer,256); // we initialize buffer
    // read from the new file descriptor
    bytes_read += recv(sockfd, (char*)&buffer, sizeof(buffer), 0);
    if(!strcmp(buffer,"exit") || !strcmp(buffer,"Exit"))
    {
      std::cout << "Client: " << usr << " has left the chat. \n";
      break;
    }
    if(bytes_read < 0)
      error("ERROR: Error reading from socket");
    std::cout << "User <" << usr <<"> | Message: " << buffer << '\n';

    bzero(buffer,256);
    bytes_written = send(sockfd, (char*)&buffer , sizeof(buffer), 0);
    if(bytes_written < 0)
      error("ERROR: Error writing to socket");
    // server_query(client_fd);
  }
}
