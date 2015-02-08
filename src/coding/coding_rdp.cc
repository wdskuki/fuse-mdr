#include "coding_rdp.hh"
#include "../filesystem/filesystem_common.hh"
#include "../filesystem/filesystem_utils.hh"
#include "../cache/cache.hh"
#include "../gui/diskusage_report.hh"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include <string>
#include <iostream>
using namespace std;

//NCFS context state
extern struct ncfs_state* NCFS_DATA;
extern FileSystemLayer* fileSystemLayer;
extern CacheLayer* cacheLayer;
extern DiskusageReport* diskusageLayer;



void print_vect(vector<int>& t){
	cout<<"+++++++++++++++++++++++++++++++"<<endl;
	vector<int>:: iterator iter = t.begin();
	for(; iter != t.end(); iter++){
		cout<<*iter<<", ";
	}
	cout<<endl;
}

Coding4Rdp::Coding4Rdp(){
	k = NCFS_DATA->data_disk_num;
	int p = k+1;
	row = p-1;
	strip_size = row;
	col = p+1;
	matrix = new int*[row];
	for(int i = 0; i < row; i++){
		matrix[i] = new int[col];
		for(int j = 0; j < col; j++){
			matrix[i][j] = 0;
		}
	}
	elems_related_elems();
	print_elem_hash();


}

Coding4Rdp::~Coding4Rdp(){
	if(matrix != NULL){
		for(int i = 0; i < row; i++){
			delete []matrix[i];
			matrix[i] = NULL;
		}
		delete []matrix;
		matrix = NULL;
	}
}

void Coding4Rdp::print(string str){
	cout<<str<<endl;
	for(int i = 0; i < row; i++){
		for(int j = 0; j < col; j++){
			cout<<matrix[i][j]<<" ";
		}
		cout<<endl;
	}
	cout<<endl;
}

void Coding4Rdp::random_data_element(){
	srand((unsigned)time(NULL));
	for(int i = 0; i < row; i++){
		for(int j = 0; j < col-2; j++){
			matrix[i][j] = rand()%10;
		}
	}
}

void Coding4Rdp::same_data_element(int e){
	for(int i = 0; i < row; i++){
		for(int j = 0; j < col-2; j++){
			matrix[i][j] = e;
		}
	}
}

void Coding4Rdp::gen_p(){
	for(int i = 0; i < row; i++){
		for(int j = 0; j < col-2; j++){
			matrix[i][col-2] ^= matrix[i][j];
		}
	}
}

void Coding4Rdp::print_elem_hash(){
	RELATED_ELEMS_HASH:: iterator iter = elem_hash.begin();
	for(; iter != elem_hash.end(); iter++){
		print_PAIR(iter->first);
		printf("--->");
		print_related_elems(iter->second);
		printf("\n");
	}
}

void Coding4Rdp::gen_q(){
	for(int i = 0; i < row; i++){
		PAIR q = make_pair(i, col-1);
		RELATED_ELEMS:: iterator iter = elem_hash[q].begin();
		for(; iter != elem_hash[q].end(); iter++){
			matrix[i][col-1] ^= matrix[iter->first][iter->second];
		}
	}
}

vector<int> Coding4Rdp::repair(int id){
	vector<int> disk;
	if(id < 0 || id >= col) 
		return disk;

	if(id != col-1){
		for(int i = 0; i < row; i++){
			int ret = 0;
			for(int j = 0; j < col-1; j++){
				if(j != id){
					ret ^= matrix[i][j];
				}
			}
			disk.push_back(ret);
		}
	}else{
		for(int i = 0; i < row; i++){
			int ret = 0;
			PAIR q = make_pair(i, col-1);
			RELATED_ELEMS:: iterator iter = elem_hash[q].begin();
			for(; iter != elem_hash[q].end(); iter++){
				ret ^= matrix[iter->first][iter->second];
			}
			disk.push_back(ret);
		}		
	}
	return disk;
}


