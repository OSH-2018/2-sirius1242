#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
extern char **environ;

int main() {
	/* 输入的命令行 */
	char cmd[256];
	char *cmds;
	/* 命令行拆解成的各部分，以空指针结尾 */
	char *args[128];
	char *arg;
	char *_pipe, **__pipe;
	int k, l;
	int i;
	int fd[2];
	while (1) {
		/* 提示符 */
		printf("# ");
		fflush(stdin);
		fgets(cmd, 256, stdin);
		/* 清理结尾的换行符 */
		for (i = 0; cmd[i] != '\n'; i++);
		cmd[i] = '\0';
		/* 拆解命令行 */
		cmds = strdup(cmd);
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

		/* 没有输入命令 */
		if (!args[0])
			continue;

		/* 内建命令 */
		if (strcmp(args[0], "cd") == 0) {
			if (args[1])
				chdir(args[1]);
			continue;
		}
		if (strcmp(args[0], "pwd") == 0) {
			char wd[4096];
			puts(getcwd(wd, 4096));
			continue;
		}
		if (strcmp(args[0], "env") == 0) {
			for(char **env = environ; *env != 0; env++)
			{
				char *thisEnv = *env;
				puts(thisEnv);
			}
			continue;
		}
		if ((strcmp(args[0], "echo") == 0) && (args[1][0]=='$')) {
			memmove(args[1], args[1]+1, strlen(args[1]));
			if(getenv(args[1]) != NULL)
				puts(getenv(args[1]));
			else
				printf("\n");
			continue;
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
			continue;
		}
		if (strcmp(args[0], "exit") == 0)
			return 0;

		/* 外部命令 */
		if (!(strchr(cmds, '|')==NULL))
		{
			for (i=0; i<k; i++)
				if(!strcmp(args[i], "|"))
				{
					free(args[i]);
					args[i] = NULL;
					l = i;
					_pipe = args[i+1];
					__pipe = &_pipe;
				}
			pid_t childPid;
			if (pipe(fd) != 0)
			{
				perror("failed to create pipe\n");
				continue;
			}
			if ((childPid = fork()) == 01)
			{
				perror("failed to fork\n");
				continue;
			}
			if(childPid == 0)
			{
				dup2(fd[1], 1);
				close(fd[0]);
				close(fd[1]);
				execvp(args[0], args);
				perror("failed to exec command 1");
				continue;
			}
			else
			{
				dup2(fd[0], 0);
				close(fd[0]);
				close(fd[1]);
				execvp(_pipe, __pipe);
				perror("failed to exec command 2");
				continue;
			}
		}
		pid_t pid = fork();
		if (pid == 0) {
			/* 子进程 */
			execvp(args[0], args);
			/* execvp失败 */
			fprintf(stderr, "%s: command not found\n", args[0]);
			return 255;
		}
		/* 父进程 */
		wait(NULL);
		for(i=0;i<k;i++)
			free(args[i]);
		free(cmds);
	}
}
