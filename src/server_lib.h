/**
  Library name: Server Library
  ----------------------------------------------------------------------
  Author:   Radim Loskot
  Date:     3. 3. 2011
  Filename: server_lib.h
  Descr:    Provides basic functions to create socket, connection 
            and sending/recieving messages from server
 */

#include <string>
#include <vector>
#include <netinet/in.h>
#include <signal.h>

using namespace std;

namespace servers {
	
	const int UNDEFINED = -1;	/**< Specifies undefined value for socket descriptor */
	const int FAILURE = -1;		/**< Specifies failure return code for functions */
	const int SUCCESS = 0;		/**< Specifies success return code for functions */

	class Server {
		
		private:
			/** 
			 * Error messages.
			 */
			static const string errors[];
		
		protected:
			pid_t pid;				/**< pid */
			struct sigaction sa;	/**< signal handle structure */
		
			/** 
			 * Print error.
			 * @param err_code of message to be printed
			 * @param note additional note which will be printed to error if is specified
			 * @param alerts source array with error messages
			 */
			unsigned int printError(unsigned int err_code, string note = "", const string alerts[] = errors);

            static void *serviceThreadHandler(void *_obj);
			
		public:
			int listenfd;                 /**< Listening descriptor */
			int connectfd;                /**< Client descriptor */
			struct sockaddr_in server;    /**< Server socket data structure */
			int (*serviceHandle)(int fd); /**< Handle function */
			bool enable_errors;           /**< Enable/Disable errors on stderr */
			bool running;                 /**< Indicates whether is server listening */
			
			/** 
			 * Server constructor.
			 */
			Server();
			/** 
			 * Size of reading buffer used during recieving message from client.
			 * @param address interface where to listen
			 * @param port listening port
			 */
			int startListening(int address, int port);

			/** 
			 * Stops listening. Ugly, fair enough, next time use non blocking sockets.
			 */
			void stopListening();

			/** 
			 * Send message to server.
			 * @param msg message which will be sent to server.
			 * @return SUCCESS/FAILURE code.
			 */
            int sendMsg(int sock_fd, const string & msg);
			/** 
			 * Reads message from server.
			 * @param msg stored recieved message.
			 * @param delimiter definition of string delimiter where client stops reading message from server. 
			 * @return SUCCESS/FAILURE code.
			 */
			int recvMsg(int sock_fd, string & msg, const string delimiter);
	};
	
}
