#include "coding_evenodd.hh"
#include "../filesystem/filesystem_common.hh"
#include "../filesystem/filesystem_utils.hh"
#include "../cache/cache.hh"
#include "../gui/diskusage_report.hh"



//NCFS context state
extern struct ncfs_state* NCFS_DATA;
extern FileSystemLayer* fileSystemLayer;
extern CacheLayer* cacheLayer;
extern DiskusageReport* diskusageLayer;


void Coding4Mdr1::print_ivec(vector<int>& ivec){
	int size = ivec.size();
	for(int i = 0; i < size; i++){
		cout<<(ivec.at(i))<<" ";
	}
	cout<<endl;
}

void Coding4Mdr1::print_iivec(vector<vector<int> >& 
	iivec){
	int iivec_size = iivec.size();
	for(int i = 0; i < iivec_size; i++){
		int ivec_size = iivec[i].size();
		cout<<"--";
		for(int j = 0; j < ivec_size; j++){
			cout<<iivec[i][j]<<" ";
		}
		cout<<endl;
	}
	cout<<endl;
}

void Coding4Mdr1::print_ivmap(map<int, 
	vector<vector<int> > >& ivmap, 
	vector<int>& stripeIndexs)
{
	set<int> iset;
	iset.insert(stripeIndexs.begin(), stripeIndexs.end());

	for(int i = 0; i < strip_size; i++){
		if(iset.find(i) == iset.end()){
			cout<<i<<": ";
			int i_size = ivmap[i].size();
			for(int j = 0; j < i_size; j++){
				int ii_size = ivmap[i][j].size();
				if(ii_size == 0){
					cout<<-1<<"\t";
					continue;
				}
				for(int t = 0; t < ii_size; t++){
					cout<<ivmap[i][j][t]<<" ";
				}
				cout<<"\t";
			}
			cout<<endl;
		}
	}
}

long long* Coding4Mdr1::
mdr_I_iterative_construct_encoding_matrixB(long long *matrix, int k)
{
	int i, j, t;
	int row = (int)pow(2, k);
	int col = k+1;
	int len = row * col;
	
	int new_k = k+1;
	int new_row = (int)pow(2, new_k);
	int new_col = new_k+1;
	int new_len = new_row * new_col;

	long long * new_matrix = new long long [new_len];
	if(new_k == 1 && matrix == NULL){
		new_matrix[0] = 1;
		new_matrix[1] = 0;
		new_matrix[2] = 0;
		new_matrix[3] = 2;
		return new_matrix;
	}else{
		for(i = 0; i < new_col; i++){
			if(i < new_col-2){
				for(j = 0; j < row; j++){
					new_matrix[j*new_col+i] = (matrix[j*col+i] 
						^ matrix[j*col+col-1]) << row;
				}
				for(j = row; j < new_row; j++){
					new_matrix[j*new_col+i] = (matrix[(j-row)*col+i] 
						^ matrix[(j-row)*col+col-1]);
				}
			}else if(i == new_col-2){
				t = 0;
				for(j = row-1; j >= 0; j--){
					new_matrix[j*new_col+i] = 1 << t;
					t++;
				}
				for(j = row; j < new_row; j++){
					new_matrix[j*new_col+i] = 0;
				}
			}else if(i == new_col-1){
				for(j = 0; j < row; j++){
					new_matrix[j*new_col+i] = 0;
				}			
				for(j = new_row-1; j >= row; j--){
					new_matrix[j*new_col+i] = 1 << t;
					t++;
				}
			}
		}
		//free(matrix);
		delete []matrix;

		return new_matrix;
	}
}

void Coding4Mdr1::mdr_print_matrix(long long* matrix, 
	int row, int col)
{
	for(int i = 0; i < row; i++){
		for(int j = 0; j < col; j++)
			printf("%lld\t",matrix[i*col+j]);
		printf("\n");
	}
}

