#include "/c/cs323/Hwk5/process-stub.h"

int *pids;
int numPids = 0;
int processInner(const CMD *cmdList, int BG);

void reapZombies(void)
{
	int pid;
	int status = 0;

	signal(SIGINT, SIG_IGN);
	for(int i = 0; i < numPids; i++)
	{
		if((pid=waitpid(pids[i], &status, WNOHANG))>0)
		{
			fprintf(stderr, "Completed: %d (%d)\n", pid, STATUS(status));
		}
	}
	signal(SIGINT, SIG_DFL);
	return;
}

int setStatus(int status)
{
	char buffer[128];
	sprintf(buffer, "%d", status);
	setenv("?", buffer, 1);          // set $? to status
    return status;
}

int setLocals(const CMD *cmdList)
{
	for(int i = 0; i < cmdList->nLocal; i++)
	{
		if(setenv(cmdList->locVar[i], cmdList->locVal[i], 1) < 0)
		{
			int status = errno;
			perror("setenv");
			return status;
		}
	}
	return 0;
}

int redirect(const CMD *cmdList)
{
	int in, out;
	int status = 0;

	if(cmdList->fromType != NONE && cmdList->fromFile != NULL)
	{
		if(cmdList->fromType == RED_IN)
		{
			in = open(cmdList->fromFile, O_RDONLY);
			if(in >= 0)
			{
				dup2(in, STDIN_FILENO);
				close(in);
			}
			else //open failed
			{
				status=errno;
				perror("open");
				return status;
			}
		}
		else if(cmdList->fromType == RED_IN_HERE)
		{
			char template[] = "templateeXXXXXX";
			in = mkstemp(template);
			if(in < 0)
			{
				status=errno;
				perror("mkstemp");
				return status;
			}
			write(in, cmdList->fromFile, strlen(cmdList->fromFile));
			close(in);
			
			in = open(template, O_RDONLY);
			if(in < 0)
			{
				status=errno;
				perror("open");
				return status;
			}
			dup2(in, STDIN_FILENO);
			close(in);
		}
	}

	if(cmdList->toType != NONE && cmdList->toFile != NULL)
	{
		if(cmdList->toType == RED_OUT)
		{
			out = open(cmdList->toFile, O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR);
			if(out >= 0)
			{
				dup2(out, STDOUT_FILENO);
				close(out);
			}
			else
			{
				status=errno;
				perror("open");
				return status;
			}
		}
		else if(cmdList->toType == RED_OUT_APP)
		{
			out = open(cmdList->toFile, O_WRONLY|O_APPEND|O_CREAT, S_IWUSR|S_IRUSR);
			if(out >= 0)
			{
				dup2(out, STDOUT_FILENO);
				close(out);
			}
			else
			{
				status=errno;
				perror("open");
				return status;
			}
		}	
	}
	return 0;
}

