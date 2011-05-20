/*
 * James Alexander: 307192962
 * Comp2129 Assignment 4
 *
 * TODO consider barriers
 *
 */
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const double D = 0.85;
static const double EPSILON = 0.005;

int ncores = 0;
int nsites = 0; 
int nedges = 0;
int convergence = 0; //flage for convergence value

unsigned char *buffer;		// input buffer
int lsize = 0;		// buffer size including null terminator
int bi = 0;			// buffer inrementer
struct site {
	float pagerank;	// pagerank of the latest iteration
	float prev_pagerank; // this updates only after 2 iterations
	char cid[20];	// page id make num
	unsigned long id;	// working page rank

	int outLinks;	// number of out links
	struct site **out;	// array of out links
	struct site **in;	// array of in links
	int inIncrementer;	// next free position of the array (instead of using a linked list)
	int outIncrementer; // next free position of the out array

};

struct site *websites = NULL; // global array of websites 


/* ============================================================================
 * Helper functions
 * ========================================================================== */
/* free allocated memory */
void free_memory(void) {

	free(buffer);
	int i;
	for (i = 0; i < nsites; ++i) {
		free(websites[i].in);
		free(websites[i].out);
	}
	free(websites);

}
/* error print function and frees memory */
void error(void) {
	printf("error\n");
	free_memory();
	exit(0);
}

/* hashing function for page id */
unsigned long hash(unsigned char *str) {
	unsigned long hash = 5381;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash;
}

/* gets the current string and increments until newline or null */
char*my_strncpy(char *dest, const char *src, size_t n){
	size_t i = 0;
	for (i = 0 ; i < n && src[i] != '\0' && src[i] != '\n' && src[i] != ' '; ++i){ // NOTE: does not copy in spaces!
		dest[i] = src[i]; 
		bi++;			// sets global buffer incrementer to the new position
}
	for ( ; i < n ; i++){
		dest[i] = '\0'; // this will make all other characters null in the string
	}
	return dest;
}

/* adds an edge boths ways */
int add_edge(unsigned long a, unsigned long b) {
	int i, 
		positionA = 0, 
		positionB = 0,	// position of the website in the website array
		counterA = 0,
		counterB = 0;	// count of number of times the website is defined

	/* iterate over all websites */
	for (i = 0; i < nsites; ++i) {	// TODO do a binary search instead? // store as trei 
		if (websites[i].id == a) {
			positionA = i;
			counterA++;
		}
		if (websites[i].id == b) {
			positionB = i;
			counterB++;
		}
	}
	/* check if the website is declared > 1 or not at all */
	if ((counterA + counterB) != 2) {
		//printf("%d %d\n", counterA, counterB);
		//printf("counter edge error\n");
		return 0;
	}

	/* set the in/out sets to point to the correct websites */
	websites[positionA].out[ websites[positionA].outIncrementer ] = &websites[positionB];
	websites[positionA].outIncrementer++;
	websites[positionB].in[ websites[positionB].inIncrementer ] = &websites[positionA]; 
	websites[positionB].inIncrementer++;		// increment to next free position in array

	return 1;	// else return true
}
/* adds a new website and initalises the data and allocates the memory for the in and out * arrays */
void add_website(int website_icr, int i) {
	char website_name[20];
	my_strncpy(website_name, (char *)&buffer[i], 20);
	websites[website_icr].id = hash((unsigned char *)website_name);		// hashed id
	strcpy(websites[website_icr].cid, website_name);		// copys the string in place but may not be needed here? use fgets or something???
	websites[website_icr].pagerank = 1.0/nsites;
	websites[website_icr].prev_pagerank = 1.0/nsites;
	websites[website_icr].in = (struct site **)calloc(sizeof(struct site), nsites);	// allocate edges array space
	websites[website_icr].out = (struct site **)calloc(sizeof(struct site), nsites);
	websites[website_icr].inIncrementer = 0;		// set the incrementer values to 0 for add_edges method
	websites[website_icr].outIncrementer = 0;
	//printf("%ld %s %f\n", websites[website_icr].id, websites[website_icr].cid, websites[website_icr].pagerank);

}

/* print off custom function */
void print_websites(void) {

	int i, j;
	for (i = 0; i < nsites; ++i) {
		printf("%s %ld %f:\n ", websites[i].cid, websites[i].id, websites[i].pagerank);
		printf("in: ");
		for (j = 0; j < nsites; ++j) {
			//printf("-%s %f-", websites[websites[i].in[j]].cid, websites[websites[i].in[j]].pagerank);
			if (websites[i].in[j] != NULL)
				printf("-%s %f- ", websites[i].in[j]->cid, websites[i].in[j]->pagerank);
		}
		printf("\n out: ");
		for (j=0; j < nsites; ++j) {
			if (websites[i].out[j] != NULL)
				printf("-%s %f- ", websites[i].out[j]->cid, websites[i].out[j]->pagerank);
		}
		
		printf("\n");
		}


}

/* prints out the pageranks of the websites */
void print_pageranks(void) {
	int i;
	for (i = 0; i < nsites; ++i) {
		printf("%s %.4f\n", websites[i].cid, websites[i].pagerank);
	}
}


/* ============================================================================
 * Input functions 
 * ========================================================================== */