long long* Coding4Mdr1::mdr_I_encoding_matrix(int k){ 
	int count;
	long long *mdr_encoding_matrix_B;
	if(k <= 0){
		printf("error: k\n");
		exit(1);
	}
	
	mdr_encoding_matrix_B = new long long [4];

	mdr_encoding_matrix_B[0] = 1;
	mdr_encoding_matrix_B[1] = 0;
	mdr_encoding_matrix_B[2] = 0;
	mdr_encoding_matrix_B[3] = 2;	
	if(k == 1)
		return mdr_encoding_matrix_B;

	count = 1;
	while(count != k){
		mdr_encoding_matrix_B = 
			mdr_I_iterative_construct_encoding_matrixB(
				mdr_encoding_matrix_B, count);
		count++;
	}
	return mdr_encoding_matrix_B;
}

vector<int> Coding4Mdr1::mdr_I_find_q_blocks_id(
	int disk_id, int block_no)
{
	vector<int> ivec;

	int row = strip_size;
	int col = NCFS_DATA->data_disk_num + 1;
	int pDisk_id = NCFS_DATA->data_disk_num; 

	int t = 1 << (strip_size - block_no -1);
	for(int i = 0; i < row; i++){
		//data block affect q disk
		if((mdr_I_encoding_matrixB[i*col+disk_id]&t) != 0){
			ivec.push_back(i);
		}

		//parity block (disk_id) affects q disk
		if((mdr_I_encoding_matrixB[i*col+pDisk_id]&t) != 0){
			ivec.push_back(i);
		}		
	}
	return ivec;
}

vector<vector<int> > Coding4Mdr1::
mdr_I_repair_qDisk_blocks_id(int block_no){
	vector<vector<int> > iivec;
	int row = strip_size;
	int col = NCFS_DATA->data_disk_num+1;
	for(int i = 0; i < col; i++){
		vector<int> ivec;
		for(int j = 0; j < strip_size; j++){
			if((mdr_I_encoding_matrixB[block_no*col+i]&
				(1<<(strip_size-j-1))) != 0){
				ivec.push_back(j);
			}
		}
		iivec.push_back(ivec);
	}
	return iivec;
}


vector<int> Coding4Mdr1::
mdr_I_repair_dpDisk_stripeIndexs_internal(
	int diskID, int val_k)
{
	vector<int> ivec;
	if(val_k == 1){
		if(diskID == 1){
			ivec.push_back(1);
			return ivec;
		}
		else if(diskID == 2){
			ivec.push_back(2);
			return ivec;
		}else{
			cout<<"diskID = "<<diskID<<endl;
			cout<<"val_k = "<<val_k<<endl;
			printf("error: fail diskID is large \
				than k in mdr_I_repair_dpDisk_stripeIndexs()\n");
			exit(1);
		}
	}

	int val_strip_size = (int)pow(2, val_k);
	int r = val_strip_size / 2;
	if(diskID >= 1 && diskID <= val_k-1){ 
		vector<int> ivec_old = 
		mdr_I_repair_dpDisk_stripeIndexs_internal(diskID, val_k-1);
		set<int> iset;
		iset.insert(ivec_old.begin(), ivec_old.end());

		for(int i = 1; i <= val_strip_size; i++){
			if((iset.find(i) != iset.end()) ||
			 (iset.find(i-r) != iset.end())){
				ivec.push_back(i);
			}
		}
		return ivec;
	}else if(diskID == val_k){
		for(int i = 1; i <= r; i++){
			ivec.push_back(i);
		}
		return ivec;
	}else if(diskID == val_k+1){
		for(int i = r+1; i <= val_strip_size; i++){
			ivec.push_back(i);
		}
		return ivec;
	}else{
		printf("error: fail diskID is large than k \
			in mdr_I_repair_dpDisk_stripeIndexs()\n");
		exit(1);
	}
}

vector<int> Coding4Mdr1::
mdr_I_repair_dpDisk_stripeIndexs(int diskID, int val_k){
	vector<int> ivec = 
	mdr_I_repair_dpDisk_stripeIndexs_internal(diskID+1, val_k);
	int ivec_size = ivec.size();

	vector<int> ivec2;
	for(int i = 0; i < ivec_size; i++){
		ivec2.push_back(ivec[i]-1);
	}
	return ivec2;
}

