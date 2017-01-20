/*
** RAD_ROUTINES - RAD sample routines
**
** This module contains sample RAD routines to use in a NUMA aware application.
**
** To compile:	$ cc/pointer=64 rad_routines
**
** Global routines:  
**
** get_max_rads - get the maximum number of RADs on the system
** get_home_rad - get the current process's home RAD
** get_rad_mem  - get the amount of OS private memory in each RAD
** get_rad_cpus - get the number of active CPUs in each RAD
*/

#define __NEW_STARLET 1

#include <efndef>
#include <iledef>
#include <jpidef>
#include <prcdef>
#include <ssdef>
#include <starlet>
#include <syidef>
#include <lib$routines>
#include <stdlib>

/* 
** Global data cells - This data is obtained once for the life of this 
** process. Zero indicates that the data is not yet obtained from the 
** system. 
**
*/
	static int max_rads=0;		
	static int max_cpus=0;		
	typedef struct _rad_cpu_id_pair {
	    int rad_id;
	    int cpu_id;
	} RAD_CPU_ID_PAIR;
	static RAD_CPU_ID_PAIR * rad_cpu_id_buffer=0;

/* 
** get_max_rads - return the maximum number of RADs on this system
**
** Inputs: none
** 
** Returns: maximum number of RADs on this system. This number is constant
** 	    for the system. It only needs to be obtained once.
*/
int get_max_rads (void)
{
	/* Local variables */
	ILEB_64 item_list[2];
	unsigned __int64 return_length;
	int status;

	/* Max RADs is a system constant, don't ask more than once */
	if (max_rads != 0) return (max_rads);

	/* Set up RAD_MAX_RADS item list */
	item_list[0].ileb_64$w_mbo 	= 1;
	item_list[0].ileb_64$l_mbmo 	= -1;
	item_list[0].ileb_64$q_length 	= 4;
	item_list[0].ileb_64$w_code 	= SYI$_RAD_MAX_RADS;
	item_list[0].ileb_64$pq_bufaddr = &max_rads;
	item_list[0].ileb_64$pq_retlen_addr = &return_length;
	item_list[1].ileb_64$w_mbo 	= 0;
	item_list[1].ileb_64$l_mbmo 	= 0;
	item_list[1].ileb_64$q_length 	= 0;
	item_list[1].ileb_64$w_code 	= 0;
	item_list[1].ileb_64$pq_bufaddr = 0;
	item_list[1].ileb_64$pq_retlen_addr = 0;

	/* Call sys$getsyiw to get the maximum number of RADs */
	status = sys$getsyiw (
			EFN$C_ENF,	/* efn 			*/
			0,		/* csiadr 		*/
			0,		/* nodename 		*/
			item_list,	/* itmlst   		*/
			0,		/* I/O status block 	*/
			0,		/* AST address		*/	
			0);		/* AST parameter        */

	/* If RAD_MAX_RADS not supported, assume 1 RAD */
	if (status == SS$_BADPARAM)
	{
            max_rads = 1;
	    status = SS$_NORMAL;
	} 

	/* sys$getsyiw shouldn't fail for any other reason */
	if (!(status&1)) 
	    lib$signal(status);		

	/* Return max RADs to caller */
	return (max_rads);		
}

/* 
** get_home_rad - return the home RAD of this process
**
** Inputs: none
**
** Returns: current process's home RAD
**
*/
int get_home_rad (void)
{
	/* Local variables */
	ILEB_64 item_list[2];
	unsigned __int64 return_length;
	int home_rad;
	int status;

	/* If only one RAD, our home RAD must be 0 */
	if (get_max_rads() == 1) 
	    return (0);

	/* Set up HOME_RAD item list */
	item_list[0].ileb_64$w_mbo 	= 1;
	item_list[0].ileb_64$l_mbmo 	= -1;
	item_list[0].ileb_64$q_length 	= 4;
	item_list[0].ileb_64$w_code 	= JPI$_HOME_RAD;
	item_list[0].ileb_64$pq_bufaddr = &home_rad;
	item_list[0].ileb_64$pq_retlen_addr = &return_length;
	item_list[1].ileb_64$w_mbo 	= 0;
	item_list[1].ileb_64$l_mbmo 	= 0;
	item_list[1].ileb_64$q_length 	= 0;
	item_list[1].ileb_64$w_code 	= 0;
	item_list[1].ileb_64$pq_bufaddr = 0;
	item_list[1].ileb_64$pq_retlen_addr = 0;

	/* Call sys$getjpiw to get this process's home RAD */
	status = sys$getjpiw (
			EFN$C_ENF,	/* efn 			*/
			0,		/* pidadr 		*/
			0,		/* prcnam 		*/
			item_list,	/* itmlst   		*/
			0,		/* I/O status block 	*/
			0,		/* AST address		*/	
			0);		/* AST parameter        */

	/* sys$getjpiw shouldn't fail */
	if (!(status&1)) 
	    lib$signal(status);		

	/* Return home RAD to caller */
	return (home_rad);	       
}

