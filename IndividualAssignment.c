#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#define SIZE 200


void sigint_handler(int sig)
{
	(void)sig;
	const char msg[] = "\nProgram interrupted. Restaurant is closing...\n";
	write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	_exit(0);
}

int main ()
{
	int kitchen_p[2], packing_p[2];
	pid_t kitchen, packing;   
	char choice[SIZE];
	char order[SIZE]; 		

	struct sigaction sa = {0};
	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);

	if (pipe(kitchen_p) == -1 || pipe(packing_p) == -1)
	{
		perror("pipe");
		return 1;
		
	}

	kitchen = fork();

	if (kitchen == 0)
	{
		signal(SIGINT, SIG_IGN);
		
		close(kitchen_p[1]);
		close(packing_p[0]);
		close(packing_p[1]);

		char receive[SIZE];
		ssize_t n = read(kitchen_p[0], receive, SIZE - 1);
		
		if (n > 0)
		{
			receive[n] = '\0';
			printf("\n\n[Kitchen Worker | child 1 ] Received the order: %s", receive);
			printf("[Kitchen Worker] is preparing the food \n");
			fflush(stdout);
		}
		
		close(kitchen_p[0]);
		_exit(0);
	
	}

	if (kitchen < 0)
	{
		perror("fork");
		return 1;

	}

	packing = fork();
	
	if (packing == 0)
	{
		signal(SIGINT, SIG_IGN);

		close(packing_p[1]);
		close(kitchen_p[0]);
		close(kitchen_p[1]);

		char receive[SIZE];
		ssize_t n = read(packing_p[0], receive, SIZE - 1);

		if (n > 0)
		{
			receive[n] = '\0';
			printf("\n\n[Packaging Worker | child 2 ] Received the order: %s", receive);
			printf("[Packaging Worker] is preparing takeaway packaging \n");
			fflush(stdout);
		}
		
		close(packing_p[0]);
		_exit(0);

	}

	if (packing < 0)
	{
		perror("fork");
		close(kitchen_p[1]);
		close(packing_p[1]);
		waitpid(kitchen, NULL, 0);
		return 1;
	}

	close(kitchen_p[0]);
	close(packing_p[0]);

	printf("\n\n===== RESTAURANT ORDER SYSTEM =====\n\n");
	printf("[Manager | parent] is ready. \n");

	while (1)
	{
		printf("Enter customer order [SET A - Burger and Coke | SET B - Fried chicken | SET C - Loaded potato]: ");
		fflush(stdout);

		if (fgets(choice, SIZE, stdin) == NULL)
		{
			close(kitchen_p[1]);
			close(packing_p[1]);
			waitpid(kitchen, NULL, 0);
			waitpid(packing, NULL , 0);
		
			return 1;
		}

		choice[strcspn(choice, "\n")] = '\0';

		if (strlen(choice) == 1)
		{
			choice[0] = toupper((unsigned char)choice[0]);
	
			if (choice[0] == 'A' || choice[0] == 'B' || choice[0] == 'C')
			{
				break;
			}
	
		}

		printf("\nInvalid choice! Please order again.\n");
	}

	if (choice[0] == 'A')
	{
		snprintf(order, SIZE, "Set A - Burger and Coke\n");
	}
	else if (choice[0] == 'B')
	{
		snprintf(order, SIZE, "Set B - Fried Chicken\n");
	}
	else
	{
		snprintf(order, SIZE, "Set C - Loaded potato\n");
	}

	printf("\nOrder accepted: %s", order);

	write(kitchen_p[1], order, strlen(order));
	write(packing_p[1], order, strlen(order));

	close(kitchen_p[1]);
	close(packing_p[1]);

	waitpid(kitchen, NULL, 0);
	waitpid(packing, NULL, 0);

	printf("\n\nManager: Order processing completed.\n");


	return 0;
}



