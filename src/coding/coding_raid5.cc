#include "coding_raid5.hh"
#include "../filesystem/filesystem_common.hh"
#include "../filesystem/filesystem_utils.hh"
#include "../cache/cache.hh"
#include "../gui/diskusage_report.hh"

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

//NCFS context state
extern struct ncfs_state* NCFS_DATA;
extern FileSystemLayer* fileSystemLayer;
extern CacheLayer* cacheLayer;
extern DiskusageReport* diskusageLayer;

/*
 * raid5_encoding: RAID 5: fault tolerance by stripped parity (type=5)
 * 
 * @param buf: buffer of data to be written to disk
 * @param size: size of the data buffer
 *
 * @return: assigned data block: disk id and block number.
 */
struct data_block_info Coding4Raid5::encode(const char* buf, int size)
{
 
	int retstat, disk_id, block_no, disk_total_num, block_size, int_block_size;
	//char *dev, *dev2;
	//int datafd, datafd2;
	int size_request, block_request, free_offset;
	struct data_block_info block_written;
	int i, j;
	int parity_disk_id;
	char *buf2, *buf_read;
	int *intbuf2, *intbuf_read, *intbuf;
	
	intbuf=(int *)buf;
		
	struct timeval t1, t2, t3, t4, t5, t6;
	double duration;

	//test print
	//printf("***get_data_block_no 1: size=%d\n",size);

	disk_total_num = NCFS_DATA->disk_total_num;
	block_size = NCFS_DATA->disk_block_size;

	int_block_size=(block_size * sizeof(char) / sizeof(int));
		
	size_request = fileSystemLayer->round_to_block_size(size);
	block_request = size_request / block_size;

	//test print
	//printf("***get_data_block_no 2: size_request=%d, block_request=%d\n"
	//		,size_request,block_request);


	//implement disk write algorithm here.
	//here use raid5: stripped block allocation plus distributed parity.
	//approach: calculate xor of the block on all data disk
	for (i=0; i < disk_total_num; i++){
		if (i ==
				(disk_total_num-1-(NCFS_DATA->free_offset[i] % disk_total_num))){
			(NCFS_DATA->free_offset[i])++;
			(NCFS_DATA->free_size[i])--;
		}
	}

	block_no = 0;
	disk_id = -1;
	free_offset = NCFS_DATA->free_offset[0];
	for (i=disk_total_num-1; i >= 0; i--){
		if ((block_request <= (NCFS_DATA->free_size[i]))
				&& (free_offset >= (NCFS_DATA->free_offset[i])))
		{
			disk_id = i;
			block_no = NCFS_DATA->free_offset[i];
			free_offset = block_no;
		}
	}


	//get block from space_list if no free block available
	if (disk_id == -1){
		if (NCFS_DATA->space_list_head != NULL){
			disk_id = NCFS_DATA->space_list_head->disk_id;
			block_no = NCFS_DATA->space_list_head->disk_block_no;
			fileSystemLayer->space_list_remove(disk_id,block_no);
		}
	}


