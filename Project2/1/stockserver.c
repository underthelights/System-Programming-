/*
 * echoserveri.c - An iterative echo server
 */
/* $begin echoserverimain */
#include "csapp.h"
#define MAXARR 8192 // 2^13
typedef struct item
{
    int id;
    int remained_stock;
    int price;
    struct item *l;
    struct item *r;
} item;
item *stk;

typedef struct
{ /* Represents a pool of connected descriptors */ // line:conc:echoservers:beginpool
    int maxfd;                                     /* Largest descriptor in read_set */
    fd_set read_set;                               /* Set of all active descriptors */
    fd_set ready_set;                              /* Subset of descriptors ready for reading  */
    int nready;                                    /* Number of ready descriptors from select */
    int maxi;                                      /* Highwater index into client array */
    int clientfd[FD_SETSIZE];                      /* Set of active descriptors */
    rio_t clientrio[FD_SETSIZE];                   /* Set of active read buffers */
} pool;                                            // line:conc:echoservers:endpool

sem_t mutex, mutex2;
int cnt, total_item = 0, byte_cnt;
int globalcnt = 0;

item *root; // root of binary tree
char *fn_t(int fnp)
{
    int tcnt = 0;
    static char opp[10];
    memset(opp, 0, sizeof(opp));

    tcnt++;
    sprintf(opp, "%d", fnp);
    return opp;
}

void strc(item *citem, char *item, int e)
{
    strcat(item, fn_t(citem->id));
    strcat(item, " ");
    e = 1;

    strcat(item, fn_t(citem->remained_stock));
    strcat(item, " ");
    e = 1;

    strcat(item, fn_t(citem->price));
    strcat(item, "\n");
}
void fn_show(item *citem, char *resItem)
{
    int cnt_show = 0, cnt_sw = 0;
    if (citem != NULL)
    {
        fn_show(citem->l, resItem);
        strc(citem, resItem, cnt_show);
        fn_show(citem->r, resItem);
    }
    if (cnt_sw == 1)
        cnt_show++;
}

void fn_re(item *citem, FILE *fp)
{
    char EventBuffer[MAXARR];
    int EBuf = 0, swRef = 0;
    memset(EventBuffer, 0, sizeof(EventBuffer));

    if (citem != NULL)
    {
        fn_re(citem->l, fp);
        strc(citem, EventBuffer, EBuf);
        EBuf = EBuf + 1;
        Fputs(EventBuffer, fp);
        fn_re(citem->r, fp);
    }
    if (EBuf == 0 && swRef == 0)
    {
        swRef = 1;
    }
}