void Coding4Rdp::elems_related_elems(){
	/*1. analysys effection of LINE_XOR */
	for(int i = 0; i < row; i++){
		PAIR p = make_pair(i, col-2);
		for(int j = 0; j < col-2; j++){
			PAIR d = make_pair(i, j);
			//data elem effected by LINE_XOR
			elem_hash[d].push_back(p);
			//P elem effected by LINE_XOR
			elem_hash[p].push_back(d);
		}
	}
	//debug("Finish 'analysys effection of LINE_XOR'\n");

	/*2. analysys effection of RDP_DIAG_XOR, include D and P*/
	for(int i = 0; i < row; i++){
		PAIR q = make_pair(i, col-1);
		
		/* 2.1 analysis relation between Q and P*/
		int m = k+1;
		int first = (i+1)%m;
		//if first >= row, this stripe line is virtual
		if(first >= 0 && first < row){
			PAIR p = make_pair(first, col-2);
			//Q elem effected by P elem
			elem_hash[q].push_back(p);
			//P elem effected by Q elem
			elem_hash[p].push_back(q);
		}
		/* 2.2 analysis relation between Q and D*/
		int t = (i+1)%m;
		for(int j = 0; j <= m-2; j++){
			if(j != t){
				int second = (i-j)%m;
				second = (second >= 0)? second : (second + m);
				PAIR d = make_pair(j, second);
				//Q elem effected by D elem
				elem_hash[q].push_back(d);
				//D elem effected by Q elem
				elem_hash[d].push_back(q);			
			}
		}
	}
	//debug("Finish 'analysys effection of RDP_DIAG_XOR'\n");		
}

vector<int> Coding4Rdp::rdp_find_q_blocks_id(int disk_id, int block_no){
	PAIR e = make_pair(block_no, disk_id);
	vector<int> ret;
	RELATED_ELEMS::iterator iter = elem_hash[e].begin();
	for(;iter != elem_hash[e].end(); iter++){
		int first = iter->first; //offset in strip
		int second = iter->second; //disk_id
		if(second == col-1){
			ret.push_back(first);
		}
	}
	if(ret.empty())
		printf("<%d, %d>, ********EMPTY******\n", block_no, disk_id);
	return ret;
}