bool Coding4Mdr1::
mdr_I_repair_if_blk_in_buf(int disk_id, int stripe_blk_offset, 
	bool** isInbuf, vector<int>& stripeIndexs)
{
	int vec_size = stripeIndexs.size();
	for(int i = 0; i < vec_size; i++){
		if((stripe_blk_offset == stripeIndexs[i]) && 
			(isInbuf[i][disk_id] == true)){
			return true;
		}
	}
	return false;
}

int Coding4Mdr1::
mdr_I_repair_chg_blkIndexOffset_in_buf(int disk_id, 
	int stripe_blk_offset, vector<int>& stripeIndexs){

	int vec_size = stripeIndexs.size();
	int count = 0;
	for(int i = 0; i < vec_size; i++){
		if(stripe_blk_offset == stripeIndexs[i])
			return count;
		count++;
	}
	return -1;
}


map<int, vector<vector<int> > > Coding4Mdr1::
mdr_I_repair_dpDisk_nonstripeIndexs_blocks_no(
	int fail_disk_id, vector<int>& stripeIndexs)
{

	map<int, vector<vector<int> > > ivmap;
	int row = strip_size;
	int col = NCFS_DATA->data_disk_num+1;

	set<int> iset;
	iset.insert(stripeIndexs.begin(), stripeIndexs.end());

	int stripeIndexs_size = stripeIndexs.size();
	for(int i = 0; i < stripeIndexs_size; i++){
		vector<vector<int> > iivec;
		int fail_disk_block_no = -1;
		for(int j = 0; j < col; j++){
			int val = mdr_I_encoding_matrixB[stripeIndexs[i]*col+j];
			//cout<<"val = "<<val<<endl;
			//bool flag = true;
			vector<int> ivec; 
			for(int t = 0; t < strip_size; t++){
				if((val&(1<<(strip_size-t-1))) != 0){
					if(iset.find(t) != iset.end()){
						//flag = false;
						ivec.push_back(t);
					}else if(iset.find(t) == iset.end() ){
						fail_disk_block_no = t;
					}
				}
			}
			// if(flag)
			// 	ivec.push_back(-1);
			iivec.push_back(ivec);
		}
		//cout<<endl;
		if(fail_disk_block_no == -1){
			printf("error in \
				mdr_I_repair_dpDisk_nonstripeIndexs_blocks_no()\n");
			exit(1);
		}

		// push Q block
		vector<int> ivec;
		ivec.push_back(stripeIndexs[i]);
		iivec.push_back(ivec);

		ivmap[fail_disk_block_no] = iivec;
	}
	return ivmap;	
}

/*
 * constructor for mdr1.
 */
Coding4Mdr1::Coding4Mdr1()
{
	mdr_I_encoding_matrixB = 
	mdr_I_encoding_matrix(NCFS_DATA->data_disk_num);
	strip_size = (int)pow(2, NCFS_DATA->data_disk_num);
	mdr_I_one_dpDisk_fail_bool_m = false;
	mdr_I_one_dpDisk_fail_bool_v = false;
}

Coding4Mdr1 :: ~Coding4Mdr1(){
	delete []mdr_I_encoding_matrixB;
}


