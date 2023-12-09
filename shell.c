#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>


#define TRUE 1
#define MAXLIST 1000

void type_prompt(char* str){
	char cwd[1024]; 
	char cwd1[]={"Dir:"}; 
	char cwd2[]={">"}; 
	strncat(str, cwd1, 1024);
	strncat(str, getcwd(cwd, sizeof(cwd)),1024);
	strncat(str,cwd2,1024);
}

void clear(char** parsedArgs){
    int clear = 1;
        while(parsedArgs[clear]!=NULL){
            parsedArgs[clear]=NULL;
            clear++;
        }
}

//切割指令與參數
void parseSpace(char* str, char** parsed) { 
    for (int i = 0; i < MAXLIST; i++) { 
        parsed[i] = strsep(&str, " "); 
        if (parsed[i] == NULL) 
            break; 
        if (strlen(parsed[i]) == 0)
            i--; 
    } 
} 

//切割指令與參數
void slice(char* str, char* word, char** collect, char** sec_collect, int opt){
	if(opt==0){
		int j = 0;
		collect[j] = strtok(str, word);
		collect[j+1] = word;
		j++;
		while(collect[j]!=NULL){
			j++;
			collect[j] = strtok(NULL, word);
			
			if (collect[j] == NULL)
				break; 
			else if (collect[j+1] != NULL){
				collect[j+1] = word;
				j++;
			}
		}
	}else if(opt==1){
		int i = 0;
		int j = 0;
		while(collect[i]!=NULL){
			char *saveptr= NULL;
			char *substr= NULL;
			
			substr = strtok_r(collect[i], word, &saveptr);
			sec_collect[j] = substr ;
			j++;
			while(substr ){
				substr = strtok_r(NULL, word, &saveptr);
				if(substr ){
					sec_collect[j] = substr;
					j++;
				}
			}
			i++;
		}
	}else if(opt==-1){
		int j = 0;
		collect[j] = strtok(str, word);
		while(collect[j]!=NULL){
			j++;
			collect[j] = strtok(NULL, word);
			if (collect[j] == NULL)
				break; 
		}
	}
}

char read_command(char* buf, char** parsedArgs, char** parsedArgsPiped){
	char* strpiped[MAXLIST];
	char* sec_strpiped[MAXLIST];
	while(buf){
		int buflen = strlen(buf);

		int i = buflen-1;
  		if( buf[i] == '\n') 
      		buf[i] = '\0';
		
		int j = 0;
		strpiped[j] = strtok(buf, "|");
		while(strpiped[j]!=NULL){
			j++;
			strpiped[j] = strtok(NULL, "|");
			if (strpiped[j] == NULL)
				break; 
		}
	
		if(strpiped[0] != NULL){
			if(strchr(strpiped[0], '>')!= NULL && strchr(strpiped[0], '<')== NULL){
				slice(strpiped[0], ">",  sec_strpiped, NULL, 0);
				slice(NULL, " ",  sec_strpiped, parsedArgs, 1);
				return 3;
				exit(0);
			}else if(strchr(strpiped[0],'<')!= NULL){
				slice(strpiped[0], "<",  sec_strpiped, NULL, 0);
				slice(NULL, " ",  sec_strpiped, parsedArgs, 1);
				if(parsedArgs[1] ==NULL){
					printf("Not enough  input argument\n");
					return 0;
					exit(0);
				}
				return 4;
				exit(0);
			}	else if(strchr(strpiped[0], '&')!= NULL){
				slice(strpiped[0], "&",  sec_strpiped, NULL, -1);
				slice(NULL, " ",  sec_strpiped, parsedArgs, 1);
				return 5;
				exit(0);
			}
		}
		
			if (strpiped[0] == NULL)
				return 0;
			else if(strpiped[1] == NULL){ //No pipe
				parseSpace(strpiped[0], parsedArgs);
				return 1;
			}
			else{// pipe
				parseSpace(strpiped[0], parsedArgs);
				parseSpace(strpiped[1], parsedArgsPiped);
				return 2;
			} 	
    	
	}
}

int ownCmdHandler(char** parsed){ 
    int CmdNum = 2, i, check = 0; 
    char* Cmds[CmdNum]; 
    char* username; 

    Cmds[0] = "exit"; 
    Cmds[1] = "cd"; 

    for (i = 0; i < CmdNum; i++) { 
        if (strcmp(parsed[0], Cmds[i]) == 0) { 
            check = i + 1; 
            break; 
        } 
    } 

    switch (check) { 
    case 1: 
        exit(0); 
    case 2: 
        chdir(parsed[1]); 
        return 1; 
    default:
        break; 
    } 

    return 0; 
} 

//signal handler
void exec(char** Args){
	
	pid_t pid = fork();
	
	if (pid!= 0){
		waitpid(pid, NULL, 0);
	}else{
		if(execvp(Args[0], Args)<0){
			printf("Command not found");
			exit(0);
		}
	}
}

//pipe liene
void pipeline(char** process1, char** process2){
	int fd[2];
	//char* buf1 =   malloc(strlen("/bin/")+strlen(process1[0])+1);
	//char* buf2 =   malloc(strlen("/bin/")+strlen(process2[0])+1);
	//stpcpy(buf1, "/bin/");
	//strcat(buf1,  process1[0]);
	//stpcpy(buf2, "/bin/");
	//strcat(buf2,  process2[0]);
	pipe(&fd[0]);
	
	pid_t pid1 = fork();

	if (pid1 == 0){
		close(fd[0]); //關掉fd(0)的read end 因為不需要從pipe讀取
		close(STDOUT_FILENO);
		dup(fd[1]);
		close(fd[1]);
		//execl(buf1, process1[1],  (char *) 0);
		if(execvp(process1[0], process1)<0){
			printf("Command not found");
			exit(0);
		}
	}else{
		pid_t pid2 = fork();
		
		if(pid2 == 0){
			close(fd[1]);
			close(STDIN_FILENO);
			dup(fd[0]);
			close(fd[0]);
			//execl(buf2, process2[1],  (char *) 0);
			if(execvp(process2[0], process2)<0){
				printf("Command not found");
				exit(0);
			}
		}else{
			waitpid(pid1, NULL, 0);
			waitpid(pid2, NULL, 0);
		}
	}
}