	if (disk_id == -1){
		printf("***get_data_block_no: ERROR disk_id = -1\n");
	}
	else{
		//start ncfs 0.11 michael. Use improved method for parity.
		buf_read = (char*)malloc(sizeof(char)*size_request);
		buf2 = (char*)malloc(sizeof(char)*size_request);

		intbuf_read=(int *)buf_read;
		intbuf2=(int *)buf2;
		
                if (NCFS_DATA->run_experiment == 1){
                        gettimeofday(&t1,NULL);
                }

		//Cache Start
		retstat = cacheLayer->readDisk(disk_id,buf2,size_request,block_no*block_size);
		//Cache End

		if (NCFS_DATA->run_experiment == 1){
                        gettimeofday(&t2,NULL);
          	}


		//for (j=0; j < size_request; j++){
		//	buf2[j] = buf2[j] ^ buf[j];
		//}

		for (j = 0; j < (long long)(size_request * sizeof(char) / sizeof(int)); ++j){
			intbuf2[j] = intbuf2[j] ^ intbuf[j];
		}
		
		NCFS_DATA->free_offset[disk_id] = block_no + block_request;
		NCFS_DATA->free_size[disk_id]
			= NCFS_DATA->free_size[disk_id] - block_request;


		parity_disk_id = disk_total_num - 1 - (block_no % disk_total_num);

                if (NCFS_DATA->run_experiment == 1){
                        gettimeofday(&t3,NULL);
                }

		//Cache Start
		retstat = cacheLayer->readDisk(parity_disk_id,buf_read,size_request,block_no*block_size);
		//Cache End

                if (NCFS_DATA->run_experiment == 1){
                        gettimeofday(&t4,NULL);
                }


		//for (j=0; j < size_request; j++){
		//	buf2[j] = buf2[j] ^ buf_read[j];
		//}
		
		for (j = 0; j < (long long)(size_request * sizeof(char) / sizeof(int)); ++j){
			intbuf2[j] = intbuf2[j] ^ intbuf_read[j];
		}
		
		if (NCFS_DATA->run_experiment == 1){
			gettimeofday(&t5,NULL);
		}

                //Cache Start
		//write data
                retstat = cacheLayer->writeDisk(disk_id,buf,size,block_no*block_size);
                //Cache End

		//Cache Start
		//write parity
		retstat = cacheLayer->writeDisk(parity_disk_id,buf2,size,block_no*block_size);
		//Cache End
		//printf("***raid5: retstat=%d\n",retstat);

		if (NCFS_DATA->run_experiment == 1){
			gettimeofday(&t6,NULL);
		}
		
		free(buf_read);
		free(buf2);
		//end ncfs 0.11 michael

		if (NCFS_DATA->run_experiment == 1){

			//Disk Read Time
                        duration = (t2.tv_sec - t1.tv_sec) + 
                                (t2.tv_usec-t1.tv_usec)/1000000.0;
                        NCFS_DATA->diskread_time += duration;

			duration = (t4.tv_sec - t3.tv_sec) + 
                                (t4.tv_usec-t3.tv_usec)/1000000.0;
                        NCFS_DATA->diskread_time += duration;
			
			//Data Encode Time
                	duration = (t3.tv_sec - t2.tv_sec) + 
                                (t3.tv_usec-t2.tv_usec)/1000000.0;
                	NCFS_DATA->encoding_time += duration;

                        duration = (t5.tv_sec - t4.tv_sec) + 
                                (t5.tv_usec-t4.tv_usec)/1000000.0;
                        NCFS_DATA->encoding_time += duration;

			//Disk Write Time
                        duration = (t6.tv_sec - t5.tv_sec) + 
                                (t6.tv_usec-t5.tv_usec)/1000000.0;
                        NCFS_DATA->diskwrite_time += duration;
		}
	}

	block_written.disk_id = disk_id;
	block_written.block_no = block_no;

	return block_written;
	
}



/*
 * decoding_raid5: RAID 5 decode
 *
 * @param disk_id: disk id               
 * @param buf: data buffer for reading data
 * @param size: size of the data buffer
 * @param offset: offset of data on disk
 *
 * @return: size of successful decoding (in bytes)
 */
