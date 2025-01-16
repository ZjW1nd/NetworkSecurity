#include "tls.h"

//模块代码来自发的样例，拼装过程中单独抽出来

int verify_callback(int preverify_ok, X509_STORE_CTX *x509_ctx)
{
    char buf[300];
    X509 *cert = X509_STORE_CTX_get_current_cert(x509_ctx);
    X509_NAME_oneline(X509_get_subject_name(cert), buf, 300);
    printf("subject = %s\n", buf);
    if (preverify_ok == 1)
        printf("Verification passed.\n");
    else{
        int err = X509_STORE_CTX_get_error(x509_ctx);
        if (err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN) {
			printf("Ignore verification result: %s.\n", X509_verify_cert_error_string(err));
			return 1;
		}
		printf("Verification failed: %s.\n", X509_verify_cert_error_string(err));
    }

    return preverify_ok;
}

// Copy
SSL *setupTLSClient(const char *hostname)
{
	// This step is no longer needed as of version 1.1.0.
    SSL_library_init();
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    const SSL_METHOD *meth;
    SSL_CTX *ctx;
    SSL *ssl;
    
    meth = SSLv23_client_method();
    ctx = SSL_CTX_new(meth);
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);
    // Step 2: Set up the server certificate and private key
	if (SSL_CTX_use_certificate_file(ctx, CLIENT_CERTF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(3);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, CLIENT_KEYF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(4);
	}
	if (!SSL_CTX_check_private_key(ctx)) {
		printf("Private key does not match the certificate public keyn");
		exit(5);
	}
    // Step 3: Create a new SSL structure for a connection
    ssl = SSL_new(ctx);

    X509_VERIFY_PARAM *vpm = SSL_get0_param(ssl);
    X509_VERIFY_PARAM_set1_host(vpm, hostname, 0);

    return ssl;
}

int setupTCPClient(const char *hostname, int port)
{
	struct sockaddr_in server_addr;

	// Get the IP address from hostname
	struct hostent *hp = gethostbyname(hostname);

	// Create a TCP socket
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Fill in the destination information (IP, port #, and family)
	memset(&server_addr, '\0', sizeof(server_addr));
	memcpy(&(server_addr.sin_addr.s_addr), hp->h_addr, hp->h_length);
	//server_addr.sin_addr.s_addr = inet_addr ("10.0.2.14"); 
	server_addr.sin_port = htons(port);
	server_addr.sin_family = AF_INET;

	// Connect to the destination
	connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));

	return sockfd;
}

// Copied from tlsserver.c:main 43-81
SSL *setupTLSServer() {
    const SSL_METHOD *meth;
    SSL_CTX *ctx;
    SSL *ssl;   

    SSL_library_init();
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    meth = SSLv23_server_method();
    ctx = SSL_CTX_new(meth);
    
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	SSL_CTX_load_verify_locations(ctx, CACERT, NULL);

	// Step 2: Set up the server certificate and private key
	if (SSL_CTX_use_certificate_file(ctx, SERVER_CERTF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(3);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, SERVER_KEYF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(4);
	}
	if (!SSL_CTX_check_private_key(ctx)) {
		fprintf(stderr, "Private key does not match the certificate public key\n");
		exit(5);
	}
	// Step 3: Create a new SSL structure for a connection
	ssl = SSL_new(ctx);

    return ssl;
}

// Copy 模块
int setupTCPServer() {
    struct sockaddr_in sa_server;
    int listen_sock;

    listen_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHK_ERR(listen_sock, "socket");
    memset(&sa_server, '\0', sizeof(sa_server));
    sa_server.sin_family = AF_INET;
    sa_server.sin_addr.s_addr = INADDR_ANY;
    sa_server.sin_port = htons(4433);
    int err = bind(listen_sock, (struct sockaddr *)&sa_server, sizeof(sa_server));
    CHK_ERR(err, "bind");
    err = listen(listen_sock, 5);
    CHK_ERR(err, "listen");
    return listen_sock;
}