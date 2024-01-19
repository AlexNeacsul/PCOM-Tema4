#include <stdlib.h>    
#include <stdio.h>
#include <unistd.h>    
#include <string.h>   
#include <sys/socket.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_del_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count, char *token, int type)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Scriu numele metodei, URL-ul, parametrii request
    if (query_params != NULL)
    {
        if (type == GET)
        {
           sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
        }
        else
        {
            sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
        }
    }
    else
    {
        if (type == GET) 
        {
            sprintf(line, "GET %s HTTP/1.1", url);
        }
        else
        {
            sprintf(line, "DELETE %s HTTP/1.1", url);
        }
    }

    compute_message(message, line);

    // Adaug host-ul
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Adaug headers si/sau cookies
    if (cookies != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "Cookie: %s", cookies[0]);
        compute_message(message, line);
    }

    // Adaug token-ul
    if (token != NULL)
    {
        memset(line, 0, LINELEN);
		sprintf(line, "Authorization: Bearer %s", token);
		compute_message(message, line);
	}

    // Adaug ultima linie
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Scriu numele metodei, URL-ul si tipul protocolului
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Adaug token-ul
    if (token != NULL)
    {
        memset(line, 0, strlen(line));
		sprintf(line, "Authorization: Bearer %s", token);
		compute_message(message, line);
	}
    
    // Adaug hostul
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    // Adaug headers necesare
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    if (body_data != NULL)
    {
		memset(body_data_buffer, 0, LINELEN);
    	strcat(body_data_buffer, "");
    	strcat(body_data_buffer, body_data[0]);
	}

    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    // Adaug cookies
    if (cookies != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "Cookie: %s", cookies[0]); // Nu se foloseste decat un cookie in tema
        compute_message(message, line);
    }

    // Adaug o linie noua la final de header
    compute_message(message, "");

    // Adaug informatia din payload
    memset(line, 0, LINELEN);
    strcat(message, body_data_buffer);

    free(line);
    return message;
}