int Coding4Raid5::decode(int disk_id, char* buf, long long size, long long offset)
{
 
	int retstat;
        struct timeval t1, t2, t3;
        double duration;

	if(NCFS_DATA->disk_status[disk_id] == 0){
                if (NCFS_DATA->run_experiment == 1){
                	gettimeofday(&t1,NULL);
                }

		retstat = cacheLayer->readDisk(disk_id,buf,size,offset);

                if (NCFS_DATA->run_experiment == 1){
                	gettimeofday(&t2,NULL);

                	duration = (t2.tv_sec - t1.tv_sec) + 
                        	(t2.tv_usec-t1.tv_usec)/1000000.0;
                        NCFS_DATA->diskread_time += duration;
                }

		return retstat;
	}
	else {
		//printf("Degraded Read\n");
		char* temp_buf;
		int i;
		long long j;
		temp_buf = (char*)malloc(sizeof(char)*size);
		memset(temp_buf, 0, size);
		memset(buf, 0, size);
		int* inttemp_buf = (int*)temp_buf;
		int* intbuf = (int*)buf;
		for(i = 0; i < NCFS_DATA->disk_total_num; ++i){
			if(i != disk_id){
				if(NCFS_DATA->disk_status[i] != 0){
					printf("Raid 5 both disk %d and %d are failed\n",disk_id,i);
					free(temp_buf);
					return -1;
				}

				if (NCFS_DATA->run_experiment == 1){
                                	gettimeofday(&t1,NULL);
                        	}

				retstat = cacheLayer->readDisk(i,temp_buf,size,offset);

				if (NCFS_DATA->run_experiment == 1){
                                	gettimeofday(&t2,NULL);
                        	}

				for(j = 0; j < (long long)(size * sizeof(char) / sizeof(int)); ++j)
					intbuf[j] = intbuf[j] ^ inttemp_buf[j];

				if (NCFS_DATA->run_experiment == 1){
                                	gettimeofday(&t3,NULL);

                                	duration = (t2.tv_sec - t1.tv_sec) + 
                                       		(t2.tv_usec-t1.tv_usec)/1000000.0;
                                	NCFS_DATA->diskread_time += duration;

                                	duration = (t3.tv_sec - t2.tv_sec) + 
                                        	(t3.tv_usec-t2.tv_usec)/1000000.0;
                                	NCFS_DATA->decoding_time += duration;
                        	}
			}
		}
		free(temp_buf);
		return size;
	}
	AbnormalError();
	return -1; 
}

/*
 * recovering_raid5: RAID 5 recover
 *
 * @param failed_disk_id: failed disk id
 * @param newdevice: new device to replace the failed disk
 *
 * @return: 0: success ; -1: wrong
 */
int Coding4Raid5::recover(int failed_disk_id,char* newdevice){
  
        //size of data to be rebuilded
	int __recoversize = NCFS_DATA->disk_size[failed_disk_id];
	int block_size = NCFS_DATA->disk_block_size;
	char* buf = (char*)malloc(sizeof(char)*block_size);
	
	struct timeval t1, t2;
        double duration;
	
	for(int i = 0; i < __recoversize; ++i){
	  
	        //reset the buf data
	        //memset(buf, 0, block_size);
		
		int offset = i * block_size;

		int retstat = fileSystemLayer->codingLayer->decode(failed_disk_id,buf,block_size,offset);

		if (NCFS_DATA->run_experiment == 1){
                      	gettimeofday(&t1,NULL);
                }
		retstat = cacheLayer->writeDisk(failed_disk_id,buf,block_size,offset);

		if (NCFS_DATA->run_experiment == 1){
		  
                        gettimeofday(&t2,NULL);

                        duration = (t2.tv_sec - t1.tv_sec) + 
                                     (t2.tv_usec-t1.tv_usec)/1000000.0;
                        NCFS_DATA->diskwrite_time += duration;

                }
                
	}

	return 0;
	
}

int Coding4Raid5::recover_enable_node_encode(int failed_disk_id, char* newdevice, int* node_socketid_array){
  
  return 0;
}

int Coding4Raid5::recover_using_multi_threads_nor(int failed_disk_id, char* newdevice, int total_sor_threads){
  
    return 0;
}

int Coding4Raid5::recover_using_multi_threads_sor(int failed_disk_id, char* newdevice
									, int total_sor_threads, int number_sor_thread){
  
    return 0;
}

int Coding4Raid5::recover_using_access_aggregation(int failed_disk_id, char* newdevice){
  
    return 0;
}

int Coding4Raid5::recover_conventional(int failed_disk_id, char* newdevice){
  
   return 0;
}