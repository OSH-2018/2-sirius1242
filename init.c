#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#define MAX_LEN 256
#define MAX_PIPE 256
extern char **environ;

int apart(char *_pipe, char *args[MAX_LEN]) // apart every command part departed by pipe
{
	char *arg;
	int i=0;
	arg = strtok(_pipe, " \t");
	while (arg != NULL)
	{
		args[i] = strdup(arg);
		arg = strtok(NULL, " \t");
		i++;
	}
	args[i] = NULL;
	return i;
}

int inner(char *args[]) //内建命令
{ 
	/* 没有输入命令 */
	if (!args[0])
		return 1;

	/* 内建命令 */
	if (strcmp(args[0], "exit") == 0)
		exit(0);
	if (strcmp(args[0], "cd") == 0)
	{
		if (args[1])
		{
			if (chdir(args[1]))
				fprintf(stderr, "cd: no such file or directory: %s\n", args[1]);
		}
		else // if no args for `cd`
			if(chdir(getenv("HOME")))
				fprintf(stderr, "please confirm your HOME variable set correctly!\n");
		return 1;
	}
	if (strcmp(args[0], "pwd") == 0)
	{
		char wd[4096];
		puts(getcwd(wd, 4096));
		return 1;
	}
	if (strcmp(args[0], "env") == 0)
	{
		for (char **env = environ; *env != 0; env++)
		{
			char *thisEnv = *env;
			puts(thisEnv);
		}
		return 1;
	}
	if ((strcmp(args[0], "echo") == 0) && (args[1][0] == '$'))
	{
		memmove(args[1], args[1] + 1, strlen(args[1]));
		if (getenv(args[1]) != NULL)
			puts(getenv(args[1]));
		else
			printf("\n");
		return 1;
	}
	if (strcmp(args[0], "export") == 0)
	{
		char *var = strtok(args[1], "=");
		char *name;
		name = strdup(var);
		var = strtok(NULL, "=");
		if(var != NULL)
			setenv(name, var, 1);
		else
			setenv(name, "", 1);
		var = strtok(NULL, "=");
		free(name);
		return 1;
	}
	if (strcmp(args[0], "unset") == 0)
	{
		unsetenv(args[1]);
		return 1;
	}
	return 0;
}
int exec_pipe(char **_pipe, int j)
{
	char *pipes[MAX_LEN];
	int prev_read = -1;
	char *arg;
	int i, k;
	k = apart(_pipe[j], pipes);
	if (_pipe[j + 1] == NULL) // the last command
	{
		execvp(pipes[0], pipes);
		perror(pipes[0]);
		exit(1);
	}
	int fd[2];
	pipe(fd);
	if (fork() == 0)
	{
		dup2(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		if(inner(pipes)) //check if the first command is inner command
			exit(0);
		execvp(pipes[0], pipes);
		perror(pipes[0]);
		exit(1);
	}
	dup2(fd[0], 0);
	close(fd[0]);
	close(fd[1]);
	for (i = 0; i <= k; i++)
		free(pipes[i]);
	exec_pipe(_pipe, j+1);
}
int main()
{
	/* 输入的命令行 */
	char cmd[MAX_LEN];
	char *cmds;
	char *tmp[MAX_LEN];
	/* 命令行拆解成的各部分，以空指针结尾 */
	char *args[128];
	char *arg;
	char *_pipe[MAX_PIPE];
	int i, k, j, l;
	while (1)
	{
		/* 提示符 */
		printf("# ");
		fflush(stdin);
		fgets(cmd, MAX_LEN, stdin);
		pid_t childPid;
		/* 清理结尾的换行符 */
		for (i = 0; cmd[i] != '\n'; i++);
		cmd[i] = '\0';
		/* 拆解命令行 */
		if (i == 0)
			continue;
		i = 0;
		cmds = strtok(cmd, "|");
		while (cmds != NULL)
		{
			_pipe[i] = strdup(cmds);
			cmds = strtok(NULL, "|");
			i++;
		}
		l = i;
		int fd[2];
		_pipe[i] = NULL;
		if (i==1) // if no pipe symbol
		{
			apart(strdup(_pipe[0]), tmp);
			if (inner(tmp))
				continue;
		}
		childPid = fork();
		if (childPid == 0)
			exec_pipe(_pipe, 0);
		waitpid(childPid, NULL, 0); // wait for child
	}
}
