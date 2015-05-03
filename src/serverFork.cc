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
 * Get the content type of file based on file extension 
 * NOTE: only works for HTML as of now
 ****************/
void get_content_type(string file_name, string& output)
{
    string file_extension = file_name.substr(file_name.find_last_of(".") + 1);
    if (file_extension == "html") {
        output = "text/html"; 
    } else if (file_extension == "jpeg" || file_extension == "jpg") {
        output = "image/jpeg";   
    } else {
        output = "None";
    }
}



/***************
 * Load text file of given name and return the characters 
 * NOTE: only works for HTML as of now
 ****************/
void load_text_file(string file_name, char*& output)
{
    string line;
    stringstream ss;
    ifstream cur_file(file_name);

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
    string res = ss.str();
    char* writable = new char[res.size() + 1];
    copy(res.begin(), res.end(), writable);
    writable[res.size()] = '\0';
    output = writable;
}

/***************
 * Load file of given name and return the characters 
 * NOTE: only works for HTML as of now
 ****************/
void load_binary_file(string file_name, char*& output)
{
    ifstream cur_file(file_name, ios::in | ios::binary | ios::ate);

    ifstream::pos_type filesize;
    char* file_contents;

    // Load the lines of the file    
    if (cur_file.is_open()) {
        filesize = cur_file.tellg();
        file_contents = new char[filesize];
        cur_file.seekg(0, ios::beg);
        if (!cur_file.read(file_contents, filesize)) {
            cout << "Failed to read file" << endl;
        }
        cur_file.close();
    } else {
        error("Error opening file");
    }
    cout << "size: " << filesize << endl;
    output = file_contents;
}


/***************
 * Load file of given name and return the data 
 * Calls either load_text_file() or load_binary_file()
 ****************/
void load_file(string file_name, char*& output)
{
    string content_type;
    int filesize;
    get_content_type(file_name, content_type);
    if (content_type == "text/html") {
        load_text_file(file_name, output);
    } else if (content_type == "image/jpeg") {
        load_binary_file(file_name, output);
    } else {
        cout << "No file";
    }
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

void assemble_http_header(string& header)
{
    // HTTP Version - Response Message
    // close / keep-alive
    // date (now)
    // Server type
    // Last-Modified
    // Content-Length
    // Content-Type
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
    string content_type;
    char* data;
    get_content_type(file_name, content_type);
    load_file(file_name, data);
    cout << "Content-Type: " << content_type << endl;
    cout << "Data: " << data << endl;

    // TODO: load the requested file data and create HTTP headers
    // Step 4: Write response to socket
    n = write(sock,"I got your message",18);
    if (n < 0) error("ERROR writing to socket");
}