int doBuiltIn(const CMD *cmdList)
{	
	int status = 0;
	int in;

	if(cmdList->fromType != NONE && cmdList->fromFile != NULL)
	{
		if(cmdList->fromType == RED_IN)
		{
			in = open(cmdList->fromFile, O_RDONLY);
			if(in >= 0)
			{
				close(in);
			}
			else //open failed
			{
				status=errno;
				perror("open, invalid file/directory");
				return setStatus(status);
			}
		}
		else if(cmdList->fromType == RED_IN_HERE)
		{
			char template[] = "templateeXXXXXX";
			in = mkstemp(template);
			if(in < 0)
			{
				status=errno;
				perror("mkstemp");
				return setStatus(status);
			}
			write(in, cmdList->fromFile, strlen(cmdList->fromFile));
			close(in);
			
			in = open(template, O_RDONLY);
			if(in < 0)
			{
				status=errno;
				perror("open, template");
				return setStatus(status);
			}
			close(in);
		}
	}

	if(strcmp(cmdList->argv[0], "wait") == 0) //change when backgrounding/zombies
	{
		if(cmdList->argc > 1)
		{
			fprintf(stderr, "wait: Too many args\n");
			return setStatus(1);
		}
		else
		{
			int pid;
			int status=0;
			signal(SIGINT, SIG_IGN);
			for(int i = 0; i < numPids; i++)
			{
				if((pid=waitpid(pids[i], &status, 0)) >= 0)
				{
					status = STATUS(status);
        			fprintf(stderr, "Completed: %d (%d)\n", pid, status);
				}
			}
			signal(SIGINT, SIG_DFL);
			return setStatus(0);
		}
	}
	else if(strcmp(cmdList->argv[0], "cd") == 0)
	{
		if(cmdList->argc == 1) // cd
		{
			if(chdir(getenv("HOME")) < 0)
			{
				status=errno;
				perror("chdir");
				return setStatus(status);
			}
			else
			{
				return setStatus(0);
			}
		}
		else if(cmdList->argc == 2)
		{
			if(strcmp(cmdList->argv[1], "-p") == 0)
			{
				char path[PATH_MAX];
				if(getcwd(path, PATH_MAX) == NULL)
				{
					status=errno;
					perror("getcwd");
					return setStatus(status);
				}
				else
				{
					if(cmdList->toType != NONE && cmdList->toFile != NULL)
					{
						int out;
						if(cmdList->toType == RED_OUT)
						{
							out = open(cmdList->toFile, O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR);
							if(out < 0)
							{
								status=errno;
								perror("open");
								return setStatus(status);
							}
							dprintf(out, path);
							dprintf(out, "\n");
							close(out);
						}
						else if(cmdList->toType == RED_OUT_APP)
						{
							out = open(cmdList->toFile, O_WRONLY|O_APPEND|O_CREAT, S_IWUSR|S_IRUSR);
							if(out < 0)
							{
								status=errno;
								perror("open");
								return setStatus(status);
							}
							dprintf(out, path);
							dprintf(out, "\n");
							close(out);
						}
					}
					else
					{
						printf("%s\n", path);
					}
					fflush(stdout);
					return setStatus(0);
				}
			}
			else
			{
				if(chdir(cmdList->argv[1]) < 0)
				{
					status = errno;
					perror("chdir");
					return setStatus(status);
				}
				return setStatus(0);
			}
		}
		else
		{
			fprintf(stderr, "cd: Too many args\n");
			return setStatus(1);
		}

	}
	else //export
	{
		if(cmdList->argc == 2)
		{
			char* nameVal = cmdList->argv[1];
			int argLength = strlen(nameVal);
			int valIndex = -1;

			if(isalpha(nameVal[0]) || nameVal[0]=='_')
			{
				for(int i = 1; i < argLength; i++)
				{
					if(nameVal[i] == '=') //split NAME and VALUE
					{
						valIndex = i;
						break;
					}
					else if(isalnum(nameVal[i]) || nameVal[i]=='_')
					{
						continue;
					}
					fprintf(stderr, "export: Invalid NAME=VALUE\n");
					return setStatus(1);
				}
			}
			else
			{
				fprintf(stderr, "export: Invalid NAME=VALUE\n");
				return setStatus(1);
			}

			if(valIndex < 1)
			{
				fprintf(stderr, "export: Invalid NAME=VALUE\n");
				return setStatus(1);
			}

			char *NAME = malloc(sizeof(char)*(valIndex+1)); //partition chars in NAME
			memcpy(NAME, nameVal, valIndex);
			NAME[valIndex] = '\0';

			int stringLen = strlen(nameVal);
			char *VALUE = malloc(sizeof(char)*(stringLen - valIndex));

			if(valIndex != stringLen-1)
			{
				memcpy(VALUE, &nameVal[valIndex+1], stringLen - valIndex - 1);
				VALUE[valIndex] = '\0';
			}
			else
			{
				VALUE[0]='\0';
			}

			if(setenv(NAME, VALUE, 1) < 0)
			{
				status=errno;
				perror("setenv");
				free(NAME);
				free(VALUE);
				return setStatus(status);
			}
			else
			{
				free(NAME);
				free(VALUE);
				return setStatus(0);
			}
		}
		else if(cmdList->argc == 3)
		{
			if(strcmp(cmdList->argv[1], "-n") == 0)
			{
				char* name = cmdList->argv[2];
				int argLength = strlen(name);

				if(isalpha(name[0]) || name[0]=='_')
				{
					for(int i = 1; i < argLength; i++)
					{
						if(isalnum(name[i]) || name[i]=='_')
						{
							continue;
						}
						else
						{
							fprintf(stderr, "export -n: Invalid NAME\n");
							return setStatus(1);
						}
					}
				}
				else
				{
					fprintf(stderr, "export -n: Invalid NAME\n");
					return setStatus(1);
				}

				if(unsetenv(name)==-1)
				{
					status=errno;
					perror("unsetenv");
					return setStatus(status);
				}
				else
				{
					return setStatus(0);
				}

			}
			else
			{
				fprintf(stderr, "export -n: Invalid args\n");
				return setStatus(1);
			}
		}
		else
		{
			fprintf(stderr, "export: Invalid args\n");
			return setStatus(1);
		}
	}
	return setStatus(0);
}

