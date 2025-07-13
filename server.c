#include"includes.h"

typedef struct{
    char method[8];
    char path[256];
}vHttpRequest_t;

//------------------------------------------------------------
// Function : vParseHttpRequest()
//------------------------------------------------------------
static int vParseHttpRequest(const char *raw ,vHttpRequest_t *req)
{
    if(sscanf(raw,"%7s %255s",req->method,req->path) != 2)
    {
        return -1;
    }
    return 0;
}

//------------------------------------------------------------
// Function : vGetMimeType()
//  @MIME type (also called media type) is a standardized way
//       to describe the type and format of a file or data.
//------------------------------------------------------------
static const char* vGetMimeType(const char *path)
{
    const char *ext = strrchr(path,'.');
    if(!ext)
        return "text/plain";
    if(strcmp(ext,".html") == 0)
        return "text/html";
    if(strcmp(ext,".css") == 0)
        return "text/css";
    if(strcmp(ext,".js") == 0)
        return "application/javascript";
    if(strcmp(ext,".png") == 0)
        return "image/png";
    if(strcmp(ext,".jpg") == 0)
        return "image/jpeg";
    if(strcmp(ext,".html") == 0)
        return "text/html";
    return "application/octet-stream";
}

//------------------------------------------------------------
// Function : vServeData()
//------------------------------------------------------------
static int vServeData(const char *req_path, char *response_buf, size_t buf_size)
{
    char full_path[512];
    if(strcmp(req_path,"/") == 0)
    {
        snprintf(full_path,sizeof(full_path),"%s/index.html",WWW_ROOT);
    }
    else
    {
        snprintf(full_path,sizeof(full_path),"%s%s",req_path);
    }
    FILE *fp = fopen(full_path,"rb");
    if(!fp)
    {
        snprintf(response_buf, buf_size,HTTP_ERR_STR);
        return 0;
    }
    //GetfileSize
    fseek(fp,0,SEEK_END);
    long filesize = ftell(fp);
    fseek(fp,0,SEEK_SET);

    char *body = malloc(filesize);
    if(!body)
    {
        fclose(fp);
        snprintf(response_buf, buf_size,HTTP_ERR_STR);
        return 0;
    }
    fread(body,1,filesize,fp);
    fclose(fp);

    const char *mime = vGetMimeType(full_path);

    int n = snprintf(response_buf, buf_size,HTTP_SUCCESS_RESP,mime,filesize);

    if (n + filesize >= buf_size)
    {
        free(body);
        snprintf(response_buf, buf_size,HTTP_ERR_STR);
        return 0;
    }
    memcpy(response_buf + n, body, filesize);
    free(body);

    return n + filesize;
}

//------------------------------------------------------------
// Function : vRunHttpServer()
//------------------------------------------------------------
void vRunHttpServer(int port)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        perror("fail to create socket");
        return;
    }

    int opt =1;
    setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in serverInfo = {0};
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(port);
    serverInfo.sin_addr.s_addr = INADDR_ANY;

    // bind
    if (bind(sfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) < 0) {
        perror("bind fail");
        close(sfd);
        return;
    }

    // listen
    if (listen(sfd, 5) == -1) {
        perror("listen fail");
        close(sfd);
        return;
    }

    struct pollfd fds[MAX_CLIENT + 1];
    int nfds = 1;

    fds[0].fd = sfd;
    fds[0].events = POLLIN;

    for (int i = 1; i <= MAX_CLIENT; i++) {
        fds[i].fd = -1;
    }

    printf("Server listening on port 5555...\n");

    char buffer[BUFFER_SIZE];
    struct sockaddr_in clientInfo;

    while (1) {
        int activity = poll(fds, nfds, -1);
        if (activity < 0) {
            perror("poll error");
            break;
        }

        // check for new connection
        if (fds[0].revents & POLLIN) {
            socklen_t clientsize = sizeof(clientInfo);
            int new_fd = accept(sfd, (struct sockaddr *)&clientInfo, &clientsize);
            if (new_fd < 0) {
                perror("accept fail");
                continue;
            }

            printf("New client connected: FD %d\n", new_fd);

            int added = 0;
            for (int i = 1; i <= MAX_CLIENT; i++) {
                if (fds[i].fd == -1) {
                    fds[i].fd = new_fd;
                    fds[i].events = POLLIN;
                    if (i >= nfds) nfds = i + 1;
                    added = 1;
                    break;
                }
            }

            if (!added) {
                printf("Too many clients. Connection rejected.\n");
                close(new_fd);
            }
        }

        // check all client sockets
        for (int i = 1; i < nfds; i++) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                int bytes = read(fds[i].fd, buffer, sizeof(buffer));
                if (bytes <= 0) {
                    printf("Client disconnected: FD %d\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else {
                    buffer[bytes] = '\0';
                    printf("Data from FD %d: %s", fds[i].fd, buffer);

                    vHttpRequest_t req;
                    if(vParseHttpRequest(buffer,&req) == 0 && strcmp(req.method,"GET") == 0)
                    {
                        char response[BUFFER_SIZE *2];
                        int resp_len = vServeData(req.path,response,sizeof(response));
                         send(fds[i].fd, response,resp_len, 0);

                    }
                    else
                    {
                        const char *bad_req =HTTP_ERR_STR;
                        send(fds[i].fd,bad_req,strlen(bad_req), 0);
                    }
                }
            }
        }
    }

    close(sfd);
}


