#include <stdio.h>       /* Input/Output */
#include <stdlib.h>      /* General Utilities */
#include <unistd.h>      /* Symbolic Constants */
#include <sys/types.h>   /* Primitive System Data Types */
#include <sys/wait.h>    /* Wait for Process Termination */
#include <errno.h>       /* Errors */
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define TOKEN_DELIMITER " \t\r\n\a"

#define DEFAULT_PATH_VAR "export PATH=/usr/bin:/bin:$HOME"
#define DEFAULT_HOME_VAR "export HOME=/home/faculty/dastacey"
#define DEFAULT_PROFILE_NAME ".CIS3110_profile"
#define DEFAULT_HISTAORY_NAME ".CIS3110_history"

struct envVar
{
  char name[20];
  char msg[100];
};
//Function definitions
char *getUserInput();
char** stringTokenizer(char* command);
int numberOfArguments(char **tokens);

int exists(const char *fname);
void keepHistory(const char **cmdMsg,const int numAgur);
int historyCmd(const char **tokens);
void initialProfile(struct envVar *eVar,int *numEnvVar);
int findSpecialSymbolInToken(char **tokes,const int numAgru,const char specialChar);
void printError(int status, char* token);
void sigquit(int signo);
char* printEnvValue(struct envVar *eVar,int *numEnvVar,char *buff, int EnvVarId);
void changeDirectory(char** tokens,struct envVar *eVar,int *numEnvVar);
void readFileOrBufferAndExecute(char **argu,const char *filename,const char *buffer);
void executeAndPrintToPipe(char **argu,int pipeIndex,const char *buff2);
void executeAndPrintToFile(char **argu,const char *filename,int fileId);
void executeCmd(char **argu);
int echoCmd(char **argu,struct envVar *eVar,int *numEnvVar);
void replaceAll(char *str, const char *oldWord, const char *newWord);

