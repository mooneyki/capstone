#ifndef NUBAJA_FAULT_H_
#define NUBAJA_FAULT_H_

struct fault 
{
	int overcurrent_fault; 
	int overtemp_fault; 
	int overvolt_fault;
	int trip;  
};
typedef struct fault fault_t;

void clear_faults ( fault_t *fault )
{
	fault->overcurrent_fault = 0;
	fault->overtemp_fault = 0;
	fault->overvolt_fault = 0;
	fault->trip = 0;
}

void print_faults ( fault_t *fault ) 
{
	if ( fault->trip ) {
		if ( fault->overcurrent_fault ) {
			printf("Overcurrent fault %d \n", fault->overcurrent_fault );
		}
		if ( fault->overtemp_fault ) {
			printf("Overtemp fault %d \n", fault->overtemp_fault );
		}
		if ( fault->overvolt_fault ) {
			printf("Overvolt fault %d \n", fault->overvolt_fault );
		}
	}
	else {
		printf("No faults.\n");
	}		
}


#endif
