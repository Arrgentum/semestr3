#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>
#include<fcntl.h>

enum {amp=1 , more=2 ,less=3, pip=4, ok=1, no=0};

struct param {
	char* argc;
	int max;
	int amp;
	char* in;
	char* out1;
	char *out2;
	char** vec;
	int error;
	struct param *next;
};

struct processes {
	int count;
	int size;
	int* pids;
};

struct list {
	int argc;
	char *elem;
	struct list *next;
};

int strequ(char* s1, char* s2)
{
	while( *s1 != '\0' && *s2 != '\0' ){
		if( *s1 != *s2  ){
			return 0;
		}
		s1++;
		s2++;
	}	
	return *s1 == *s2;
}	

int length( char* s )
{
	int i=0;
	while( *s != '\0' ){
		i++;
		s++;
	}
	return i;
}

char* strdupe(char * s)
{
   	int len = length(s);
	char *news = malloc(sizeof(char)*(len+1));
   	char *p = news;
   	while (*s) {
      		*p = *s;
       		 p++;
		s++;
	}
	*p = 0;
	return news;
}

void freevector(char** p)
{
	int i=0;
	while( p[i]) {
		free(p[i]);
		p[i++]=NULL;
	}
	free(p);
}

char** vectordup(char** old, int m)
{
	char **k;
	char *s;
	int i;
	k = malloc(sizeof(char*)*(m+1));
	for(i = 0; i < m; i++) {
		s = old[i];
		k[i] = strdupe(s);
	}
	k[m] = NULL;
	return k;
}

struct list *data(char *s)
{
	struct list *m;
	m = malloc(sizeof(struct list));
	m->elem = s;
	m->next = NULL;
	return m;
};

int kb(char c, int i)
{
	if (c == '"') {
		i = !i;
	}
	return i;
}

char* reallc(char *s, int size)
{
	char *p = malloc(size+8), *q = p, *t = s;
	while ( *(s) != '\0') {
		*q = *s;
		q++;
		s++;
	} 
	p[size-1] = '\0';
	free(t);
	return p;
}

int condition1( char c)
{
	if (c=='&')
	       	return amp;
	if (c=='>')
	       	return more;
	if (c=='<')
	       	return less;
	if (c=='|')
	       	return pip;
	return 0;
}

int condition2( char*s , char last)
{
	if (s == NULL )
       		return ok; 
	if (strequ(s, ">>"))
       		return ok; 
	if (isspace(last))
       		return ok; 
	if (condition1(s[0])!=0)
       		return no; 
	if (condition1(last)!=0)
       		return no; 
	return ok;
}

void outputlist( struct list *k)
{
	int i;
	char *s;
	while (k != NULL) {
		s = k->elem;
		i = 0;
		while (s[i]) {
			if (s[i] != '"') {
				putchar( s[i] );
			}	
			i++;	
		}
		if ( k->next != NULL)
		       	putchar ('\n');
		k = k->next;
	}
	putchar('\n');
}

char* gsttring( int v, int *last, int *i, int* carry)
{
	char *s;
	int size = 1, alloc = 4;
	int c = getchar();
	s = malloc(alloc);
	if (condition1(v)!=0) {
		if (*i==0)
	       		*carry = 1;  
		if (v == '>' && c == '>'){
			s[1] = s[0] = '>';
			s[2] = '\0';
		} else {
			s[0] = v;
			s[1] = '\0';
		}
		*last = c;
		return s;
	}		
	if ( v != '"' ) {
		s[0] = v;
		s[1] = 0;
	}else{
		size = s[0] = 0;
		*i =1;
		alloc = 1;
	}
	while(((!isspace(c) &&! condition1(c))!=0 || *i==1) && c!='\n'&&c!=EOF)
	{
		*i = kb (c, *i);
		if ( c != '"' ) {
			if (alloc <= size+1 ) {
				alloc *= 2;
				s = reallc(s, alloc * sizeof(char));
			}
			s[size++] = c;
			s[size] = '\0';
		}	
		c = getchar();
	}
	*last = c;
	return s;
}

