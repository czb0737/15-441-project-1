/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h> 
//#include <sys/time.h> 

#define ECHO_PORT 9999
#define BUF_SIZE 4096
#define MAX_CLIENT 2048
#define MAX_MSG 2048

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    /*
     * Initialing
     */
    fd_set fds, current_fd; //Storage all fds
    int state = 0;  //Tell the state of fds in Select
    struct timeval timeout = {3, 0};
    //timeout.tv_sec = 3;
    //timeout.tv_usec = 0;
    int sock, client_sock;
    int client_set[MAX_CLIENT];
    for (int i = 0; i < MAX_CLIENT; ++i)
    {
        client_set[i] = -1;
    }
    char* msg[MAX_MSG];
    int cli_msg[MAX_MSG];
    int msg_len[MAX_MSG];
    for (int i = 0; i < MAX_MSG; ++i)
    {
        msg[i] = NULL;
        cli_msg[i] = -1;
        msg_len[i] = -1;
    }
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char* buf;

    fprintf(stdout, "----- Echo Server -----\n");
    
    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    //unsigned long flag;
    //ioctlsocket(sock,FIONBIO, (u_long FAR*) &flag);

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    //Add server socket socket to fds
    FD_ZERO(&fds);
    FD_ZERO(&current_fd);
    FD_SET(sock, &fds);
    int max_fd = sock;

    /* finally, loop waiting for input and then write it back */
    while (1)
    {
        //Infinitely listening if there is any fd is active
        current_fd = fds;
        state = select(max_fd + 1, &current_fd, NULL, NULL, &timeout);
        //If select goes wrong, then exit
        if (state < 0)
        {
            perror("Select wrong: ");
            exit(1);
        }
        if(state == 0) continue;
        //printf("!1\n");
        //Search all the fds in fds
        if (FD_ISSET(sock, &current_fd))
        {
            printf("!3\n");
            
            //If there is client connect in, try to accept it
            cli_size = sizeof(cli_addr);
            if ((client_sock = accept(sock, (struct sockaddr *) &cli_addr,
                                     &cli_size)) == -1)
            {
               close(sock);
               fprintf(stderr, "Error accepting connection.\n");
               return EXIT_FAILURE;
            }
            for (int i = 0; i < MAX_CLIENT; ++i)
            {
                if (client_set[i] == -1)
                {
                    client_set[i] = client_sock;
                    max_fd = client_sock > max_fd ? client_sock : max_fd;
                    break;
                }
            }
            //If accept successfully, add the client fd in fds
            FD_SET(client_sock, &fds);
        }
        for(int i = 0; i < MAX_CLIENT; ++i)
        {
            if((client_sock = client_set[i]) < 0) continue;
            //printf("i: %d\n", i);
            if(FD_ISSET(client_sock, &current_fd))
            {
                printf("!5\n");
                readret = 0;
                buf = malloc(BUF_SIZE * sizeof(char));
                while((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1)
                {
                   printf("%d\n", (int)readret);
                   if(readret == 0)    FD_CLR(client_sock, &fds);
                   for (int i = 0; i < MAX_MSG; ++i)
                   {
                       if (msg[i] == NULL)
                       {
                           msg[i] = buf;
                           cli_msg[i] = client_sock;
                           msg_len[i] = readret;
                           buf = malloc(BUF_SIZE * sizeof(char));
                           break;
                       }
                   }
                   printf("!6\n");
                   //memset(buf, 0, BUF_SIZE);
                   //FD_CLR(client_sock, &fds);
                   client_set[i] = -1;
                   if(readret != BUF_SIZE)  break;
                } 

                if (readret == -1)
                {
                   close_socket(client_sock);
                   close_socket(sock);
                   fprintf(stderr, "Error reading from client socket.\n");
                   return EXIT_FAILURE;
                }

                //if (close_socket(client_sock))
                //{
                //   close_socket(sock);
                //   fprintf(stderr, "Error closing client socket.\n");
                //   return EXIT_FAILURE;
                //}
            }
        }
        //printf("!7\n");
        for (int i = 0; i < MAX_MSG; ++i)
        {
            if(msg[i] == NULL)  continue;
            printf("!8\n");
            if (send(cli_msg[i], msg[i], msg_len[i], 0) != msg_len[i])
            {
               close_socket(client_sock);
               close_socket(sock);
               fprintf(stderr, "Error sending to client.\n");
               return EXIT_FAILURE;
            }
            cli_msg[i] = -1;
            free(msg[i]);
            msg[i] = NULL;
            msg_len[i] = -1;
        }
        
    }

    close_socket(sock);

    return EXIT_SUCCESS;
}
