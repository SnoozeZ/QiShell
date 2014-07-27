/* Developed by Snooze 
 * 2014-07-27 */
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h> 
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
char *redirect_left[2];
char *redirect_right[2];

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

char* redirect_parser(char *cmd){
	char *res;
	size_t cnt =0;
	while((res=strsep(&cmd,">"))!=NULL){
		//printf("%s\n",res);
		redirect_right[cnt++] = strdup(res);
	}
	cmd = redirect_right[0];
	
	cnt = 0;
	res = NULL;
	while((res=strsep(&cmd,"<"))!=NULL){
		//printf("%s\n",res);
		redirect_left[cnt++] = strdup(res);
	}
	return redirect_left[0];
}

void pipe_parser(char * cmd){
	char *res;
	size_t cnt = 0;
	//tokenize the command string by '|'
	while((res = strsep(&cmd,"|")) != NULL){
		//printf("%s\n",res);
		pipes[cnt++] = strdup(res);
	}
	pipes[cnt] = NULL;
	pipNumber = cnt;
}

void command_parser(char * cmd,char **arg)
{
    char * res;
    size_t cnt = 0;
    /* tokenize the command string by space */
    while ((res = strsep(&cmd, " ")) != NULL) {
        //printf("%s\n", res);
        arg[cnt++] = strdup(res);
    }
    arg[cnt] = NULL;
    
}


void multi_fork(char ** pipes){	//construct pipe for each pair of command
	int i=0;	
	for(i=0;i<pipNumber; i++){
		if(0 == i || pipNumber-1 == i)
			pipes[i] = redirect_parser(pipes[i]);
		command_parser(pipes[i],args[i]);

		if(i!=pipNumber-1)
			pipe(pipfds[i]);
		if(fork() == 0){
			if(0 == i && redirect_left[1]){
				int fd=open(redirect_left[1],O_RDONLY);
				dup2(fd,0);
				close(fd);
			}
			if(pipNumber-1 == i && redirect_right[1]){
				int fd_out =open(redirect_right[1],O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
				dup2(fd_out,1);
				close(fd_out);
			}
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
    system("clear");
 	printf("==Welcome to QiShell==\n");
    while (1) {			
    	pipNumber=0;
        idx = 0;
        bzero(line, LINE_MAX + 1);
 
        c = fgetc(stdin);
        while (c && c != '\n') {	//get the input from the keyboard
            line[idx++] = c;
            c = fgetc(stdin);
        }
        pipe_parser(line);
        //redirect_parser(line);//remove the ">","<" and the redirect file name in the line (The "line" is changed!)
        multi_fork(pipes);	//

    }
    return 0;
}

//http://www.cnblogs.com/weidagang2046/p/io-redirection.html
//http://kenby.iteye.com/blog/1165923
//http://whitepig.sinaapp.com/archives/124