struct list *build(char *k, int *n, int *m)
{
	int i= 0, g = 0, carry;
	int c = ' ', last = ' ';
	char *s = NULL;
	struct list *l1 = NULL, *l2;
	while (c != EOF && c !='\n') {
		carry = 0;
		if (condition2(s, last)==1 ) { 
			c = getchar() ;
		}
		i = kb(c, i);
		if (!isspace(c) || i == 1) {
			s = gsttring(c, &last, &i, &carry);
			if (l1 == NULL) {
				l2 = data(s);
				l1 = l2;
				g++;
			} else {
				l2->next = data(s);
				l2 = l2->next;
				g++;
			}
			l2->argc = carry;
			c = last;
			if (last == '\n' || last == EOF) {
				break;
			}
		}
	}
	*m = g;
	*k = c;
	*n = i;
	return l1;
}

void printfvector ( char** A)
{
	int i=0;
	printf(" --- ");
	while(A[i]!=NULL) {
		printf("%s  ", A[i]);
		i++;
	}
	printf("\n");
}

void output( struct param *k)
{
	while (k != NULL) {
		printfvector(k->vec);
		k = k->next;
	}	
}

void freelist( struct list **k)
{
	struct list *q;
	while ( (*k) != NULL) {
		q = *k;
		free( (*k)->elem );
		(*k) = (*k)->next;
		free(q);
	}
}

void freeparam( struct param **A)
{
	struct param *B;
	while ( (*A) != NULL) {
		B = *A;
		free((*A)->argc);
		free((*A)->in);
		free((*A)->out1);
		free((*A)->out2);
		//printf("freevector\n");
		if ((*A)->vec) 
			freevector((*A)->vec);
		(*A) = (*A)->next;
		free( B);
	} 
}

char** vector(struct list *l1, int m)
{
	char **k;
	char *s, *p;
	int i;
	k = malloc(sizeof(char*)*(m+1));
	for(i = 0; i < m; i++) {
		s = l1->elem;
		k[i] = malloc(sizeof(char)*(length(s)+1));
		p = k[i];
		while( *s != '\0' ) {
			*p = *s;
			p++;
			s++;
		}
		*p = '\0';
		l1 = l1->next;
	}
	k[m] = NULL;
	return k;
}

void check_ampersand(struct param **A)
{
	int i=0;
	while ((*A)->vec[i]) {
		if (strequ((*A)->vec[i], "&") && i != (*A)->max - 1) {
			(*A)->error = 1;
			perror("error use ampersand");
		}
		i++;
	}
	if ( strequ( (*A)->vec[(*A)->max - 1], "&" )) {
		(*A)->amp = 1;
		(*A)->vec[(*A)->max -1] = NULL;
		(*A)->max--;
	}
} 

int assignment(struct param **A, char* s, int i)
{
	char* s1 = (*A)->vec[i+1];
	if (strequ(s,"<")) {
		if ((*A)->in==NULL) {
			(*A)->in =strdupe(s1);
			//printf("redirect from file '%s'\n",(*A)->in);
		} else {
			(*A)->error = 1;
			perror("input redirection error");
			return 0;
		}
	}
	if (strequ(s,">")) { 
		if ((*A)->out1==NULL) {
			(*A)->out1 = strdupe(s1);
			//printf("redirect to file '%s'\n",(*A)->out1);
		} else {
			(*A)->error = 1;
			perror("output redirection error");
			return 0;
		}
	}
	if (strequ(s,">>")) {
		if ((*A)->out2==NULL) {	
			(*A)->out2 = strdupe(s1);
			//printf("redirect to file '%s'\n",(*A)->out2);
		} else {
			(*A)->error = 1;
			perror("output redirection error");
			return 0;
		}
	}
	return 1;
}

void check(struct param **A, int *m, char* s)
{
	int i=0;
	while ((*A)->vec[i]) {
		if(strequ((*A)->vec[i], s) && (*A)->argc[i] =='1') {
			if(i!=(*A)->max-1 && !condition1((*A)->vec[i+1][0]))
			{
				if (assignment(A,s,i) && (*m-1>i))
			       	*m = i ; 
			} else {
				(*A)->error = 1;
			}
		}
		i++;
	}
}

