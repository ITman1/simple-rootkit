/**
  Library name: Server Library
  ----------------------------------------------------------------------
  Author:   Radim Loskot
  Date:     3. 3. 2011
  Filename: server_lib.h
  Descr:    Provides basic functions to create socket, connection 
            and sending/recieving messages from server
 */

/**
 * Modified: 17. 10. 2012
 * Filename: server_lib.h
 * Descr:    Fork and child processes replaced for threads.
 *
 * @todo use non blocking sockets or run server in separate thread
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include <iostream>
#include <utility>

#include "server_lib.h"

#define QUEUE (2)

using namespace servers;

/** 
 * An enunumeration of all handled errors which can occur.
 */
enum server_errors {
	ESRV_SIGHLD     = 0,	/**< enum error on handling SIGHLD */  
	ESRV_SOCK       = 1,	/**< enum error on creating socket*/
	ESRV_BIND       = 2,	/**< enum attaching port error */
	ESRV_LISTEN     = 3,	/**< enum error on creating listening */
	ESRV_FORK       = 4,	/**< enum unable to create process */
	ESRV_ACCEPT     = 5,	/**< enum error on accepting connection */
	ESRV_SOCKWRITE  = 6,	/**< enum socket write error */
	ESRV_SOCKREAD   = 7,	/**< enum socket read error */
	ESRV_NOSERVICE  = 8		/**< enum undefined service function */
};

/** 
 * Error messages.
 */
const string Server::errors[] = {
	"Server error: Cannot set signal (SIGHLD) blocking",
	"Server error: Unable to create listening socket", 
	"Server error: Unable to open passive mode (bind)", 
	"Server error: Unable to open passive mode (listen)",
	"Server error: Unable to create proccess for servicing of client",
	"Server error: Unable to create socket for communication with client",
	"Server error: Unable to send a message to remote socket", 
	"Server error: Error occured during reading from remote socket",
	"Server error: No service specified for server"
};

/** 
 * Size of reading buffer used during recieving message from client.
 */
const int BUFF_SIZE = 100;

/** 
 * Server constructor.
 */
Server::Server() {
	running = false;
	/* FIXME: Uncomment this, commented only due to problems with system()
    sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		printError(ESRV_SIGHLD);       */
	
	// IPv4 only
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
}

/** 
 * Stops listening. Ugly, fair enough, next time use non blocking sockets.
 */
void Server::stopListening() {
	if (running) {
		running = false;         
		close(listenfd);
	}	
}

void *Server::serviceThreadHandler(void *_obj) {
    pair <Server*, int> *obj = reinterpret_cast<pair <Server*, int>  *>(_obj);
    Server* server = obj->first;
    int connectfd = obj->second;

    int (*handler)(int fd) =  server->serviceHandle;
    if (handler != NULL) {
        server->serviceHandle(connectfd); // run specific user function
    }

    close(connectfd);
    delete obj;

    pthread_exit(NULL);
}

/** 
 * Size of reading buffer used during recieving message from client.
 * @param address interface where to listen
 * @param port listening port
 */
int Server::startListening(int address, int port) {
	if (serviceHandle == NULL) 
		return printError(ESRV_NOSERVICE);
		
	server.sin_addr.s_addr = htonl(address);
	server.sin_port = htons(port);
	
	// creates socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
		return printError(ESRV_SOCK);
	
	// Re-use address after proper ending of server	
	int value = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int));

	// attaches socket to specific port
	if (bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1) 
		return printError(ESRV_BIND);

	// passive open
	if (listen(listenfd, QUEUE) == -1) 
		return printError(ESRV_LISTEN);
	running = true;
	while (running) {
		if ((connectfd = accept(listenfd, NULL, NULL)) == -1) {
			if (!running) break; 
			return printError(ESRV_ACCEPT);
		}

        pair <Server*, int>  *threadArgs = new pair <Server*, int>(this, connectfd);
        pthread_t serviceThread;
        if (pthread_create(&serviceThread, NULL, &serviceThreadHandler, reinterpret_cast<void *>(threadArgs)) != 0) {
            delete threadArgs;
            if (!running) break;
            return printError(ESRV_FORK);
        }
	}

    stopListening();
	return SUCCESS;
}

/** 
 * Send message to server.
 * @param sock_fd socket descriptor where to send message.
 * @param msg message which will be sent to server.
 * @return SUCCESS/FAILURE code.
 */
int Server::sendMsg(int sock_fd, const string &msg) {
	if (write(sock_fd, msg.c_str(), msg.length()) < 0 ) {
		printError(ESRV_SOCKWRITE);   
		return FAILURE;
	} else {
		return SUCCESS;
	}
}

/** 
 * Reads message from server.
 * @param sock_fd socket descriptor where to read message
 * @param msg stored recieved message.
 * @param delimiter definition of string delimiter where client stops reading message from server. 
 * @return SUCCESS/FAILURE code.
 */
int Server::recvMsg(int sock_fd, std::string & msg, const std::string delimiter) {
	char buffer[BUFF_SIZE];      

	size_t pos;
	int n;
	msg.clear();
	
	while ((n = read(sock_fd, buffer, BUFF_SIZE - 1)) > 0) {        
		buffer[n] = '\0'; // Ensures end of recieved part - protecs against buffer garbage
		msg += buffer;    // Store recently recieved part
		
		// Check whether wasnt delimiter in recieved message
		if ((pos = msg.find(delimiter)) != string::npos) {
			// Remove all behind delimiter - instead
			msg = msg.substr(0, pos);
			break; // Stop reading
		}
	}

	// No message recieved or connection was closed due to some error
	if (msg.empty() || n == -1) {
		printError(ESRV_SOCKREAD);
		return FAILURE;
	}
	
	return SUCCESS;
}

/** 
 * Print error.
 * @param err_code of message to be printed
 * @param note additional note which will be printed to error if is specified
 * @param alerts source array with error messages
 */
unsigned int Server::printError(unsigned int err_code, string note, const string alerts[]) {
	if (enable_errors) {
		cerr << alerts[err_code] << note << endl;
	}
	return err_code;
}
