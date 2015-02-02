#ifndef __CODING_MDR_I_NEW_HH__
#define __CODING_MDR_I_NEW_HH__

#include "coding.hh"

#include <vector>
#include <map>
using namespace std;

class Coding4Mdr1 : public CodingLayer {
private:


public:
	Coding4Mdr1();
	~Coding4Mdr1();
	struct data_block_info encode(const char *buf, int size);
	int decode(int disk_id, char *buf, long long size, long long offset);
	int mdr_I_recover_oneStripeGroup(int disk_id, char *buf, long long size,
								long long offset, char*** pread_stripes);
	int mdr_I_get_strip_size(){return strip_size;}

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