struct data_block_info Coding4Rdp::
encode(const char *buf, int size)
{
	int retstat, disk_id, block_no, disk_total_num, block_size;
	int size_request, block_request, free_offset;
	struct data_block_info block_written;
	int i, j;
	int parity_disk_id, code_disk_id;
	char *buf2, *buf3, *buf_read;
	//dongsheng wei
	char *buf_parity_disk, *buf_code_disk;
	int data_disk_num;
	vector<int> q_blocks_no;
	//	int strip_size;
	//dongsheng wei
	char temp_char;
	int data_disk_coeff;

	struct timeval t1, t2, t3;
	//dongsheng wei
	struct timeval t4, t5, t6;
	//dongsheng wei
	double duration;

	disk_total_num = NCFS_DATA->disk_total_num;
	block_size = NCFS_DATA->disk_block_size;

	size_request = fileSystemLayer->round_to_block_size(size);
	block_request = size_request / block_size;

	data_disk_num = disk_total_num - 2;




	//implement disk write algorithm here.
	//here use raid6.

	//dognsheng wei
	// for (i = 0; i < disk_total_num; i++) {
	// 	//mark blocks of code_disk and parity_disk
	// 	if ((i ==
	// 	     (disk_total_num - 1 -
	// 	      (NCFS_DATA->free_offset[i] % disk_total_num)))
	// 	    || (i ==
	// 		(disk_total_num - 1 -
	// 		 ((NCFS_DATA->free_offset[i] + 1) % disk_total_num)))) {
	// 		(NCFS_DATA->free_offset[i])++;
	// 		(NCFS_DATA->free_size[i])--;
	// 	}
	// }

	block_no = 0;
	disk_id = -1;
	free_offset = NCFS_DATA->free_offset[0];
	//dongsheng wei
	for (i = disk_total_num - 3; i >= 0; i--) {
		if ((block_request <= (NCFS_DATA->free_size[i]))
		    && (free_offset >= (NCFS_DATA->free_offset[i]))) {
			disk_id = i;
			block_no = NCFS_DATA->free_offset[i];
			free_offset = block_no;
		}
	}

	//dongsheng wei
	if(disk_id == 0){
		(NCFS_DATA->free_offset[disk_total_num-2])++;
		(NCFS_DATA->free_size[disk_total_num-2])--;
		if(block_no % strip_size == 0){
			(NCFS_DATA->free_offset[disk_total_num-1]) += strip_size;
			(NCFS_DATA->free_size[disk_total_num-1]) -= strip_size;
		}
	}

	//get block from space_list if no free block available
	if (disk_id == -1) {
		if (NCFS_DATA->space_list_head != NULL) {
			disk_id = NCFS_DATA->space_list_head->disk_id;
			block_no = NCFS_DATA->space_list_head->disk_block_no;
			fileSystemLayer->space_list_remove(disk_id, block_no);
		}
	}

	if (disk_id == -1) {
		printf("***get_data_block_no: ERROR disk_id = -1\n");
	} else {
		NCFS_DATA->free_offset[disk_id] = block_no + block_request;
		NCFS_DATA->free_size[disk_id]
		    = NCFS_DATA->free_size[disk_id] - block_request;

		int p_disk_id = disk_total_num - 2;
		int q_disk_id = disk_total_num - 1;
		
		// //dongsheng wei
		char* buf_p_disk = (char *)malloc(sizeof(char) * size_request);
		char* buf_q_disk = (char *)malloc(sizeof(char) * size_request);


		//read P block
        if (NCFS_DATA->run_experiment == 1){
        	gettimeofday(&t1,NULL);
        }

		retstat = cacheLayer->readDisk(p_disk_id, buf_p_disk, 
			size_request, block_no * block_size);		

		if (NCFS_DATA->run_experiment == 1){
			gettimeofday(&t2,NULL);
			duration = (t2.tv_sec - t1.tv_sec) + 
				(t2.tv_usec-t1.tv_usec)/1000000.0;
			NCFS_DATA->diskread_time += duration;			
		}


		//calculate new P block
		for (j = 0; j < size; j++) {
			buf_p_disk[j] = buf_p_disk[j] ^ buf[j];
		}

		if (NCFS_DATA->run_experiment == 1){
			gettimeofday(&t1,NULL);
			duration = (t1.tv_sec - t2.tv_sec) + 
				(t1.tv_usec-t2.tv_usec)/1000000.0;
			NCFS_DATA->encoding_time += duration;
		}

		//write new P block
		retstat = cacheLayer->writeDisk(p_disk_id, buf_p_disk, size, 
			block_no * block_size);		

		if (NCFS_DATA->run_experiment == 1){
			gettimeofday(&t2,NULL);
			duration = (t2.tv_sec - t1.tv_sec) + 
				(t2.tv_usec-t1.tv_usec)/1000000.0;
			NCFS_DATA->diskwrite_time += duration;			
		}

		int strip_num = block_no / strip_size;
		int strip_offset = block_no % strip_size;

		q_blocks_no = rdp_find_q_blocks_id(disk_id, strip_offset);
		printf("<<%d, %d>>\n", strip_offset, disk_id);
		 cout<<"q_blocks_no\n";
		 print_vect(q_blocks_no);

		// //call global print, not the member function print
		// ::print(q_blocks_no);

		int q_blk_num = q_blocks_no.size();
		for(i = 0; i < q_blk_num; i++){
			int q_blk_no = q_blocks_no[i] + strip_num * strip_size;

			//read Q blk
			memset(buf_q_disk, 0, size_request);
	        
	        if (NCFS_DATA->run_experiment == 1){
        		gettimeofday(&t1,NULL);
        	}

			retstat = cacheLayer->readDisk(q_disk_id, buf_q_disk, 
				size_request, q_blk_no * block_size);	
			
			printf("READ_Q, <%d, %d>, retstat = %d\n", q_blk_no, q_disk_id, retstat);

		    if (NCFS_DATA->run_experiment == 1){
        		gettimeofday(&t2,NULL);
        		duration = (t2.tv_sec - t1.tv_sec) + 
					(t2.tv_usec-t1.tv_usec)/1000000.0;
				NCFS_DATA->diskread_time += duration;
        	}

			//calculate new Q blk
			for (j = 0; j < size; j++) {
				buf_q_disk[j] = buf_q_disk[j] ^ buf[j];
				//buf_q_disk[j] = buf_q_disk[j] ^ '1';
			}

		    if (NCFS_DATA->run_experiment == 1){
        		gettimeofday(&t1,NULL);
        		duration = (t1.tv_sec - t2.tv_sec) + 
					(t1.tv_usec-t2.tv_usec)/1000000.0;
				NCFS_DATA->encoding_time += duration;        	
        	}		
        	
			//write new Q blk
			retstat = cacheLayer->writeDisk(q_disk_id, buf_q_disk, 
				size_request, q_blk_no*block_size);		

			printf("WRITE_Q, <%d, %d>, retstat = %d\n", q_blk_no, q_disk_id, retstat);

			if (NCFS_DATA->run_experiment == 1){
        		gettimeofday(&t2,NULL);
        		duration = (t2.tv_sec - t1.tv_sec) + 
					(t2.tv_usec-t1.tv_usec)/1000000.0;
				NCFS_DATA->diskwrite_time += duration;
        	}	
		}

		//write buf
		if (NCFS_DATA->run_experiment == 1){
       		gettimeofday(&t1,NULL);
        }

		retstat = cacheLayer->writeDisk(disk_id, buf, size, 
			block_no * block_size);			
		if (NCFS_DATA->run_experiment == 1){
       		gettimeofday(&t2,NULL);
        	duration = (t2.tv_sec - t1.tv_sec) + 
				(t2.tv_usec-t1.tv_usec)/1000000.0;
			NCFS_DATA->diskwrite_time += duration;
        }
		
		free(buf_p_disk);
		free(buf_q_disk);
	}

	block_written.disk_id = disk_id;
	block_written.block_no = block_no;
	return block_written;
}





