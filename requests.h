#ifndef _REQUESTS_
#define _REQUESTS_

#define GET 0
#define DELETE 1

// Calculez si returnez un GET request string
char *compute_get_del_request(char *host, char *url, char *query_params,
							char **cookies, int cookies_count, char *token, int type);

// Calculez si returnez un POST request string
char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
							int body_data_fields_count, char** cookies, int cookies_count, char *token);

#endif
