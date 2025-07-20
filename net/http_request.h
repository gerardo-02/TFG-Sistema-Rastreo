#ifndef HTTP_REQUEST_H
#define	HTTP_REQUEST_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#ifdef	__cplusplus
extern "C" {
#endif

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */
    /* ************************************************************************** */


    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */

    /* ************************************************************************** */
    typedef enum {
        HTTP_METHOD_POST = 0,
        HTTP_METHOD_GET,
    } t_HTTP_METHODS;

    typedef struct {
        char *raw;
        uint32_t length;
    } t_HTTP_REQUEST_HANDLE;

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    t_HTTP_REQUEST_HANDLE *http_request_init (t_HTTP_METHODS method, char *host, int port, char *path, char *query);
    int http_request_add_generic(t_HTTP_REQUEST_HANDLE *rh, char *label, char *content);
    int http_request_add_accept(t_HTTP_REQUEST_HANDLE *rh, char *accept);
    int http_request_add_bearer_token(t_HTTP_REQUEST_HANDLE *rh, char *token);
    int http_request_add_content_user_pass(t_HTTP_REQUEST_HANDLE *rh, char *user, char *pass);
    void http_request_delete(t_HTTP_REQUEST_HANDLE **handle);

#ifdef	__cplusplus
}
#endif

#endif	/* HTTP_REQUEST_H */