/*
** get_rad_mem - return how much memory is in each RAD
**
** Inputs: buffer - address of buffer
**	   buffer_length - length of buffer passed in
**
** Outputs: 
**	buffer - filled in with page count array indexed by RAD
**
** Return values:
**	SS$_NORMAL - success
**	SS$_BUFFEROVF - user buffer not large enough for return array
** 	SS$_INSFMEM - error allocating dynamic memory with malloc()
** 	error status values from sys$getsyiw 
**
*/
int get_rad_mem (int * buffer, int buffer_length)
{
	/* Local type definition */
	typedef struct _rad_mem_pair {
	    int rad_id;
	    int page_count;
	} RAD_MEM_PAIR;

	/* Local variables */
	ILEB_64 item_list[2];
	unsigned __int64 return_length;
	int status,i,rad;
	RAD_MEM_PAIR * rad_mem_buffer;
	int memsize;

 	/* Check the length of the user's buffer */
	if (buffer_length < get_max_rads()*sizeof(int))
	    return (SS$_BUFFEROVF);

        /* If only one RAD, just get system memory size */
	if (get_max_rads() == 1)
	{
	    /* Set up MEMSIZE item list */
	    item_list[0].ileb_64$w_mbo 		= 1;
	    item_list[0].ileb_64$l_mbmo 	= -1;
 	    item_list[0].ileb_64$q_length 	= 4;
	    item_list[0].ileb_64$w_code 	= SYI$_MEMSIZE;
	    item_list[0].ileb_64$pq_bufaddr 	= &memsize;
	    item_list[0].ileb_64$pq_retlen_addr = &return_length;
	    item_list[1].ileb_64$w_mbo 		= 0;
	    item_list[1].ileb_64$l_mbmo 	= 0;
	    item_list[1].ileb_64$q_length 	= 0;
	    item_list[1].ileb_64$w_code 	= 0;
	    item_list[1].ileb_64$pq_bufaddr 	= 0;
	    item_list[1].ileb_64$pq_retlen_addr = 0;

	    /* Call sys$getsyiw to get memsize */
	    status = sys$getsyiw (
			EFN$C_ENF,	/* efn 			*/
			0,		/* csiadr 		*/
			0,		/* nodename 		*/
			item_list,	/* itmlst   		*/
			0,		/* I/O status block 	*/
			0,		/* AST address		*/	
			0);		/* AST parameter        */
	   
	    /* On success, return page count in the user's buffer */
	    if (status&1)
	      	buffer[0] = memsize;
	    return (status);
	}

	/* Allocate RAD/MEM array */
	rad_mem_buffer = malloc (get_max_rads()*sizeof(RAD_MEM_PAIR));
	if (rad_mem_buffer == 0) return (SS$_INSFMEM);

	/* Set up RAD_MEMSIZE item list */
	item_list[0].ileb_64$w_mbo 	= 1;
	item_list[0].ileb_64$l_mbmo 	= -1;
	item_list[0].ileb_64$q_length 	= get_max_rads()*sizeof(RAD_MEM_PAIR);
	item_list[0].ileb_64$w_code 	= SYI$_RAD_MEMSIZE;
	item_list[0].ileb_64$pq_bufaddr = rad_mem_buffer;
	item_list[0].ileb_64$pq_retlen_addr = &return_length;
	item_list[1].ileb_64$w_mbo 	= 0;
	item_list[1].ileb_64$l_mbmo 	= 0;
	item_list[1].ileb_64$q_length 	= 0;
	item_list[1].ileb_64$w_code 	= 0;
	item_list[1].ileb_64$pq_bufaddr = 0;
	item_list[1].ileb_64$pq_retlen_addr = 0;

	/* Call sys$getsyiw to get RAD/MEM array */
	status = sys$getsyiw (
			EFN$C_ENF,	/* efn 			*/
			0,		/* csiadr 		*/
			0,		/* nodename 		*/
			item_list,	/* itmlst   		*/
			0,		/* I/O status block 	*/
			0,		/* AST address		*/	
			0);		/* AST parameter        */

	/* On success, return requested info */
	if (status&1)
	{
	    /* For each RAD, add up the page count */
	    for (rad=0; rad<get_max_rads(); rad++)
	    {
		buffer[rad] = 0;
		for (i=0; i<get_max_rads(); i++)
		    if (rad_mem_buffer[i].rad_id == rad)
		    	buffer[rad] += rad_mem_buffer[i].page_count;
	    }
	}

	/* Free RAD/MEM array and return status */
	free (rad_mem_buffer);
	return (status);
}

