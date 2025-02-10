#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdbool.h>

void http_client_init(void);

bool http_client_post(const char* json_data);


#endif	//	HTTP_CLIENT_H