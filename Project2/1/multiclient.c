#include "csapp.h"
#include <time.h>
#define MAX_CLIENT 100
#define ORDER_PER_CLIENT 10
#define STOCK_NUM 5
#define BUY_SELL_MAX 10

int main(int argc, char **argv) 
{
	struct timeval start;	/* starting time */
	struct timeval end;	/* ending time */
	unsigned long e_usec;	/* elapsed microseconds */

	pid_t pids[MAX_CLIENT];
	int runprocess = 0, status, i;
	int clientfd, num_client;
	char *host, *port, buf[MAXLINE], tmp[3];
	rio_t rio;

	if (argc != 4) {
		fprintf(stderr, "usage: %s <host> <port> <client#>\n", argv[0]);
		exit(0);
	} 

	host = argv[1];
	port = argv[2];
	num_client = atoi(argv[3]);
	printf("client num = %d\n\n", num_client);
	gettimeofday(&start, 0);	/* mark the start time */

	clock_t s,e;
	double result;
	s = clock();

	while(runprocess < num_client)
	{
		pids[runprocess] = fork();
		if(pids[runprocess] < 0) return -1;
		
		else if(pids[runprocess] == 0)
		{
			printf("child %ld\n", (long)getpid());
			clientfd = Open_clientfd(host, port);
			Rio_readinitb(&rio, clientfd);
			srand((unsigned int) getpid());

			for(i=0;i<ORDER_PER_CLIENT;i++){
				int option = rand() % 3;
				
				if(option == 0)
				{
					strcpy(buf, "show\n");
				}
				else if(option == 1){
					int list_num = rand() % STOCK_NUM + 1;
					int num_to_buy = rand() % BUY_SELL_MAX + 1;

					strcpy(buf, "buy ");
					sprintf(tmp, "%d", list_num);
					strcat(buf, tmp);
					strcat(buf, " ");
					sprintf(tmp, "%d", num_to_buy);
					strcat(buf, tmp);
					strcat(buf, "\n");
				}
				else if(option == 2){
					int list_num = rand() % STOCK_NUM + 1; 
					int num_to_sell = rand() % BUY_SELL_MAX + 1;
					
					strcpy(buf, "sell ");
					sprintf(tmp, "%d", list_num);
					strcat(buf, tmp);
					strcat(buf, " ");
					sprintf(tmp, "%d", num_to_sell);
					strcat(buf, tmp);
					strcat(buf, "\n");
				}
				printf("%s", buf);

				Rio_writen(clientfd, buf, strlen(buf));
				Rio_readnb(&rio, buf, 8192);
				// Fputs(buf, stdout);
				// usleep(1000000);
			}
			Close(clientfd);
			exit(0);
		}
		runprocess++;
		printf("\n");
	}
	for(i=0;i<num_client;i++){
		waitpid(pids[i], &status, 0);
	}

	gettimeofday(&end, 0);
	
	e_usec = ((end.tv_sec * 1000000) + end.tv_usec) - ((start.tv_sec * 1000000) + start.tv_usec);
	printf("elapsed time: %lu microseconds\n", e_usec);
	printf("elapsed time: %f seconds\n", e_usec*0.0000001);
	e = clock();
	result = (double)(e-s);
	printf("elapsed time %fs\n",result*0.01);
	return 0;
}
