#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include<sys/utsname.h>
#include<sys/wait.h>
#include <mqueue.h>
#include <time.h>
#include <string.h>

/*Gets The Length Of Char[]*/
int lengthOfChar(char p[])
{
    int i = 0;
    while(p[i] != '\0')
    {
        i++;
    }
    return i;
}

//converts a char to a int
int intConvert(char p[])
{
    int num = 0;
    int n = lengthOfChar(p);

    for (int i = 0; i < n; i++)
    {
       num = num * 10 + (p[i] - 48);
    }
    return num;
}

//converts a int back into a char
char* intToChar(int p)
{
    // Count digits in number N
    int m = p;
    int digit = 0;

    while (m) {

        // Increment number of digits
        digit++;

        // Truncate the last
        // digit from the number
        m /= 10;
    }

    // Declare char array for result
    char* arr;

    // Declare duplicate char array
    char arr1[digit];

    // Memory allocation of array
    arr = (char*)malloc(digit);

    // Separating integer into digits and
    // accommodate it to character array
    int index = 0;
    while (p) {

        arr1[++index] = p % 10 + '0';

        // Truncate the last number
        p /= 10;
    }

    // Reverse the array for result
    int i;
    for (i = 0; i < index; i++) {
        arr[i] = arr1[index - i];
    }

    // Char array truncate by null
    arr[i] = '\0';

    // Return char array
    return (char*)arr;
}

const int MSG_CAPACITY = 10,MSG_SIZE = 128;