int Coding4Rdp::decode(int disk_id, char *buf, long long size, long long offset){
	int retstat;
	char *temp_buf;

	int disk_failed_no;
	int disk_another_failed_id;	//id of second failed disk

	int q_disk_id, p_disk_id;
	int block_size;
	long long block_no;
	int disk_total_num;
	int data_disk_coeff;
	char temp_char;

	int g1, g2, g12;
	char *P_temp;
	char *Q_temp;

	struct timeval t1, t2, t3;
	double duration;

	memset(buf, 0, size);

	temp_buf = (char *)malloc(sizeof(char) * size);
	memset(temp_buf, 0, size);

	vector<vector<int> > repair_q_blocks_no;

	disk_total_num = NCFS_DATA->disk_total_num;
	block_size = NCFS_DATA->disk_block_size;
	block_no = offset / block_size;

	q_disk_id = disk_total_num - 1;
	p_disk_id = disk_total_num - 2;

	if (NCFS_DATA->disk_status[disk_id] == 0) {

		if (NCFS_DATA->run_experiment == 1){
        	gettimeofday(&t1,NULL);
        }

		retstat = cacheLayer->readDisk(disk_id, buf, size, offset);
		if (NCFS_DATA->run_experiment == 1){
        	gettimeofday(&t2,NULL);
        	duration = (t2.tv_sec - t1.tv_sec) + 
				(t2.tv_usec-t1.tv_usec)/1000000.0;
			NCFS_DATA->diskread_time += duration;
        }		

		return retstat;
	}
	else{
		//check the number of failed disks
		disk_failed_no = 0;
		for (int i = 0; i < disk_total_num; i++) {
			if (NCFS_DATA->disk_status[i] == 1) {
				(disk_failed_no)++;
				if (i != disk_id) {
					disk_another_failed_id = i;
				}
			}
		}

		int strip_num = block_no / strip_size;
		int strip_offset = block_no % strip_size;
		
		if(disk_failed_no == 1){ //fail disk id = disk_id
			if((disk_id >= 0) && (disk_id < disk_total_num-1)){
				//fail disk(disk_id) is data disk or P disk	
				for(int i = 0; i < disk_total_num-1; i++){
					if(i != disk_id){
						if (NCFS_DATA->run_experiment == 1){
							gettimeofday(&t1,NULL);
						}
						retstat = cacheLayer->readDisk(i, temp_buf, size, block_no*block_size);
						if (NCFS_DATA->run_experiment == 1){
							gettimeofday(&t2,NULL);
						}
						duration = (t2.tv_sec - t1.tv_sec) + 
						(t2.tv_usec-t1.tv_usec)/1000000.0;
						NCFS_DATA->diskread_time += duration;						
						for(long long j = 0; j < size; j++){
							buf[j] = buf[j] ^ temp_buf[j];
						}
						if (NCFS_DATA->run_experiment == 1){
							gettimeofday(&t1,NULL);
						}
						duration = (t1.tv_sec - t2.tv_sec) + 
						(t1.tv_usec-t2.tv_usec)/1000000.0;
						NCFS_DATA->decoding_time += duration;					
					}
				}
			}else{////fail disk(disk_id) is data Q disk 
				PAIR q = make_pair(strip_offset, disk_id);
				RELATED_ELEMS::iterator iter = elem_hash[q].begin();
				for(; iter != elem_hash[q].end(); iter++){
					int read_disk_id = iter->second;
					int read_blk_offset = iter->first + strip_num*strip_size;
					if (NCFS_DATA->run_experiment == 1){
						gettimeofday(&t1,NULL);
					}
					retstat = cacheLayer->readDisk(read_disk_id, 
						temp_buf, size, read_blk_offset*block_size);
					if (NCFS_DATA->run_experiment == 1){
						gettimeofday(&t2,NULL);
					}
					duration = (t2.tv_sec - t1.tv_sec) + 
					(t2.tv_usec-t1.tv_usec)/1000000.0;
					NCFS_DATA->diskread_time += duration;						
					for(long long j = 0; j < size; j++){
						buf[j] = buf[j] ^ temp_buf[j];
					}
					if (NCFS_DATA->run_experiment == 1){
						gettimeofday(&t1,NULL);
					}
					duration = (t1.tv_sec - t2.tv_sec) + 
					(t1.tv_usec-t2.tv_usec)/1000000.0;
					NCFS_DATA->decoding_time += duration;
				}
			}
		}
		else{
			printf("RDP number of failed disks is larger than 1.\n");
			return -1;
		}
		free(temp_buf);
		return size;
	}
	AbnormalError();
	return -1;
}

