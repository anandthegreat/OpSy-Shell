#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<signal.h>

enum type {
  OVERWRITE2FILE, APPEND2FILE, READFROMFILE, PIPE, OPRDRCT, ERRDRCT, ERRTOFD, CHDIR, NORMAL
  //>>            >            <             |     1>       2>
} Type;

void dontKillMe();											//function prototype
void trimLeadingSpaces(char *str, char ch);

void tokeniser(char *input,char **args){
  int i=0;
  args[i] = strtok(input," ");                              //arr[0] will have command name
  while(args[i]!=NULL) args[++i] = strtok(NULL," ");        //read next token from buffer
  args[++i]=NULL;
}

// void changeSTDOUT(){
//   close(1);
// }

enum type redirectionChecker(char *input,char **args){

  char *p=strstr(input,">>");                               //check if >> exists in input string
  char *q=strstr(input,">");                                //check if > exists in input string
  char *r=strstr(input,"<");                                //check if < exists in the input string
  char *s=strstr(input,"|");                                //check if | exists in the input string
  char *outputRedirect=strstr(input,"1>");
  char *errorRedirect2File=strstr(input,"2>");
  char *errorRedirect2FD=strstr(input,"&");
  char *changeDir=strstr(input,"cd");
//   int i=0;
//   args[i] = strtok(input,">");                           //arr[0] will have command name
//   while(args[i]!=NULL) args[++i] = strtok(NULL,">");
// printf("%d\n", i);

  if(changeDir!=NULL) return CHDIR;
  else if(p!=NULL)      return APPEND2FILE; //should be above q!=NULL check
  else if(errorRedirect2FD!=NULL) return ERRTOFD;
  else if(outputRedirect!=NULL) return OPRDRCT;
  else if(errorRedirect2File!=NULL) return ERRDRCT;
  else if(q!=NULL) return OVERWRITE2FILE;
  else if(r!=NULL) return READFROMFILE;
  else if(s!=NULL) return PIPE;
  else return NORMAL;
//   args[++i]=NULL;

}
void redirectionExecution(char *input,char *filename,char **args,enum type r_type){ //r_type = redirection type


  if(r_type==CHDIR){


  }
  else if(r_type==APPEND2FILE)             // >> case
//  if(filename[0]=='>') //for append not working
  {
    //printf("%s\n",">> case" );
    close(1);
    int fd;
    if(fd= open(filename,O_CREAT|O_WRONLY|O_APPEND,0777)==-1){
      perror("error: failed to open/create file");
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
    close(1);
    int fd;
    if(fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
      perror("error: failed to open/create file");
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
      perror("error: failed to open file");
    }
    else{
    tokeniser(input,args);
    if(execvp(args[0],args)<0){
      perror("error: exec failed");
      exit(1);
      }
    }
  }

  else if(r_type==PIPE){
    int fd[2];
    if(pipe(fd)==-1){
      perror("error: failed to create pipe");
    }
    printf("%s\n",filename);
    pid_t pipe_child = fork();
    /*
      if the command is : "ls | wc - l", the child will execute ls,
      and send it to the stdin of parent (who executes wc -l)
    */
    if(pipe_child==0){
        close(1);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);
        tokeniser(input,args);
        if(execvp(args[0],args)<0){                   //execute command and check if unsuccessful
          perror("error: exec failed");
          exit(1);
        }
    }
    else if(pipe_child>0){
        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);
        tokeniser(filename,args);
        if(execvp(args[0],args)<0){                   //execute command and check if unsuccessful
          perror("error: exec failed");
          exit(1);
        }
    }
    else perror("error: unable to fork");
  }

  else if(r_type==OPRDRCT){
    close(1);
    int fd;
    if(fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
      perror("error: failed to open/create file");
    }
    else{
      tokeniser(input,args);
      if(execvp(args[0],args)<0){
        perror("error: exec failed");
        exit(1);
      }
    }
  }
  else if(r_type==ERRDRCT){
    close(2);
    int fd;
    if(fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
      perror("error: failed to open/create file");
    }
    else{
      tokeniser(input,args);
      if(execvp(args[0],args)<0){
        perror("error: exec failed");
        exit(1);
      }
    }
  }
  else if(r_type==ERRTOFD){
    // printf("in here \n");
    close(1);
    int fd;
    if(fd = open("error.txt",O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
      perror("error: failed to open/create file");
    }
    close(2);
    dup(1);
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
    //*************child process************
    if (pID==0){
      signal(SIGINT, SIG_DFL);
      char *args[20];
      Type=redirectionChecker(input, args);

      if(Type==CHDIR){
        char *dirname;
        strtok(input," ");
        dirname= strtok(NULL," ");
        chdir(dirname);
      }
      else if(Type==NORMAL)
      {
        tokeniser(input,args);
        if(execvp(args[0],args)<0){                   //execute command and check if unsuccessful
          perror("error: exec failed");
          exit(1);
        }
      }
      else if(Type==OVERWRITE2FILE){                  // >
        char *filename;
        strtok(input,">");
        filename= strtok(NULL," ");
        redirectionExecution(input,filename,args,OVERWRITE2FILE);
      }
      else if(Type==APPEND2FILE){                     // >>
        char *filename;
        char *ptr = input;
        filename=strtok_r(ptr,">>",&ptr);
        filename=strtok_r(ptr,">>",&ptr);
        filename=strtok(filename," ");
        redirectionExecution(input,filename,args,APPEND2FILE);
      }
      else if(Type==READFROMFILE){
        char *filename;
        strtok(input,"<");
        filename= strtok(NULL," ");
        redirectionExecution(input,filename,args,READFROMFILE);
      }
      else if(Type==PIPE){
        char *filename;
        // printf("%s180\n", filename);
        strtok(input,"|");
        // printf("%s182\n", filename);
        filename= strtok(NULL,"|");       //filename will be command after pipe in this case
        // printf("%s\n184", filename);
        trimLeadingSpaces(filename,' ');
        redirectionExecution(input,filename,args,PIPE);
      }
      else if(Type==OPRDRCT){
        char *filename;
        // printf("%s188\n", input);
        strtok(input,"1>");
        // printf("%s190\n", input);
        filename= strtok(NULL,"1>");       //filename will be command after 1> in this case
        // printf("%s192\n", filename);
        trimLeadingSpaces(filename,' ');
        redirectionExecution(input,filename,args,OPRDRCT);
      }
      else if(Type==ERRDRCT){
        char *filename;
        strtok(input,"2>");
        filename= strtok(NULL,"2>");       //filename will be command after pipe in this case
        trimLeadingSpaces(filename,' ');
        redirectionExecution(input,filename,args,ERRDRCT);
      }
      else if(Type==ERRTOFD){
        char *filename;
        strtok(input,"2>");
        filename= strtok(NULL,"2>");       //filename will be command after pipe in this case
        trimLeadingSpaces(filename,' ');
        redirectionExecution(input,filename,args,ERRTOFD);
      }

    }
    //****************parent process(OpSy Shell)******************
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

void trimLeadingSpaces(char *str, char ch)      //to remove leading white spaces
{
    char *p, *q;
    int done=0;
    for (q = p = str; *p; p++){
      if (*p != ch || done==1){
        *q++ = *p;
        done=1;
      }
      else {
        done=1;
      }
    }
    *q = '\0';
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
