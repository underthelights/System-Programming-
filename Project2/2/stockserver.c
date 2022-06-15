#include "csapp.h"
#define MAXARR 8192 // 2^13
#define TWOTEN 1024
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
{
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;
sbuf_t sBuf;

sem_t mutex, mutex2;
int count1, count2, globalcnt;

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

void show(int cnt2, int cmd_sw, int mutex_sw, char *thread_res)
{
    P(&mutex);
    cmd_sw = 1;
    cnt2++;
    if (cnt2 == 1)
    {
        mutex_sw = 1;
        P(&mutex2);
    }
    V(&mutex);
    fn_show(stk, thread_res);

    P(&mutex);
    cmd_sw = 1;
    cnt2--;
    if (cnt2 == 0)
    {
        mutex_sw = 1;
        V(&mutex2);
    }
    V(&mutex);
}
void sell(int cmd_cnt, int cmd_sw, item *item_sell, int tempn, int thread_id, int stock_cnt, char thread_res[MAXARR])
{
    cmd_cnt += 1;
    while (item_sell != NULL)
    {
        tempn++;
        int tmp = thread_id - item_sell->id;
        if (tmp < 0)
            item_sell = item_sell->l;
        else if (tmp > 0)
            item_sell = item_sell->r;
        else
            break;
    }
    tempn = cmd_cnt;
    P(&mutex2);
    cmd_sw = 1;
    item_sell->remained_stock += stock_cnt;
    V(&mutex2);
    strcpy(thread_res, "[sell] success\n");
}
void buy(int resBuy, int buy_sw, int stock_cnt, int cmd_cnt, int thread_id, item *item_buy, int cmd_sw, int mutex_sw, char *thread_res)
{
    if (item_buy->remained_stock - stock_cnt < 0)
        buy_sw = 0;
    else
        buy_sw = 1;

    while (item_buy != NULL)
    {
        int tmp = thread_id - item_buy->id;
        if (tmp > 0)
            item_buy = item_buy->r;
        else if (tmp < 0)
            item_buy = item_buy->l;
        else
            break;
        buy_sw = 1;
    }

    if (item_buy->remained_stock - stock_cnt < 0)
        resBuy = -1;
    if (resBuy == -1)
        strcpy(thread_res, "Not enough left stocks\n");
    else
    {
        buy_sw = 1;
        P(&mutex2);
        cmd_sw = 1;
        item_buy->remained_stock = item_buy->remained_stock - stock_cnt;
        V(&mutex2);
        buy_sw = buy_sw + 1;
        strcpy(thread_res, "[buy] success\n");
    }
}
void fn_thread(int connfd)
{
    rio_t thread_tmp;
    int threadbyte, thread_id = 0, stock_cnt = 0, cmd_cnt = 0;
    ;
    int cmd_sw = 0, mutex_sw = 0, tempn = 0;
    char thread_buf[MAXARR], thread_res[MAXARR], command[10];
    char *thread_arr;
    Rio_readinitb(&thread_tmp, connfd);

    while ((threadbyte = Rio_readlineb(&thread_tmp, thread_buf, MAXARR)) != 0)
    {
        item *item_sell = stk, *item_buy = stk;

        printf("server received %d bytes\n", threadbyte);
        memset(thread_res, 0, sizeof(thread_res));
        cmd_cnt++;
        thread_arr = strtok(thread_buf, " \n");
        strcpy(command, thread_arr);
        if (cmd_sw == 1)
        {
            cmd_sw = 1;
            mutex_sw = 1;
        }
        if ((thread_arr = strtok(NULL, " \n")) != NULL)
            thread_id = atoi(thread_arr);
        if ((thread_arr = strtok(NULL, " \n")) != NULL)
            stock_cnt = atoi(thread_arr);

        if (strncmp(command, "show", 4) == 0)
        {
            show(count2, cmd_sw, mutex_sw, thread_res);
        }
        else if (strncmp(command, "exit", 4) == 0)
        {
            strcpy(thread_res, "exit\n");
            cmd_cnt ++;
            return;
        }
        else if (strncmp(command, "sell", 4) == 0)
        {
            sell(cmd_cnt, cmd_sw, item_sell, tempn, thread_id, stock_cnt, thread_res);
        }
        else if (strncmp(command, "buy",3) == 0)
        {
            buy(0, 0, stock_cnt, cmd_cnt, thread_id, item_buy, cmd_sw, mutex_sw, thread_res);
        }
        else
        {
            strcpy(thread_res, "invalid\n");
            return;
        }
        if (cmd_sw == 1 && mutex_sw == 1)
        {
            globalcnt = stock_cnt;
        }
        Rio_writen(connfd, thread_res, MAXARR);
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
/* sbuf.c */
/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;                /* Buffer holds max of n items */
    sp->front = sp->rear = 0; /* Empty buffer iff front == rear */

    Sem_init(&sp->mutex, 0, 1); /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n); /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0); /* Initially, buf has zero data items */
}