void Coding4Rdp::
pre_read_into_buf(int failed_disk_id,
				  int strip_num, 
				  char*** buf_blocks){
	int disk_total_num = NCFS_DATA->disk_total_num;
	int block_size = NCFS_DATA->disk_block_size;
	int non_read_col; //another column which is not read into buffer
	struct timeval t1, t2, t3;
	double duration;
	cout<<"*********in pre_read_into_buf**********"<<endl;
	if(failed_disk_id != disk_total_num-1){ // failed_disk_id is D or P
		for(int i = 0; i < disk_total_num-1; i++){
			if(i != failed_disk_id){
				int read_disk_id = i;
				for(int j = 0; j < strip_size; j++){
					int read_blk_offset = strip_num*strip_size+j;
					if (NCFS_DATA->run_experiment == 1){
						gettimeofday(&t1,NULL);
					}
					cacheLayer->readDisk(read_disk_id, buf_blocks[j][read_disk_id], 
						block_size, read_blk_offset*block_size);
					if (NCFS_DATA->run_experiment == 1){
						gettimeofday(&t2,NULL);
					}
					duration = (t2.tv_sec - t1.tv_sec) + 
					(t2.tv_usec-t1.tv_usec)/1000000.0;
					NCFS_DATA->diskread_time += duration;				
				}
			}
		}
	}else{ // failed_disk_id is Q
		printf("strip_size = %d\n", strip_size);
		for(int j = 0; j < strip_size; j++){
			PAIR q = make_pair(j, failed_disk_id);
			RELATED_ELEMS::iterator iter = elem_hash[q].begin();
			print_PAIR(q);
			cout<<"---->";
			for(; iter != elem_hash[q].end(); iter++){
				int read_strip_offset = iter->first;
				int read_disk_id = iter->second;
				int read_blk_offset = read_strip_offset + strip_num * strip_size;
				if (NCFS_DATA->run_experiment == 1){
					gettimeofday(&t1,NULL);
				}
				cacheLayer->readDisk(read_disk_id, 
					buf_blocks[read_strip_offset][read_disk_id], 
					block_size, read_blk_offset*block_size);
				printf("<%d, %d>.. ", read_strip_offset, read_disk_id);
				if (NCFS_DATA->run_experiment == 1){
					gettimeofday(&t2,NULL);
				}
				duration = (t2.tv_sec - t1.tv_sec) + 
				(t2.tv_usec-t1.tv_usec)/1000000.0;
				NCFS_DATA->diskread_time += duration;					
			}
			cout<<endl;
		}
	}
}

// int Coding4Rdp::
// rdp_recover_block(int disk_id, 
// 						  char *buf, 
// 						  long long size, 
// 						  long long offset,
// 						  char*** pread_stripes,
// 						  bool& in_buf)
// {
// 	int retstat;
// 	char *temp_buf;

// 	int disk_failed_no;
// 	int disk_another_failed_id;	//id of second failed disk

// 	int q_disk_id, p_disk_id;
// 	int block_size;
// 	long long block_no;
// 	int disk_total_num;
// 	int data_disk_coeff;
// 	char temp_char;

// 	int g1, g2, g12;
// 	char *P_temp;
// 	char *Q_temp;

