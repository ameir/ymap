/*
 * yahooproxy.c
 * Version 0.1 - 1 March 2010
 *
 * Use of this program is bound by the GPLv2
 * Written by Ameir Abdeldayem
 * http://www.ameir.net
 *
 * Synopsis:
 * This program makes use of the "id" trick to enable IMAP access to Yahoo! Mail.
 * It sends the command "ID ("GUID" "1")" before logging in.  This procedure
 * is documented here:
 * http://en.wikipedia.org/wiki/Yahoo!_Mail#Free_IMAP_and_SMTPs_access
 *
 * Usage:
 * Compile this C program using your standard compiler; I have only tested it
 * with GCC on Linux (gcc yahooproxy.c -o yahooproxy).  Make the program executable
 * (chmod +x yahooproxy) and run it (without command-line arguments).  The server
 * will now listen for incoming  * connections on port 3490.  The port and other
 * information can be changed in the program if desired.
 *
 * Known Issues/TODO:
 * - Sometimes messages don't load entirely due to the way I check for a complete
 * message from the server ("OK" and "completed")
 * - Some clients might send commands in uppercase; this program only checks for
 * a lowercase "login"
 * - Investigate SSL possibility
 * - Look into performance improvements
 * - Clean up code
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <resolv.h>



#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

// send messages to remote client

char *clientmsg(int handle, char *string) {

    if (send(handle, string, strlen(string), 0) == -1)
        perror("send");

    char *temp = "string";
    return temp;

}

void error(char *msg) {
    perror(msg);
    exit(1);
}
void panic(char *msg);
#define panic(m)	{perror(m); abort();}

int main(void) {
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof (int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *) & their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *) & their_addr),
                s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener

            clientmsg(new_fd, "Welcome to Yahoo! IMAP Proxy!\r\n");

            //  char buffer[256];
            int bufsize = 1024; /* a 1K buffer */
            char *buffer = malloc(bufsize);
            char *reply = malloc(bufsize);


            /*****************************************************/

            /* Initiate connection */
            struct hostent* host;
            struct sockaddr_in addr;
            int sd;

            char server[] = "imap.mail.yahoo.com";
            int port = 143;



            /*---Get server's IP and standard service connection--*/

            if ((host = gethostbyname(server)) == 0) {
                perror("gethostbyname");
                exit(1);
            }
            /*printf("Server %s has IP address = %s\n", server,
                    inet_ntoa(*(long*) host->h_addr_list[0]));
             */
            port = htons(port);

            /*---Create socket and connect to server---*/
            sd = socket(PF_INET, SOCK_STREAM, 0); /* create socket */
            if (sd < 0)
                panic("socket");
            memset(&addr, 0, sizeof (addr)); /* create & zero struct */
            addr.sin_family = AF_INET; /* select internet protocol */
            addr.sin_port = port; /* set the port # */
            addr.sin_addr.s_addr = *(long*) (host->h_addr_list[0]); /* set the addr */

            /*---If connection successful, send the message and read results---*/
            int connection = connect(sd, (struct sockaddr*) & addr, sizeof (addr));
            if (connection == 0) {
                printf("Connected to %s\n", server);
            }
            /*****************************************************/


            /*
             * buffer = commands from client
             * reply = response from server
             */

            int n, i;
            char *string;
            i = 0;
            do {
                //  bzero(buffer, strlen(buffer));
                memset(buffer, 0, strlen(buffer)); /* create & zero struct */
                memset(reply, 0, strlen(reply)); /* create & zero struct */


                int k = 0;
                if (connection == 0) {
                    i = 1;
                    int repsize;
                    do {
                        memset(reply, 0, strlen(reply)); /* create & zero struct */
                        printf("[Chunk %d]: ", i);
                        // read from server
                        repsize = read(sd, reply, bufsize);
                        printf("message length = %d %d\n", repsize, strlen(reply));

                        if (repsize == 0) {
                            k++;
                            printf("Detected %d blank message(s)\n", k);
                            // if we have more than 5 empty messages, kill the inf loop
                            if (k > 5) {
                                printf("server: closing connection with %s\n", s);
                                close(sd);
                                close(new_fd);
                                exit(0);
                            }

                        }

                        printf("[%s]:  %s\n", server, reply);

                        // write to client
                        n = write(new_fd, reply, strlen(reply));
                        if (n < 0) {
                            error("ERROR writing to socket");
                            printf("ERROR writing to socket");
                        }

                        i++;
                    } while (strstr(reply, " OK ") == NULL && strstr(reply, "completed") == NULL || strlen(reply) == bufsize);
                    printf("out of loop \n");


                    // receive input from client
                    n = read(new_fd, buffer, bufsize);
                    //  n = read(new_fd, buffer, 255);
                    if (n < 0)
                        error("ERROR reading from socket");

                    printf("[%s]:  %s\n", s, buffer);

                    if (strstr(buffer, "login") != NULL) {

                        // write to server
                        n = write(sd, "2 id (\"GUID\" \"1\")\r\n", strlen("2 id (\"GUID\" \"1\")\r\n"));
                        if (n < 0) {
                            error("ERROR writing to socket");
                            printf("ERROR writing to socket");
                        }

                        repsize = read(sd, reply, bufsize);
                        printf("value of n = %d %d\n", repsize, strlen(reply));

                        printf("[%s]:  %s\n", server, reply);

                        printf("%s", reply);
                    }

                    if (n == -1) {
                        perror("recv");
                        exit(1);
                    }

                    /*
                                        if (read(sd, reply, bufsize) == -1) {
                                            perror("recv");
                                            exit(1);
                                        }
                     */


                    /*
// copy first 15 chars of command string to string
string = strncpy(string, buffer, 15);

// convert commands to uppercase
// http://www.roseindia.net/c-tutorials/c-string-uppercase.shtml
for (i = 0; string[i]; i++)
    string[i] = toupper(string[i]);


if (strstr(string, "LOGIN") != NULL) {
    printf("Issued LOGIN command %s\n", buffer);
    //     reply = "GUID\r\n";
    //     write(new_fd, reply, strlen(reply));
}

if (strcmp("EXIT\r\n", buffer) == 0) {
    printf("Client %s typed exit\n", s);
}
                     */


                    // char command[] = "1 capability\r\n";
                    /* send a message to the server PORT on machine HOST */
                    if (write(sd, buffer, strlen(buffer)) == -1) {
                        perror("send");
                        exit(1);
                    }



                    /* spew-out the results and bail out of here! */
                    //    printf("%s\n", reply);

                    //fclose(fp);                           /* close the socket */
                } else
                    panic("connect");
                /***********/



            } while (strcmp(buffer, "EXIT\r\n") != 0);


            //  sendcmd(new_fd, "1 CAPABILITY\r\n");


            /*
            char string1[] = "Welcome to Yahoo! IMAP Proxy!\n";
                                    if (send(new_fd, string1, sizeof(string1), 0) == -1)
                                            perror("send");
             */

            printf("server: closing connection with %s\n", s);
            close(new_fd);


            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }

    return 0;
}
