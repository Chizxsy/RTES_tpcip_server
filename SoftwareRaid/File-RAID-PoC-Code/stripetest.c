#include <stdio.h>
#include <stdlib.h>

#include <syslog.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <string.h>

#include "raidlib.h"




int main(int argc, char *argv[])
{
    int bytesWritten, bytesRestored;
    char rc;
    int chunkToRebuild=0;
    struct timeval beginning_time_val;
    struct timeval end_time_val;
    
    struct timeval diff_time_val;

	
	int average_sec=0;
	int average_msec=0;

    if(argc < 3)
    {
        printf("useage: stripetest inputfile outputfile <sector to restore>\n");
        exit(-1);
    }
    
    if(argc >= 4)
    {
	sscanf(argv[3], "%d", &chunkToRebuild);
        printf("chunk to restore = %d\n", chunkToRebuild);
    }
    
    syslog(LOG_CRIT, "************** Without any optiimisations **************");
   
    
	// Note the beginning time
	gettimeofday(&beginning_time_val, (struct timezone *)0);
	bytesWritten=stripeFile(argv[1], 0); 
	// Note the end time
	gettimeofday(&end_time_val, (struct timezone *)0);

	// Caalculate time difference
	if(end_time_val.tv_usec < beginning_time_val.tv_usec)
	{
		diff_time_val.tv_usec = end_time_val.tv_usec+1000000-beginning_time_val.tv_usec;
		diff_time_val.tv_sec = end_time_val.tv_sec-1-beginning_time_val.tv_sec;
	}
	else
	{
		diff_time_val.tv_usec = end_time_val.tv_usec-beginning_time_val.tv_usec;
		diff_time_val.tv_sec = end_time_val.tv_sec-beginning_time_val.tv_sec;
	}
	
	average_sec = (int)diff_time_val.tv_sec;
	average_msec = (int)diff_time_val.tv_usec/1000;

	syslog(LOG_CRIT, "Time to complete stripeFile = %d sec: %d msec", (int)(average_sec), (int)(average_msec));
	
	
    printf("input file was written as 4 data chunks + 1 XOR parity - could have been on 5 devices\n");
    printf("Remove chunk %d and enter g for go - could have been on 5 devices\n", chunkToRebuild);
    printf("Hit return to start rebuild:");

    rc=getchar();

    printf("working on restoring file ...\n");


	// Note the beginning time
	gettimeofday(&beginning_time_val, (struct timezone *)0);
	bytesRestored=restoreFile(argv[2], 0, bytesWritten, chunkToRebuild); 
	// Note the end time
	gettimeofday(&end_time_val, (struct timezone *)0);


	// Caalculate time difference
	if(end_time_val.tv_usec < beginning_time_val.tv_usec)
	{
		diff_time_val.tv_usec = end_time_val.tv_usec+1000000-beginning_time_val.tv_usec;
		diff_time_val.tv_sec = end_time_val.tv_sec-1-beginning_time_val.tv_sec;
	}
	else
	{
		diff_time_val.tv_usec = end_time_val.tv_usec-beginning_time_val.tv_usec;
		diff_time_val.tv_sec = end_time_val.tv_sec-beginning_time_val.tv_sec;

	}

	average_sec = (int)diff_time_val.tv_sec;
	average_msec = (int)diff_time_val.tv_usec/1000;

	syslog(LOG_CRIT, "Time to complete restoreFile = %d sec: %d msec", (int)(average_sec), (int)(average_msec));
	
	syslog(LOG_CRIT, "************** END **************");
    printf("FINISHED\n");
        
}