// 	struct timeval t1, t2, t3;
// 	double duration;

// 	memset(buf, 0, size);

// 	vector<vector<int> > repair_q_blocks_no;

// 	disk_total_num = NCFS_DATA->disk_total_num;
// 	block_size = NCFS_DATA->disk_block_size;
// 	block_no = offset / block_size;

// 	q_disk_id = disk_total_num - 1;
// 	p_disk_id = disk_total_num - 2;

// 	if (NCFS_DATA->disk_status[disk_id] == 0) {

// 		retstat = cacheLayer->readDisk(disk_id, buf, size, offset);
// 		return retstat;
// 	}
// 	else{
// 		//check the number of failed disks
// 		disk_failed_no = 0;
// 		for (int i = 0; i < disk_total_num; i++) {
// 			if (NCFS_DATA->disk_status[i] == 1) {
// 				(disk_failed_no)++;
// 				if (i != disk_id) {
// 					disk_another_failed_id = i;
// 				}
// 			}
// 		}

// 		int strip_num = block_no / strip_size;
// 		int strip_offset = block_no % strip_size;
		
// 		if(!in_buf){
// 			in_buf = !in_buf;
// 			pre_read_into_buf(disk_id, strip_num, pread_stripes);
// 		}


// 		if(disk_failed_no == 1){ //fail disk id = disk_id
// 			if((disk_id >= 0) && (disk_id < disk_total_num-1)){
// 				//fail disk(disk_id) is data disk or P disk	
// 				for(int i = 0; i < disk_total_num-1; i++){
// 					if(i != disk_id){					
// 						for(long long j = 0; j < size; j++){
// 							buf[j] = buf[j] ^ pread_stripes[strip_offset][i][j];
// 						}					
// 					}
// 				}
// 			}else{//fail disk(disk_id) is data Q disk
// 				PAIR q = make_pair(strip_offset, disk_id);
// 				RELATED_ELEMS::iterator iter = elem_hash[q].begin();
// 				for(; iter != elem_hash[q].end(); iter++){
// 					int read_disk_id = iter->second;
// 					int read_strip_offset = iter->first;	
// 					for(long long j = 0; j < size; j++){
// 						buf[j] = buf[j] ^ 
// 							pread_stripes[read_strip_offset][read_disk_id][j];
// 					}
// 				}
// 			}
// 		}
// 		else{
// 			printf("RDP number of failed disks is larger than 1.\n");
// 			return -1;
// 		}
// 	}
// 	AbnormalError();
// 	return -1;
// }

