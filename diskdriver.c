/*
 NAME:              ONG MING THYE, DERRICK
 SIT MATRIC NO:     13AGC039H
 GUID:              2110010O
 DATE CREATED:      09 MAR 14
 DATE COMPLETED:1   2 MAR 14
 SUBMISSION DATE:   14 MAR 14
 ASSIGNMENT:        0S3 ASSESSED CODING EXERCISE 2013-2014
 FILENAME:          DISKDRIVER.C
 */

#include "diskdriver.h"
#include "sectordescriptorcreator.h"
#include "freesectordescriptorstore_full.h"
#include "BoundedBuffer.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX 10


//global variable
FreeSectorDescriptorStore fsds;
DiskDevice disk;
BoundedBuffer workQ;
BoundedBuffer readQ;
pthread_mutex_t vmutex;
pthread_cond_t vcond = PTHREAD_COND_INITIALIZER;


//defining the structure of the voucher
typedef struct Voucher {
	int status;
	SectorDescriptor sd;
} Vouchers;


//defining the write function.
//main usage is to write onto the sector.

void *write() {

	Vouchers *vou;
	int status;

	while (1) {
		vou = (Vouchers *) blockingReadBB(workQ);
		status = write_sector(disk, &(vou->sd));

		pthread_mutex_lock(&vmutex); 
		vou->status = status;
		pthread_cond_broadcast(&vcond);
		pthread_mutex_unlock(&vmutex);
	
		blocking_put_sd(fsds, &(vou->sd));
	}
}


// defining the write function
// main usage is to read for a certain sector
void *read() {

	Vouchers *vou;
	int status;

	while (1) {
		vou = (Vouchers *) blockingReadBB(readQ);
		status = read_sector(disk, &(vou->sd));

		pthread_mutex_lock(&vmutex); 
		vou->status = status;
		pthread_cond_broadcast(&vcond);
		pthread_mutex_unlock(&vmutex);
	}
}

/*
 * called before any other methods to allow you to initialize data
 * structures and to start any internal threads.
 *
 * Arguments:
 *   dd: the DiskDevice that you must drive
 *   mem_start, mem_length: some memory for SectorDescriptors
 *   fsds_ptr: you hand back a FreeSectorDescriptorStore constructed
 *             from the memory provided in the two previous arguments
 
 */
void init_disk_driver(DiskDevice dd, void *mem_start, unsigned long mem_length, FreeSectorDescriptorStore *fsds_ptr) {

	disk = dd;	
	*fsds_ptr = create_fsds();
	create_free_sector_descriptors(*fsds_ptr, mem_start, mem_length);
	fsds = fsds_ptr;
    
	workQ = createBB(MAX);
	readQ = createBB(MAX);

    pthread_t workThread;
	if (pthread_create(&workThread, NULL, write, NULL)) {
		printf("Error: failed to create writing thread\n");
		exit(1);
	} else {
        printf ("Write Thread Created");
    }

    
    pthread_t readThread;
	if (pthread_create(&readThread, NULL, read, NULL)) {
		printf("Error: failed to create reading thread\n");
		exit(1);
	}else {
        printf ("Read Thread Created");
    }

}

/*
 * the following calls are used to write a sector to the disk
 * the nonblocking call must return promptly, returning 1 if successful at
 * queueing up the write, 0 if not (in case internal buffers are full)
 * the blocking call will usually return promptly, but there may be
 * a delay while it waits for space in your buffers.
 * neither call should delay until the sector is actually written to the disk
 * for a successful nonblocking call and for the blocking call, a voucher is
 * returned that is required to determine the success/failure of the write
 
 */
void blocking_write_sector(SectorDescriptor sd, Voucher *v) {

	Vouchers *vou = (Vouchers *)malloc(sizeof(Vouchers));
	vou->status = -1;
	vou->sd = sd;

	*v = (Voucher) vou;

	blockingWriteBB(workQ, (BufferedItem) vou);

}

int nonblocking_write_sector(SectorDescriptor sd, Voucher *v) {

	Vouchers *vou = (Vouchers *)malloc(sizeof(Vouchers));
	vou->status = -1;
	vou->sd = sd;

	*v = (Voucher) vou;

	return nonblockingWriteBB(workQ, (BufferedItem) vou);

}


/*
 * the following calls are used to initiate the read of a sector from the disk
 * the nonblocking call must return promptly, returning 1 if successful at
 * queueing up the read, 0 if not (in case internal buffers are full)
 * the blocking callwill usually return promptly, but there may be
 * a delay while it waits for space in your buffers.
 * neither call should delay until the sector is actually read from the disk
 * for successful nonblocking call and for the blocking call, a voucher is
 * returned that is required to collect the sector after the read completes.
 */
void blocking_read_sector(SectorDescriptor sd, Voucher *v) {

	Vouchers *vou = (Vouchers *)malloc(sizeof(Vouchers));
	vou->status = -1;
	vou->sd = sd;

	*v = (Voucher) vou;

	blockingWriteBB(readQ, (BufferedItem) vou);

}
int nonblocking_read_sector(SectorDescriptor sd, Voucher *v) {

	Vouchers *vou = (Vouchers *)malloc(sizeof(Vouchers));
	vou->status = -1;
	vou->sd = sd;

	*v = (Voucher) vou;

	return nonblockingWriteBB(readQ, (BufferedItem) vou);

}

/*
 * the following call is used to retrieve the status of the read or write
 * the return value is 1 if successful, 0 if not
 * the calling application is blocked until the read/write has completed
 * if a successful read, the associated SectorDescriptor is returned in sd
 */
int redeem_voucher(Voucher v, SectorDescriptor *sd) {

	Vouchers *vou = (Vouchers *) v;
	int status;

	pthread_mutex_lock(&vmutex); 
	while (vou->status == -1) {
		pthread_cond_wait(&vcond, &vmutex);
	}
	*sd = vou->sd;
	status = vou->status;
	free(vou);
    //free(v);
	pthread_mutex_unlock(&vmutex);
    pthread_mutex_destroy(&vmutex);
    pthread_cond_destroy(&vcond);
	
	return status;
}
