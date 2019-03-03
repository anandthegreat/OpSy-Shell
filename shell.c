#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<signal.h>

enum type { 
  OVERWRITE2FILE, APPEND2FILE, READFROMFILE, NORMAL 
} Type;

void dontKillMe();											//function prototypes

void tokeniser(char *input,char **args){
  int i=0;
  args[i] = strtok(input," ");                              //arr[0] will have command name
  while(args[i]!=NULL) args[++i] = strtok(NULL," ");        //read next token from buffer
  args[++i]=NULL;
}

enum type redirectionChecker(char *input,char **args){

  char *p=strstr(input,">");                                //check if > exists in input string
  char *q=strstr(input,">>");                               //check if >> exists in input string
  char *r=strstr(input,"<");
//   int i=0;
//   args[i] = strtok(input,">");                           //arr[0] will have command name
//   while(args[i]!=NULL) args[++i] = strtok(NULL,">");
// printf("%d\n", i);

  
  if(q!=NULL)      return APPEND2FILE; //should be above p!=NULL check
  else if(p!=NULL) return OVERWRITE2FILE;
  else if(r!=NULL) return READFROMFILE;
  else return NORMAL;
//
//   args[++i]=NULL;

}
void redirectionExecution(char *input,char *filename,char **args,enum type r_type){ //r_type = redirection type
  printf("====>%s\n",filename );

  if(r_type==APPEND2FILE)             // >> case
//  if(filename[0]=='>') //for append not working
  {
    //printf("%s\n",">> case" );
    close(1);
    int fd;
    if(fd= open(filename,O_CREAT|O_WRONLY|O_APPEND,0777)==-1){
      perror("failed to open/create file");
    }
    else{
    tokeniser(input,args);
    if(execvp(args[0],args)<0){
      perror("error: exec failed");
      exit(1);
      }
    }
  }

  else if(r_type==OVERWRITE2FILE){     // > case

    // printf("%d\n",filename[0]=='>');
    close(1);
    int fd;
    if(fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
      perror("failed to open/create file");
    }
    else{
    tokeniser(input,args);
    if(execvp(args[0],args)<0){
      perror("error: exec failed");
      exit(1);
      }
    }
  }

  else if(r_type==READFROMFILE){
    close(0);
    int fd;
    if(fd= open(filename,O_RDONLY)==-1){
      perror("failed to open file");
    } 
    else{
    tokeniser(input,args);
    if(execvp(args[0],args)<0){
      perror("error: exec failed");
      exit(1);
      }
    }
  }

}

void commandExecution(char input[])
{
    pid_t pID=fork();
    if (pID==0){
      signal(SIGINT, SIG_DFL);	
      char *args[20];
      Type=redirectionChecker(input, args);
      if(Type==NORMAL)
      {
        tokeniser(input,args);
        if(execvp(args[0],args)<0){                   //execute command and check if unsuccessful
          perror("error: exec failed");
          exit(1);
        }
      }
      else if(Type==OVERWRITE2FILE){                   // >
        char *filename;
        strtok(input,">");
        filename= strtok(NULL," ");
        redirectionExecution(input,filename,args,OVERWRITE2FILE);
      }
      else if(Type==APPEND2FILE){                  // >>
        char *filename;
        char *ptr = input;
        filename=strtok_r(ptr,">>",&ptr);
        filename=strtok_r(ptr,">>",&ptr);
        filename=strtok(filename," ");
        printf("%s\n",filename);
        redirectionExecution(input,filename,args,APPEND2FILE);
      }
      else if(Type==READFROMFILE){
        char *filename;
        strtok(input,"<");
        filename= strtok(NULL," ");
        redirectionExecution(input,filename,args,READFROMFILE);
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