int Coding4Rdp::
rdp_recover_block2(int disk_id, 
						  char *buf, 
						  long long size, 
						  long long offset,
						  char*** pread_stripes,
						  bool& in_buf,
						  double& func_cost_time)
{
	int retstat;
	char *temp_buf;

	int disk_failed_no;
	int disk_another_failed_id;	//id of second failed disk

	int q_disk_id, p_disk_id;
	int block_size;
	long long block_no;
	int disk_total_num;
	int data_disk_coeff;
	char temp_char;

	int g1, g2, g12;
	char *P_temp;
	char *Q_temp;

	struct timeval t1, t2, t3, starttime, endtime;
	double duration;

	memset(buf, 0, size);

	vector<vector<int> > repair_q_blocks_no;

	disk_total_num = NCFS_DATA->disk_total_num;
	block_size = NCFS_DATA->disk_block_size;
	block_no = offset / block_size;

	q_disk_id = disk_total_num - 1;
	p_disk_id = disk_total_num - 2;

	if (NCFS_DATA->disk_status[disk_id] == 0) {
		if (NCFS_DATA->run_experiment == 1){
       		gettimeofday(&t1,NULL);
        }
		retstat = cacheLayer->readDisk(disk_id, buf, size, offset);
		if (NCFS_DATA->run_experiment == 1){
       		gettimeofday(&t2,NULL);
        	duration = (t2.tv_sec - t1.tv_sec) + 
				(t2.tv_usec-t1.tv_usec)/1000000.0;
			NCFS_DATA->diskread_time += duration;
        }
		return retstat;
	}
	else{
		//check the number of failed disks
		disk_failed_no = 0;
		for (int i = 0; i < disk_total_num; i++) {
			if (NCFS_DATA->disk_status[i] == 1) {
				(disk_failed_no)++;
				if (i != disk_id) {
					disk_another_failed_id = i;
				}
			}
		}

		int strip_num = block_no / strip_size;
		int strip_offset = block_no % strip_size;
		
		if(!in_buf){
			in_buf = !in_buf;
			gettimeofday(&starttime,NULL);
			pre_read_into_buf(disk_id, strip_num, pread_stripes);
			gettimeofday(&endtime,NULL);
			func_cost_time += endtime.tv_sec - starttime.tv_sec + (endtime.tv_usec-starttime.tv_usec)/1000000.0;
		}


		if(disk_failed_no == 1){ //fail disk id = disk_id
			if((disk_id >= 0) && (disk_id < disk_total_num-1)){
				//fail disk(disk_id) is data disk or P disk	
				for(int i = 0; i < disk_total_num-1; i++){
					if(i != disk_id){					
						if (NCFS_DATA->run_experiment == 1){
				       		gettimeofday(&t1,NULL);
				        }						
						for(long long j = 0; j < size; j++){
							buf[j] = buf[j] ^ pread_stripes[strip_offset][i][j];
						}
						if (NCFS_DATA->run_experiment == 1){
				       		gettimeofday(&t2,NULL);
				        	duration = (t2.tv_sec - t1.tv_sec) + 
								(t2.tv_usec-t1.tv_usec)/1000000.0;
							NCFS_DATA->encoding_time += duration;
				        }					
					}
				}
			}else{//fail disk(disk_id) is data Q disk
				printf("<%d, %d>===>", strip_offset, disk_id);
				PAIR q = make_pair(strip_offset, disk_id);
				RELATED_ELEMS::iterator iter = elem_hash[q].begin();
				for(; iter != elem_hash[q].end(); iter++){
					//cout<<"+++++++recover Q disk+++++++++"<<endl;
					int read_disk_id = iter->second;
					int read_strip_offset = iter->first;
					printf("<%d, %d>, ", read_strip_offset, read_disk_id);
					if (NCFS_DATA->run_experiment == 1){
				  		gettimeofday(&t1,NULL);
			        }		
					for(long long j = 0; j < size; j++){
						buf[j] = buf[j] ^ 
							pread_stripes[read_strip_offset][read_disk_id][j];
					}
					if (NCFS_DATA->run_experiment == 1){
			       		gettimeofday(&t2,NULL);
			        	duration = (t2.tv_sec - t1.tv_sec) + 
							(t2.tv_usec-t1.tv_usec)/1000000.0;
						NCFS_DATA->encoding_time += duration;
				        }
				}
				cout<<endl;
			}
		}
		else{
			printf("RDP number of failed disks is larger than 1.\n");
			return -1;
		}
	}
	AbnormalError();
	return -1;
}



/*
 * recovering_rdp: rdp recover
 *
 * @param failed_disk_id: failed disk id
 * @param newdevice: new device to replace the failed disk
 *
 * @return: 0: success ; -1: wrong
 */
int Coding4Rdp::recover(int failed_disk_id,char* newdevice)
{
        //size of data to be rebuilded
	int __recoversize = NCFS_DATA->disk_size[failed_disk_id];
	int block_size = NCFS_DATA->disk_block_size;
	char buf[block_size];	
	
	for(int i = 0; i < __recoversize; ++i){
	  
	        //reset the buf data
	        memset(buf, 0, block_size);
		
		int offset = i * block_size;

		int retstat = fileSystemLayer->codingLayer->decode(
			failed_disk_id,buf,block_size,offset);

		retstat = cacheLayer->writeDisk(
			failed_disk_id,buf,block_size,offset);

	}

	return 0;
	
}


int Coding4Rdp::
recover_enable_node_encode(
	int failed_disk_id, 
	char* newdevice, 
	int* node_socketid_array)
{
	return 0;
}

int Coding4Rdp::
recover_using_multi_threads_nor(
	int failed_disk_id, 
	char* newdevice, 
	int total_sor_threads)
{  
    return 0;
}

int Coding4Rdp::
recover_using_multi_threads_sor(
	int failed_disk_id, 
	char* newdevice,
	int total_sor_threads, 
	int number_sor_thread)
{
    return 0;
}

int Coding4Rdp::
recover_using_access_aggregation(
	int failed_disk_id, 
	char* newdevice)
{
    return 0;
}

int Coding4Rdp::
recover_conventional(
	int failed_disk_id, 
	char* newdevice)
{  
   return 0;
}