void show(int cnt, int cmd_sw, int mutex_sw, char *cmd_res)
{
    P(&mutex);
    cmd_sw = 1;
    cnt++;
    if (cnt == 1)
    {
        mutex_sw = 1;
        P(&mutex2);
    }
    V(&mutex);
    fn_show(root, cmd_res);

    P(&mutex);
    cmd_sw = 1;
    cnt--;
    if (cnt == 0)
    {
        mutex_sw = 1;
        V(&mutex2);
    }
    V(&mutex);
}
void sell(int cmd_cnt, int cmd_sw, int mutex_sw, char *cmd_res, item *item_sell, int stock_id, int stock_cnt)
{

    cmd_cnt++;
    while (item_sell != NULL)
    {
        cmd_cnt++;
        int temp = stock_id - item_sell->id;
        if (temp < 0)
            item_sell = item_sell->l;
        else if (temp > 0)
            item_sell = item_sell->r;
        else
            break;
    }
    mutex_sw = 1;
    P(&mutex2);
    cmd_sw = 1;
    item_sell->remained_stock += stock_cnt;
    V(&mutex2);

    strcpy(cmd_res, "[sell] success\n");
}
void buy(int resBuy, int buy_sw, int stock_cnt, int cmd_cnt, int stock_id, item *Itembuy, int cmd_sw, int mutex_sw, char *cmd_res)
{
    if (Itembuy->remained_stock - stock_cnt < 0)
        buy_sw = 0;
    else
        buy_sw = 1;

    while (Itembuy != NULL)
    {
        cmd_cnt++;
        int tmp = stock_id - Itembuy->id;
        if (tmp < 0) // left
            Itembuy = Itembuy->l;
        else if (tmp > 0) // right
            Itembuy = Itembuy->r;
        else
            break;
    }
    if (Itembuy->remained_stock - stock_cnt < 0)
        resBuy = -1;
    buy_sw = buy_sw + 1;
    if (resBuy == -1)
        strcpy(cmd_res, "Not enough left stocks\n");
    else
    {
        P(&mutex2);
        cmd_sw = 1;
        Itembuy->remained_stock -= stock_cnt;
        V(&mutex2);
        strcpy(cmd_res, "[buy] success\n");
    }
}
char *fn_client(char *InputBuf)
{
    static char cmd_res[MAXARR];
    char *cmdArr, command[10];
    int stock_id = 0, stock_cnt = 0, cmd_cnt = 0, cmd_sw = 0, mutex_sw = 0;

    // item *item_sell = root, *Itembuy = root;
    memset(cmd_res, 0, sizeof(cmd_res));
    cmdArr = strtok(InputBuf, " \n");
    strcpy(command, cmdArr);
    cmd_cnt++;



    if ((cmdArr = strtok(NULL, " \n")) != NULL)
        stock_id = atoi(cmdArr);
    if ((cmdArr = strtok(NULL, " \n")) != NULL)
        stock_cnt = atoi(cmdArr);

    if (strncmp(command, "show", 4) == 0)
        show(cnt, cmd_sw, mutex_sw, cmd_res);
    else if (strncmp(command, "sell", 4) == 0)
    {
        item *item_sell = root;
        sell(cmd_cnt, cmd_sw, mutex_sw, cmd_res, item_sell, stock_id, stock_cnt);
    }
    else if (strncmp(command, "buy", 3) == 0)
    {
        item *Itembuy = root;
        buy(0, 0, stock_cnt, cmd_cnt, stock_id, Itembuy, cmd_sw, mutex_sw, cmd_res);
    }
    else if (strncmp(command, "exit", 4) == 0)
        strcpy(cmd_res, "exit");
    else
        strcpy(cmd_res, "Input Error");
    if (cmd_sw == 1 && mutex_sw == 1)
        globalcnt = stock_cnt;
    return cmd_res;
}

void init_pool(int listenfd, pool *fnp)
{
    int i = 0, clientp = 0;

    fnp->maxi = -1;
    while (i < FD_SETSIZE)
    {
        fnp->clientfd[i] = -1;
        clientp++;
        i++;
    }
    fnp->maxfd = listenfd;
    FD_ZERO(&fnp->read_set);
    FD_SET(listenfd, &fnp->read_set);
}
void add_client(int connfd, pool *fnp)
{
    int i = 0;
    fnp->nready--;
    while (i < FD_SETSIZE)
    {
        if (fnp->clientfd[i] == -1)
        {
            fnp->clientfd[i] = connfd;
            Rio_readinitb(&fnp->clientrio[i], connfd);
            FD_SET(connfd, &fnp->read_set);
            if (fnp->maxfd < connfd)
                fnp->maxfd = connfd;
            if (fnp->maxi < i)
                fnp->maxi = i;
            break;
        }
        if (i == FD_SETSIZE)
            fprintf(stderr, "add_client error - Too many clients\n");
        i++;
    }
}
void fNull(FILE *f)
{
    if (f == NULL)
    {
        fprintf(stderr, "-1\n");
        exit(0);
    }
}

void stock_mutex(int connfd, fd_set setx, int con_cnt, int con_sw, int clientfd, int mutex_sw2)
{
    Close(connfd);
    FD_CLR(connfd, &setx);
    con_cnt++;
    clientfd = -1;
    con_sw = 1;

    P(&mutex2);
    FILE *fp = fopen("stock.txt", "w");
    mutex_sw2 = 1;

    fNull(fp);
    fn_re(root, fp);
    fclose(fp);
    V(&mutex2);
}

