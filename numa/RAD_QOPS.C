/*
** RAD_QOPS - Sample program that creates a BATCH process in a specific RAD
** 		and queries for it back
**
** To compile:	$ cc rad_qops.c
** To link:	$ link rad_qops
**
** To run:	run rad_qops
*/

#define __NEW_STARLET 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <starlet.h>
#include <iosbdef.h>
#include <iledef.h>
#include <lib$routines.h>
#include <ssdef.h>
#include <stsdef.h>
#include <syidef.h>
#include <quidef.h>
#include <sjcdef.h>
#include <jbcmsgdef.h>
#include <descrip.h>
#include <efndef.h>

/* RAD specific Queue Examples forward declerations*/

static int create_queue_rad(char *queue_name, int rad);
static int get_queue_rad(char *queue_name, int *rad);


/* Delete queue */
static int delete_queue (char *queue_name)
{
	struct _iosb iosb = {0};
	int status = SS$_NORMAL;
	int qlen = strlen(queue_name);

   /* Check if queue exists */
	ILE3 quiitems[] =
    { qlen, QUI$_SEARCH_NAME, queue_name, NULL,
      0, 0, NULL, NULL };

   status = sys$getquiw (0, QUI$_DISPLAY_QUEUE, 0, quiitems, &iosb, NULL, 0);
   if (status != SS$_NORMAL) return (status);
   /* return if queue does not exists */
   if (iosb.iosb$w_status == JBC$_NOSUCHQUE) return (SS$_NORMAL);

   /* Delete queue */
   	ILE3 jbcitems[] =
    { qlen, SJC$_QUEUE, queue_name, NULL,
	  0, SJC$_BATCH, NULL, NULL,
      0, 0, NULL, NULL };

   status = sys$sndjbcw (1, SJC$_RESET_QUEUE, 0, jbcitems, &iosb, NULL, 0);
   if (status != SS$_NORMAL) return (status);

   status = sys$sndjbcw (1, SJC$_DELETE_QUEUE, 0, jbcitems, &iosb, NULL, 0);
   if (status != SS$_NORMAL) return (status);
}


/* Create queue */
static int create_queue (char *queue_name, int rad)
{
	int status = 0;
	struct _iosb iosb = {0};
	int qlen = strlen(queue_name);

	/* Initially we delete the queue */
    delete_queue (queue_name);

	/* Fill the item list to create a BATCH Queue */
	ILE3 jbcitems[] =
    { qlen, SJC$_QUEUE, queue_name, NULL,
	  0, SJC$_BATCH, NULL, NULL,
	  sizeof (rad), SJC$_RAD, &rad, NULL,
      0, 0, NULL, NULL };

   status = sys$sndjbcw (1, SJC$_CREATE_QUEUE, 0, jbcitems, &iosb, NULL, 0);
   if (status != SS$_NORMAL) return (status);

   return (SS$_NORMAL);
}

static int create_queue_rad(char *queue_name, int rad)
{
	int status = SS$_NORMAL;
	status = create_queue(queue_name, rad);
	if (status != SS$_NORMAL) return (status);

	return SS$_NORMAL;
}

static int get_queue_rad(char *queue_name, int *rad)
{
	int status = 0;
	struct _iosb iosb = {0};
	int queue_status = QUI$M_QUEUE_UNAVAILABLE;
	unsigned int temp_rad = 0;
	char queue[64]= { '\0'};
	char wild[] = "*";
	unsigned long int flags = QUI$M_SEARCH_BATCH;
	char found = TRUE;

	ILE3 getquiitems[] =
    { sizeof(wild) - 1, QUI$_SEARCH_NAME, &wild, NULL,
	  sizeof (flags), QUI$_SEARCH_FLAGS, &flags, NULL,
	  sizeof (queue), QUI$_QUEUE_NAME, queue, NULL,
      sizeof (temp_rad), QUI$_RAD , &temp_rad, NULL,
      0, 0, NULL, NULL };

	/* we will cancel any operations going on */

	 status = sys$getquiw ( 0, QUI$_CANCEL_OPERATION, 0, 0, 0, 0, 0 );

	 /* we should walk through the complete QUEUE list to find the one which we are looking for */
	 while (found)
	 {
		status = sys$getquiw (EFN$C_ENF,
                                 QUI$_DISPLAY_QUEUE,
                                 0,
                                 getquiitems,
                                 &iosb,
                                 0,
                                 0);
		if (status != SS$_NORMAL) return (status);

		if(memcmp (queue_name, queue, sizeof(queue_name)) == 0)
			found = FALSE;

		if (iosb.iosb$w_status != JBC$_NOMOREQUE)
			continue;
		else found = FALSE;

	 }

   *rad = temp_rad;

	return SS$_NORMAL;

}

main ()
{

 int status = SS$_NORMAL;
 int rad = 0;
 static char queue[]= "RADTEST";

 status = create_queue(queue, 3);
 if (status != SS$_NORMAL)
	printf("Error creating queue \n");
 /*get rad info on the created queue*/

 status = get_queue_rad(queue, &rad);
 if (status != SS$_NORMAL)
	printf("Error getting RAD of the queue \n");
 else
	printf("Queue %s belongs to RAD %d \n", queue, rad);

}



