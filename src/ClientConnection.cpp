//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transaction.
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"

int Define_socket_TCP(int port)
{

  struct sockaddr_in sin;
  int s;

  s = socket(AF_INET, SOCK_STREAM, 0);

  if (s < 0)
  {
    errexit("Can't create socket: %s\n", strerror(errno));
  }

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);

  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    errexit("Can't make bind with port: %s\n", strerror(errno));
  }

  if (listen(s, 5) < 0)
  {
    errexit("Failed in listening: %s\n", strerror(errno));
  }

  return s;
}


ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }
    
    ok = true;
    data_socket = -1;
    parar = false;
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int connect_TCP( uint32_t address,  uint16_t  port) {
    
    struct sockaddr_in sin; //Storage for the address and port
    struct hostent *hent; //Storege for the host information
    struct in_addr addr;
    int s; //Socket descriptor

    memset(&sin, 0, sizeof(sin)); //Socket_sin inicialization to 0
    sin.sin_family = AF_INET; //IPv4 addresses
    sin.sin_port = htons(port); //We use htons in order to change it to big-endian

    addr.s_addr = htonl(address);
    hent = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);

    if (hent == NULL) {
        errexit("Can't obtain host information: %s\n", strerror(errno));
        return -1;
    } else {
        memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
    }

    s = socket(AF_INET, SOCK_STREAM, 0); //We inicialize the socket descriptor with socket() function
    
    if (s < 0) //We make sure that is everything done
        errexit("Failed trying to create the socket\n", strerror(errno)); //error message

    if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) //We establish the TCP conection
        errexit ("It is imposible to connect with: ", address, strerror(errno)); //error message

    return s; // You must return the socket descriptor.

}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {

      fscanf(fd, "%s", command);
      if (COMMAND("USER")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n");
      }
      else if (COMMAND("PWD")) {
	   
      }
      else if (COMMAND("PASS")) {
        fscanf(fd, "%s", arg);
        if(strcmp(arg,"1234") == 0){
            fprintf(fd, "230 User logged in\n");
        }
        else{
            fprintf(fd, "530 Not logged in.\n");
            parar = true;
        }
	   
      }
      else if (COMMAND("PORT")) {
	  // To be implemented by students
        int r1, r2, r3, r4, p1, p2; //in order to store ip address and port

        int num_read = fscanf(fd, "%d, %d, %d, %d, %d, %d", &r1, &r2, &r3, &r4, &p1, &p2); //read the integers of the ip address and the port, %d in order to specify the format
        if (num_read != 6) { //If fscanf failes, error messages
            fprintf (stderr, "Error reading PORT command\n");
            fprintf(fd, "500 Syntax error in parameters or arguments\n");
            fflush(fd);
            return;
        }
        printf("(%d, %d, %d, %d, %d, %d)", r1, r2, r3, r4, p1, p2); //Prints out the values of the six integers that were read from the client

        uint32_t address = r1 << 24 | r2 << 16 | r3 << 8 | r4; //Constructs the IP address and port number from the individual components.
        uint16_t port = (p1 << 8) | p2;

        data_socket = connect_TCP(address, port); //Call the function connect_TCP with the constructed address and port as arguments, to establish connection
    
        fprintf(fd, "200 Command okay\n"); //sends a response back to the client indicating that the command was successful
        fflush(fd); //flushes the buffer
      }
      else if (COMMAND("PASV")) {
	    // To be implemented by student
        int socket = Define_socket_TCP(0); //Creates a sockets in port 0

        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        int result = getsockname(socket, (struct sockaddr* )& addr, &len); //addr has the socket address

        uint16_t pp = addr.sin_port; //We obtain the port number
        int p1 = pp >> 8; //Get the higher number of the port
        int p2 = pp & 0xFF; // Gets the lower number of the port

        if (result < 0) { //In case there are any errors
            fprintf(fd, "421 Service not available, closing control connection. \n");
            fflush(fd);
            return;
        }

        fprintf(fd, "227 Entering Passive Mode (127.0.0.1,%d,%d)\n", p2, p1);

        len = sizeof(addr);
        fflush(fd);
        result = accept(socket, (struct sockaddr* )& addr, &len); //We accept the conection, stoping the program until there is conection.

        if (result < 0) { //In case there are any errors
            fprintf(fd, "421 Service not available, closing control connection.\n");
            fflush(fd);
            return;
        }
        data_socket = result; //We assign data_socket
      }
      else if (COMMAND("STOR") ) {
	    // To be implemented by students
        fscanf(fd, "%s", arg); //We read the file and save it in arg

        fprintf(fd, "150 File status okay, about to open data connection.\n");
        fflush(fd);
        FILE *file = fopen(arg, "wb+"); //We open the file in binary writting mode "wb+"

        if (file == NULL) { //We make sure thath the file is open
            fprintf (fd, "425 Can't open data connection.\n");
            fflush(fd);
            close(data_socket);
            break;
        }

        fprintf(fd, "125 Data connection already open, transfer starting.\n");
        
        if (file != NULL) {
          char *buffer[MAX_BUFF]; //Buffer with MAX_BUFF size
          while (1) {
            int bytes_read = recv(data_socket, buffer, MAX_BUFF, 0); //We recieve the data from the client from the socket
            fwrite(buffer, 1, bytes_read, file); //We write the data in the buffer
            if (bytes_read != MAX_BUFF) { //If we doesn'r recieve anything we can break the while loop, there's nothing to recieve.
                break;
            }
          }
          //We close everything
          fprintf(fd, "226 CLosing data connection.\n");
          fflush(fd);
          fclose(file);
          close(data_socket);
        }
      }
      else if (COMMAND("RETR")) {
	   // To be implemented by students
       fscanf(fd, "%s", arg); //We read the file and save it in arg

       FILE *file = fopen(arg, "rb"); //Wr open the file in read mode

       if (file != NULL) { //We make sure that we opened the file
        fprintf(fd, "125 Data connection already open, transfer starting.\n");
        fflush(fd);
        char *buffer[MAX_BUFF]; //Buffer with MAX_BUFF size

        while(1) {
            int bytes_read = fread(buffer, 1, MAX_BUFF, file); //we read the data and save it on the buffer

            send(data_socket, buffer, bytes_read, 0); //we send the buffer
            if (bytes_read != MAX_BUFF) { //We break in case we read the max amount
                break;
            }
        }
        fprintf(fd, "226 Closing data connection.\n"); //close connection
        fflush(fd);

        fclose(file);
        close(data_socket);
       } else {
        //close connection
        fprintf(fd, "425 Can't open data connection.\n");
        fflush(fd);
        close (data_socket);
        break;
       }
      }
      else if (COMMAND("LIST")) {
	   // To be implemented by students	
       int pipefd[2]; //We save the pipe files descriptor
       int res = pipe(pipefd); //We make a pipe and save the descriptor in pipefd

       pid_t pid = fork(); //We start a child proccess and save it pid

       char buffer[1024]; //We make a buffer with 1024 size

       if (pid == 0) { //Child process
          int result = dup2(pipefd[1], STDOUT_FILENO); //standard output of the child process to the writing end of the pipe
          int result2 = dup2(pipefd[1], STDERR_FILENO); //standard error output of the child process to the writing end of the pipe
          if (result < 0 || result2 < 0) { //Verify any errors
            return;
          }
          close(pipefd[1]); //We close the descriptors
          close(pipefd[0]); 

          char* const args[2] = {(char*)"ls", (char*)"-l"}; //we declare the command ls and -l to ve used in execvp

          execvp(args[0], args); //We replace the child process with ls -l

       }
       if (pid > 0) { //Father process
            read(pipefd[0], buffer, 1023); //we read the pipe data
            fprintf(fd, "%s", buffer); //we print it
            fflush(fd);
       }

       fprintf(fd, "250 Requested file action okay, completed\n");
       fflush(fd);
      }
      else if (COMMAND("SYST")) {
           fprintf(fd, "215 UNIX Type: L8.\n");   
      }

      else if (COMMAND("TYPE")) {
	  fscanf(fd, "%s", arg);
	  fprintf(fd, "200 OK\n");   
      }
     
      else if (COMMAND("QUIT")) {
        fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
        close(data_socket);	
        parar=true;
        break;
      }
  
      else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
	
      }
      
    }
    
    fclose(fd);

    
    return;
  
};
