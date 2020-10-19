// SPDX-License-Identifier: MIT

#include <stddef.h>

#include "serial_test.h"
#include "data.h"
#include "serial_test.h"
#include "can_test.h"




int main(int argc, char * argv[])
{

	printf("______________________________________________________________________________________\n");
	printf("______________________________________________________________________________________\n\n");
	printf("\tAndroid factory test client===>>>\n");
	printf("______________________________________________________________________________________\n");
	printf("______________________________________________________________________________________\n\n");


	// char userinput[LENUSERINPUT];
	// struct command* cmd;

	while(1)
	{
		//printf("\t^_^ please input [START] and click [Enter] end for the [%d]'s factory test^_^\t\n:", ++test_count);
		//fgets(userinput, LENUSERINPUT, stdin);
		//cmd = userinputtocommand(userinput);
		//if(cmd == NULL)
		//	continue;
		AndriodProduct my_android = {'\0', 0, 0, 0, 0, true};
		printf("***************************************************\n");
		printf("***************************************************\n");
		printf("***************  Step1: serial_process  ***********\n");
		printf("***************************************************\n");
		printf("*****************************************\n");
		serial_test(&my_android);
		can_process(&my_android);

		if( strlen(my_android.cpu_sn) != 0)
		{
			save_test_result(&my_android);

		}
		

	}
	

	
	
	
}