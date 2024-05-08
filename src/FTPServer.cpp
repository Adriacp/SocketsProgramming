//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                        Main class of the FTP server
// 
//****************************************************************************
#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 #include <unistd.h>


#include <pthread.h>

#include <list>

#include "common.h"
#include "FTPServer.h"
#include "ClientConnection.h"


int define_socket_TCP(int port) {
   // Include the code for defining the socket.
  struct sockaddr_in sin; //Storage for the address and port
  int s; //Socket descriptor
  s = socket(AF_INET, SOCK_STREAM, 0); //We inicialize the socket descriptor with socket() function
  
  if (s < 0) { //We make sure that is everything done
    errexit("Failed trying to creathe the socket:\n", strerror(errno)); //error message
  }
  memset(&sin, 0, sizeof(sin)); //local inicialization to 0
  sin.sin_family = AF_INET; //IPv4 addresses
  sin.sin_addr.s_addr = INADDR_ANY; //Assign the local adrres to local
  sin.sin_port = htons(port); //We use htons in order to change it to big-endian

  if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) { //associate the socket with the address and port specified in local
    errexit("Failed trying to bind with port:\n", strerror(errno)); //if bind() fails, print an error message
  }

  if(listen(s, 5) < 0) { //We put the socket in listening mode, with a maximum of 5 on the conection queue
    errexit("Failed trying to do the listening: \n", strerror(errno)); //error message
  }
  return s; //return socket descriptor
}





// This function is executed when the thread is executed.
void* run_client_connection(void *c) {
    ClientConnection *connection = (ClientConnection *)c;
    connection->WaitForRequests();
  
    return NULL;
}



FTPServer::FTPServer(int port) {
    this->port = port;
  
}


// Parada del servidor.
void FTPServer::stop() {
    close(msock);
    shutdown(msock, SHUT_RDWR);

}


// Starting of the server
void FTPServer::run() {

    struct sockaddr_in fsin;
    int ssock;
    socklen_t alen = sizeof(fsin);
    msock = define_socket_TCP(port);  // This function must be implemented by you.
    while (1) {
	pthread_t thread;
        ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
        if(ssock < 0)
            errexit("Fallo en el accept: %s\n", strerror(errno));
	
	ClientConnection *connection = new ClientConnection(ssock);
	
	// Here a thread is created in order to process multiple
	// requests simultaneously
	pthread_create(&thread, NULL, run_client_connection, (void*)connection);
       
    }

}
