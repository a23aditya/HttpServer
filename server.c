#include"includes.h"

#define MAX_EVENTS 10000

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

void vNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}


//------------------------------------------------------------
// Function : vRunHttpServer()
//------------------------------------------------------------
void vRunHttpServer(int port)
{
    int sfd =socket(AF_INET,SOCK_STREAM,0);
    if(sfd < 0)
    {
        perror("fail to create socket\n");
    }
    int opt=1;
    setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in serverInfo ={0};
    serverInfo.sin_addr.s_addr=INADDR_ANY;
    serverInfo.sin_family=AF_INET;
    serverInfo.sin_port=htons(port);

    if(bind(sfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo)) < 0)
    {
        perror("bind fail\n");
        close(sfd);
        return;
    }
    if(listen(sfd,128) < 0)
    {
        perror("listen fail");
        close(sfd);
        return;
    }
    // for non-blocking
    vNonBlocking(sfd);

    int epfd = epoll_create1(0);
    if(epfd < 0)
    {
        perror("epoll create fail\n");
        close(sfd);
        return;
    }

    struct epoll_event ev,events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd =sfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,sfd,&ev);

    char buffer[BUFFER_SIZE];
    printf("Server listening on port %d...\n", port);

    while(1)
    {
        int nfds = epoll_wait(epfd,events,MAX_EVENTS,-1);
        if(nfds < 0)
        {
            perror("epoll_wait fail\n");
            break;
        }

        for(int i=0;i<nfds;i++)
        {
            int fd = events[i].data.fd;
            if(fd == sfd)
            {
                struct sockaddr_in clientInfo;
                socklen_t clientLen = sizeof(clientInfo);
                int client_fd = accept(sfd,(struct sockaddr *)&clientInfo,&clientLen);
                if(client_fd < 0)
                {
                    perror("accept");
                    continue;
                }
                vNonBlocking(client_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                epoll_ctl(epfd,EPOLL_CTL_ADD,client_fd,&ev);
                printf("New client connected: FD %d\n", client_fd);
            }
            else
            {
                int bytes = read(fd,buffer,sizeof(buffer) -1);
                if(bytes < 0)
                {
                    printf("Client disconnected: FD %d\n", fd);
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                }
                else
                {
                    buffer[bytes] = '\0';
                    printf("Data from FD %d: %s", fd, buffer);

                    vHttpRequest_t req;
                    if (vParseHttpRequest(buffer, &req) == 0 && strcmp(req.method, "GET") == 0)
                    {
                        char response[BUFFER_SIZE * 2];
                        int resp_len = vServeData(req.path, response, sizeof(response));
                        send(fd, response, resp_len, 0);
                    }
                    else
                    {
                        send(fd, HTTP_ERR_STR, strlen(HTTP_ERR_STR), 0);
                    }
                }
            }
        }
    }
   close(sfd);

}


