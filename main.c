/***********************************************
* CS 344 Operating Systems 1
* Assignment 3
* Name: Jeffrey Ngo
* Due Date: 2/8/2021
* Citation: Class Learning Modules
* Citation: Owner: CodeVault, Title: Replace Substrings in C,
* Accessed: Jan 2021, URL: https://www.youtube.com/watch?v=0qSU0nxIZiE
************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MAX_LEN 2049
#define MAX_ARG 513

// Flag for the foreground only mode
int foreground = 1;

// A function that searches string for a word to be replaced with a new word.
// Citation: Owner: CodeVault, Title: Replace Substrings in C,
// Accessed: Jan 2021, URL: https://www.youtube.com/watch?v=0qSU0nxIZiE
char* replaceWord(char* inputStr, char* word, char* newWord)
{
	// Search for string for match with user input word
	char* userStr = strstr(inputStr, word);
	// If no matches
	if (userStr == NULL)
	{
		return NULL;
	}

	// Copy characters from str2 and str1
	// + 1 for the null terminator
	// memmove incase if the new word is longer than the old word and overlaps
	memmove(userStr + strlen(newWord), userStr + strlen(word),
		strlen(userStr) - strlen(word) + 1);

	// replace with the new word
	memcpy(userStr, newWord, strlen(newWord));
	return userStr + strlen(newWord);
}

// Followed the example from Learning Module Signal Handling Api
// signal handler for SIGTSTP
void handle_SIGTSTP(int signo)
{
	if (foreground == 1)
	{
		char* message = "\nEntering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, strlen(message));
		fflush(stdout);
		foreground = 0;
	}
	else
	{
		char* message = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, message, strlen(message));
		fflush(stdout);
		foreground = 1;
	}
}

int main(int argc, char* argv[])
{
	while (1)
	{
		char userInput[MAX_LEN];
		char* token;
		char inputFile[MAX_ARG];
		char outputFile[MAX_ARG];
		char* arr[MAX_ARG];
		
		int i = 0;

		int fd;
		int result;

		// Flags for "<" , ">" and background
		int redirIn = 0;
		int redirOut = 0;
		int background = 0;

		// Get process ID
		// Convert the processId to a string instead of an int
		// so that it can be swapped
		pid_t processId = getpid();
		char pid[256];
		sprintf(pid, "%d", processId);
		pid_t childPid;
		int status = 0;
		int statusVal;

		// Initialize the arrays
		memset(arr, '\0', MAX_ARG);
		memset(inputFile, '\0', MAX_ARG);
		memset(outputFile, '\0', MAX_ARG);

		// From Learning Module Signal Handling API Example
		// Signal to interrupt Ctrl + C
		struct sigaction SIGINT_action = { { 0 } };
		SIGINT_action.sa_handler = SIG_IGN;
		sigfillset(&SIGINT_action.sa_mask);
		SIGINT_action.sa_flags = 0;
		sigaction(SIGINT, &SIGINT_action, NULL);

		// Similar to the one above followed the Learning Module Example
		// Switch between foreground mode only
		// Signal for Ctrl + Z to switch to and exit foreground only mode
		struct sigaction SIGTSTP_action = { { 0 } };
		SIGTSTP_action.sa_handler = handle_SIGTSTP;
		sigfillset(&SIGTSTP_action.sa_mask);
		SIGTSTP_action.sa_flags = SA_RESTART;
		sigaction(SIGTSTP, &SIGTSTP_action, NULL);

		// Print the prompt for each command line
		// Gets the user input
		printf(": ");
		fflush(stdout);
		fgets(userInput, MAX_LEN, stdin);

		// Calls the replaceWord function for the expansion of
		// variable "$$"
		while(replaceWord(userInput, "$$", pid));

		// empty the array
		memset(arr, '\0', MAX_ARG);

		// Remove the blank line from fgets
		strtok(userInput, "\n");
		// tokenize the string
		token = strtok(userInput, " ");

		// parse user input
		while (token != NULL)
		{
			// if redirects
			// Set flag to 1 (true)
			// Use strtok again to get the word after the redirection
			// Copy that word 
			if (strcmp(token, "<") == 0)
			{
				redirIn = 1;
				token = strtok(NULL, " ");
				strcpy(inputFile, token);
				
			}
			// if redirects
			// Set flag to 1 (true)
			// Use strtok again to get the word after the redirection
			// Copy that word
			else if (strcmp(token, ">") == 0)
			{
				redirOut = 1;
				token = strtok(NULL, " ");
				strcpy(outputFile, token);
			}
			// Set the flag to 1 (true) for background processes
			else if (strcmp(token, "&") == 0)
			{
				background = 1;
				break;
			}
			else
			{
				arr[i] = strdup(token);
				token = strtok(NULL, " ");	
			}
			i++;
		}

		// Ignore lines that start with # or blank lines by doing nothing
		if (*arr[0] == '#')
		{
			;
		}
		else if (*arr[0] == '\n')
		{
			;
		}

		// Built in command that exits the program
		else if (strcmp(arr[0], "exit") == 0)
		{
			exit(0);
		}

		// Built in command that changes directories
		// Goes to home directory if a specific directory was not entered
		else if (strcmp(arr[0], "cd") == 0)
		{
			if (arr[1] == NULL)
			{
				chdir(getenv("HOME"));
			}
			else
			{
				chdir(arr[1]);
			}
		}
		
		// Built in command that prints the status
		else if (strcmp(arr[0], "status") == 0)
		{
			printf("exit value %d\n", statusVal);
			fflush(stdout);
		}

		// fork, parent processes, child process and execvp for other commands
		else
		{
			// Spawn child process
			processId = fork();

			switch (processId)
			{
			case -1:
				perror("Error: fork() has failed\n");
				fflush(stderr);
				exit(1);
				break;

			case 0:

				// foreground processes
				if (background == 0)
				{
					// Register default handler as the signal handler
					SIGINT_action.sa_handler = SIG_DFL;
					// Install our signal handler
					sigaction(SIGINT, &SIGINT_action, NULL);

					// Redirection for input
					// Opens file for read only
					// Prints an error message if file cannot be opened
					// Dup2 to copy file discriptor
					if (redirIn == 1)
					{
						fd = open(inputFile, O_RDONLY);
						if (fd == -1)
						{
							printf("Error: Unable to open file: %s for input\n", inputFile);
							fflush(stdout);
							exit(1);
						}

						result = dup2(fd, STDIN_FILENO);
						if (result == -1)
						{
							printf("Error: dup2() has failed\n");
							fflush(stdout);
							exit(1);
						}
						close(fd);
					}

					// Redirect outputfile
					// Open or create file and truncate file if it exists
					// Prints an error message if file cannot be opened
					// Dup2 to copy file discriptor
					if (redirOut == 1)
					{
						int fd2 = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (fd2 == -1)
						{
							printf("Error: Unable to open output file\n");
							fflush(stdout);
							exit(1);
						}

						result = dup2(fd2, STDOUT_FILENO);
						if (result == -1)
						{
							printf("Error: dup2() has failed\n");
							fflush(stdout);
							exit(1);
						}
						close(fd2);
					}
				}

				// background processes
				if (background == 1)
				{
					// background with no redirection
					// Opens file for read only
					// Prints an error message if file cannot be opened
					// Dup2 to copy file discriptor
					if (redirIn == 0)
					{
						fd = open("/dev/null", O_RDONLY);
						if (fd == -1)
						{
							printf("Error: Unable to open input file\n");
							fflush(stdout);
							exit(1);
						}

						result = dup2(fd, STDIN_FILENO);
						if (result == -1)
						{
							printf("Error: dup2() has failed\n");
							fflush(stdout);
							exit(1);
						}
						close(fd);
					}

					// background with no redirection
					// Open or create file and truncate file if it exists
					// Prints an error message if file cannot be opened
					// Dup2 to copy file discriptor
					if (redirOut == 0)
					{
						int fd2 = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (fd2 == -1)
						{
							printf("Error: Unable to open output file\n");
							fflush(stdout);
							exit(1);
						}

						result = dup2(fd2, STDOUT_FILENO);
						if (result == -1)
						{
							printf("Error: dup2() has failed\n");
							fflush(stdout);
							exit(1);
						}
						close(fd2);
					}
					
					// background with redirection
					// Opens file for read only
					// Prints an error message if file cannot be opened
					// Dup2 to copy file discriptor
					if (redirIn == 1)
					{
						fd = open(inputFile, O_RDONLY);
						if (fd == -1)
						{
							printf("Error: Unable to open input file\n");
							fflush(stdout);
							exit(1);
						}

						result = dup2(fd, STDIN_FILENO);
						if (result == -1)
						{
							printf("Error: dup2() has failed\n");
							fflush(stdout);
							exit(1);
						}
						close(fd);
					}

					// background with redirection
					// Open or create file and truncate file if it exists
					// Prints an error message if file cannot be opened
					// Dup2 to copy file discriptor
					if (redirOut == 1)
					{
						int fd2 = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (fd2 == -1)
						{
							printf("Error: Unable to open output file\n");
							fflush(stdout);
							exit(1);
						}

						result = dup2(fd2, STDOUT_FILENO);
						if (result == -1)
						{
							printf("Error: dup2() has failed\n");
							fflush(stdout);
							exit(1);
						}
						close(fd2);
					}
				}
				
				// Execute command
				// Return value for exec(), If the return value is -1
				// it indicates an error, print a message if theres an error
				int execVal = execvp(arr[0], arr);
				if (execVal == -1)
				{
					printf("No file or Directory with the name: %s\n", arr[0]);
					fflush(stdout);
					exit(1);
				}
				break;
				
			default:
				// Parent processes
				if (background == 1 && foreground == 1)
				{
					childPid = waitpid(processId, &status, WNOHANG);
					printf("Background pid is: %d\n", processId);
					fflush(stdout);
				}
				else
				{
					childPid = waitpid(processId, &status, 0);

					// Followed the example in Learning Module:
					// Process API - Monitoring Child Processes
					// Gets the status value of exit
					// Prints terminated message if Ctrl + C was used
					if (WIFEXITED(status))
					{
						statusVal = WEXITSTATUS(status);
					}
					else if (WIFSIGNALED(status))
					{
						statusVal = WTERMSIG(status);
						printf("terminated by signal %d\n", status);
						fflush(stdout);
					}
				}
				break;
			}

			// Checks if processes has completed
			// Gets the status value of exit or signal
			// childPid > 0 (waits for the child whose process ID is equal to the value of pid)
			childPid = waitpid(-1, &status, WNOHANG);
			while (childPid > 0)
			{
				if (WIFEXITED(status))
				{
					printf("background pid %d is done: exit value %d\n", childPid, WEXITSTATUS(status));
					fflush(stdout);
				}
				else
				{
					printf("background pid %d is done: terminated by signal %d\n", childPid,
						WTERMSIG(status));
					fflush(stdout);
				}
				childPid = waitpid(-1, &status, WNOHANG);
			}
		}
	}
	return 0;
}