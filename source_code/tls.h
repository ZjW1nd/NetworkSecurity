#ifndef TLS_H
#define TLS_H

#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netdb.h>
#include <string.h>

/* define HOME to be dir for key and cert files... */
#define HOME	"../certification/"

/* Make these what you want for cert & key files */
#define CLIENT_CERTF    HOME"/client/client.crt"
#define CLIENT_KEYF     HOME"/client/client.key"
#define SERVER_CERTF	HOME"/server/server.crt"
#define SERVER_KEYF	    HOME"/server/server.key"
#define CACERT	        HOME"ca.crt"

#define CHK_NULL(x)	if ((x)==NULL) exit (1)
#define CHK_SSL(err)	if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }
#define CHK_ERR(err,s)	if ((err)==-1) { perror(s); exit(1); }


// Copy
SSL *setupTLSClient(const char *hostname);
SSL *setupTLSServer();

int setupTCPClient(const char *hostname, int port);
int setupTCPServer();


#endif // TLS_H