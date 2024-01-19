#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h>     
#include <string.h>     
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>      
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>

#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define REGISTER "/api/v1/tema/auth/register"
#define LOGIN "/api/v1/tema/auth/login"
#define ACCESS "/api/v1/tema/library/access"
#define BOOKS "/api/v1/tema/library/books"
#define LOGOUT "/api/v1/tema/auth/logout"
#define JSON "application/json"

struct status
{
    int connection;
    int library;
};


char *recv_post_req(int socket, char *host, char *url, char *body, char *token)
{
    char *body_data[1];
    body_data[0] = body; 
	char *message = compute_post_request(host, url, JSON, body_data, 1, NULL, 0, token);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

char *recv_get_del_req(int socket, char *host, char *url, char *token, char *cookies[1], int get_del)
{
	char *message = compute_get_del_request(host, url, NULL, cookies, 1, token, get_del);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

int check_num(char *num)
{
    for (int i = 0; i <= strlen(num) - 1; i++)
	{
		if (!isdigit(num[i]))
		{
			return 0;
		}
	}

	return 1;
}

int main(int argc, char *argv[]) {
	struct status status;
    status.connection = status.library = 0;
	char host[] = "34.254.242.81";
	int port = 8080;
	int socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

	char *cookies[1], token[BUFLEN];
	char command[BUFLEN];
	while (1)
	{
		fgets(command, BUFLEN, stdin);

		// Deschid conexiunea la server pentru fiecare comanda
		socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);
		
		if(strncmp(command, "register", 8) == 0 || strncmp(command, "login", 5) == 0)
		{
			// Salvez numele si parola utilizatorului inregistrat 
			char username[BUFLEN];
            char password[BUFLEN];

			printf("username=");
            fgets(username, BUFLEN, stdin);

            int ok = 1;
			// Verific ca username-ul si parola sa nu fie nule sau sa contina spatiu
            if (username[0] == '\n' || strstr(username, " "))
			{
                ok = 0;
            }

            username[strlen(username) - 1] = '\0';

            printf("password=");
            fgets(password, BUFLEN, stdin);

            if (password[0] == '\n' || strstr(password, " "))
			{
                ok = 0;
            }

            password[strlen(password) - 1] = '\0';

            if (!ok)
			{
                fprintf(stderr, "FAILED! Invalid username/password!\n");
                continue;
            }

			JSON_Value *value = json_value_init_object();
			JSON_Object *object = json_value_get_object(value);
			json_object_set_string(object, "username", username);
			json_object_set_string(object, "password", password);
			char *user = json_serialize_to_string(value);

			if (strncmp(command, "register", 8) == 0)
			{ 
				// Register
                char *res = recv_post_req(socket, host, REGISTER, user, NULL);
				if (strstr(res, "is taken") != NULL) {
					fprintf(stderr, "FAILED! The username is already taken!\n");
				} else {
					printf("SUCCESS! You are now registred.\n");
				}

			}
			else
			{ 
				// Login
                char *res = recv_post_req(socket, host, LOGIN, user, NULL);
				char *cookie = strstr(res, "Set-Cookie: ");

				if (cookie == NULL)
				{
					// Verific parola si username-ul
					if (strstr(res, "not good"))
					{
			            fprintf(stderr, "FAILED! Wrong password!\n");
		            }
					else if (strstr(res, "No account"))
					{
			            fprintf(stderr, "FAILED! Username doesn't exist!\n");
		            }

					status.connection = 0;
					status.library = 0;
					continue;
				}
				
				// Retin cookie-ul prin care demonstrez ca sunt logat
				strtok(cookie, ";");
				cookie += 12;
				
				cookies[0] = cookie;

				if (cookie != NULL)
				{
					printf("SUCCESS! You are now logged in!\n");
				}

				status.connection = 1;
			}
		}
		else if (strncmp(command, "enter_library", 13) == 0)
		{
			if (status.connection == 1)
			{
				// Daca sunt logat si nu sunt in librarie, retin token-ul pentru verificarea accesului
				// daca nu afisez eroarea aferenta
                char *res = recv_get_del_req(socket, host, ACCESS, token, cookies, GET);
				char *tok = strstr(res, "token");

				// Verific ca token-ul sa nu fie nul
				if (tok == NULL)
				{
					fprintf(stderr, "FAILED! Access denied to the library!\n");
				}
				else
				{
					tok += 8;
					memset(token, 0, BUFLEN);
					strcpy(token, tok);
                    int new_len = (int)strlen(token) - 2;
					token[new_len] = '\0';

					status.library = 1;
					printf("SUCCESS!\n");
				}
			}
			else if (!status.connection) // Daca nu sunt logat intorc mesajul de eroare
			{
				fprintf(stderr, "FAILED! You are not logged in!\n");
			}
		}
		else if (strncmp(command, "get_books", 9) == 0)
		{	
			// Verific status.library si in functie de el afisez toate cartile din library sau mesajul de eroare aferent
			if (status.library == 1)
			{
				// Daca sunt in library, caut cartile retinute
				char *res = recv_get_del_req(socket, host, BOOKS, token, cookies, GET);
				char *books = strstr(res, "[");

                printf("Books are:\n\n");

                JSON_Value *val = json_parse_string(books);

                JSON_Array *arr = json_value_get_array(val);

                for (int i = 0; i < json_array_get_count(arr); ++i) {
                    JSON_Object *obj = json_array_get_object(arr, i);
                    printf("id=%.0lf\n", json_object_get_number(obj, "id"));
                    printf("title=%s\n", json_object_get_string(obj, "title"));
                    printf("-----------------------------\n");
                }

			}
			else
			{
				fprintf(stderr, "FAILED! You are not in the library.\n");
			}
		}
		else if (strncmp(command, "get_book", 8) == 0)
		{
			// Verific status.library si in functie de el merg mai departe la verificarea ID-ului sau afisez
			// mesajul de eroare aferent
			if (status.library == 1)
			{
				char route[BUFLEN], id[BUFLEN];

				printf("id=");
				fgets(id, BUFLEN, stdin);

				// Verific daca ID-ul introdus este corect daca nu afisez erorile aferente
                if (id[0] == '\n')
				{
                    fprintf(stderr, "FAILED! The ID is null!\n");
                    continue;
                }

                id[strlen(id) - 1] = '\0';	

                if (!check_num(id))
				{
                    fprintf(stderr, "FAILED! The format is wrong!\n");
                    continue;
                }

				sprintf(route, "%s/%d", BOOKS, atoi(id));

				// Caut prin cartile retinute
				char* res = recv_get_del_req(socket, host, route, token, cookies, GET);

				// Dupa ce caut ID-ul respectiv afisez cartea sau mesajul de eroare
				if (strstr(res, "No book") != NULL)
				{
					fprintf(stderr, "FAILED! The book was not found!\n");
				}
				else
				{
                    char *book = strstr(res, "{");
					JSON_Value *val = json_parse_string(book);
			        JSON_Object *obj = json_value_get_object(val);

			        printf("\nBook:\n");
			        printf("title=%s\n", json_object_get_string(obj, "title"));
			        printf("author=%s\n", json_object_get_string(obj, "author"));
			        printf("genre=%s\n", json_object_get_string(obj, "genre"));
			        printf("publisher=%s\n", json_object_get_string(obj, "publisher"));
			        printf("page_count=%.0lf\n", json_object_get_number(obj, "page_count"));
                }
			}
			else 
			{
				fprintf(stderr, "FAILED! You are not in the library.\n");
			}
		}
		else if (strncmp(command, "add_book", 8) == 0)
		{
			// Verific status.library si in functie de el merg mai departe la introducerea datelor sau afisez
			// mesajul de eroare aferent
			if (status.library == 1)
			{
				char title[BUFLEN], author[BUFLEN], genre[BUFLEN], publisher[BUFLEN], page_count[BUFLEN];
				int pages, ok = 1;

				printf("title=");
                fgets(title, BUFLEN, stdin);

                if (title[0] == '\n')
				{
                    ok = 0;
                }

                title[strlen(title) - 1] = '\0';

                printf("author=");
                fgets(author, BUFLEN, stdin);

                if (author[0] == '\n')
				{
                    ok = 0;
                }

                author[strlen(author) - 1] = '\0';

                printf("genre=");
                fgets(genre, BUFLEN, stdin);

                if (genre[0] == '\n')
				{
                    ok = 0;
                }

                genre[strlen(genre) - 1] = '\0';

                printf("publisher=");
                fgets(publisher, BUFLEN, stdin);

                if (publisher[0] == '\n')
				{
                    ok = 0;
                }

                publisher[strlen(publisher) - 1] = '\0';

                printf("page_count=");
                fgets(page_count, BUFLEN, stdin);

                if (page_count[0] == '\n')
				{
                    ok = 0;
                }

                page_count[strlen(page_count) - 1] = '\0';

                if (!check_num(page_count))
				{
                    ok = 0;
                }
				
				// Verific dupa fiecare camp completat daca acesta este null sau in cazul page_count
				// daca este null sau este un numar pozitiv si la final in functie de flag-ul "ok" adaug
				// cartea sau afisez eroarea aferenta
                if (!ok)
				{
                    fprintf(stderr, "FAILED! The format is wrong!\n");
                    continue;
                }

				pages = atoi(page_count);

				// Construiesc payload si trimit request
				JSON_Value *value = json_value_init_object();
				JSON_Object *object = json_value_get_object(value);
				json_object_set_string(object, "title", title);
				json_object_set_string(object, "author", author);
				json_object_set_string(object, "genre", genre);
				json_object_set_number(object, "page_count", pages);
				json_object_set_string(object, "publisher", publisher);
				char *book = json_serialize_to_string(value);

				recv_post_req(socket, host, BOOKS, book, token);
				printf("SUCCESS!\n");
			}
			else
			{
				fprintf(stderr, "FAILED! You are not in the library.\n");
			}
		}
		else if (strncmp(command, "delete_book", 11) == 0)
		{
			// Verific status.library si in functie de el merg mai departe la verificarea ID-ului sau afisez
			// mesajul de eroare aferent
			if (status.library == 1)
			{
				char route[BUFLEN], id[BUFLEN];

				printf("id=");
				fgets(id, BUFLEN, stdin);

				// Verific daca ID-ul este nul sau nu este un numar pozitiv
                if (id[0] == '\n')
				{
                    fprintf(stderr, "FAILED! The ID is null!\n");
                    continue;
                }
				
                id[strlen(id) - 1] = '\0';	

                if (!check_num(id))
				{
                    fprintf(stderr, "FAILED! The format is wrong!\n");
                    continue;
                }

				sprintf(route, "%s/%d", BOOKS, atoi(id));

				// Se sterge cartea cu ID-ul dat daca exista sau se afiseaza eroarea aferenta
				char *deleted = strstr(recv_get_del_req(socket, host, route, token, cookies, DELETE), "No book was deleted!");

				if (deleted != NULL)
					{
						fprintf(stderr, "FAILED! The book ID is invalid!\n");
					}
					else
					{
						printf("SUCCESS!\n");
					}
			} 
			else 
			{
				fprintf(stderr, "FAILED! You are not in the library.\n");
			}
		}
        else if (strncmp(command, "logout", 6) == 0)
		{
			if (status.connection == 1)
			{
				// Daca sunt conectat, trimit comanda de delogare daca nu afisez
				// eroarea aferenta
				status.connection = 0;
				status.library = 0;
				recv_get_del_req(socket, host, LOGOUT, token, cookies, GET);
				printf("SUCCESS!\n");
			}
			else
			{ 
				fprintf(stderr, "FAILED! You are not logged in!\n");
			}
		}

		else if(strncmp(command, "exit", 4) == 0)
		{
			// Ies din while
			break;
		}
		// Opesc conexiunea la server
		close_connection(socket);
	}
}