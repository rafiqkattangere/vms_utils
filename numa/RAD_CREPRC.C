/*
** RAD_CREPRC - Sample program that creates a process on each RAD in the system
** 		that contains both memory and active CPUs
**
** To compile:	$ cc/pointer=64 rad_creprc
** To link:	$ link rad_creprc + rad_routines
** To run:	Create rad_crmpsc.com in your sys$login directory with  
**		$ run rad_creprc
*/
#define __NEW_STARLET 1

#include <descrip>
#include <efndef>
#include <prcdef>
#include <ssdef>
#include <starlet>
#include <stdio>
#include <stdlib>

/* External routines from rad_routines */
int get_max_rads (void);
int get_rad_mem (int * buffer, int buffer_length);
int get_rad_cpus (int * buffer, int buffer_length);

/*
** create_process - create a process on the specified rad
**
** Inputs: rad - RAD to create process on
**
** Output: none
**
** Returns: 
**	SS$_NORMAL or error status from sys$creprc
**
*/
int create_process (int rad)
{
	/* Local variables */
 	int status;
	unsigned int pid;
	char * ptr;

	/* Image to run is loginout */
	$DESCRIPTOR (image,"sys$system:loginout.exe");

	/* Created process invokes the rad_crmpsc command procedure */
	$DESCRIPTOR (input,"rad_crmpsc.com");

	/* Discard output and error - just for example purposes */
	$DESCRIPTOR (output,"nl:");
	$DESCRIPTOR (error,"nl:");

	/* Declare process name descriptor */
	char prcnam_text[] =  "rad_crmpsc_0";
	struct dsc$descriptor_s prcnam;
	
	/* Initialize process name descriptor */
	prcnam.dsc$w_length = sizeof(prcnam_text)-1;
	prcnam.dsc$b_dtype = DSC$K_DTYPE_T;
	prcnam.dsc$b_class = DSC$K_CLASS_S;
	prcnam.dsc$a_pointer = prcnam_text;

	/* Include RAD number in the process name */
	prcnam_text[sizeof(prcnam_text)-2] += rad;

	/* Create process on specified RAD */
	if (get_max_rads() == 1)
	
	    /* If only one RAD, don't supply home RAD argument */
	    status = sys$creprc (&pid,
			&image,         /* image  */
			&input,		/* input  */
			&output,    	/* output */
			&error,         /* error  */
			0,		/* prvadr */
			0,		/* quota  */
			&prcnam, 	/* prcnam */
			4,		/* baspri */
			0,		/* uic    */
			0,		/* mbxunt */
			PRC$M_DETACH	/* stsflg */ 
	    );

	else
	    /* If more than one RAD, specify home RAD */
	    status = sys$creprc (&pid,
			&image,         /* image  */
			&input,		/* input  */
			&output,    	/* output */
			&error,         /* error  */
			0,		/* prvadr */
			0,		/* quota  */
			&prcnam, 	/* prcnam */
			4,		/* baspri */
			0,		/* uic    */
			0,		/* mbxunt */
			PRC$M_DETACH|PRC$M_HOME_RAD, 
			0,		/* itmlst */
			0,		/* node   */
			rad		/* home rad */
	    );

	/* Return status */
	return (status);
}

/* 
** Create a process on each RAD that contains both memory and active CPUs 
*/
main (void)
{
	/* Local variables */
	int status;
	int max_rads;
	int rad;
	int * cpu_array;
	int * mem_array;

	/* Determine the maximum number of RADs on this system */
	max_rads = get_max_rads();

	/* Get RAD/MEM info */
	mem_array = malloc (max_rads*sizeof(int));	
	if (mem_array == 0) return (SS$_INSFMEM);
	status = get_rad_mem (mem_array,max_rads*sizeof(int));
	if (!(status&1)) exit(status);

	/* Get RAD/CPU info */
	cpu_array = malloc (max_rads*sizeof(int));
	if (cpu_array == 0) return (SS$_INSFMEM);
	status = get_rad_cpus (cpu_array,max_rads*sizeof(int));
	if (!(status&1)) exit(status);

	/* Create a process on each RAD with CPUs and memory */
	for (rad=0; rad<max_rads; rad++)
	{
	    if (mem_array[rad] && cpu_array[rad])
	    {
		printf ("Creating process on RAD %d\n",rad);
	   	status = create_process (rad);
	   	if (!(status&1)) exit(status);
	    }
	}
	printf ("All done.\n");

	/* Return success */
	return (SS$_NORMAL);
}