void makevector( struct param** A, int i)
{
	char** p; 
	int j=0;
	if (i != (*A)->max) {
		p = malloc(sizeof(char*) *(i+1));
		while (!condition1( (*A)->vec[j][0]) ) {
			p[j] = (*A)->vec[j];
			j++;
		}
		p[j] = NULL;
		(*A)->vec = p;
		(*A)->max = i;
	}
}
		
void check1( struct param **A)
{
	int i;
	check_ampersand(A);
	i = (*A)->max;
	check( A, &i, "<");
	check( A, &i, ">");
	check( A, &i, ">>");
	if ((*A)->out1 && (*A)->out2) {
		(*A)->error = 1;
		perror("2 types of output redirection");
	}
	makevector( A, i);
}

void input_output1( struct param ** A)
{
	int g, h;
	if ((*A)->error == 1 )
       		return ;
	if ((*A)->in){
		h = open((*A)->in, O_RDONLY);
		if (h<0) {
			printf("error open\n");
		} else {
			dup2(h,0);
			close(h);
		}
	}
	if ((*A)->out1) {
		g = creat((*A)->out1,0666);
		if (g<0) {
			printf("error creat\n");	
		} else {
			dup2(g,1);
			close(g);
		}
	}
	if ((*A)->out2) {
		g = open((*A)->out2, O_APPEND|O_WRONLY);
		if (g<0) {
			printf("error open\n");
		} else {
			dup2(g,1);
			close(g);
		}
	}
}

void addpid( struct processes *ps, int pid)
{
	ps->count ++;
	if (ps->count >=ps->size)
	{
		ps->size *=2;
		ps->pids = (int*)reallc((void*)ps->pids, ps->size*sizeof(int));
	} 
	ps->pids[ps->count-1]=pid;
}

void exec(struct param **A, struct processes *ps, int fd0)
{
	pid_t pid;
	pid = fork();
	switch( pid ) {
		case -1:
			printf("Error paralleling\n");
			break;
		case 0:
			close(fd0);
			if((*A)->in || (*A)->out1 || (*A)->out2) {
				input_output1(A);
			}
			execvp((*A)->vec[0],(*A)->vec);
			printf("Error using execvp command\n");
			exit(0);
			break;
		default:
			if (!(*A)->amp){
				addpid(ps,pid);
			}
	}
}

void parallel(struct param **A, struct processes *ps, int fd0)
{
	int t;
	char* s;
	check1(A);
	//printfvector((*A)->vec);
	if( (*A)->vec[0] == NULL) 
		return;
	if (!(*A) -> error) {
		s = (*A)->vec[0];
		if( strequ(s, "cd") &&  (*A)->vec[1] != NULL) {
			s = (*A)->vec[1];
			t = chdir(s) ;
			if (t == -1 ){
		      		printf("Error name directory\n");
			}
		} else {	
			exec(A,ps,fd0);
		}
	}
}

void delete_zombie( void )
{
	int f, t;
	do  {
		t = waitpid(-1, &f, WNOHANG);
		if (t>0) { 
			//printf("A process has successfully finished\n");
		}
	} while( t>0 );
}

int seach(char** argv)
{
	int i=0;
	while (argv[i]) {
		if (strequ(argv[i],"|")) {
			return i;
		}
		i++;
	}
	return -i;
}

void FillZeros(struct param *A)
{
	A->max = 0;
	A->amp = 0;
	A->in = NULL;
	A->out1 = NULL;
	A->out2 = NULL;
	A->vec = NULL;
	A->error = 0;
	A->next = NULL;
	A->argc = NULL;
}

void ifconveer(char** argv, struct param **A, char* argc)
{
	struct param *B = *A;
	int m;
	while (*argv){
		B->max = seach(argv);
		m = 0;
		if (B->max>0) {
			B->argc = malloc(sizeof(char)*(B->max+1));
			B->vec = malloc(sizeof(char*)*(B->max+1));
			while (seach(argv)>0 && !strequ(*argv, "|")) {
				B->argc[m] = argc[0];
				B->vec[m++] = strdupe(*argv);
				argv++;
				argc++;
			}
		} else {
			B->max = -B->max;
			B->argc=malloc(sizeof(char)*(B->max+1));
			B->vec=malloc(sizeof(char*)*(B->max+1));
			while (*argv) {
				B->argc[m] = argc[0];
				B->vec[m++] = strdupe(*argv);
				argc++;
				argv++;
			}
			B->next = NULL;
			B->argc[B->max]='\0';
			B->vec[B->max] = NULL;
			check_ampersand(&B);
			(*A)->amp = B->amp;
			return;
		}
		B->vec[B->max] = NULL;
		B->argc[B->max] = '\0';
		if (*argv) {
			argc++;
			argv++;
			B->next = malloc(sizeof(struct param));
			FillZeros(B->next);
		}
		B = B->next;
	}
}

