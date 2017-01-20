/*
** RAD_CRMPSC - Sample program that creates a memory-resident global section 
**		on the process's home RAD
**
** To compile:	$ cc/pointer=64/prefix=all rad_crmpsc
** To link:	$ link rad_crmpsc + rad_routines
** To run:	$ run rad_crmpsc
*/

#define __NEW_STARLET 1

#include <stdio>
#include <stdlib>
#include <descrip>
#include <gen64def>
#include <secdef>
#include <ssdef>
#include <starlet>
#include <vadef>
#include <signal>

/* External routines from rad_routines */
int get_home_rad (void);
int get_max_rads (void);

/*
** create_mres - create a memory-resident global section on the specified rad
**
** Inputs: rad - RAD to create global section on
**	   size - size of global section
**
** Output: return_va - address at which global section was mapped 
** 
** Returns:
**	   SS$_NORMAL or error status from sys$create_region_64 or 
** 	   sys$crmpsc_gdzro_64
**                                     
*/
int create_mres (int rad, unsigned __int64 mres_length, void ** return_va)
{
	/* Local variables */
 	int status;
	GENERIC_64 region_id;
	void * region_va;
	unsigned __int64  region_length; 
	void * start_va;
	unsigned __int64  section_length; 

	/* Declare global section name descriptor */
	char secnam_text[] =  "rad_crmpsc_0";
	struct dsc64$descriptor_s secnam;

	/* Initialize global section name descriptor */
	secnam.dsc64$w_mbo = 1;
	secnam.dsc64$l_mbmo = -1;
	secnam.dsc64$q_length = sizeof(secnam_text)-1;
	secnam.dsc64$b_dtype = DSC64$K_DTYPE_T;
	secnam.dsc64$b_class = DSC64$K_CLASS_S;
	secnam.dsc64$pq_pointer = secnam_text;

	/* Include RAD number in the global section name */
	secnam_text[sizeof(secnam_text)-2] += rad;

	/* 
	** Create huge region where we can share page tables with other
	** processes that map to this same global section.
	*/
	region_length = 64;
	region_length *= 1024*1024*1024;
	status = sys$create_region_64 (
            region_length,       	/* 64gb        */
	    0,				/* Region prot */
	    VA$M_SHARED_PTS,		/* Flags       */
	    &region_id,			/* Region ID   */
	    &region_va,			/* Region VA   */
	    &region_length		/* Region length */
	);

	/* Return on error */
	if (!(status&1)) return (status);

	/* Create the global section */
	if (get_max_rads() == 1) 

	    /* If only one RAD, don't supply RAD hint */
	    status = sys$crmpsc_gdzro_64 (
            	&secnam,		/* Section name */
	   	0,			/* Ident        */
	   	0,			/* Protection   */
	   	mres_length,		/* Length       */
	   	&region_id,		/* Region ID    */
	   	0,			/* Offset       */
	   	0,			/* Access mode  */
	   	SEC$M_SYSGBL|SEC$M_EXPREG, 
	   	&start_va,		/* Return VA    */
	   	&section_length         /* Return Length */
	    );

        else
	    /* If more than one RAD, supply RAD hint */
	    status = sys$crmpsc_gdzro_64 (
           	&secnam,		/* Section name */
	   	0,			/* Ident        */
	   	0,			/* Protection   */
	   	mres_length,		/* Length       */
	   	&region_id,		/* Region ID    */
	   	0,			/* Offset       */
	   	0,			/* Access mode  */
	   	SEC$M_SYSGBL|SEC$M_EXPREG|SEC$M_RAD_HINT,	
	   	&start_va,		/* Return VA    */
	   	&section_length,     	/* Return Length */
	   	0,			/* Start VA     */
		0,			/* Map length   */
	   	0,			/* Reserved length */
	   	1<<rad			/* RAD mask     */
	    );

	/* Return VA on success */
	if (status&1) *return_va = start_va;
	return (status);
}

/* 
** Create a memory resident global section on this process's home RAD
*/
main (void)
{
	int status;
	int home_rad;
	void *mres_va;
	unsigned __int64 mres_length;
	unsigned __int64 * ptr;

	/* Get our process's home RAD */
	home_rad = get_home_rad();

	/* Create an 8MB global section */
	mres_length = 8*1024*1024;
	status = create_mres (home_rad, mres_length, &mres_va); 
	if (!(status&1)) exit(status);

	/* Loop writing to the global section periodically */
	ptr = mres_va;
	while (1)
	{
	    /* Read and write global section */
	    *ptr = *ptr+1;

	    /* Update pointer. If we're above VA range, start at beginning. */
	    ptr = ptr+64;
	    if ((unsigned __int64)ptr >= 
	       ((unsigned __int64)ptr + mres_length))
	 	ptr = mres_va;

	    /* Wait for one second */
	    sleep (1);           
	}

	/* Make the compiler happy */
	return (SS$_NORMAL);
}
