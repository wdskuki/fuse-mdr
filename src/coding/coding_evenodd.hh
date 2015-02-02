#ifndef __CODING_EVENODD_HH__
#define __CODING_EVENODD_HH__
#include "coding.hh"
class Coding4Evenodd : public CodingLayer {
private:

public:
	Coding4Evenodd();
	~Coding4Evenodd();

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