int procSimple(const CMD *cmdList, int BG)
{
	int status = 0;
	if((strcmp(cmdList->argv[0], "cd")== 0 || strcmp(cmdList->argv[0], "export") == 0
		|| strcmp(cmdList->argv[0], "wait") == 0) && !BG)
	{
		return setStatus((doBuiltIn(cmdList)));
	}

	int pid = fork();
	
	if(pid < 0) //fork error bc max out child processes
	{
		status=errno;
		perror("fork");
		setStatus(status);
		return status;
	}
	else if(pid == 0) //child process
	{
		if((strcmp(cmdList->argv[0], "cd") == 0 || strcmp(cmdList->argv[0], "export") == 0
		|| strcmp(cmdList->argv[0], "wait") == 0))
		{
			status=doBuiltIn(cmdList);
			setStatus(status);
			exit(status);
		}
		else
		{
			if((status=setLocals(cmdList)) != 0)
			{
				setStatus(status);
				exit(status);
			}
			if((status=redirect(cmdList)) != 0)
			{
				setStatus(status);
				exit(status);
			}
			execvp(cmdList->argv[0], cmdList->argv);
			status=errno;
			perror("execvp"); //execvp returned; failed
			setStatus(status);
			exit(status);
		}
	}
	else
	{	
		if(BG)
		{
			numPids++;
			REALLOC(pids, numPids);
			pids[numPids-1] = pid;
			fprintf(stderr, "Backgrounded: %d\n", pid);
			return setStatus(0);
		}
		else
		{
			signal(SIGINT, SIG_IGN);
			waitpid(pid, &status, 0);
			signal(SIGINT, SIG_DFL);
			status = STATUS(status);
			return setStatus(status);
		}
	}
}

int procSub(const CMD *cmdList, int BG)
{
	int pid = fork();
	int status = 0;

	if(pid < 0)
	{
		status=errno;
		perror("subcommand fork");
		return setStatus(status);
	}
	else if(pid == 0)
	{
		if((status=setLocals(cmdList)) != 0)
		{
			setStatus(status);
			exit(status);
		}
		if((status=redirect(cmdList)) != 0)
		{
			setStatus(status);
			exit(status);
		}
		exit(processInner(cmdList->left, 0));
	}
	else
	{
		if(BG)
		{
			numPids++;
			REALLOC(pids, numPids);
			pids[numPids-1] = pid;
			fprintf(stderr, "Backgrounded: %d\n", pid);
			return setStatus(0);
		}
		else
		{
			reapZombies();
			signal(SIGINT, SIG_IGN);
			waitpid(pid, &status, 0);
			signal(SIGINT, SIG_DFL);
			status = STATUS(status);
			return setStatus(status);
		}
	}
}

