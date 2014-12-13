#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
char* historyLog[10][MAX_LINE];
int point;
int* jobs;

// char* historyLog[10][MAX_LINE];

/**
 * setup() reads in the next command line, separating it into distinct
 * tokens using whitespace (space or tab) as delimiters. setup() sets
 * the args parameter as a null-terminated string.
 */
void setup(char inputBuffer[], char *args[], int *background)
{
  int length, /* # of characters in the 			   int i = 0;
			   printf("starting... \n");
			   printf("%s\n", args[0]);
			   while(args[i] != NULL)
			   {
				   historyLog[point][i] = strdup(args[i]);
				   printf("arg entered is: %s \n", historyLog[point][i]);
				   i++;
			   }

			   printf("\n");

			   point++;command line */
    i,        /* loop index for accessing inputBuffer array */
    start,    /* index where beginning of next command parameter is */
    ct;       /* index of where to place the next parameter into args[] */

  ct = 0;

  /* read what the user enters on the command line */
  length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

  start = -1;
  if (length == 0) {
    /* ctrl-d was entered, quit the shell normally */
    printf("\n");
    exit(0);
  }
  if (length < 0) {
    /* somthing wrong; terminate with error code of -1 */
    perror("Reading the command");
    exit(-1);
  }

  /* examine every character in the inputBuffer */
  for (i = 0; i < length; i++) {
    switch (inputBuffer[i]){
    case ' ':
    case '\t':               /* argument separators */
      if(start != -1){
	args[ct] = &inputBuffer[start];    /* set up pointer */
	ct++;
      }
      inputBuffer[i] = '\0'; /* add a null char; make a C string */
      start = -1;
      break;
    case '\n':                 /* should be the final char examined */
      if (start != -1){
	args[ct] = &inputBuffer[start];
	ct++;
      }
      inputBuffer[i] = '\0';
      args[ct] = NULL; /* no more arguments to this command */
      break;
    default :             /* some other character */
      if (inputBuffer[i] == '&'){
	*background  = 1;
	inputBuffer[i] = '\0';
	start = -1;
      } else if (start == -1)
	start = i;
    }
  }

  args[ct] = NULL; /* just in case the input line was > MAX_LINE */
}

/* Displays the history */
void showHistory()
{
	int i;
    int j;

    for(i = 0; i < 10; i++)
    {
    	if(historyLog[i][0] == NULL)
    	{
    		break;
    	}

    	printf("%s ", historyLog[i][0]);

    	for(j = 1; j < MAX_LINE/2; j++)
    	{
    		if(historyLog[i][j] == NULL)
    		{
    			break;
    		}
    		else
    		{
                printf("%s ", historyLog[i][j]);
    		}
    	}
    	printf("\n");
    }

    exit(0);
}

/* Adds commands to history */

void addToHistory(char* args[])
{
	   if(point <= 9)
	   {
		   int i = 0;
		   while(args[i] != NULL)
		   {
			   historyLog[point][i] = strdup(args[i]);
			   i++;
		   }

		   point++;
	   }

	   else
	   {
		   int start = 0;
		   for(start = 0; start < 9; start++)
		   {
			   //printf("changing %s to %s", historyLog[start][0], historyLog[start+1][0]);
		       strcpy(historyLog[start][0], historyLog[start + 1][0]);

		       int j = 1;
		       while(j < MAX_LINE/2)
		       {
		    	   //printf("while 1 ...");
		    	   if(historyLog[start][j] != NULL)
		    	   {
		    	       historyLog[start][j] = NULL;
		    	   }
		    	   else
		    	   {
		    		   //printf("break \n");
		    		   break;
		    	   }

		    	   j++;
		       }

		       int k = 1;
		       while(k < MAX_LINE/2)
		       {
		    	   if(historyLog[start + 1][k] != NULL)
				   {
					   historyLog[start][k] = historyLog[start + 1][k];
				   }
		    	   else
		    	   {
		    		   break;
		    	   }

		    	   k++;
		       }
		   }

		   historyLog[9][0] = strdup(args[0]);
		   int check = 1;
		   for(check = 1; check < MAX_LINE/2; check++)
		   {
			   if(historyLog[9][check] != NULL)
			   {
				   historyLog[9][check] = NULL;
			   }
			   else
			   {
				   break;
			   }
		   }

		   int counter = 1;
		   while(counter < MAX_LINE/2)
		   {
			   if(args[counter] != NULL)
			   {
				   historyLog[9][counter] = strdup(args[counter]);
			   }
			   else
			   {
				   break;
			   }
			   counter++;
		   }
	   }

}

/* Find the index of the command with the character first */

