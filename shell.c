#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<signal.h>
#include<ctype.h>

/*
ABSTRACT PSEUDOCODE BELOW
--------------------------
p,q,r,s... are commands which can contain file redirection
input is what the user input obviously...duh
input variant= p|q|r|s... stdout & stderr redirection is always at the end of the input ...
first inputExecution is called in main which manages std(out/err) redirection and then calls inputExecutionSansStd
inputExecutionSansStd breaks(splits) the input into different commands i.e. p|q|r|s.. is broken into p,q,r,s...
(for now piping isn't implemented)... p,q,r,s... are executed individually in commandExecution..
commandExecution checks if its a normal command or if there is any file redirection with the redirectionChecker
irrespective of presence of redirection or not exec is called
---------------------------------------------
*/

enum type {
  OVERWRITE2FILE, APPEND2FILE, OVERWRITE2FILE_READFROMFILE, APPEND2FILE_READFROMFILE, READFROMFILE, NORMAL, 
  //>>            >            <             |        
  STDOUT2FILE, STDERR2FILE, STDERR2STDOUT,NORMALSTD,STDOUT2FILE_AND_STDERR2FILE, STDOUT2FILE_AND_STDERR2STDOUT, CHDIR
  //1>            2>          2>&1                    1> 2>						 1> 2>&							cd
} Type;
//Type is types of redirection and std redirection

void dontKillMe();    //SIGINT handler for our shell
void trimLeadingSpaces(char *str, char ch);
void tokeniser(char *input,char **args);
void commandSeparator(char *input,char **commands);
void dirChange(char *input);
void inputExecution(char input[]);
void commandExecution(char *input,char **args);

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

        else if (strstr(input,"cd")!=NULL){
          dirChange(input);
        }

        else if(strcmp(input,"\n")!=0)
          inputExecution(input);
      }
  }
  printf("exiting...\n");
  return 0;
}

void dontKillMe(){
  printf("\n%s","$ ");
  fflush(stdout);
}

