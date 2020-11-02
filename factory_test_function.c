#include "factory_test_function.h"

#include "data.h"
#include <stdio.h>
#include <stdlib.h>


// static const char commandlist[NCOMMANDS][10] = 
// {
// 	"START",
// 	"END"
// };

//#define LENUSERINPUT 1024

// struct command* userinputtocommand(char s[LENUSERINPUT])
// {
// 	//printf("userinput:%s\n",s);
// 	struct command* cmd = (struct command*) malloc(sizeof(struct command));
// 	cmd->comid = -1;
// 	cmd->count = -1;
// 	int i, j;
// 	char* token;
// 	char* savestate;
// 	for(i=0; ;i++, s=NULL)
// 	{
// 		token = strtok_r(s, " \t\n", &savestate);
// 		//printf("token:%s\n",token);
// 		if(token == NULL)
// 			break;

// 		if(cmd->comid == -1 )
// 		{
// 			for(j=0; i<NCOMMANDS; j++)
// 			{
// 				if(!strcmp(token, commandlist[j]))
// 				{
// 					cmd->comid = j;
// 					break;
// 				}
// 			}
// 		}
// 		else
// 		{
// 			if(cmd->comid == START)
// 			{
// 				append_count(cmd, token);
// 			}
// 		}

// 	}

// 	if(cmd->comid != -1 && cmd->count != -1)
// 	{
// 		printf("\t\tcmd:[%s]\n", enum2str(cmd->comid));
// 		printf("\t\tcount:[%d]\n",cmd->count);
// 		return cmd;
// 	}
// 	else
// 	{
// 		if(cmd->comid == -1)
// 			printf("Please input [START] \n");
// 		else
// 		{
// 			printf("pleas after [START] input count num\n");
// 		}
		
// 		fprintf(stderr, "\t Error parsing command\n");
// 		return NULL;
// 	}

// }


int diff_ms(const struct timespec *t1, const struct timespec *t2)
{
	struct timespec diff;

	diff.tv_sec = t1->tv_sec - t2->tv_sec;
	diff.tv_nsec = t1->tv_nsec - t2->tv_nsec;
	if (diff.tv_nsec < 0) {
		diff.tv_sec--;
		diff.tv_nsec += 1000000000;
	}
	return (diff.tv_sec * 1000 + diff.tv_nsec/1000000);
}

void printf_func_mark(char* func)
{
	printf("########################################################\n");
	printf("\t\t***  %s  ***\n", func);
	printf("########################################################\n");
}

void wait_save(char* port)
{
	printf("waite save[%s]\n",port);
	usleep(1000);
}


void dump_data(unsigned char * b, int count)
{
	printf("%i bytes: ", count);
	int i;
	for (i=0; i < count; i++) {
		printf("%02x ", b[i]);
	}

	printf("\n");
}

void dump_data_ascii(unsigned char * b, int count)
{
	int i;
	for (i=0; i < count; i++) {
		printf("%c", b[i]);
	}
}