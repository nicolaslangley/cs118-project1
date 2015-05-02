/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */

// Includes for loading files
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>
using namespace std;


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void dostuff(int); /* function prototype */

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    // Declare socket variables
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    struct sigaction sa;          // for signal SIGCHLD

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    // Step 1: Create Socket and set port number
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
   
    // Bind the socket to the server address  
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");

    // Step 2: Listen for connections on the socket
    listen(sockfd,5);

    clilen = sizeof(cli_addr);
    
    /****** Kill Zombie Processes ******/
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    /*********************************/

    // Step 2 Cont: Wait for connections to socket    
    while (1) {
        // Create file descriptor for new connection
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        
        if (newsockfd < 0) 
            error("ERROR on accept");
        
        pid = fork(); //create a new process
        if (pid < 0)
            error("ERROR on fork");
        
        if (pid == 0)  { // fork() returns a value of 0 to the child process
            close(sockfd);
            dostuff(newsockfd);
            exit(0);
        }
        else //returns the process ID of the child process to the parent
            close(newsockfd); // parent doesn't need this 
    } /* end of while */
    return 0; /* we never get here */
}

/***************
 * Load file of given name and return the characters 
 * NOTE: only works for HTML as of now
 ****************/
const char* load_file(const char* file_name)
{
    cout << "In Load_File: " << file_name << endl;
    string line;
    stringstream ss;
    ifstream cur_file(file_name);
    string data = "";

    // Load the lines of the file    
    if (cur_file.is_open()) {
        ss.clear();
        ss.str("");
        while (getline(cur_file, line)) {
            ss << line;
        }            
    } else {
        error("Error opening file");
    }
    data = ss.str();
    return data.c_str();
}


/***************
 * Get the content type of file based on file extension 
 * NOTE: only works for HTML as of now
 ****************/
const char* content_type(const char* file_name)
{
    string file_type = "";
    string fn(file_name);
    if (fn.substr(fn.find_last_of(".") + 1) == "html") {
        file_type = "html"; 
    } else {
        file_type = "None";
    }
    return file_type.c_str();
}

/***************
 * Retrieve requested file from HTTP GET 
 * NOTE: only works for HTML as of now
 ****************/
void returnFilePath(char * input, string& output){

    //find "GET " in String, read until next ' '
    char * locationOfGET = strstr(input,"GET");  //returns location of first char

    char ObjectPath[255];

    locationOfGET += 5;
    
    char * locationEndOfPath;
    locationEndOfPath = strchr(locationOfGET,' ');
    
    int index = (int)(locationEndOfPath - locationOfGET);
    
    printf("\n");

    stringstream ss;    
    string res = "";
    int i = 0;
    while (i < index) {
        ss << locationOfGET[i];
        i++;
    }
    output = ss.str();
    cout << output << endl; 

}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
    int n;
    char buffer[256];
       
    bzero(buffer,256);

    // Step 3: Read from socket into buffer
    // TODO: parse the requested file from the request
    n = read(sock,buffer,255);
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);
    string file_name;
    returnFilePath(buffer, file_name);
    cout << "File name: " << file_name << endl;

    cout << "Content-Type: " << content_type(file_name.c_str()) << endl;
    cout << "Data: " << load_file(file_name.c_str()) << endl;

    // TODO: load the requested file data and create HTTP headers

    // Step 4: Write response to socket
    n = write(sock,"I got your message",18);
    if (n < 0) error("ERROR writing to socket");
}