struct data_block_info Coding4Evenodd::
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
		for (j = 0; j < size_request; j++) {
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

		q_blocks_no = mdr_I_find_q_blocks_id(disk_id, strip_offset);

		// cout<<"q_blocks_no\n";
		// print_ivec(q_blocks_no);

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

		    if (NCFS_DATA->run_experiment == 1){
        		gettimeofday(&t2,NULL);
        		duration = (t2.tv_sec - t1.tv_sec) + 
					(t2.tv_usec-t1.tv_usec)/1000000.0;
				NCFS_DATA->diskread_time += duration;
        	}

			//calculate new Q blk
			for (j = 0; j < size_request; j++) {
				buf_q_disk[j] = buf_q_disk[j] ^ buf[j];
			}

		    if (NCFS_DATA->run_experiment == 1){
        		gettimeofday(&t1,NULL);
        		duration = (t1.tv_sec - t2.tv_sec) + 
					(t1.tv_usec-t2.tv_usec)/1000000.0;
				NCFS_DATA->encoding_time += duration;        	
        	}		

			//write new Q blk
			retstat = cacheLayer->writeDisk(q_disk_id, buf_q_disk, size, 
				q_blk_no * block_size);		

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

int Coding4Mdr1::decode(int disk_id, char *buf, long long size,
				long long offset)
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

				//find the stripe index which is must be read
				if(mdr_I_one_dpDisk_fail_bool_v == false){
					mdr_I_one_dpDisk_fail_bool_v = true;
					mdr_I_one_dpDisk_fail_stripeIndex = 
						mdr_I_repair_dpDisk_stripeIndexs(
							disk_id, NCFS_DATA->data_disk_num);
				}
				
				// cout<<"mdr_I_one_dpDisk_fail_stripeIndex:\n";
				// print_ivec(mdr_I_one_dpDisk_fail_stripeIndex);

				set<int> stripeIndexs_set;
				stripeIndexs_set.insert(
					mdr_I_one_dpDisk_fail_stripeIndex.begin(), 
					mdr_I_one_dpDisk_fail_stripeIndex.end()
					);

				//if the needed blk is in the stripeIndexs
				if(stripeIndexs_set.find(strip_offset) 
					!= stripeIndexs_set.end()){

					for(int i = 0; i < disk_total_num-1; i++){
						if(i != disk_id){
							retstat = cacheLayer->readDisk(
								i, temp_buf, size, offset);
							for(long long j = 0; j < size; j++){
								buf[j] = buf[j] ^ temp_buf[j];
							}
						}
					}
				}
				else{// if the neede blk is not in the stripeIndexs
					if(mdr_I_one_dpDisk_fail_bool_m == false){
						mdr_I_one_dpDisk_fail_bool_m = true;

						mdr_I_one_dpDisk_fail_nonStripeIndex = 
							mdr_I_repair_dpDisk_nonstripeIndexs_blocks_no(
								disk_id, mdr_I_one_dpDisk_fail_stripeIndex);
					}					

					// cout<<"mdr_I_one_dpDisk_fail_nonStripeIndex\n";
					// print_ivmap(mdr_I_one_dpDisk_fail_nonStripeIndex,
					// mdr_I_one_dpDisk_fail_stripeIndex);

					vector<vector<int> > iivec = 
					mdr_I_one_dpDisk_fail_nonStripeIndex[strip_offset];

					//cout<<"iivec:\n";
					//print_iivec(iivec);
					int iivec_size = iivec.size();
					//cout<<"iivec_size = "<<iivec_size<<endl;

					if(iivec_size != NCFS_DATA->disk_total_num){
						printf("error: repair reading blks num\n");
						exit(1);
					}

					int debug_count = 0;

					for(int i = 0; i < iivec_size; i++){
						if(!iivec[i].empty()){	
							//the needed blk should be repaired firstly
							if(i == disk_id){
								int ivec_size = iivec[i].size();
								for(int i2 = 0; i2 < ivec_size; i2++){
									int i2_blk_num = 
									iivec[i][i2]+strip_num*strip_size;

									for(int ii = 0; 
										ii < disk_total_num-1; 
										ii++){
										if(ii != i){
											//printf("debug_count = %d, 
											//[disk_id, block_no] = [%d, %d]\n",
											// ++debug_count, ii, i2_blk_num);
											if (NCFS_DATA->run_experiment == 1){
        										gettimeofday(&t1,NULL);
        									}
											retstat = cacheLayer->readDisk(
												ii, temp_buf, size, i2_blk_num*block_size);
										
											if (NCFS_DATA->run_experiment == 1){
        										gettimeofday(&t2,NULL);
        										duration = (t2.tv_sec - t1.tv_sec) + 
													(t2.tv_usec-t1.tv_usec)/1000000.0;
												NCFS_DATA->diskread_time += duration;
        									}

											for(long long j = 0; j < size; j++){
												buf[j] = buf[j] ^ temp_buf[j];
											}

											if (NCFS_DATA->run_experiment == 1){
        										gettimeofday(&t1,NULL);
        										duration = (t1.tv_sec - t2.tv_sec) + 
													(t1.tv_usec-t2.tv_usec)/1000000.0;
												NCFS_DATA->decoding_time += duration;
        									}

										}
									}
								}
							}
					
							//all the needed blks can be read directly
							if(i != disk_id){
								int ivec_size = iivec[i].size();
								for(int ii = 0; ii < ivec_size; ii++){
									int blk_num = iivec[i][ii]+
									strip_num * strip_size;

									//printf("debug_count = %d, 
									//[disk_id, block_no] = [%d, %d]\n",
									// ++debug_count, i, blk_num);

									if (NCFS_DATA->run_experiment == 1){
        								gettimeofday(&t1,NULL);
        							}		
									
									retstat = cacheLayer->readDisk(
										i, temp_buf, size, blk_num * block_size);

									if (NCFS_DATA->run_experiment == 1){
        								gettimeofday(&t2,NULL);
        								duration = (t2.tv_sec - t1.tv_sec) + 
											(t2.tv_usec-t1.tv_usec)/1000000.0;
										NCFS_DATA->diskread_time += duration;
        							}
									
									for(long long j = 0; j < size; j++){
										buf[j] = buf[j] ^ temp_buf[j];
									}
											
									if (NCFS_DATA->run_experiment == 1){
        								gettimeofday(&t1,NULL);
        								duration = (t1.tv_sec - t2.tv_sec) + 
											(t1.tv_usec-t2.tv_usec)/1000000.0;
										NCFS_DATA->decoding_time += duration;
        							}
        							
								}
							}
						}
					}
				}
			}
			else if(disk_id = disk_total_num - 1){
				//fail disk(disk_id) is Q disk
				//TODO:
					
			}
		}
		else{
			printf("MDR_I number of failed disks is larger than 1.\n");
			return -1;
		}

		free(temp_buf);
		return size;

	}
	AbnormalError();
	return -1;
}