/* 
** get_max_cpus - return the maximum number of CPUs on this system
**
** Inputs: none
**
** Returns: maximum number of CPUs on this system. This number is constant
** 	    for the system. It only needs to be obtained once.
**
*/
static int get_max_cpus (void)
{
	/* Local variables */
	ILEB_64 item_list[2];
	unsigned __int64 return_length;
	int status;

	/* Max CPUs is a system constant, don't ask more than once */
	if (max_cpus != 0) return (max_cpus);

	/* Set up MAX_CPUS item list */
	item_list[0].ileb_64$w_mbo 	= 1;
	item_list[0].ileb_64$l_mbmo 	= -1;
	item_list[0].ileb_64$q_length 	= 4;
	item_list[0].ileb_64$w_code 	= SYI$_MAX_CPUS;
	item_list[0].ileb_64$pq_bufaddr = &max_cpus;
	item_list[0].ileb_64$pq_retlen_addr = &return_length;
	item_list[1].ileb_64$w_mbo 	= 0;
	item_list[1].ileb_64$l_mbmo 	= 0;
	item_list[1].ileb_64$q_length 	= 0;
	item_list[1].ileb_64$w_code 	= 0;
	item_list[1].ileb_64$pq_bufaddr = 0;
	item_list[1].ileb_64$pq_retlen_addr = 0;

	/* Call sys$getsyiw to get the maximum number of CPUs */
	status = sys$getsyiw (
			EFN$C_ENF,	/* efn 			*/
			0,		/* csiadr 		*/
			0,		/* nodename 		*/
			item_list,	/* itmlst   		*/
			0,		/* I/O status block 	*/
			0,		/* AST address		*/	
			0);		/* AST parameter        */

	/* sys$getsyiw shouldn't fail */
	if (!(status&1)) 
	    lib$signal(status);		

	/* Return max number of CPUs */
	return (max_cpus);	       
}