int procPipe(const CMD *cmdList, int BG)
{
	if(BG) 
	{
		int status = 0;
        int pid = fork();
        if(pid < 0) 
        {   
        	status=errno;                            
            perror("fork pipe");
            return setStatus(status);
        }
        else if(pid == 0) 
        {                        
            status=procPipe(cmdList, 0);
            exit(status);
        } 
        else  // parent process
        {           
        	numPids++;
			REALLOC(pids, numPids);
        	pids[numPids-1] = pid;

            fprintf(stderr, "Backgrounded: %d\n", pid);
            status = 0;
            return setStatus(status);
        }
    }
    else
    {
	int pipes = 0; //number of commands in chain
	const CMD *temp;
	for(temp = cmdList; temp->type == PIPE; temp = temp->left)
	{ 
        pipes++;
    }
    pipes++;

	int pid; //child pid
	int fd[2];	//read and write file descriptors for pipe
	int status = 0; //child status
	int pidArr[pipes]; //array of pids for each child
	int fdin; //end of last pipe, original stdin

	const CMD* commands[pipes];
	int index = pipes-1;

	for(temp = cmdList; temp->type == PIPE; temp = temp->left)
	{
		commands[index] = temp->right;
		index--;
	}

	commands[index] = temp;
	fdin = 0;

	for(int i = 0; i < pipes-1; i++)
	{
		if(pipe(fd) || (pid = fork()) < 0)
		{
			status=errno;
			perror("pipe");
			return setStatus(status);
		}
		else if(pid == 0) //child process
		{
			close(fd[0]); //don't read from new pipe
			if(fdin != 0) //stdin = read[last pipe]
			{
				dup2(fdin,0);
				close(fdin);
			}

			if(fd[1] != 1) //stdout = write[new pipe]
			{
				dup2(fd[1], 1);
				close(fd[1]);
			}

			if(commands[i]->type==SIMPLE)
			{
				if((strcmp(commands[i]->argv[0], "cd")== 0 || strcmp(commands[i]->argv[0], "export") == 0
					|| strcmp(commands[i]->argv[0], "wait") == 0))
				{

					status=doBuiltIn(commands[i]);
					setStatus(status);
					exit(status);
				}
				else
				{
					if((status=setLocals(commands[i])) != 0)
					{
						setStatus(status);
						exit(status);
					}
					if((status=redirect(commands[i])) != 0)
					{
						setStatus(status);
						exit(status);
					}
					execvp(commands[i]->argv[0], commands[i]->argv);
					status=errno;
					perror("execvp"); //execvp returned; failed
					setStatus(status);
					exit(status);
				}
			}
			else
			{
				if((status=setLocals(commands[i])) != 0)
					{
						setStatus(status);
						exit(status);
					}
				if((status=redirect(commands[i])) != 0)
				{
					setStatus(status);
						exit(status);
				}
				exit(processInner(commands[i]->left, 0));
			}
		}
		else //parent process
		{
			pidArr[i] = pid; //save child pid
			if(i > 0)
			{
				close(fdin); //close read[last pipe]
			}
			fdin = fd[0];
			close(fd[1]);
		}
	}
	
	if((pid = fork()) < 0) //create last process
	{
		status=errno;
		perror("pipe fork");
		return setStatus(status);
	}
	else if(pid == 0) //child process
	{
		if(fdin != 0) //stdin = read[last pipe]
		{
			dup2(fdin,0);
			close(fdin);
		}

		if(commands[pipes-1]->type == SIMPLE)
		{
			if((strcmp(commands[pipes-1]->argv[0], "cd")== 0 || strcmp(commands[pipes-1]->argv[0], "export") == 0
					|| strcmp(commands[pipes-1]->argv[0], "wait") == 0))
			{

				status=doBuiltIn(commands[pipes-1]);
				setStatus(status);
				exit(status);
			}
			else
			{
				if((status=setLocals(commands[pipes-1])) != 0)
				{
					setStatus(status);
					exit(status);
				}

				if((status=redirect(commands[pipes-1])) != 0)
				{
					setStatus(status);
					exit(status);
				}

				execvp(commands[pipes-1]->argv[0], commands[pipes-1]->argv);
				status=errno;
				perror("execvp"); //execvp returned; failed
				setStatus(status);
				exit(status);
			}
		}
		else
		{
			if((status=setLocals(commands[pipes-1])) != 0)
			{
				setStatus(status);
				exit(status);
			}

			if((status=redirect(commands[pipes-1])) != 0)
			{
				setStatus(status);
				exit(status);
			}
			exit(processInner(commands[pipes-1]->left, 0));
		}	
	}
	else //parent process
	{
		pidArr[pipes-1] = pid;
		close(fdin);
	}

	int finalStatus = 0;
	
	signal(SIGINT, SIG_IGN);
	for(int i = 0; i < pipes; i++) //wait for children to die
	{   
    	pid = waitpid(pidArr[i], &status, 0);

    	if(status != 0) // child failed
        {
            finalStatus = status;   // save error status
        }
    }
    signal(SIGINT, SIG_DFL);
    
    finalStatus = STATUS(finalStatus);

    return setStatus(finalStatus);
}
}

