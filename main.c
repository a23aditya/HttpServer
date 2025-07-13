#include"includes.h"


int main(int argc , char *argv[])
{
	int opt;
	int port = PORT_NO;

	while ((opt = getopt(argc, argv, "p:h")) != -1) 
	{
    		switch (opt) 
		{
        		case 'p':
            			port = atoi(optarg);
            			break;
        		case 'h':
            			printf("Usage: ./server -p <port>\nDefault port: 5555\n");
            			return -1;
        		default:
            			printf("Usage: ./server -p <port>\nDefault port: 5555\n");
            			return -1;
    		}
	}	
	vRunHttpServer(port);
}