int findCommandIndex(char* first)
{
    int latest = 9;
    for(latest = 9; latest >= 0; latest--)
    {
    	if(historyLog[latest][0] != NULL)
    	{
			if(historyLog[latest][0][0] == *first)
			{
				return latest;
			}
    	}
    }

    return -1;
}

/* Find the index of the last command */
int findLastCommandIndex()
{
	int i = 0;
	for(i = 0; i < 10; i++)
	{
		if(historyLog[i][0] == NULL)
		{
			return i-1;
		}
	}

	return -1;
}

int main(void)
{
   char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
   int background; /* equals 1 if a command is followed by '&' */
   char *args[MAX_LINE+1]; /* command line (of 80) has max of 40 arguments */
   jobs = malloc(1 * sizeof(int));  //initilizes the dynamic int array 
   int count = 0;
   int limit = 1;

   while (1){ /* Program terminates normally inside setup */
       background = 0;
       printf(" COMMAND->\n");
       
       setup(inputBuffer,args,&background); /* get next command */
		/* the steps are:
		(1) fork a child process using fork()
		(2) the child process will invoke execvp()
		(3) if background == 1, the parent will wait,
		otherwise returns to the setup() function. */

       if(strcmp(args[0], "exit") == 0)  //exits the shell
       {
           printf("Exiting shell ... \n");
	   free(jobs);
	   exit(0);
       }

       pid_t process = fork();  //child process created
       int childStatus;

       if(process > 0)  //parent process
       {
    	   if(background == 1)
    	   {
	           //background = 0;
		   int pid = process;
                   jobs[count] = process;
	           setpgid(process, 0);
	           
	           count++;

               if(count == limit)
               {
	               jobs = realloc(jobs, (count+2)*sizeof(int));  //reallocates memory if the size of the array exceeds
	               limit = limit + 1;
               }

    	       continue;
    	   }

    	   else
    	   {
    	       wait(&childStatus);
    	   }
       }

       else if(process == 0)
       {
    	   if(strcmp(args[0], "history") == 0)
    	   {
	           //execvp(historyLog[0][0], historyLog[0]);
    		   showHistory();

    	   }
    	   else if(strcmp(args[0], "r") == 0 && args[1] == NULL)
    	   {
    		   int index = findLastCommandIndex();
    		   if(index != -1)
    		   {
				   execvp(historyLog[index][0], historyLog[index]);
				   fprintf(stderr, "Could not execute command \n");
				   exit(1);
    		   }
    	   }
    	   else if(strcmp(args[0], "r") == 0 && args[1] != NULL)
    	   {
               int index = findCommandIndex(args[1]);
	       printf("index is %d\n", index);
               if(index != -1)
               {
		   if(args[1] != "cd")
		   {
            	       execvp(historyLog[index][0], historyLog[index]);
            	       fprintf(stderr, "Could not execute command \n");
            	       exit(1);
		   }
		   else
		   {
                       if(chdir(historyLog[index][1]) == -1)
		       {
			       fprintf(stderr, "Directory does not exist");
			       exit(1);
		       }
		   }
               }
    	   }
		   else if(strcmp(args[0], "cd") == 0)
		   {
		       if(chdir(args[1]) == -1)
		       {
		    	   fprintf(stderr, "Directory does not exist \n");
		    	   exit(1);
		       }
		   }
		   else if(strcmp(args[0], "jobs") == 0)
		   {
			   printf("Jobs currently running are: ");
			   int a;
			   for(a = 0; a < limit; a++)
			   {
		               if(jobs[a] != 0)
			       {
			           printf("%d   ", jobs[a]);
			       }
			       else
			       {
				       break;
			       }
			   }
			   printf("\n");
                           exit(0);
			   //execvp("ps", jobs);
		   }
           else if(strcmp(args[1], "fg") == 0)
	   {
		   int status;
		   waitpid(atoi(args[1]), &status, 0);
		   continue;
	   }
    	   else
    	   {
    	       execvp(args[0], args);
               fprintf(stderr, "Could not execute command \n");
	       exit(1);
    	   }
       }

       else
       {
    	   fprintf(stderr, "Fork failed.\n");
    	   exit(1);
       }

       if(strcmp(args[0],"history") != 0)
	   {
    	   if(strcmp(args[0], "r") != 0)
    	   {
    		   addToHistory(args);
    	   }
    	   else if(strcmp(args[0], "r") == 0 && args[1] == NULL)
    	   {
    		   int index = findLastCommandIndex();
    		   if(index != -1)
    		   {
    		       addToHistory(historyLog[index]);
    		   }
    	   }
    	   else if(strcmp(args[0], "r") == 0 && args[1] != NULL)
    	   {
    		   int index = findCommandIndex(args[1]);
    		   if(index != -1)
    		   {
    			   addToHistory(historyLog[index]);
    		   }
    	   }
	   }
   }
}
