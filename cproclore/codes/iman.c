#include "iman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

void fetch_man_page(const char *command)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *host;
    char request[BUFFER_SIZE], response[BUFFER_SIZE];
    char *host_name = "man.he.net";
    char *page_template = "/?topic=%s&section=all"; 
    char page[100];

   
    snprintf(page, sizeof(page), page_template, command);

   
    host = gethostbyname(host_name);
    if (host == NULL)
    {
        fprintf(stderr, "Error: Unknown host\n");
        return;
    }

   
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket");
        exit(1);
    }

    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    memcpy(&server_addr.sin_addr.s_addr, host->h_addr, host->h_length);

   
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error connecting");
        close(sockfd);
        exit(1);
    }

 
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             page, host_name);

   
    if (send(sockfd, request, strlen(request), 0) < 0)
    {
        perror("Error sending request");
        close(sockfd);
        exit(1);
    }

   
    int ang_count = 0;
    ssize_t received;
    while ((received = recv(sockfd, response, BUFFER_SIZE - 1, 0)) > 0)
    {
        response[received] = '\0'; 
        char *html_start;
        if (ang_count == 0)
        {
            html_start = strstr(response, "<");
            ang_count++;

            int k = 0;
            for (int i = 0; i < strlen(html_start); i++)
            {
                if (k == 0 && html_start[i] == '<')
                {
                    k = 1;
                }
                else if (k == 0)
                {
                    printf("%c", html_start[i]);
                }
                else if (k == 1 && html_start[i] == '>')
                {
                    k = 0;
                }
            }
        }
        else
        {
            int k = 0;
            for (int i = 0; i < strlen(response); i++)
            {
                if (k == 0 && response[i] == '<')
                {
                    k = 1;
                }
                else if (k == 0)
                {
                    printf("%c", response[i]);
                }
                else if (k == 1 && response[i] == '>')
                {
                    k = 0;
                }
            }
        }
    }

    if (received < 0)
    {
        perror("Error receiving response");
    }

   
    close(sockfd);
}

void iman(char **instr, int k)
{
    if (k <= 2)
    {
        printf("INVALID INPUT\n");
    }
    else
    {
        fetch_man_page(instr[2]);
    }
}