int procAnd(const CMD *cmdList, int BG)
{
	int status = 0;

	if(BG) 
	{
        int pid = fork();
        if(pid < 0) 
        {   
        	status=errno;                            
            perror("fork And");
            return setStatus(status);
        }
        else if(pid == 0) 
        {                        
            if((status=processInner(cmdList->left, 0)) == 0)
            {
                status=processInner(cmdList->right, 0);
            }
            exit(status);
        } 
        else  // parent process
        {           
        	numPids++;
			REALLOC(pids, numPids);
        	pids[numPids-1] = pid;

            fprintf(stderr, "Backgrounded: %d\n", pid);
            status = 0;
            return setStatus(status);
        }
    }
    else
    {
		if((status = processInner(cmdList->left, BG)) == 0)
		{
        	return processInner(cmdList->right, BG);
		}
    	else
    	{
        	return setStatus(status);
   	 	}
	}
}

int procOr(const CMD *cmdList, int BG)
{
	int status = 0;
	if(BG)
	{
		int pid = fork();
		if(pid<0)
		{
			status=errno;
			perror("fork Or");
			return setStatus(status);
		}
		else if(pid==0)
		{
			if((status=processInner(cmdList->left, 0)) != 0)
            {
                status=processInner(cmdList->right, 0);
            }
            exit(status);
		}
		else
		{
			numPids++;
			REALLOC(pids, numPids);
			pids[numPids-1] = pid;
			fprintf(stderr, "Backgrounded: %d\n", pid);
            status = 0;
            return setStatus(status);
		}
	}
	else
	{
		if((status = processInner(cmdList->left, BG)) == 0)
		{
        	return setStatus(status);
   	 	}
    	else
    	{
			return processInner(cmdList->right, BG);
    	}
	}
}

int procBG(const CMD *cmdList, int root)
{
	int status=0;
	if(root) //root of background command, first BG
	{
		if(cmdList->left->type == SEP_BG)
		{
			status=procBG(cmdList->left, 0);
			if(cmdList->right != NULL)
			{
				status=processInner(cmdList->right, 0);
			}
		}
		else
		{
			status=processInner(cmdList->left, 1);
			if(cmdList->right != NULL)
			{
				status=processInner(cmdList->right, 0);
			}
		}
	}
	else
	{
		if(cmdList->left->type == SEP_BG)
		{
			status=procBG(cmdList->left, 0);
		}
		else
		{	
			status=processInner(cmdList->left, 1);
		}
		status=processInner(cmdList->right, 1);
	}

	return setStatus(status);
}

int processInner(const CMD *cmdList, int BG)
{
	if(cmdList->type == SIMPLE)
	{
		return procSimple(cmdList, BG);
	}
	else if(cmdList->type == SUBCMD)
	{
		return procSub(cmdList, BG);
	}
	else if(cmdList->type == PIPE)
	{
		return procPipe(cmdList, BG);
	}
	else if(cmdList->type == SEP_AND)
	{
		return procAnd(cmdList, BG);
	}
	else if(cmdList->type == SEP_OR)
	{
		return procOr(cmdList, BG);
	}
	else if(cmdList->type == SEP_BG)
	{
		return procBG(cmdList, 1);
	}
	else if(cmdList->type == SEP_END)
	{
		if(cmdList->right==NULL)
		{
			return processInner(cmdList->left, 0);
		}
		else
		{
			processInner(cmdList->left, 0);
			return processInner(cmdList->right, BG);
		}
	}
	else
	{
		fprintf(stderr, "Invalid CMD tree\n");
		return 1;
	}

}

// Execute command list CMDLIST and return status of last command executed
int process(const CMD *cmdList)
{
	int ret= processInner(cmdList, 0);
	reapZombies();
	return ret;
}
