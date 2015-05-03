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

// Includes for getting data
#include <time.h>
#include <sys/stat.h>
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
void load_text_file(string file_name, char*& output, string& result)
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
        result = "200 OK";
    } else {
        error("Error opening file");
        result = "404 Not Found";
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
void load_binary_file(string file_name, char*& output, string& result)
{
    ifstream cur_file(file_name, ios::in | ios::binary | ios::ate);

    ifstream::pos_type filesize;
    char* file_contents;

    // Load the lines of the file    
    if (cur_file.is_open()) {
        filesize = cur_file.tellg();
        file_contents = new char[filesize];
        cur_file.seekg(0, ios::beg);
        result = "200 OK";
        if (!cur_file.read(file_contents, filesize)) {
            cout << "Failed to read file" << endl;
            result = "404 Not Found";
        }
        cur_file.close();
    } else {
        error("Error opening file");
        result = "404 Not Found";
    }
    cout << "size: " << filesize << endl;
    output = file_contents;
}


/***************
 * Load file of given name and return the data 
 * Calls either load_text_file() or load_binary_file()
 ****************/
void load_file(string file_name, char*& output, string& result, string& last_modified,
               string& content_length, string& content_type)
{
    int filesize;
    get_content_type(file_name, content_type);
    // Download the data based on the content type
    if (content_type == "text/html") {
        load_text_file(file_name, output, result);
    } else if (content_type == "image/jpeg") {
        load_binary_file(file_name, output, result);
    } else {
        cout << "No file found" << endl;
        result = "404 Not Found";
    }


    // Load the last modified date for this data
    struct stat attr;
    stat(file_name.c_str(), &attr);
    
    last_modified = string(ctime(&attr.st_mtime));
    // Strip trailing newline character
    if (last_modified.size () > 0) last_modified.resize (last_modified.size() - 1);

    // Get the size of the file
    ifstream file(file_name, ios::ate | ios::binary);
    content_length = to_string(file.tellg());
    file.close();
}

/***************
 * Retrieve requested file from HTTP GET 
 * NOTE: only works for HTML as of now
 ****************/
void return_file_path(char * input, string& output){

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


void assemble_http_header(char*& header, string request_res, string last_modified, string connect_status,
                          string content_length, string content_type)
{
    // HTTP Version - Response Message
    // close / keep-alive
    // date (now)
    // Server type
    // Last-Modified
    // Content-Length
    // Content-Type

    // Get the current date
    time_t timev = time(NULL);
    string date = string(ctime(&timev));
    if (date.size () > 0) date.resize (date.size() - 1);
    string server = "Placeholder";

    stringstream ss;
    // Add the HTTP version and response message
    // TODO: change response message based on actual outcome
    ss << "HTTP/1.1 " << request_res << "\r\n";
    ss << "Connection: " << connect_status << "\r\n";
    ss << "Date: " << date << "\r\n";
    ss << "Server: " << server << "\r\n";
    ss << "Last-Modified: " << last_modified << "\r\n";
    ss << "Content-Length: " << content_length << "\r\n";
    ss << "Content-Type: " << content_type << "\r\n";
    ss << "\r\n";

    string res = ss.str();
    char* writable = new char[res.size() + 1];
    copy(res.begin(), res.end(), writable);
    writable[res.size()] = '\0';
    header = writable;
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
    printf("Here is the message: \n%s\n",buffer);
    string file_name;
    return_file_path(buffer, file_name);
    cout << "File name: " << file_name << endl;


    // TODO: load the requested file data and create HTTP headers
    string result, last_modified, connect_status, content_length, content_type;

    // Load the file data
    char* data;
    load_file(file_name, data, result, last_modified, content_length, content_type);
   
    // Set connection status based on result 
    connect_status = "keep-alive";

    char* header;
    assemble_http_header(header, result, last_modified, connect_status, content_length, content_type);
    cout << "Header: \n" << header << endl;

    // Step 4: Write response to socket
    n = write(sock,header,18);

    if (n < 0) error("ERROR writing to socket");
}