void handler(int sig);
#define NUM_CHILD 30
#define MAX_MSG 100
struct processAttrib
{ 
  int pid;
  char cmdName[50];

};
struct processAttrib children[NUM_CHILD];
void registerNewChild(struct processAttrib *children, char **cmd,const int numAgru,const int pid);
void reportWhoDone(struct processAttrib *children);
int main () {

    int numChild = 0;
   pid_t childpid;   /* child's process id */
   int status;       /* for parent process: child's exit status */
   char *input = "";
   unsigned char isBackground = 0;
    struct sigaction sigact;
    sigact.sa_flags = 0;
    sigemptyset(&sigact.sa_mask);
    memset (&sigact, 0, sizeof (sigact));
    sigact.sa_handler = &handler;
    sigaction(SIGCHLD, &sigact, 0);

struct envVar eVar[40];
int numEVar =0;

int i = 0;
//initial buffer
  for(i = 0;i<NUM_CHILD;i++)
  children[i].pid = -1;
   

   while (strcmp("exit", input) != 0){

      // reset condition varible
    initialProfile(eVar,&numEVar); 
      // print prompt 
      printf("%s>", getcwd(NULL, 0));
      input = getUserInput();

      char** tokens = stringTokenizer(input);
      int arguments = numberOfArguments(tokens);
      if(echoCmd(tokens,eVar,&numEVar) == 1)continue;
      keepHistory(tokens,arguments);
      if(tokens[0] == 0) continue;  // protect Segmentation fault 11 
     if(strncmp("exit", tokens[0], 4) == 0)
         break;

            char *lastCharacter = tokens[arguments-1];
            if(strcmp("&", lastCharacter) == 0)
            {
              isBackground = 1;
                *lastCharacter = '\0';

            } else if(strcmp("cd", tokens[0]) == 0)
            {
               changeDirectory(tokens,eVar,&numEVar);
               continue;
            }
      childpid = fork();
      
      if((isBackground == 1) && (childpid != 0))
        registerNewChild(children,tokens,arguments,childpid);
      if ( childpid >= 0 ) {
         if ( childpid == 0 ) {   

            char isPipeFound = findSpecialSymbolInToken(tokens,arguments,'|');
            char isStdInProcess = findSpecialSymbolInToken(tokens,arguments,'<');   
            char isStdOutProcess = findSpecialSymbolInToken(tokens,arguments,'>');
            if (isStdOutProcess > 0)
            {
              tokens[isStdOutProcess] = '\0';
              executeAndPrintToFile(tokens,tokens[isStdOutProcess+1],isStdOutProcess+1);
            }
            else if (isPipeFound > 0)
            {
              executeAndPrintToPipe(tokens,isPipeFound,0);
            }
            else if(isStdInProcess <= 0)
            {
                executeCmd(tokens);
            }
            else
            {
              tokens[isStdInProcess][0] = '\0';
              readFileOrBufferAndExecute(tokens,tokens[isStdInProcess+1],0);
            }

//executeAndPrintToPipe
            //print any error if exists
            // printf("%s\n", strerror(errno));
            
            // printError(status, tokens[0]);

            

            exit(status);
         } else {
            //printf("PARENT %d waiting for  child %d\n",getpid(),childpid);
            if(isBackground== 0)
              {
                waitpid(childpid,&status,0);
      //printf("i am here \n");

              }
            //printf("PARENT: Child's exit code is %d\n",WEXITSTATUS(status));
         }
      }
      else {
         perror("fork");
         exit(-1);
      }      free(tokens);
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

void changeDirectory(char** tokens,struct envVar *eVar,int *numEnvVar)
{
  int i = 0;
  char buff[200];
  int isFoundSpecialPath = 0;

  if(tokens[1][0] == '$')
  {
    for(i=0;i<*numEnvVar;i++)
      if(strcmp(&tokens[1][1],eVar[i].name) == 0) // found
      {
        printEnvValue(eVar,numEnvVar,buff,i);
        i = *numEnvVar;
        isFoundSpecialPath = 1;
      }
  }
  else if (tokens[1][0] == '~')
  {    
    for(i=0;i<*numEnvVar;i++)
      if(strcmp("HOME",eVar[i].name) == 0) // found
      {
        printEnvValue(eVar,numEnvVar,buff,i);
        i = *numEnvVar;
        isFoundSpecialPath = 1;
      }
  }
  if(isFoundSpecialPath == 0)
  {
    memcpy(buff,tokens[1],sizeof(tokens[1]));
  }
  if (chdir(buff) != 0)
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
void registerNewChild(struct processAttrib *children, char **cmd,const int numAgru,const int pid)
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


int findSpecialSymbolInToken(char **tokes,const int numAgru,const char specialChar)
{
  int i = -1;
  int k = -1;
  for (i = 0 ; i < numAgru; i++)
  {
    if(tokes[i][0] == specialChar)
    {
      k = i;
      i = numAgru; 
    } 
  } 
  return k;
}

void executeAndPrintToPipe(char **argu,int pipeIndex,const char *buff2)
{
  int fds[2];
  pipe(fds);
  pid_t pid = fork();
  if (pid == 0)
  {

      char isStdInProcess = 0;
      close(fds[0]);    // close reading end in the child

      dup2(fds[1], 1);  // send stdout to the pipe
      dup2(fds[1], 2);  // send stderr to the pipe

      close(fds[1]);
      argu[pipeIndex][0] = '\0';

            isStdInProcess = findSpecialSymbolInToken(argu,pipeIndex-1,'<');
            if(isStdInProcess <= 0)
            { 
              //printf("Pint command : %s \n",argu[0]);
              executeCmd(argu);
              exit(0);
            }
            else if (buff2 == 0)
            {
              argu[isStdInProcess] = '\0';
              readFileOrBufferAndExecute(argu,argu[isStdInProcess+1],0);
            }
            else
            {
              argu[1] = '\0';
              readFileOrBufferAndExecute(argu,argu[1],buff2);
            }
  }
  else
  {
      // parent

      char buffer[1024];
      buffer[0] = '\0';
      close(fds[1]);  // close the write end of the pipe in the parent
      int numAgur = 0;
      int size = read(fds[0], buffer, sizeof(buffer));
      char **token3 = &argu[pipeIndex+1];
      buffer[size] = 0;

            numAgur = numberOfArguments(token3);
            char isPipeFound = findSpecialSymbolInToken(token3,numAgur,'|');
            if(isPipeFound > 0)
            {
              executeAndPrintToPipe(token3,isPipeFound,buffer);
            }
            else
            {
                readFileOrBufferAndExecute(token3,0,buffer);
            }
      waitpid(pid,NULL,0);
  }
}
void readFileOrBufferAndExecute(char **argu,const char *filename,const char *buffer)
{
    int fds[2];
    pid_t pid;
    pipe (fds);
    pid = fork();
    if (pid == 0)
    {
       close(fds[1]);
       dup2 (fds[0],STDIN_FILENO);
        executeCmd(argu);
      // exit(0);
    }
    else
    {

       FILE *stream;
       close (fds[0]);
       stream = fdopen (fds[1],"w");
      if(filename != 0)
      {
       char buff1[100];
       FILE *pf = fopen(filename,"r");
        while(fgets (buff1, 100, pf)!=NULL)
        {
          fprintf( stream,"%s",buff1);
        }
      }
      else
      {
          char *p = strtok(buffer, "\n");
          while(p)
          {
            //printf("%s\n", p); //Store into array or variables
            fprintf( stream,"%s\n",p);
            p=strtok(NULL, "\n");
          }
      }
      fflush(stream);
       close (fds[1]);
       waitpid (pid,NULL,0);
    }
}
int exists(const char *fname)
{
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

void keepHistory(const char **cmdMsg,const int numAgur)
{
  FILE *file = 0;
  int i = 0;

    file = fopen(DEFAULT_HISTAORY_NAME,"a");
     for(i = 0;i < numAgur;i++)
      fprintf(file,"%s ",cmdMsg[i]);  
    fprintf(file,"\n");
    fclose(file);
  
}
void executeAndPrintToFile(char **argu,const char *filename,int fileId)
{

    pid_t pid = fork();
    char **token2 = 0;
    int numAgur = 0;
  if (pid == 0)
  {
    // child
    FILE *fp = fopen(filename,"w");
    if (fp)fclose(fp); // clear out
    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    dup2(fd, 1); 
    dup2(fd, 2);   
    close(fd);    
    int numAgur = numberOfArguments(argu);
     char isPipeFound = findSpecialSymbolInToken(argu,numAgur,'|');
     char isStdInProcess = findSpecialSymbolInToken(argu,numAgur,'<');
     if (isPipeFound > 0)
     {
       executeAndPrintToPipe(argu,isPipeFound,0);
     }
     else if (isStdInProcess > 0)
     { 
        argu[isStdInProcess] = 0;
        readFileOrBufferAndExecute(argu,argu[isStdInProcess+1],0);
     }
     else
     {
        executeCmd(argu);
     }
  }
  else
  {
       waitpid (pid,NULL,0);
       token2 = &argu[fileId];
        numAgur = numberOfArguments(token2);
       char isPipeFound = findSpecialSymbolInToken(token2,numAgur,'|');
       if(isPipeFound > 0)
       {
         token2[isPipeFound] = 0;
         readFileOrBufferAndExecute(&token2[isPipeFound+1],filename,0);
       }
  }
}

int historyCmd(const char **tokens)
{

    int ret = -1; // not ok

    int arguments = numberOfArguments(tokens);
    if(strncmp("history",tokens[0],7) ==0)
    {

        char historyBuff[100][120]; // for 100 lines and 120 characters per line
        int i = 0;
        char buffer[100];
        FILE *file;
        int numLines = 0;
        int linePrintValue = 0;
        // it should be number starting from 1
     
        if(arguments > 1)
        {          
          if ((tokens[1][0] > '0') && (tokens[1][0] <= '9'))
          {
            linePrintValue = atoi(tokens[1]);
          }
          else if ((tokens[1][0] == '-') && (tokens[1][1] == 'c'))
          {
            file = fopen(DEFAULT_HISTAORY_NAME,"w");
            fclose(file);
            keepHistory(tokens,arguments);
            linePrintValue = 1;
          }
          else if(tokens[1][0] == '>')
          {
              //do nothing
          }
          else if (tokens[1][0] == '\0')
          {
              //do nothing
          }
          else
          { 
              //implement '>' here. or nothing]
            return ret;
          }
        }
        else
        {
          linePrintValue = -2;
        }

        file = fopen(DEFAULT_HISTAORY_NAME,"r");
        while(fgets (buffer, 100, file)!=NULL)
        {
         // if (tokens[1] )
          if (i < 100)
          {
            numLines++;
            sprintf(historyBuff[i],"%d %s",1+i++,buffer);
          }
          else
          {
            break;
          }
        }
        if (linePrintValue == -2) linePrintValue = numLines;
        if (linePrintValue > numLines) linePrintValue = numLines;
        for(i =(numLines-linePrintValue) ;i<numLines ;i++)
        {
          printf("%s",historyBuff[i]);
        }
        ret = 1;
    }
    else
    {
    }
    return ret;
}

void executeCmd(char **argu)
{
  if(historyCmd(argu) == -1)
  {

    execvp(argu[0],argu);
  }
}

void initialProfile(struct envVar *eVar,int *numEnvVar)
{
  FILE *fp = 0;
  int i = 0;
  int numEnvV = 0;
  char buff[100];
  //DEFAULT_PROFILE_NAME
  if (exists(DEFAULT_PROFILE_NAME) == 0) // Not
  {
      //set default value
    fp = fopen(DEFAULT_PROFILE_NAME,"w");
    fprintf(fp,"%s\n",DEFAULT_PATH_VAR);
    fprintf(fp,"%s\n",DEFAULT_HOME_VAR);
    fclose(fp);
  }
  // load to variable 
  fp = fopen(DEFAULT_PROFILE_NAME,"r");
  if (fp)
  {
      char *firstSpaceCh;
      char *firstEqualCh;

        while(fgets (buff, 100, fp)!=NULL)
        {
          //fprintf( stream,"%s",buff);
          firstSpaceCh = strstr(buff,"t");
          firstEqualCh = strstr(buff,"=");
          *firstEqualCh = '\0';
          strcpy(eVar[i].name,firstSpaceCh+2);
          
          firstEqualCh++;
          strcpy(eVar[i].msg,firstEqualCh);
          if(eVar[i].msg[strlen(eVar[i].msg)-1 ] == '\n')
          {
            eVar[i].msg[strlen(eVar[i].msg)-1 ] = '\0';
          }
          numEnvV++;
         // sscanf(buff,"export %s=%s\n",eVar[i].name,eVar[i].msg);
          i++;
        }

        *numEnvVar = numEnvV;
        
        fclose(fp);
  }
}

int echoCmd(char **argu,struct envVar *eVar,int *numEnvVar)
{
  int ret = 0;
  int i =0;
  int isFound = 0;
  char *pch = 0;
  char buff[400];
  buff[0] = '\0';
  if(argu[0] != 0)  // protect Segmentation fault 11 
  if(strncmp("echo", argu[0], 4) == 0)
  {
            int arguments = numberOfArguments(argu);
      if(arguments == 1)
      {
        printf("\n");
      }
      else if(argu[1][0] == '$')
      {
        for(i=0;i<*numEnvVar;i++)
          if(strcmp(&argu[1][1],eVar[i].name) == 0) // found
          {
              printf("%s\n",printEnvValue(eVar,numEnvVar,buff,i));
              i = *numEnvVar;
              isFound = 1;
          }
        if(isFound == 0)
        {            
            if( arguments > 1)
            for(i = 1;i < arguments;i++)
             printf("%s ",argu[1]);
           printf("\n");
        }
      }
      else
      {
            if( arguments > 1)
            for(i = 1;i < arguments;i++)
             printf("%s ",argu[1]);
           printf("\n");
      }
      ret = 1;
  }

 return ret;
}


char* printEnvValue(struct envVar *eVar,int *numEnvVar,char *buff, int EnvVarId)
{
  
  int i = 0;
  sprintf(buff,"%s",eVar[EnvVarId].msg);
  for (i = 0;i < *numEnvVar;i++)
  {
    if(i != EnvVarId)
    {
      replaceAll(buff,eVar[i].name,eVar[i].msg);
    }
  }
  
    return buff;
}

void replaceAll(char *str, const char *oldWord, const char *newWord)
{
    char *pos, temp[300];
    int index = 0;
    int owlen;

    owlen = strlen(oldWord);


    /*
     * Repeat till all occurrences are replaced. 
     */
    while ((pos = strstr(str, oldWord)) != NULL)
    {
        // Bakup current line
        strcpy(temp, str);

        // Index of current found word
        index = pos - str;

        // Terminate str after word found index
        str[index] = '\0';

        // Concatenate str with new word 
        strcat(str, newWord);
        
        // Concatenate str with remaining words after 
        // oldword found index.
        strcat(str, temp + index + owlen);
    }
}