void dirChange(char *input){
  char tempinput[100];
  strcpy(tempinput,input);

  char *dirname;
  strtok(tempinput," ");
  dirname= strtok(NULL," ");
  if(strlen(dirname)!=0){
    if(chdir(dirname)!=0){
      perror("error:dir does not exist");
      exit(1);
    }
  }
  // else chdir("$HOME");
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

void tokeniser(char *input,char **args){
  int i=0;
  args[i] = strtok(input," ");                              //arr[0] will have command name
  while(args[i]!=NULL) args[++i] = strtok(NULL," ");        //read next token from buffer
  args[++i]=NULL;
}

void commandSeparator(char *input,char **commands)        //separates commads and adds commands to array
{
    char *s=strstr(input,"|");
    if(s!=NULL){
      int i=0;
      commands[i] = strtok(input,"|");                              //arr[0] will have command name
      while(commands[i]!=NULL) 
      	{
      		commands[++i] = strtok(NULL,"|");    //read next token from buffer
      	}
      commands[++i]=NULL;
    }
    else{
      commands[0]=input;
      commands[1]=NULL;
    }
}

enum type stdRedirectionChecker(char *input,char **args){   //checks which checks std redirection

  char *t=strstr(input,"1>");                               //check if stdout redirection exists in the input string
  char *u=strstr(input,"2>");                               //check if stderr redirection exists in the input string
  char *v=strstr(input,"2>&1");                               //check if stderr redirection to stdout exists in the input string

  if (t!=NULL && v!=NULL) 	  return STDOUT2FILE_AND_STDERR2STDOUT;
  else if(t!=NULL && u!=NULL) return STDOUT2FILE_AND_STDERR2FILE; 
  else if(v!=NULL) return STDERR2STDOUT;
  else if(t!=NULL) return STDOUT2FILE;
  else if(u!=NULL) return STDERR2FILE;
  else return NORMALSTD;
}

enum type redirectionChecker(char *input,char **args){   //checks if redirection exists, not std redirection

  char *p=strstr(input,">>");                               //check if >> exists in input string
  char *q=strstr(input,">");                                //check if > exists in input string
  char *r=strstr(input,"<");                                //check if < exists in the input string

  if(p!=NULL && r!=NULL) return APPEND2FILE_READFROMFILE;                           //should be above q!=NULL check
  else if(q!=NULL && r!=NULL) return OVERWRITE2FILE_READFROMFILE;
  else if(p!=NULL) return APPEND2FILE;
  else if(q!=NULL) return OVERWRITE2FILE;
  else if(r!=NULL) return READFROMFILE;
  else return NORMAL;
}

void inputExecutionSansStd(char *input,char **args){        //input execution regardeless of std redirection
  char *commands[20];
  char *fileStart=strstr(input,"<");
  char inputFileName[20];

  commandSeparator(input,commands);
  int numOfCommands=0;
  while (commands[numOfCommands]!=NULL)
    numOfCommands++;
 
  int i;
  for (i = 0; i < numOfCommands-1; i++) {
    //pipe from left to right [q|w|e|r=(((q|w)|e)|r)] innermost brackets first obviously
    int fd[2];
    if(pipe(fd)==-1){
      perror("error: failed to create pipe");
      exit(1);
    }

	if (fork()==0) {
       close(1);
       dup(fd[1]);
       commandExecution(commands[i],args);
   	}

   	close(0);
   	dup(fd[0]);
    close(fd[1]);
  }

  commandExecution(commands[i],args); 
}

void inputExecution(char input[]) //input execution with std redirection
{                                 //calls inputExecutionSansStd for every std redirection
    char *args[20];
    pid_t pID=fork();
    //*************child process************
    if (pID==0){
		signal(SIGINT, SIG_DFL);
		// --------------std handelling
    	Type=stdRedirectionChecker(input,args);

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    if(Type==STDOUT2FILE_AND_STDERR2STDOUT){
     
      char *parts[20];
      char *filename;
      char fakeInput[100];
      int y=0;
	  for(int x=0;;x++){
			if(isdigit(input[x])==0){		//if not a digit
				fakeInput[y++]=input[x];
			}
			else {
				if(input[x+1]=='>')
				{
					break;
				}
				else fakeInput[y++]=input[x];
			}
		}
	  fakeInput[y]='\0';
      tokeniser(input,parts);
      for(int i=0;parts[i]!=NULL;i++){
        if(strstr(parts[i],"1>")!=NULL && strlen(parts[i])==2){
          filename=parts[i+1];
        }
        else if(strstr(parts[i],"1>")!=NULL){
          filename=parts[i];
          *filename++;
          *filename++;
        }

      }

      int fd;      
      close(1);
      if(fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
      	perror("error: failed to open/create file");
   		exit(1);
	}

	  close(2);
	  dup(1);
      inputExecutionSansStd(fakeInput,args);
  
  }

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    
    else if(Type==STDOUT2FILE_AND_STDERR2FILE){
      char *parts[20];
      char *filename2,*filename;
      char fakeInput[100];
      int y=0;
	  for(int x=0;;x++){
			if(isdigit(input[x])==0){		//if not a digit
				fakeInput[y++]=input[x];
			}
			else {
				if(input[x+1]=='>')
				{
					break;
				}
				else fakeInput[y++]=input[x];
			}
		}
	  fakeInput[y]='\0';
	    
      tokeniser(input,parts);
      for(int i=0;parts[i]!=NULL;i++){
        if(strstr(parts[i],"1>")!=NULL && strlen(parts[i])==2){
          filename=parts[i+1];
        }
        else if(strstr(parts[i],"1>")!=NULL){
          filename=parts[i];
          *filename++;
          *filename++;
        }

        if(strstr(parts[i],"2>")!=NULL && strlen(parts[i])==2){
          filename2=parts[i+1];
        }
        else if(strstr(parts[i],"2>")!=NULL){
          filename2=parts[i];
          *filename2++;
          *filename2++;
        }
      }

      int fd;
      int fd2;

     close(2);
     if(fd = open(filename2,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
        perror("error: failed to open/create file");
     }
  	
  	close(1);
  	if(fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
  		perror("error: failed to open/create file");
  		exit(1);
	}

    inputExecutionSansStd(fakeInput,args);
      

   }

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    else if(Type==STDOUT2FILE){
      char *filename;
      strtok(input,"1>");
      filename= strtok(NULL,"1>");       //filename will be command after 1> in this case
      trimLeadingSpaces(filename,' ');
      close(1);
      int fd;
      if(fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
        perror("error: failed to open/create file");
      }
      else{
        inputExecutionSansStd(input,args);
      }
    }

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    else if(Type==STDERR2FILE){
    char *filename;
    strtok(input,"2>");
    filename= strtok(NULL,"2>");       //filename will be command after pipe in this case
    trimLeadingSpaces(filename,' ');
  
	  close(2);
	  int fd;
	  if(fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
	    perror("error: failed to open/create file");
	  }
      else{
        inputExecutionSansStd(input,args);
      }

    }

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    else if(Type==STDERR2STDOUT){
        char *filename;
        strtok(input,"2>");
        filename= strtok(NULL,"2>");       //filename will be command after pipe in this case
        trimLeadingSpaces(filename,' ');

      close(2);
      dup(1);
      inputExecutionSansStd(input,args);		
    
    }

// --------------end of STD handling ...now normal handling
      else{
        inputExecutionSansStd(input,args);
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

void commandExecution(char *input,char **args)    //executes every command based on redirection present
{
  Type=redirectionChecker(input,args);

  if(Type==NORMAL){
    tokeniser(input,args);
    if(execvp(args[0],args)<0){                   //execute command and check if unsuccessful
      perror("error: exec failed");
      exit(1);
    }
  }
  else if(Type==APPEND2FILE){
    char *filename;
    char *ptr = input;
    filename=strtok_r(ptr,">>",&ptr);
    filename=strtok_r(ptr,">>",&ptr);
    filename=strtok(filename," ");

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
  else if(Type==OVERWRITE2FILE){
    char *filename;
    strtok(input,">");
    filename= strtok(NULL," ");
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
  else if(Type==READFROMFILE){
    char *filename;
    strtok(input,"<");
    filename= strtok(NULL," ");
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

  else if(Type==APPEND2FILE_READFROMFILE){
  	
    char *filename2;
    char *ptr = input;
    filename2=strtok_r(ptr,">>",&ptr);
    filename2=strtok_r(ptr,">>",&ptr);
    filename2=strtok(filename2," ");

    close(1);
    int fd2;
    if(fd2= open(filename2,O_CREAT|O_WRONLY|O_APPEND,0777)==-1){
      perror("error: failed to open/create file");
      exit(1);
    }

    char *filename;
    strtok(input,"<");
    filename= strtok(NULL," ");
    close(0);
    int fd;
    if(fd= open(filename,O_RDONLY)==-1){
      perror("error: failed to open file");
      exit(1);
    }

    else{
    tokeniser(input,args);
    if(execvp(args[0],args)<0){
      perror("error: exec failed");
      exit(1);
      }
    }
  }

  else if(Type==OVERWRITE2FILE_READFROMFILE){
  	char *filename2;
    strtok(input,">");
    filename2= strtok(NULL," ");
    close(1);
    int fd2;
    if(fd2 = open(filename2,O_WRONLY|O_CREAT|O_TRUNC,0777)==-1){
      perror("error: failed to open/create file");
      exit(1);
    }

  	char *filename;
    strtok(input,"<");
    filename= strtok(NULL," ");
    close(0);
    int fd;
    if(fd= open(filename,O_RDONLY)==-1){
      perror("error: failed to open file");
      exit(1);
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