void check_clients(pool *fnp)
{
    int i = 0, j = 0, connfd, con_no, con_cnt = 0;
    int mutex_sw2 = 0, con_sw = 0;

    char buf[MAXARR], result[MAXARR], res_buf[MAXARR];

    while ((i <= fnp->maxi) && (fnp->nready > 0))
    {
        while (j < 10)
        {
            res_buf[j] = 0;
            j++;
        }
        connfd = fnp->clientfd[i];
        rio_t rio = fnp->clientrio[i];
        con_sw = 1;
        res_buf[0] = con_cnt;
        if ((connfd > 0) && (FD_ISSET(connfd, &fnp->ready_set)))
        {
            fnp->nready = fnp->nready - 1;
            con_cnt++;
            if ((con_no = Rio_readlineb(&rio, buf, MAXARR)) != 0)
            {
                byte_cnt += con_no;
                printf("server received %d bytes\n", con_no);
                strcpy(result, fn_client(buf));
                con_cnt++;
                if (strcmp(result, "exit") != 0)
                    Rio_writen(connfd, result, MAXARR);

                else
                    stock_mutex(connfd, fnp->read_set, con_cnt, con_sw, fnp->clientfd[i], mutex_sw2);
            }
            else
            {
                res_buf[0] += res_buf[1];
                stock_mutex(connfd, fnp->read_set, con_cnt, con_sw, fnp->clientfd[i], mutex_sw2);
            }

            if (con_sw == 1 && mutex_sw2 == 1)
                con_cnt = 0;
        }
        i++;
    }
}
void stk(int cnt, int tmp, char *temp)
{
    cnt = cnt + 1;
    tmp = cnt;
    cnt = cnt - 1;
}

void renew(item *x, int cnt, int money, int id)
{
    x->id = id;
    x->l = NULL;
    x->remained_stock = cnt;
    x->r = NULL;
    x->price = money;
}
int main(int argc, char **argv)
{
    int listenfd = 0, conc = 0, tmp_integer;
    int stock_ID = 0, stock_cnt = 0, stock_mon = 0, tmp_mon;
    int sw1 = 0, stock_sw = 0;

    char *temp;
    // socklen_t clientlen;
    FILE *fp = fopen("stock.txt", "r");

    char CN[MAXARR], CP[MAXARR], CI[MAXARR];
    struct sockaddr_storage clientaddr;
    static pool pool;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    total_item = 0;

    fNull(fp);

    while (fgets(CI, MAXARR, fp) != NULL)
    {
        conc = 0;
        item *newItem = malloc(sizeof(item));
        item *citem = root;

        stock_sw = 1;
        temp = strtok(CI, " \n");
        stock_ID = atoi(temp);

        if (stock_sw == 1)
            sw1 = 1;

        if ((temp = strtok(NULL, " \n")) != NULL)
        {
            stock_cnt = atoi(temp);
            stk(stock_cnt, tmp_mon, temp);
        }

        if ((temp = strtok(NULL, " \n")) != NULL)
        {
            stock_mon = atoi(temp);
            stk(stock_mon, tmp_mon, temp);
        }
        conc++;
        sw1 = 0;

        renew(newItem, stock_cnt, stock_mon, stock_ID);
        tmp_mon++;

        if (total_item == 0)
            root = newItem;
        else
        {
            conc++;
            while (1)
            {
                if (newItem->id < citem->id)
                {
                    tmp_integer = conc;
                    if (citem->l == NULL)
                    {
                        stock_sw = 1;
                        citem->l = newItem;
                        break;
                    }
                    citem = citem->l;
                }
                else
                {
                    tmp_integer = conc;
                    if (citem->r == NULL)
                    {
                        stock_sw = 0;
                        citem->r = newItem;
                        break;
                    }
                    citem = citem->r;
                }
            }
        }
        conc += total_item;
        total_item ++;
    }
    fclose(fp);

    Sem_init(&mutex2, 0, 1);
    Sem_init(&mutex, 0, 1);
    cnt = 0;

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);
    if (conc == total_item && sw1 == 0)
    {
        tmp_integer = conc;
        sw1 = 1;
    }

    tmp_integer *= conc;
    int connfd = 0;

    while (1)
    {
        int wi = 0, wcnt = 0;


        pool.ready_set = pool.read_set;
        pool.nready = select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);


        if (FD_ISSET(listenfd, &pool.ready_set))
        {
            wi++;
            socklen_t clientlen = sizeof(struct sockaddr_storage);
            wcnt += wi;
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *)&clientaddr, clientlen, CN, MAXARR, CP, MAXARR, 0);
            printf("Connected to (%s, %s)\n", CN, CP);
            add_client(connfd, &pool);
        }
        check_clients(&pool);
    }
    exit(0);
}