/* read input from stdin using fread */
int read_input(void) {
	FILE *f;

	/* get input size and allocate memory */
	f = stdin;
	fseek(f, 0L, SEEK_END);
	lsize = ftell(f);
	fseek (f, 0L, SEEK_SET);
	buffer = (unsigned char *)calloc(sizeof(unsigned char), lsize);

	/* read into buffer checking for errors*/
	if (fread(buffer, 1, lsize-1, stdin)) {
		if (feof(stdin)) {
			printf("eof error\n"); //temp
			return 0;
		}
		if (ferror(stdin)) {
			printf("file input error\n");
			return 0;
		}
	}
	return 1;

}
/* parse input from memory NOTE the 'else if' statements of control */
int parse_input(void) {	
	//int i;
	int website_icr = 0, //website incrementer
		edge_icr = 0;
	
	/* to store buffer chunks of memory before being converted or pased on. Note the memory limits of up to 15 chars for nsites */
	unsigned char tempSite[20], tempOutsite[20], tempNedges[15], tempNsites[15], tempNcores[4]; // to store tempoary strings before passed on
	unsigned long ltempSite, ltempOutsite;


	/* iterate through the buffer input */
	for (bi = 0; bi < lsize; ++bi) {
		/* check for null characters in buffer otherwise increment */
		if (buffer[bi] != '\n' && buffer[bi] != ' ') {	// NOTE: consider this when working out values
			/* set num cores and sites */
			if (bi == 0) {
				my_strncpy((char *)tempNcores, (char *)&buffer[bi], 15);
				ncores = atoi((char *)tempNcores);
				if (ncores < 1) {
					//printf("ncores error");
					return 0;	// not enough cores
				}
			}
			else if (bi == 2)	{
				my_strncpy((char *)tempNsites, (char *)&buffer[bi], 15);
				nsites = atoi((char *)tempNsites);
				if (nsites < 1) {
					//printf("nsites error");
					return 0;	// not enough websites
				}
				/* allocate memory for the website array */
				websites = (struct site *)calloc(sizeof(struct site), nsites);
			}
			else if (bi > 3) {
				/* add website names and initalise website data NOTE that bi is icremented in add_website method */
				if (website_icr < nsites) {								//check that we dont add in other data	
					add_website(website_icr, bi); 
					website_icr++;

				}
				/* set num edges and rudamentry check for num of edges */
				else if( nedges == 0) { // check to see if nedges have been set
					my_strncpy((char *)tempNedges, (char *)&buffer[bi], 15);
					nedges = atoi((char *)tempNedges); 
					if (nedges < nsites-1){		// check for are least nsite edges
						//printf("nedges error\n");
						return 0;
					}
					
				}
				/* add edge */
				else if (edge_icr < nedges) {	
					my_strncpy((char *)tempSite, (char *)&buffer[bi], 20);
					my_strncpy((char *)tempOutsite, (char *)&buffer[bi+1], 20);	// assuming the buffer bi is correct
					bi++;
					ltempSite = hash(tempSite);			// first website - CHECK HERE
					ltempOutsite = hash(tempOutsite);		// next website in buffer
					//printf("calling add edge with %s %s %ld %ld\n", tempSite, tempOutsite, ltempSite, ltempOutsite);
					if(add_edge(ltempSite, ltempOutsite)){	// this will add an edge both ways
						edge_icr++;
					}
					else {
						//printf("add edge error\n");
						return 0;
					}		
				}
			}
		}
	}
	return 1;
}

/* ============================================================================
 * PageRank Operations
 * ========================================================================== */
/* calculates the sum of the in[] array sites */
float sum_in(int icr) {
	int j;
	float tempSum = 0;
	for (j = 0; j < nsites && websites[icr].in[j] != NULL; ++j) {
		
		tempSum += (websites[icr].in[j]->prev_pagerank/websites[icr].in[j]->outIncrementer);
		//printf("%d\n", websites[icr].in[j]->outIncrementer);
		//printf("%s\n", websites[icr].in[j]->cid);
		//printf("%s\n", websites[icr].cid);
	}

	return tempSum;
}
/* calculates and store the pagerank in the respective data struct pointed to by icr */
float calculate_pagerank(int icr) {
	float tempPr = ((1 - D)/nsites) + (D*(sum_in(icr)));
	float tempCv = 0;

	/* shift the values down - Can do a check for first iteration later*/
	websites[icr].prev_pagerank = websites[icr].pagerank;
	/* set the new pagerank */
	websites[icr].pagerank = tempPr;

	/*iterative returns the pagerank difference per icrement */
	tempCv = websites[icr].pagerank - websites[icr].prev_pagerank;
	tempCv = tempCv * tempCv;	//square the result then return
	return tempCv;
}

/* performs sqrt and sets global convergence flag */
void check_convergence(float cv_sum) {
	cv_sum = sqrtf(cv_sum);
	if (cv_sum < EPSILON){
		convergence = 1;
	}
	else
		convergence = 0;
}


/* ============================================================================
 * Thread management functions
 * ========================================================================== */
void get_pagerank(void) {
	int i;
	float converge_sum = 0;
	/* sequential solution */
	while(!convergence) {
		for (i = 0; i < nsites; ++i) {
			converge_sum += calculate_pagerank(i);
			}
		check_convergence(converge_sum);
		converge_sum = 0;
	}
}


/* ============================================================================
 * Main Function
 * ========================================================================== */

int
main(void) {
	/* read in input */
	if (read_input()) {

		/* parse input from memory buffer */
		if (parse_input()) {
			get_pagerank();


		}
		else
			error();		//error from parse

	}
	else
		error();		//error from read input

	//printf("%d %d\n", ncores, nsites);
	//print_websites();
	print_pageranks();
	free_memory();
	return 0;
}
