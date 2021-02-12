#include <stdio.h>       /* Input/Output */
#include <stdlib.h>      /* General Utilities */
#include <unistd.h>      /* Symbolic Constants */
#include <sys/types.h>   /* Primitive System Data Types */
#include <sys/wait.h>    /* Wait for Process Termination */
#include <errno.h>       /* Errors */
#include <string.h>
#include <signal.h>

#define TOKEN_DELIMITER " \t\r\n\a"

//Function definitions
char *getUserInput();
char** stringTokenizer(char* command);
int numberOfArguments(char **tokens);
void printError(int status, char* token);
void sigquit(int signo);
void changeDirectory(char** tokens);

void handler(int sig);
#define NUM_CHILD 30
struct processAttrib
{ 
  int pid;
  char cmdName[50];

};
struct processAttrib children[NUM_CHILD];
void registerNewChild(struct processAttrib *children,const char **cmd,const int numAgru,const int pid);
void reportWhoDone(struct processAttrib *children);
int main () {

    int numChild = 0;
   pid_t childpid;   /* child's process id */
   int status;       /* for parent process: child's exit status */
   char *input = "";
   int background = 0;
    struct sigaction sigact;
    sigact.sa_flags = 0;
    sigemptyset(&sigact.sa_mask);
    memset (&sigact, 0, sizeof (sigact));
    sigact.sa_handler = &handler;
    sigaction(SIGCHLD, &sigact, 0);

int i = 0;
//initial buffer
  for(i = 0;i<NUM_CHILD;i++)
  children[i].pid = -1;
   
   while (strcmp("exit", input) != 0){
      background = 0;
      printf("%s>", getcwd(NULL, 0));
      input = getUserInput();

      char** tokens = stringTokenizer(input);
      int arguments = numberOfArguments(tokens);
      if(tokens[0] == 0) continue;
     if(strncmp("exit", tokens[0], 4) == 0)
         break;
            char *lastCharacter = tokens[arguments-1];
            
            if(strcmp("&", lastCharacter) == 0)
            {
              background = 1;
                *lastCharacter = '\0';

            } else if(strcmp("cd", tokens[0]) == 0)
            {
               changeDirectory(tokens);
            }
             else {
              
            }
      childpid = fork();
      
      if((background == 1) && (childpid != 0))
        registerNewChild(children,tokens,arguments,childpid);
      if ( childpid >= 0 ) {
         if ( childpid == 0 ) {   


               execvp(tokens[0], tokens);


            //print any error if exists
            // printf("%s\n", strerror(errno));
            
            // printError(status, tokens[0]);

            

            exit(status);
         } else {
            //printf("PARENT %d waiting for  child %d\n",getpid(),childpid);
            if(background== 0)
              {
                waitpid(childpid,&status,0);
              }
            //printf("PARENT: Child's exit code is %d\n",WEXITSTATUS(status));
         }
      }
      else {
         perror("fork");
         exit(-1);
      }
      
      
      free(tokens);
   }
   printf("\n---------------------------EXITED----------------------------\n");
   exit(0);

}


void printError(int status, char* token)
{
   if(status < 0)
         printf("-bash: %s: command not found\n", token);
}


void sigquit(int signo) {
    printf("CHILD (%d): termination request from PARENT (%d)...ta ta for now...\n", getpid(), getppid());
    exit(0);
}

void changeDirectory(char** tokens){
     if (chdir(tokens[1]) != 0)
       perror("chdir() to /error failed");
}



int numberOfArguments(char **tokens) {
   int arguments = 0;
   
   while(*tokens != NULL) {
      //printf("%s \n", *tokens);
      tokens++;
      arguments++;
   }
   return arguments;
}


/* get the user's input by having method, in which it returns a pointer to where the char is located/stored */
char* getUserInput()
{
	char *input, c = ' ';
	int size = 2;
	int index = 0;

	input = (char *) malloc(1* sizeof(char));

	while(c != EOF && c != '\n')
	{
		c = getchar();
		input[index] = c;

		input = (char *) realloc(input, sizeof(char) * size);
		size++;
		index++;
	}
	input[index-1] = '\0';

	return input;
}

char** stringTokenizer(char* command) {
   char** tokens = (char **) malloc(60* sizeof(char*));
   int i = 0;
   char *singleToken;

   singleToken = strtok(command, TOKEN_DELIMITER);
   
   while(singleToken != NULL) {
      tokens[i] = singleToken;
      i++;

      /* Subsequent call is required by passing NULL as input to find the remaining tokens */
      singleToken = strtok(NULL, TOKEN_DELIMITER);
   }

   /* NULL terminate the tokens' last element or position which is i */
   tokens[i] = NULL;
   return tokens;
}

void handler(int sig)
{
/* Clean up the child process. */
int status;
wait (&status);
reportWhoDone(children);
}
/*

struct processAttrib
{ 
  int pid;
  char cmdName[50];

};
struct processAttrib children[NUM_CHILD];
void registerNewChild(struct processAttrib *children,const char *cmd,const pid);
void reportWhoDone(struct processAttrib *children);
*/
void reportWhoDone(struct processAttrib *children)
{  
int status;
  int i = 0;
  int value = 0;
  for (i = 0 ;i <NUM_CHILD;i++)
  {

      if(children[i].pid != -1)
      {
          value = waitpid(children[i].pid,&status,WNOHANG);
          if(value != 0)
          {          
              printf("\n[%d]+ Done %d              ",i,children[i].pid);
              printf("%s\n",children[i].cmdName);
              children[i].pid = -1;
          }
      }
  }
}
void registerNewChild(struct processAttrib *children,const char **cmd,const int numAgru,const int pid)
{
  int j = 0;
  int i = 0;
  char buff[50];
  buff[0] = '\0';
  for (i = 0 ;i <NUM_CHILD;i++)
  {
      if(children[i].pid == -1)
      {
          children[i].pid = pid;
          memset(children[i].cmdName,0,sizeof(children[i].cmdName));
          for (j = 0;j < (numAgru-1);j++)
          {
            sprintf(buff,"%s %s",buff,cmd[j]);
          }
          sprintf(children[i].cmdName,"%s",buff);
          //memcpy(children[i].cmdName,cmd,strlen(cmd));
          printf("[%d] %d\n",i,pid);
          break;
      }
  }
}