int Coding4Mdr1::
mdr_I_recover_oneStripeGroup(
	int disk_id, char *buf, long long size,
	long long offset, char*** pread_stripes)
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

		retstat = cacheLayer->readDisk(disk_id, buf, size, offset);
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

				//find the stripe indexs which is must be read
				if(mdr_I_one_dpDisk_fail_bool_v == false){
					mdr_I_one_dpDisk_fail_bool_v = true;
					mdr_I_one_dpDisk_fail_stripeIndex = 
						mdr_I_repair_dpDisk_stripeIndexs(
							disk_id, NCFS_DATA->data_disk_num);
				}

				//find the block indexs which must be repaired by the stripes in the buf
				if(mdr_I_one_dpDisk_fail_bool_m == false){
					mdr_I_one_dpDisk_fail_bool_m = true;

					mdr_I_one_dpDisk_fail_nonStripeIndex = 
						mdr_I_repair_dpDisk_nonstripeIndexs_blocks_no(disk_id, 
						 	mdr_I_one_dpDisk_fail_stripeIndex);
				}				

				//read the essencial stripes into the pread_stripes
				int s_size = mdr_I_one_dpDisk_fail_stripeIndex.size();
				for(int i = 0; i < s_size; i++){
					int blk_num = strip_num*strip_size
					 + mdr_I_one_dpDisk_fail_stripeIndex[i];
					for(int j = 0; j < disk_total_num; j++){
						if(j != disk_id){
							retstat = cacheLayer->readDisk(
								j, pread_stripes[i][j], 
								block_size, 
								blk_num*block_size);

							  // FILE * pFile;
							  // char filename[50];
							  // sprintf(filename, "./wds/pread_stripe_%d_%d", i, j);
							  // pFile = fopen(filename, "wb");
							  // fwrite (pread_stripes[i][j], sizeof(char), 
							  // 					block_size, pFile);
							  // fclose (pFile);

						}
					}
				}

				//repair the blk in the buf strips
				for(int i = 0; i < s_size; i++){
					for(int j = 0; j < disk_total_num -1; j++){
						if(j != disk_id){
							for(long long j2 = 0; 
								j2 < block_size; 
								j2++){
								pread_stripes[i][disk_id][j2] ^=
								 pread_stripes[i][j][j2];
							}
						}
					}
					int buf_offset = mdr_I_one_dpDisk_fail_stripeIndex[i];

					//cout<<"buf_offset*block_size = "<<buf_offset*block_size<<endl;

					// if(i == 0){
					// 	FILE * pFile;
					//     char filename[50];
					// 	sprintf(filename, "./wds/pread_stripe");
					// 	pFile = fopen(filename, "wb");
					// 	fwrite (pread_stripes[i][disk_id], sizeof(char), 
					// 							block_size, pFile);
					// 	fclose (pFile);
					// }
					
					//strcpy(buf+(buf_offset*block_size), pread_stripes[i][disk_id]);
					for(long long j2 = 0; 
						j2 < block_size; 
						j2++){
						*(buf+(buf_offset*block_size+j2)) =
						 pread_stripes[i][disk_id][j2];
					}

					// if(i == 0){
					// 	FILE * pFile;
					//     char filename[50];
					// 	sprintf(filename, "./wds/buf");
					// 	pFile = fopen(filename, "wb");
					// 	fwrite (buf, sizeof(char), block_size, pFile);
					// 	fclose (pFile);
					// }				

				}

				//repair other blks in the failed disk
				set<int> stripeIndexs_set;
				stripeIndexs_set.insert(
					mdr_I_one_dpDisk_fail_stripeIndex.begin(), 
					mdr_I_one_dpDisk_fail_stripeIndex.end()
					);

				for(int i = 0; i < strip_size; i++){
					if(stripeIndexs_set.find(i) ==
					 stripeIndexs_set.end()){
						vector<vector<int> > iivec =
						 mdr_I_one_dpDisk_fail_nonStripeIndex[i];
						
						int iivec_size = iivec.size();
						if(iivec_size != NCFS_DATA->disk_total_num){
							printf("error: repair reading blks num\n");
							exit(1);
						}
				
						int blk_index = i;

						for(int j = 0; j < iivec_size; j++){
						
						int parcitipant_disk_id = j;

							if(!iivec[j].empty()){
								int ivec_size = iivec[j].size();
								for(int i2 = 0; i2 < ivec_size; i2++){
									int par_disk_blk = 
									mdr_I_repair_chg_blkIndexOffset_in_buf(
										parcitipant_disk_id,iivec[j][i2],
										 mdr_I_one_dpDisk_fail_stripeIndex);
									if(par_disk_blk == -1){
										cout<<"parcitipant_disk_id = "<<parcitipant_disk_id<<endl;
										printf("iivec[%d][%d] = %d\n", j, i2, iivec[j][i2]);
										//cout<<"ivec[j][i2] = "<<iivec[j][i2]<<endl;

										print_ivec(mdr_I_one_dpDisk_fail_stripeIndex);
										printf("error: par_disk_blk\n");
										exit(1);				
									}

									char *src = pread_stripes[par_disk_blk][parcitipant_disk_id];
									char *des = buf + blk_index*block_size;

									for(long long j2 = 0; j2 < block_size; j2++){
										des[j2] ^= src[j2];
									}

								}
							}
						}						
					}
				}				
			}
			else if(disk_id = disk_total_num - 1){
				//fail disk(disk_id) is Q disk
				//TODO:	
			}
		}
		else{
			printf("MDR_I number of failed disks is larger than 1.\n");
			return -1;
		}

		free(temp_buf);
		return size;

	}
	AbnormalError();
	return -1;
}


/*
 * recovering_mdr1: MDR 1 recover
 *
 * @param failed_disk_id: failed disk id
 * @param newdevice: new device to replace the failed disk
 *
 * @return: 0: success ; -1: wrong
 */
int Coding4Mdr1::recover(int failed_disk_id,char* newdevice)
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

int Coding4Mdr1::
recover_enable_node_encode(
	int failed_disk_id, 
	char* newdevice, 
	int* node_socketid_array)
{
	return 0;
}

int Coding4Mdr1::
recover_using_multi_threads_nor(
	int failed_disk_id, 
	char* newdevice, 
	int total_sor_threads)
{  
    return 0;
}

int Coding4Mdr1::
recover_using_multi_threads_sor(
	int failed_disk_id, 
	char* newdevice,
	int total_sor_threads, 
	int number_sor_thread)
{
    return 0;
}

int Coding4Mdr1::
recover_using_access_aggregation(
	int failed_disk_id, 
	char* newdevice)
{
    return 0;
}

int Coding4Mdr1::
recover_conventional(
	int failed_disk_id, 
	char* newdevice)
{  
   return 0;
}