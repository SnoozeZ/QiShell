/* Developed by Snooze 
 * 2014-07-27 */
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h> 
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
int pipfds[PIPE_NR_MAX][2];					//the array of pipes 
char *args[PIPE_NR_MAX][ARG_NR_MAX + 1];	//the command name and arguements's tokens of each command
char *commands[PIPE_NR_MAX+1];				//the array of each command
char *redirect_left[2];						//handle the redirect "<"
char *redirect_right[2];					//handle the redirect ">"
 
int pipNumber = 0;							//amount of pipes			
char line[LINE_MAX + 1];					//input got from the keyboard


void pr_exit(const char * name, int status)	//function to handle error
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

char* redirect_parser(char *cmd){	//function to parse ">" and "<"
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

void pipe_parser(char * cmd){	//function to parse "|"
	char *res;
	size_t cnt = 0;
	//tokenize the command string by '|'
	while((res = strsep(&cmd,"|")) != NULL){
		//printf("%s\n",res);
		commands[cnt++] = strdup(res);
	}
	commands[cnt] = NULL;
	pipNumber = cnt;
}

void command_parser(char * cmd,char **arg)	//function to parse " " 
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


void multi_fork(char ** commands){	//construct pipe for each pair of command, and excute it
	int i=0;	
	for(i=0;i<pipNumber; i++){
		if(0 == i || pipNumber-1 == i)
			commands[i] = redirect_parser(commands[i]);
		command_parser(commands[i],args[i]);

		if(i!=pipNumber-1)
			pipe(pipfds[i]);
		if(fork() == 0){		
			if(0 == i && redirect_left[1]){		//redirect "<"
				int fd_in=open(redirect_left[1],O_RDONLY);
				dup2(fd_in,0);
				close(fd_in);
			}
			if(pipNumber-1 == i && redirect_right[1]){	//redirect ">"
				int fd_out =open(redirect_right[1],O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
				dup2(fd_out,1);
				close(fd_out);
			}
			if(0!=i){			//if not the first command
				//last pipe
				int *fd = pipfds[i-1];
				dup2(fd[0],0);
				close(fd[0]);
				close(fd[1]);
			}
			if(pipNumber-1!=i){	//if not the last command
				//next pipe
				int *fd = pipfds[i];
				fd = pipfds[i];
				dup2(fd[1],1);
				close(fd[0]);
				close(fd[1]);
			}
			if(!strcmp(args[i][0],"cd")){	//if the command is "cd", adopt chdir() to handle it
				chdir(args[i][1]);
				}
			else{
				int r = execvp(args[i][0], args[i]);	//execute the command
            	printf("ret : %d\n", r);
            	CHKERR(r, args[i][0]);
            }
			
			break;	
		}
		else
			if(pipfds[i][0]!=0)		//close the pipe of father process
				//close(pipfds[i][0]);
				close(pipfds[i][1]);
	}
	//father
		int stat;
		while(-1 != wait(&stat));
		//pr_exit(args[pipNumber-1][0], stat);	//show the error info
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
    	redirect_left[0]=redirect_left[1] =NULL;
    	redirect_right[0]=redirect_right[1] =NULL;
    	pipNumber=0;
        idx = 0;
        bzero(line, LINE_MAX + 1);
 
        c = fgetc(stdin);
        while (c && c != '\n') {	//get the input from the keyboard
            line[idx++] = c;
            c = fgetc(stdin);
        }
        
        pipe_parser(line);		//parse the input by "|" 
        multi_fork(commands);	//fork child processes to execute the commands, and construct the pipes between them if required

    }
    return 0;
}

//http://www.cnblogs.com/weidagang2046/p/io-redirection.html
//http://kenby.iteye.com/blog/1165923
//http://whitepig.sinaapp.com/archives/124
