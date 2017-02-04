/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor :  Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdint.h>
//#include <stdarg.h>
//#include <sys/socket.h>
//#include <sys/un.h>
//#include <sys/select.h>
//#include <sys/time.h>
//#include <sys/types.h>
//#include <sys/mman.h>
//#include <fcntl.h>
//#include <unistd.h>
//#include <errno.h>
//#include <netinet/in.h>
//#include <stdbool.h>

#define LINES 10

void print_val(unsigned int delta, int digit, int nuldigit, unsigned int value)
{
	if (value < delta) value = 0;
	else value -= delta;

	unsigned val1 = value / 100;
	value -= (100*val1);
	unsigned val2 = value / 10;
	value -= (10*val2);
	fprintf(stdout,"image place image %d %d\n", digit, nuldigit+val1);
	fprintf(stdout,"image place image %d %d\n", digit+1, nuldigit+val2);
	fprintf(stdout,"image place image %d %d\n", digit+3, nuldigit+value);
}

int main(int argc, char *argv[])
{
	char str[100];
	int index[3];
	unsigned int delta[3];
	unsigned int val[5];
	unsigned int avg0[LINES];
	unsigned int avg1[LINES];
	unsigned int avg2[LINES];
	unsigned int total0 = 0;
	unsigned int total1 = 0;
	unsigned int total2 = 0;
	memset((void*)&avg0,0,sizeof(avg0));
	memset((void*)&avg1,0,sizeof(avg1));
	memset((void*)&avg2,0,sizeof(avg2));
	if (argc < 7 ||
		sscanf(argv[1],"%u",&delta[0]) != 1 ||
		sscanf(argv[2],"%u",&delta[1]) != 1 ||
		sscanf(argv[3],"%u",&delta[2]) != 1 ||
		sscanf(argv[4],"%d",&index[0]) != 1 ||
		sscanf(argv[5],"%d",&index[1]) != 1 ||
		sscanf(argv[6],"%d",&index[2]) != 1 ||
		index[0] > 4 ||
		index[1] > 4 ||
		index[2] > 4 ) {
		fprintf(stderr, "invalid arguments\nUsage : %s <delta 1> <delta 2> <delta 3> <bar 1> <bar 2> <bar 3>\n",argv[0]);
		exit(1);
	}
	int line = 0;
	int n = 0;
	//int prev = LINES - 1;
	while (gets(str) && *str) {
		if (sscanf(str, "%u,%u,%u,%u,%u",
			&val[0], &val[1], &val[2], &val[3], &val[4]) != 5) break;
		for (int i=0 ; i < 5; i++) if (val[i] > 999) val[i] = 999;
		total0 = total0 + val[index[0]] - avg0[n];
		total1 = total1 + val[index[1]] - avg1[n];
		total2 = total2 + val[index[2]] - avg2[n];
		avg0[n] = val[index[0]];
		avg1[n] = val[index[1]];
		avg2[n] = val[index[2]];
		if (n == LINES - 1) {
			print_val(delta[0], 30, 20, total0/LINES);
			print_val(delta[1], 34, 20, total1/LINES);
			print_val(delta[2], 38, 20, total2/LINES);
			fflush(stdout);
		}

		line++;
		n = line % LINES;
	}
	return 0;
}