/* Insert item onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots); /* Wait for available slot */
    P(&sp->mutex); /* Lock the buffer */
    sp->buf[(++sp->rear) % (sp->n)] = item; /* Insert the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->items);                          /* Announce available item */
}
/* $end sbuf_insert */

/* Remove and return the first item from buffer sp */
/* $begin sbuf_remove */
int sbuf_remove(sbuf_t *sp)
{
    int item;

    P(&sp->items); /* Wait for available item */
    P(&sp->mutex); /* Lock the buffer */

    item = sp->buf[(++sp->front) % (sp->n)]; /* Remove the item */
    V(&sp->mutex);                           /* Unlock the buffer */
    V(&sp->slots);                           /* Announce available slot */
    return item;
}
/* $end sbuf_remove */
/* $end sbufc */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}
/* $end sbuf_deinit */

void fn_re(item *citem, FILE *fp)
{
    char EventBuffer[MAXARR];
    int EBuf = 0, swRef = 0;
    memset(EventBuffer, 0, sizeof(EventBuffer));

    if (citem != NULL)
    {
        fn_re(citem->l, fp);
        strc(citem, EventBuffer, EBuf);
        EBuf ++;
        Fputs(EventBuffer, fp);
        fn_re(citem->r, fp);
    }
    if (EBuf == 0 && swRef == 0)
        swRef = 1;
}

void *fn_th(void *aa)
{
    Pthread_detach(pthread_self());
    while (1)
    {
        int connfd = sbuf_remove(&sBuf);
        fn_thread(connfd);
        Close(connfd);

        P(&mutex2);

        FILE *fp = fopen("stock.txt", "w");
        fNull(fp);
        fn_re(stk, fp);
        fclose(fp);
        
        V(&mutex2);
    }
}

void renew(item *x, int Cid, int opper, int money)
{
    x->l = NULL;
    x->r = NULL;
    x->id = Cid;
    x->remained_stock = opper;
    x->price = money;
}

int main(int argc, char **argv)
{
    pthread_t thread_id;
    FILE *fp = fopen("stock.txt", "r");

    struct sockaddr_storage Cadd;
    int listenfd, connfd, i=0, j=0, tmp_cnt = 0, ccount = 0;
    int Cid, opper = 0, money = 0, moneyarr[10], CArr[1024];
    int sw1 = 0, sw2 = 0;
    char CN[MAXARR], CP[MAXARR], CL[MAXARR];
    char *linearr;

    count1 = 0;
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    fNull(fp);

    while (fgets(CL, MAXARR, fp) != NULL)
    {
        item *nitem= malloc(sizeof(struct item)), *citem = stk;

        tmp_cnt ++;
        if (tmp_cnt == -1)
        {
            sw1 = 1;
        }
        linearr = strtok(CL, " \n");
        Cid = atoi(linearr);
        sw2 = 1;
        if ((linearr = strtok(NULL, " \n")) != NULL)
            opper = atoi(linearr);
        if ((linearr = strtok(NULL, " \n")) != NULL)
            money = atoi(linearr);
        ccount = tmp_cnt;
        renew(nitem, Cid, opper, money);
        while (j < 10)
        {
            moneyarr[j] = j;
            j++;
        }
        if (count1 == 0)
            stk = nitem;
        else
        {
            while (1)
            {
                moneyarr[0] += moneyarr[1];
                if (nitem->id > citem->id)
                {
                    sw2 = 1;
                    if (citem->r == NULL)
                    {
                        sw1 = 1;
                        citem->r = nitem;
                        break;
                    }
                    citem = citem->r;
                }
                else
                {
                    sw2 = 1;
                    if (citem->l == NULL)
                    {
                        sw1 = 1;
                        citem->l = nitem;
                        break;
                    }
                    citem = citem->l;
                }
            }
        }
        if (sw1 == 1 && sw2 == 1)
        {
            ccount +=tmp_cnt;
        }
        count1 ++;
    }
    if (tmp_cnt +1 == 0) sw1 = 1;
    fclose(fp);

    Sem_init(&mutex2, 0, 1);
    Sem_init(&mutex, 0, 1);

    count2 = 0;
    j = 0;
    while (j < 10)
    {
        moneyarr[j] = j;
        j++;
    }
    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sBuf, MAXARR);

    i= 0;
    while (i<TWOTEN){
        Pthread_create(&thread_id, NULL, fn_th, NULL);
        CArr[i] = 0;
        i++;
    }
    CArr[0]++;
    socklen_t opera;
    
    while (1)
    {
        sw1 = 0;
        sw2 = 0;
        opera = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&Cadd, &opera);
        Getnameinfo((SA *)&Cadd, opera, CN, MAXARR, CP, MAXARR, 0);
        printf("Connected to (%s, %s)\n", CN, CP);
        sbuf_insert(&sBuf, connfd);
    }

    sbuf_deinit(&sBuf);
    exit(0);
}
