#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
extern char **environ;

int inner(char *args[]) { //内建命令
	/* 没有输入命令 */
	if (!args[0])
		return 1;

	/* 内建命令 */
	if (strcmp(args[0], "cd") == 0) {
		if (args[1])
			if(chdir(args[1]))
				fprintf(stderr, "cd: no such file or directory: %s\n", args[1]);
		return 1;
	}
	if (strcmp(args[0], "pwd") == 0) {
		char wd[4096];
		puts(getcwd(wd, 4096));
		return 1;
	}
	if (strcmp(args[0], "env") == 0) {
		for(char **env = environ; *env != 0; env++)
		{
			char *thisEnv = *env;
			puts(thisEnv);
		}
		return 1;
	}
	if ((strcmp(args[0], "echo") == 0) && (args[1][0]=='$')) {
		memmove(args[1], args[1]+1, strlen(args[1]));
		if(getenv(args[1]) != NULL)
			puts(getenv(args[1]));
		else
			printf("\n");
		return 1;
	}
	if (strcmp(args[0], "export") == 0) {
		char *var = strtok(args[1], "=");
		char *name;
		name = strdup(var);
		var = strtok(NULL, "=");
		//val = strdup(var);
		setenv(name, var, 1);
		var = strtok(NULL, "=");
		free(name);
		return 1;
	}
	return 0;
}
int main() {
	/* 输入的命令行 */
	char cmd[256];
	char *cmds;
	/* 命令行拆解成的各部分，以空指针结尾 */
	char *args[128];
	char *arg;
	char *_pipe;
	int i, k;
	while (1) {
		/* 提示符 */
		printf("# ");
		fflush(stdin);
		fgets(cmd, 256, stdin);
		/* 清理结尾的换行符 */
		for (i = 0; cmd[i] != '\n'; i++);
		cmd[i] = '\0';
		/* 拆解命令行 */
		if (i==0)
			continue;
		cmds = strdup(strtok(cmd, "|"));
		_pipe = strtok(NULL, "|");
		arg = strtok(cmd, " \t");
		i = 0;
		while (arg != NULL)
		{
			args[i] = strdup(arg);
			arg = strtok(NULL, " \t");
			i++;
		}
		args[i] = NULL;
		k = i;

		/*退出*/
		if (strcmp(args[0], "exit") == 0){
			for(i=0;i<k;i++)
				free(args[i]);
			free(cmds);;
			return 0;
		}

		if (_pipe != NULL)
		{
			int fd[2];
			char *pipes[256];
			arg = strtok(_pipe, " \t");
			i = 0;
			while (arg != NULL)
			{
				pipes[i] = strdup(arg);
				arg = strtok(NULL, " \t");
				i++;
			}
			pipes[i] = NULL;
			k = i;
			pid_t childPid;
			if (pipe(fd) == -1)
			{
				perror("failed to create pipe\n");
			}
			//switch (fork())
			//{
			//case -1:
			//	perror("failed to fork\n");
			//	continue;
			//	;
			childPid = fork();
			if (childPid == 0)
			{
				dup2(fd[1], 1);
				close(fd[0]);
				//close(fd[1]);
				execvp(args[0], args);
				perror("failed to exec command 1");
				continue;
			}
			childPid = fork();
			if(childPid == 0)
			{
				dup2(fd[0], 0);
				close(fd[1]);
				//close(fd[0]);
				execvp(pipes[0], pipes);
				perror("failed to exec command 2");
				continue;
			}
			close(fd[0]);
			close(fd[1]);
			for (i = 0; i < k; i++)
				free(args[i]);
			free(cmds);
			;
			continue;
		}

		if (inner(args))
		{
			for (i = 0; i < k; i++)
				free(args[i]);
			free(cmds);
			continue;
		}
		/* 外部命令 */
		pid_t pid = fork();
		if (pid == 0)
		{
			/* 子进程 */
			execvp(args[0], args);
			/* execvp失败 */
			fprintf(stderr, "%s: command not found\n", args[0]);
			return 255;
		}
		/* 父进程 */
		wait(NULL);
	}
}