void allfree(struct list **k, struct param **A, char** argv, char* argc)
{
	//printf("freeargc\n");
	if (argc!=NULL)
       		free(argc);
	//printf("freelist\n");
	if (k!=NULL)
       		freelist(k);
	//printf("freevector\n");
	if (argv!=NULL)
       		freevector(argv);
	//printf("freeparam\n");
	if (A!=NULL)
       		freeparam(A);
}

void print_commands(struct param *A) {
 	if (A== NULL)
   		 return;
	// printf("vector:");
	// printfvector(A->vec);
   	if (A->in != NULL)
       		printf("<%s\n", A->in); 
   	if (A->out1 != NULL)
       		printf("<%s\n", A->out1);
   	if (A->out2 != NULL)
       		printf("<<%s\n", A->out2);
   	if (A->next != NULL)
       		print_commands(A->next);
	if (A->amp)
       		printf("%d\n",A->amp); 
}

char* carrystring(struct list *k, int m)
{
	char *v = malloc(sizeof(int)*(m+1));
	int i;
	for(i=0; i<m; i++){
		v[i] = k->argc+'0';
		k = k->next;
	}
	v[m] = '\0';
	return v;
}

void rmpid(struct processes *ps, int pid)
{
	int i,j;
	//fprintf(stderr, "delete %d \n", pid);
	for (i=0;i < ps->count; i++)
	{
		if (pid == ps->pids[i])
		{
			for (j=i+1; j<ps->count; j++){
				ps->pids[j-1]=ps->pids[j];
			}
			ps->count--;
		}
	}
}

void main_part2(struct param *A, int ampersand, struct processes *ps)
{
	int m, s0, s1, fd[2];
	print_commands(A);
	m=0;
	while (A!=NULL && A->error == 0 ) {
		A->amp = ampersand;
		s0 = dup(0);
		s1 = dup(1);
		if (m>0) {
			dup2(fd[0], 0);
			close(fd[0]);
		}
		fd[0] = -1;
		if (A->next) {
			pipe(fd);
			dup2(fd[1], 1);
			close(fd[1]);
		} 
		if (A->max && A->vec[0][0] != EOF) { 
			parallel(&A,ps, fd[0]);
		}
		dup2(s1, 1);
		close(s1);
		dup2(s0 ,0);
		close(s0);
		A = A->next;
		m++;
	}
}

int main()
{
	struct param *A, *first;
	int m, pid;
	char **argv = NULL,*argc, c=' ';
	struct list *k;
	struct processes ps;
	ps.count = 0;;
	ps.size = 10;
	ps.pids = malloc(ps.size * sizeof(int));
	while ( c!= EOF) {
		printf(":::");
		A = malloc(sizeof(struct param));
		FillZeros(A);
		first = A;
		k = build( &c, &(A->error), &m);
		if (m>0 && k->elem[0] != EOF) {
			if (A->error == 0) {
				argv = vector(k, m);
				argc = carrystring(k, m);
				if (seach(argv)>0) {
					ifconveer(argv ,&A, argc); 
				} else { 
					A->max = m ;
					A->argc = strdupe(argc); 
					A->vec = vectordup(argv,m);
				}
				main_part2(A,first->amp, &ps);
				while (ps.count != 0){
					pid = waitpid(0,NULL,WNOHANG);
					if (pid >0){
						rmpid(&ps, pid);
					}
				} 
			} else 
				perror("Unmatching quotes\n");
			allfree(&k , &first, argv, argc);
		}
	}
	//printf("delete_zombie\n");
	delete_zombie();
	return 0;
}
