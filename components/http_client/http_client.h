#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H


#define SERVER_URL "http://192.168.100.6:55555/api/data"

#include <stdbool.h>

void http_client_init(void);

bool http_client_post(const char* json_data);


#endif	//	HTTP_CLIENT_H