int main(int argc,char *argv[])
{
    //if required amount of argument not met terminates
	if (argc < 2)
	{
		printf("Wrong number of command line arguments\n");
		return 0;
	}

	int f1, fd[2], MAXLINE = 0,done = 0,stat;

    //Finding the maxline byte length in argv
    for(int i = 0; i < argc; i++)
    {
        if(intConvert(argv[i]) >= 128 && intConvert(argv[i]) <=256)
        {
            MAXLINE = intConvert(argv[i]);
            break;
        }
    }
    
    if(MAXLINE == 0)
    {
    	MAXLINE = 128;
    }
	
    char *q_name = "/msg_Queue";
    
    struct mq_attr attr;
    attr.mq_maxmsg = MSG_CAPACITY;
    attr.mq_msgsize = MAXLINE;

    // Initialize the message queue.
    mqd_t mqd = mq_open(q_name,  O_CREAT | O_RDWR, 0664, &attr);
    if (mqd == (mqd_t) -1)
    {
     perror("Error in mq_open");
    mq_unlink(q_name);
    return 2;
    }

    //Initialize the pipe
	if (pipe(fd) == -1) 
	{
		printf("pipe failed");
		return 0;
	}

    //Creating Parent Child
    pid_t pid = fork();

	if (pid < 0)
	{
        printf("fork failed");
        return 0;
    }
    
    /*CHILD PROCESS*/
    else if(pid == 0)
    {
        char line[MAXLINE];
        int newLine = 0, wordCount = 0, charCount = 0, lineVal = 0;
        char tea [] = "done";

    	struct timespec timeout = {0, 0};

		while(intConvert(line) != intConvert(tea))
		{
            close (fd[1]); // write end of pipe
            read (fd[0], line, MAXLINE);
            
            if(intConvert(line) != intConvert(tea))
            {    
                for(int i = 0; i < MAXLINE; i++)
                {
                    /*Counts The Amount Of New Lines*/
                    if(line[i] == '\n')
                    {
                        newLine++;
                    }
                    /*Counts The Amount Of Characters*/
                    if ((line[i] >= 'a' && line[i] <= 'z') || (line[i] >= 'A' && line[i] <= 'Z'))
                    {
                        charCount++;
                    }
                    //fix word count
                    if(line[i]==' ' || line[i]=='\n' || line[i]=='\t')
                    {
                        wordCount++;
                        if(line[i + 1] == '\0')
                        {
                            wordCount--;
                        }
                    }
                }
            }
        }  
        /*POXIS QUEUE*/
            /*SEND FIRST MESSAGE*/
            if (mq_timedsend(mqd, intToChar(newLine), 11, 1, &timeout) == -1)
  		    { 
            printf("Child: mq_send error");
            return 4;
  		    }

            /*SEND SECOND MESSAGE*/
            if (mq_timedsend(mqd, intToChar(charCount), 11, 1, &timeout) == -1)
  		    { 
            printf("Child: mq_send error");
            return 4;
  		    }

            /*SEND THIRD MESSAGE*/
            if (mq_timedsend(mqd, intToChar(wordCount), 11, 1, &timeout) == -1)
  		    { 
            printf("Child: mq_send error");
            return 4;
  		    }

            /*SEND FOURTH MESSAGE*/
            if (mq_timedsend(mqd, intToChar(MAXLINE), 11, 1, &timeout) == -1)
  		    { 
            printf("Child: mq_send error");
            return 4;
  		    }
        printf("Child Terminating");
        exit(0); 
    }

	/*PARENT PROCESS*/
    else
    {	
        char line[MAXLINE];	
        
        char hostName[150];
        gethostname(hostName, 50 + 1);

        int parentID = getpid();
        int parentProcessID = getppid();
        
        struct utsname uts;
        uname(&uts);
        
        char cwd[256];
        getcwd(cwd,256);
        
        char buf[MSG_SIZE];
	    unsigned int prio;
	    struct timespec timeout = {0, 0};

        for(int i = 0; i < argc; i++)
        {
            if ((f1 = open(argv[i], O_RDONLY, 0)) != -1)
            {
                while ((read( f1, line, MAXLINE)) != '\0')
                {
                    close (fd[0]); // read end of pipe
                    write (fd[1], line, MAXLINE);
                }
                close(f1);
                break;
            }
        }
        //dummy message to end child
        write(fd[1],"done",15);

        waitpid(pid, &stat, 0);
        
        /*POXIS RECIEVE*/

        /*FIRST MESSAGE*/
        ssize_t numRead = mq_timedreceive(mqd, buf, attr.mq_msgsize, &prio, &timeout);
	    if (numRead == -1) 
		{
  		printf("Parent: mq_read error");
  		return 5;
		}
        int newLine = intConvert(buf);

        /*SECOND MESSAGE*/
        numRead = mq_timedreceive(mqd, buf, attr.mq_msgsize, &prio, &timeout);
        if (numRead == -1) 
		{
  		    printf("Child: mq_read error");
  		    return 5;
		}
        int charCount = intConvert(buf);

        /*THIRD MESSAGE*/
        numRead = mq_timedreceive(mqd, buf, attr.mq_msgsize, &prio, &timeout);
        if (numRead == -1) 
		{
  		    printf("Child: mq_read error");
  		    return 5;
		}
        int wordCount = intConvert(buf);

        /*FOURTH MESSAGE*/
        numRead = mq_timedreceive(mqd, buf, attr.mq_msgsize, &prio, &timeout);
        if (numRead == -1) 
		{
  		    printf("Child: mq_read error");
  		    return 5;
		}
        int maxLine = intConvert(buf);

        numRead = mq_timedreceive(mqd, buf, attr.mq_msgsize, &prio, &timeout);
        if (mq_close(mqd) == -1) 
		{ 
  		    printf("Child: close error");
   		    mq_unlink(q_name);
  		    return 3;
		}
        if (mq_unlink(q_name) == -1) 
		{ 
            printf("Child: mq_unlink error");
            mq_unlink(q_name);
            return 3;
		}
        char liner [] = "-l", wordss [] = "-w",charss [] = "-m", lengthLine [] = "-L";
        if(intConvert(argv[1]) == intConvert(liner))
        {
            printf("\n\nAmount of NewLines:%d",newLine);
        }
        else if(intConvert(argv[1]) == intConvert(wordss))
        {
            printf("\n\nAmount of Words:%d", wordCount);
        }
        else if(intConvert(argv[1]) == intConvert(charss))
        {
            printf("\n\nAmount of Characters:%d", charCount);
        }
        else if(intConvert(argv[1]) == intConvert(lengthLine))
        {
            printf("\n\nLongest Line Length:%d", maxLine);
        }
        else
        {
            printf("\n\nAmount of NewLines:%d",newLine);
            printf("\nAmount of Words:%d", wordCount);
            printf("\nAmount of Characters:%d", charCount);
            printf("\nLongest Line Length:%d", maxLine);
        }

        printf("\n\nParent: %d",parentID);
        printf("\nParent ProcessID: %d",parentProcessID);
        printf("\nProcess current working directory is: %s",cwd);
        printf("\nHostName: %d",hostName);

        printf("\nOS name is:%s",uts.sysname);
        printf("\nOS Release:%s",uts.release);
        printf("\nOS Version:%s",uts.version);
        printf("\n"); 

        printf("\nParent Terminating");
        exit(0);
    }  
	return 0;
}