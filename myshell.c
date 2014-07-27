/* By JackalDire, Jan 29 2010 
 * Tested on Linux Kernel 2.6.32, gcc 4.4.3 */
#include <unistd.h>
#include <sys/wait.h>
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h> 
 
#define LINE_MAX 8192
#define ARG_MAX 1024
#define ARG_NR_MAX 32
#define PIPE_NR_MAX 32	
 
#define CHKERR(ret, msg) if (ret < 0) {		\
        fprintf(stderr, "ERROR : \"%s\", %s\n",	\
                msg, strerror(errno)); 	\
        exit(-1);\
    } 
int pipfds[PIPE_NR_MAX][2];
char *args[PIPE_NR_MAX][ARG_NR_MAX + 1];
char *pipes[PIPE_NR_MAX+1];	
extern char ** environ;
 
int pipNumber = 0;
 
char line[LINE_MAX + 1];


void pr_exit(const char * name, int status)
{
    if (WIFEXITED(status)) // exit normally
        return;
    else if (WIFSIGNALED(status))
        fprintf(stderr, "%s exit abnormally, signal %d caught%s.\n",
                name, WTERMSIG(status),	
#ifdef WCOREDUMP
            WCOREDUMP(status) ? " (core file generated)" : "");
#else
            "");
#endif
    else if (WIFSTOPPED(status))
        fprintf(stderr, "child stopped, signal %d caught.",
                WSTOPSIG(status));
}

void pipe_parser(char * cmd){
	char *res;
	size_t cnt = 0;
	//tokenize the command string by '|'
	while((res = strsep(&cmd,"|")) != NULL){
		printf("%s\n",res);
		pipes[cnt++] = strdup(res);
	}
	pipes[cnt] = NULL;
	pipNumber = cnt;
}

void parse_command(char * cmd,char **arg)
{
    char * res;
    size_t cnt = 0;
    /* tokenize the command string by space */
    while ((res = strsep(&cmd, " ")) != NULL) {
        printf("%s\n", res);
        arg[cnt++] = strdup(res);
    }
    arg[cnt] = NULL;
    
}

void multi_fork(char ** pipes){	//construct pipe for each pair of command
	int  b=0;
	int i=0;
	for(i=0;i<pipNumber;i++)
		parse_command(pipes[i],args[i]);
	
		
	for(i=0;i<pipNumber; i++){
		if(i!=pipNumber-1)
			pipe(pipfds[i]);
		//printf("caonima222\n");
		if(fork() == 0){
			if(0!=i){
				//last pipe
				int *fd = pipfds[i-1];
				dup2(fd[0],0);
				close(fd[0]);
				close(fd[1]);
			}
			if(pipNumber-1!=i){
				//next pipe
				int *fd = pipfds[i];
				fd = pipfds[i];
				dup2(fd[1],1);
				close(fd[0]);
				close(fd[1]);
			}
				int r = execvp(args[i][0], args[i]);
            	printf("ret : %dn", r);
            	CHKERR(r, args[i][0]);
			
			break;	
		}
		else
			if(pipfds[i][0]!=0)
				//close(pipfds[i][0]);
				close(pipfds[i][1]);
	}
	//father
		int stat;
		while(-1 != wait(&stat));
		pr_exit(args[pipNumber-1][0], stat);
}
 

int main(int argc, char * argv[])
{
    char c;
    size_t idx;
    int r;
    int status;
 
    while (1) {			//get the input from the keyboard
    	pipNumber=0;
        idx = 0;
        bzero(line, LINE_MAX + 1);
 
        c = fgetc(stdin);
        while (c && c != '\n') {
            line[idx++] = c;
            c = fgetc(stdin);
        }
        
        pipe_parser(line);
        multi_fork(pipes);

    }
    return 0;
}

/*
void myrun()
{
	int i, status, fd;
	pid_t pid;
 
	for (i=0; info[i].prog!=NULL; i++){
		if (info[i+1].prog != NULL){//后面有程序，则创建管道myfd[i][]
			if (pipe(&myfd[i]) < 0) //也可以是pipe(myfd[i])，数组做优质变成指针
				perror("pipe");
		}
		if ( (pid=fork()) < 0) 
			perror("fork"); 
		else if (pid == 0){ 
			if (myfd[i][0] != 0){//本进程和后一进程之间的管道读端 
				close(myfd[i][0]);//关闭读端 
				dup2(myfd[i][1],STDOUT_FILENO); 
				} 
			if (i > 0)
				if (myfd[i-1][0] != 0){//本进程与前一进程之间的管道读端
					close(myfd[i-1][1]);//关闭写端
					dup2(myfd[i-1][0],STDIN_FILENO);
				}
			if (strlen(info[i].infile) != 0){ //不能是 if (info[i].infile != NULL)，因为infile不是指针
				if ( (fd = open(info[i].infile,O_RDONLY)) < 0){
					perror("open");
					exit(-1);
				}
				dup2(fd,STDIN_FILENO);
				close(fd);
			}
 
			if (strlen(info[i].outfile) != 0){
				if ( (fd = open(info[i].outfile, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) < 0){
					perror("open");
					exit(-1);
				}
				dup2(fd,STDOUT_FILENO);
				close(fd);
			}
 
		execvp(info[i].prog, info[i].arg);
		perror("exec");
	}
	else
		//wait(&status);//不可以在这等待子程序终止，应该在for结束之后
		if (myfd[i][0] != 0){//本进程和后一进程之间的管道读端
		//close(myfd[i][0]);//如果父进程关闭读端，读端引用变为0,因为第二个子进程还没产生,这样的话导致往写端write的进程终止
			close(myfd[i][1]);
}
 
}
 
//waitpid(pid, &status, 0);这个会阻塞
wait(&status);
}
*/
