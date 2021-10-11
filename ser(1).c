#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>

enum {conlim = 5, buf_size = 1000};
enum exits{badbind,badlisten,badsocket};

struct elem{
    int fd;
    int last_index;
    char *buf;
    struct elem *next;
};

int global_param = 0;
int repeat = 1;

void handler( int s)
{
    repeat = 0;
}

void delete_fd_from_list(struct elem **user, int fd)
{
    struct elem *tmp;
    struct elem *del;
    if ((*user)->fd == fd)
        *user = (*user)->next;
    tmp = *user;
    while(tmp){
        if (tmp->next && tmp->next->fd == fd){
            del = tmp->next;
            tmp->next = tmp->next->next;
            free(del);
        }
        tmp = tmp->next;
    }
}

void listnew(struct elem **s,int diskr)
{
    struct elem *help,*tmp;
    int handler;
    char str[]={
        "Welcome\n"
        "There are some comands: \n"
        "sum <number> - adds any signed integer to counter\n"
        "global - Value of global server counter\n"
        "<ENTER> - Value of global server counter too\n"
    };
    if ((handler=accept(diskr,NULL,NULL)) ==-1) {
        perror("accept");
    }
    write(handler,str,sizeof(str));
    help = *s;
    if (help!=NULL)
        while ((help->next)!=NULL)
            help=help->next;
    tmp = malloc(sizeof(**s));
    tmp->fd = handler;  
    tmp->buf = malloc(buf_size*sizeof(char));
    tmp->next = NULL;
    tmp->last_index = 0;
    if ((*s) == NULL) 
        *s = tmp;
    else 
        help->next = tmp;
}

int creat(fd_set *readfds, struct elem **user, int max)
{
    struct elem *L = *user;
    while(L!=NULL){
        FD_SET(L->fd, readfds);
        if (L->fd > max)
            max = L->fd;
        L = L->next;
    }
    return max;
}

int make_int(char *str)
{
    int i = 0, port = 0;
    while(str[i]!=0){
        port = 10*port+str[i]-'0';
        i++;
    }
    return port;
}

void bind_and_listen(int ls, char* str)
{
    int i=1;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(make_int(str));
    addr.sin_addr.s_addr = INADDR_ANY;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int));
    if (0 != bind(ls, (struct sockaddr *) &addr, sizeof(addr)) )
    {
        perror("bind");
            exit(badbind);
    }
    if (-1 == listen(ls,conlim)) {
        perror("listen");
        exit(badlisten);
    }
}

void clear(struct elem **p,int *counter)
{
    (*counter)--;
    shutdown((*p)->fd,2);
    close((*p)->fd);
}

int strequ(char *str1 , char* str2)
{
    int i = 0;
    if (!str1) 
        return 0;
    while (str1[i]!=0 && str2[i]!=0){
        if (str1[i] != str2[i]) 
            return 0;
        i++;
    }
    return 1;
}

int condition(char i)
{   
    if ((i>='0' && i<='9') || i=='-') 
    return 1;
    return 0;
}

int sum(struct elem **user)
{
    int flag = 1 ,i=0, num=0;
    while(i<=2) {
        i++;
    }
    i++;
    if ((*user)->buf[i] == '-') {
        flag = -1;
        i++;
    }
    while((*user)->buf[i]!=0){
        if (condition((*user)->buf[i]) != 1) 
            return -1;
        num = 10*num+(*user)->buf[i++]-'0';
    }
    num = flag * num;
    global_param += num;
    return 1;
}

void command_handler(struct elem **user)
{   
    static const char err1[] = "wrong command\n";
    static const char err2[] = "wrong argument\n";
    char answer[20]; 
    int length;
    if (strequ((*user)->buf , "global")) {
        length = sprintf(answer, "param=%d\n", global_param);
        write((*user)->fd, answer, length);
    }
    else if (strequ((*user)->buf, "sum")) {
        if (sum(user) == -1){
            write((*user)->fd, err2, sizeof(err2));
        }
    } else {
        write((*user)->fd, err1, sizeof(err1));
    }
}

int readbuf(struct elem **p,int *counter, int max,int *flag)
{
    struct elem *tmp = *p;
    char *buffer = malloc(buf_size*sizeof(char));
    int i=0, rd =read(tmp->fd,buffer,buf_size -1);
    if (rd == -1) {
        perror("read");
    }
    if (rd == 0) {
            clear(p,counter);
            return tmp->fd;
    }
    if (*counter == max || *flag){ 
            *flag = 1;
            buffer[rd] = '0';
        while(buffer[i]!='\n' && buffer[i]!='\r'){
        (*p)->buf[(*p)->last_index++] = buffer[i++];
        }
        (*p)->buf[(*p)->last_index] = 0;
        if (buffer[i]=='\r' || buffer[i] == '\n') {
        (*p)->last_index = 0;
        command_handler(p);
        }
        free(buffer);
    }
    return 0;
}

void delete_struct(struct elem **p)
{
    struct elem *q;
    while (*p){
    q = *p;
    free((*p)->buf);
    (*p) = (*p)->next;
    free(q);
    }
}

void many_clients(int ls)
{
   char str[] = "Sorry, many clients want to connect with server\n";
   int fd;
   if ((fd = accept(ls, NULL, NULL)) ==-1) 
        perror("accept");
   else{
        write(fd, str, sizeof(str));
        shutdown(fd, 2);
        close(fd);
   }
}

int main(int argc, char** argv)
{
   int ls, max, selec, counter=0, fd, flag = 0;
   struct elem *client=NULL, *tmp=NULL;
   signal(SIGINT, handler);
   if (argc == 1){
    fprintf(stderr, "You need to port number\n");
        exit(1);
    }
    if (argc > 2)   
        max = make_int(argv[2]); 
    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls==-1){
        perror("socket");
        exit(badsocket);
    }
    bind_and_listen(ls, argv[1]);
    while(repeat) {
        fd_set readfds;
        int number = ls;
        FD_ZERO(&readfds);
        FD_SET(number, &readfds);
        number = creat(&readfds, &client ,number);
        selec = select(number+1, &readfds, NULL, NULL, NULL);
    if (selec <1) 
            perror("select");
    if (FD_ISSET(ls,&readfds)) {
        if (counter < max) {
                listnew(&client,ls);
                counter++;
            } else 
                many_clients(ls);
        }
        tmp = client;
        while(tmp!=NULL){
            if (FD_ISSET(tmp->fd, &readfds)){
                if ((fd =readbuf(&tmp, &counter, max, &flag))>0){
                    delete_fd_from_list(&client, fd);
                }
            }
            tmp = tmp->next;
        }
    }
    delete_struct(&client);
    return 0;
}
