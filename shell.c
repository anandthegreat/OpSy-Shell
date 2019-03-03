#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<signal.h>

void dontKillMe();											//function prototypes

void tokeniser(char *input,char **args){
  int i=0;
  args[i] = strtok(input," ");                              //arr[0] will have command name
  while(args[i]!=NULL) args[++i] = strtok(NULL," ");        //read next token from buffer
  args[++i]=NULL;
}

int redirectionChecker(char *input,char **args){

  char *p=strstr(input,">");                                //check if > exists in input string
//   int i=0;
//   args[i] = strtok(input,">");                           //arr[0] will have command name
//   while(args[i]!=NULL) args[++i] = strtok(NULL,">");
// printf("%d\n", i);
  if(p!=NULL) return 1;
  else return 0;
//
//   args[++i]=NULL;

}
void redirectionExecution(char *input,char *filename,char **args){
  printf("====>%s\n",filename );

  if(filename[0]=='>') //for append not working
  {

    close(1);
    int fd = open(filename,O_WRONLY|O_APPEND,0777);
    tokeniser(input,args);
    if(execvp(args[0],args)<0){
      perror("error: exec failed");
      exit(1);
    }
  }
  else{
    // printf("%d\n",filename[0]=='>');
    close(1);
    int fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777);
    tokeniser(input,args);
    if(execvp(args[0],args)<0){
      perror("error: exec failed");
      exit(1);
    }
  }
}

void commandExecution(char input[])
{
    pid_t pID=fork();
    if (pID==0){
      signal(SIGINT, SIG_DFL);	
      char *args[20];
      if(redirectionChecker(input, args)==0)
      {
        tokeniser(input,args);
        if(execvp(args[0],args)<0){                   //execute command and check if unsuccessful
          perror("error: exec failed");
          exit(1);
        }
      }
      else{
        char *filename;
        strtok(input,">");
        filename= strtok(NULL," ");
        redirectionExecution(input,filename,args);
      }
    }
    else if(pID>0){
      signal(SIGINT, dontKillMe);
      int p=wait(NULL);
    }
    else{
      perror("error: unable to fork");
    }
}


void dontKillMe(){
	printf("\n%s","$ ");
	fflush(stdout);
}


int main()
{
  signal(SIGINT,dontKillMe);
  char input[100];
  char line[100];
  while(1)
  {    
  		printf("%s","$ " );
  		fgets(line,100,stdin);
  		if(strcmp(line,"\n")!=0)
  		{
  		sscanf(line,"%[^\n]%*c",input);
        //scanf ("%[^\n]%*c", input);
        if (strcmp(input,"exit")==0) 
          break;
     
        if(strcmp(input,"\n")!=0)
       		commandExecution(input);
 	    }
  }
  printf("exiting...\n");
	return 0;
}
