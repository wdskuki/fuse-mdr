#ifndef __CODING_MDR_I_HH__
#define __CODING_MDR_I_HH__

#include "coding.hh"

#include <vector>
#include <map>
using namespace std;

class Coding4Mdr1 : public CodingLayer {
private:
	long long* mdr_I_encoding_matrixB;
	long long strip_size;
	bool mdr_I_one_dpDisk_fail_bool_m;
	bool mdr_I_one_dpDisk_fail_bool_v;
	bool mdr_I_one_qDisk_fail_bool;
	map<int, vector<vector<int> > > mdr_I_one_dpDisk_fail_nonStripeIndex;
	map<int, vector<vector<int> > > mdr_I_one_qDisk_fail_stripeIndex;
	vector<int> mdr_I_one_dpDisk_fail_stripeIndex;
	
	long long* mdr_I_iterative_construct_encoding_matrixB(long long *matrix, int k);
	long long* mdr_I_encoding_matrix(int k);
	
	vector<int> mdr_I_find_q_blocks_id(int disk_id, int block_no);
	vector<vector<int> > mdr_I_repair_qDisk_blocks_id(int block_no);
//	void mdr_I_repair_qDisk_stripeIndexs_blocks_no();

	vector<int> mdr_I_repair_dpDisk_stripeIndexs_internal(int diskID, int val_k);
	vector<int> mdr_I_repair_dpDisk_stripeIndexs(int diskID, int val_k);
	bool mdr_I_repair_if_blk_in_buf(int disk_id, int stripe_blk_offset, bool ** isInbuf,
									 vector<int>& mdr_I_one_dpDisk_fail_stripeIndex);
	int mdr_I_repair_chg_blkIndexOffset_in_buf(int disk_id, 
		int stripe_blk_offset, vector<int>& stripeIndexs);


	map<int, vector<vector<int> > > mdr_I_repair_dpDisk_nonstripeIndexs_blocks_no(
		int fail_disk_id, vector<int>& stripeIndexs);

	void print_ivec(vector<int>& ivec);
	void print_iivec(vector<vector<int> >& iivec);
	void print_ivmap(map<int, vector<vector<int> > >& ivmap, vector<int>& stripeIndexs);
	void mdr_print_matrix(long long* matrix, int row, int col);	
	void print_qDisk_related_block_no(map<int, vector<vector<int> > > & ivvmap);
	void pre_read_into_buf(int failed_disk_id, int strip_num, char*** buf_blocks);
public:
	Coding4Mdr1();
	~Coding4Mdr1();
	struct data_block_info encode(const char *buf, int size);
	int decode(int disk_id, char *buf, long long size, long long offset);
	int mdr_I_recover_oneStripeGroup(int disk_id, char *buf, long long size,
								long long offset, char*** pread_stripes);
	long long mdr_I_get_strip_size(){return strip_size;}

	virtual int recover(int failed_disk_id,char* newdevice);     
	virtual int recover_enable_node_encode(int failed_disk_id, char* newdevice, int* node_socketid_array);
	//recover the failed node using nor_multithreads
	virtual int recover_using_multi_threads_nor(int failed_disk_id, char* newdevice, int total_sor_threads);
	
	//recover the failed node using sor_multithreads
	virtual int recover_using_multi_threads_sor(int failed_disk_id, char* newdevice
									, int total_sor_threads, int number_sor_thread);
	virtual int recover_using_access_aggregation(int failed_disk_id, char* newdevice);
	
	virtual int recover_conventional(int failed_disk_id,char* newdevice);
};

#endif