/*
** get_rad_cpus - return number of active CPUs for each RAD
**
** Inputs: buffer - address of buffer
**	   buffer_length - length of buffer passed in
**
** Outputs: 
**	buffer - filled in with CPU count array indexed by RAD
**
** Return values:
**	SS$_NORMAL - success
**	SS$_BUFFEROVF - user buffer not large enough for return array
** 	SS$_INSFMEM - error allocating dynamic memory with malloc()
** 	error status values from sys$getsyiw 
**
*/
int get_rad_cpus (int * buffer, int buffer_length)
{
	/* Local variables */
	ILEB_64 item_list[2];
	unsigned __int64 return_length;
	int status;
	int i;
	int rad,cpu;
	unsigned __int64 active_cpu_mask;

 	/* Check the length of the user's buffer */
	if (buffer_length < get_max_rads()*sizeof(int))
	    return (SS$_BUFFEROVF);

	/* Set up ACTIVE_CPU_MASK item list */
	item_list[0].ileb_64$w_mbo 	= 1;
	item_list[0].ileb_64$l_mbmo 	= -1;
	item_list[0].ileb_64$q_length 	= 8;
	item_list[0].ileb_64$w_code 	= SYI$_ACTIVE_CPU_MASK;
	item_list[0].ileb_64$pq_bufaddr = &active_cpu_mask;
	item_list[0].ileb_64$pq_retlen_addr = &return_length;
	item_list[1].ileb_64$w_mbo 	= 0;
	item_list[1].ileb_64$l_mbmo 	= 0;
	item_list[1].ileb_64$q_length 	= 0;
	item_list[1].ileb_64$w_code 	= 0;
	item_list[1].ileb_64$pq_bufaddr = 0;
	item_list[1].ileb_64$pq_retlen_addr = 0;

	/* Call sys$getsyiw to get active cpu mask */
	status = sys$getsyiw (
			EFN$C_ENF,	/* efn 			*/
			0,		/* csiadr 		*/
			0,		/* nodename 		*/
			item_list,	/* itmlst   		*/
			0,		/* I/O status block 	*/
			0,		/* AST address		*/	
			0);		/* AST parameter        */

	/* Return on error */
	if (!(status&1)) 
	    return (status);

	/* If only one RAD, all active CPUs are in RAD 0 */
	if (get_max_rads() == 1)
	{
	    /* Count the number of CPUs in the active CPU mask */
	    buffer[0] = 0;
	    for (cpu=0; cpu<get_max_cpus(); cpu++)
	    {
		if ((active_cpu_mask>>cpu)&1)
		    buffer[0]++;
	    }
	    return (SS$_NORMAL);
	}

	/* Get the RAD/CPU id info */
	if (rad_cpu_id_buffer == 0)
	{
	#ifdef __IA64
		/*allocate space for cpus in ILM RAD also on IA64*/
	  rad_cpu_id_buffer = 
	        malloc ((get_max_cpus()*2+1)*sizeof(RAD_CPU_ID_PAIR));
	#else
      rad_cpu_id_buffer =
               malloc ((get_max_cpus()+1)*sizeof(RAD_CPU_ID_PAIR));
	#endif
		if (rad_cpu_id_buffer == 0) return (SS$_INSFMEM);

	    /* Set up RAD_CPUS item list */
	    item_list[0].ileb_64$w_mbo = 1;
	    item_list[0].ileb_64$l_mbmo = -1;
    #ifdef __ia64
 	    item_list[0].ileb_64$q_length = (max_cpus*2+1)*sizeof(RAD_CPU_ID_PAIR);
	#else
        item_list[0].ileb_64$q_length = (max_cpus+1)*sizeof(RAD_CPU_ID_PAIR);
    #endif
        item_list[0].ileb_64$w_code 	    = SYI$_RAD_CPUS;
	    item_list[0].ileb_64$pq_bufaddr 	= rad_cpu_id_buffer;
	    item_list[0].ileb_64$pq_retlen_addr = &return_length;
	    item_list[1].ileb_64$w_mbo 		= 0;
	    item_list[1].ileb_64$l_mbmo 	= 0;
	    item_list[1].ileb_64$q_length 	= 0;
	    item_list[1].ileb_64$w_code 	= 0;
	    item_list[1].ileb_64$pq_bufaddr 	= 0;
	    item_list[1].ileb_64$pq_retlen_addr = 0;

	    /* Call sys$getsyiw to get the RAD/CPU info */
	    status = sys$getsyiw (
			EFN$C_ENF,	/* efn 			*/
			0,		/* csiadr 		*/
			0,		/* nodename 		*/
			item_list,	/* itmlst   		*/
			0,		/* I/O status block 	*/
			0,		/* AST address		*/	
			0);		/* AST parameter        */

	    /* On error, free memory and return */
	    if (!(status&1)) 
	    {
		free (rad_cpu_id_buffer);
		rad_cpu_id_buffer=0;
		return (status);
	    }
	}

	/* Clear CPU count for each rad */
	for (rad=0; rad<get_max_rads(); rad++)
	    buffer[rad] = 0;
			
	/* Loop through RAD/CPU array counting active CPUs in each RAD */
	i = 0;
	while (rad_cpu_id_buffer[i].cpu_id != -1)
	{
	    /* Get a RAD/CPU pair */
	    rad = rad_cpu_id_buffer[i].rad_id;
	    cpu = rad_cpu_id_buffer[i].cpu_id;

	    /* Count this CPU if it is in the active cpu mask */
	    if ((active_cpu_mask>>cpu)&1)
	    	buffer[rad]++;
	    i++;
	}
	return (SS$_NORMAL);		
}