static inline int move_descriptor(int oldfd, int newfd){
    if (oldfd == -1 || newfd == -1) {
        if (oldfd != -1)
            close(oldfd);
        if (newfd != -1)
            close(newfd);
    }

    if (oldfd == newfd)
        return 0;

    if (dup2(oldfd, newfd) == -1) {
        const int  saved_errno = errno;
        close(oldfd);
        close(newfd);
    }

    if (close(oldfd) == -1) {
        const int  saved_errno = errno;
        close(newfd);
    }

    return 0;
}

void fileIO(char** cmd, char* inp, char* outp, int option){
	
	//char* cmd1[MAXLIST];
	pid_t pid = fork();
	pid_t pid2;
	char* cmd1[]={cmd[0],NULL};
	switch(pid){
		case 0:
			if(option==0){
				int fd;
				fd=open(outp, O_CREAT | O_TRUNC | O_WRONLY, 0644);
				dup2(fd, STDOUT_FILENO);
				close(fd);
				if(execvp(cmd[0], cmd)==-1){
					printf("Error\n");
					kill(getpid(), 0);
				}
			}else if(option==1){
				int fd;
				fd=open(inp, O_RDONLY);
				if (fd == -1) {
					printf("No exsiting file\n");
					exit(0);
				}
				if (move_descriptor(fd, STDIN_FILENO)) {
				}
				//dup2(fd, STDIN_FILENO);
				//close(fd);
				
				fd=open(outp, O_CREAT | O_TRUNC | O_WRONLY, 0644); 
				dup2(fd, STDOUT_FILENO);
				close(fd);
				
				if(execvp(cmd[0], cmd)==-1){
						printf("Error\n");
						kill(getpid(), 0);
					}
				
			}
			break;
		case -1:
			printf("Child process could not be created\n");
			return;
			break;
		default:
			waitpid(pid, NULL, 0);
	}
		waitpid(pid, NULL, 0);
		
	
}

void beforeCMD(char** args, char** args_aux){
	int j =0;
	while ( args[j] != NULL){
		if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0)){
			break;
		}
		args_aux[j] = args[j];
		j++;
	}
}

void runBg(char **args){	 
	 int err = -1;
	 
	 pid_t pid = fork();
	 if(pid==-1){
		 printf("Child process could not be created\n");
		 return;
	 }

	if(pid==0){
		
		signal(SIGINT, SIG_IGN);
		
		char* currentDirectory = (char*) calloc(1024, sizeof(char));
		setenv("parent",getcwd(currentDirectory, 1024),1);	
		
		if (execvp(args[0],args)==err){
			printf("Command not found");
			kill(getpid(),SIGTERM);
		}
		
	 }
}
 

void main(){
	char* username = getenv("USER"); 
    printf("Hello @%s", username);
    printf("\nWelcome to 1104526shell\n"); 
	printf("\n"); 
	
    while (TRUE)
    {
		char* input;
		char* parsedArgs[MAXLIST], *parsedArgsPiped[MAXLIST];
		char* args[MAXLIST];
		int type = 0;
		int count = 0;
		char cwd[1024]; 
		
		type_prompt(cwd);
		input = readline(cwd);
		cwd[0]='\0';
		add_history(input);
		
		int i = strlen(input)-1;
  		if( input[i] == '\n') 
      		input[i] = '\0';
		
		if(strtok(input,"\n\t")==NULL) continue;
		type = read_command(input, parsedArgs, parsedArgsPiped);
		
		if(type != 0 && type != 1 && type != 2){
			beforeCMD(parsedArgs,  args);
		}
		
		if (type == 0)
		{
			/* code */
		}else if (type == 1)
		{
			if (ownCmdHandler(parsedArgs)){
				/* code */
			}else{
				exec(parsedArgs);
			}
		}else if(type == 2){
			pipeline(parsedArgs, parsedArgsPiped);
		}else if(type == 3){
			while(parsedArgs[count]!=NULL){
				if (strcmp(parsedArgs[count],">") == 0){
					if (parsedArgs[count+1] == NULL){
						printf("Not enough input arguments\n");
						exit(-1);
					}
					fileIO(args,NULL,parsedArgs[count+1],0);
				}
				count++;
			}

		}else if(type == 4){
			while(parsedArgs[count]!=NULL){
				if (strcmp(parsedArgs[count],"<") == 0){
					if (parsedArgs[count+1] == NULL){
						printf("Not enough input arguments\n");
						exit(-1);
					}else if(strchr(parsedArgs[count+1], '>')!= NULL ){
						char* sec_parsedArgs[MAXLIST];
						slice(NULL,">",parsedArgs, sec_parsedArgs, 1);
						fileIO(args,sec_parsedArgs[count+1],sec_parsedArgs[count+2], 1);
					}else{
						fileIO(args,parsedArgs[count+1],NULL, 1);
					}
					//fileIO(args,parsedArgs[count+1],parsedArgs[count+1], 1);
					
				}
				count++;
			}
		}else if(type == 5){
			runBg(parsedArgs);
		}
		clear(parsedArgs);
    }
    