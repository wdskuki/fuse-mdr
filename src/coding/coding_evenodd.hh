#ifndef __CODING_EVENODD_HH__
#define __CODING_EVENODD_HH__

#include "coding.hh"

#include <stdio.h>

#include <vector>
#include <string>
#include <iostream>
#include <list>
#include <map>
using namespace std;

void print(vector<int>& t);

typedef pair<int, int> PAIR;
typedef list<PAIR> RELATED_ELEMS;
typedef map<PAIR, RELATED_ELEMS> RELATED_ELEMS_HASH;

class Coding4Evenodd : public CodingLayer {
private:
	int k; //data disk num
	int **matrix; //row*col
	int row, col;
	int strip_size;
	RELATED_ELEMS_HASH elem_hash;

	RELATED_ELEMS diagonal_elements();
	RELATED_ELEMS p_related_elems(const PAIR& e);
	RELATED_ELEMS q_related_elems(const PAIR& e, const RELATED_ELEMS& diag_elems);

	vector<int> evenodd_find_q_blocks_id(int disk_id, int block_no);
	void pre_read_into_buf(int failed_disk_id, int strip_num, char*** buf_blocks);
public:
	Coding4Evenodd();
	~Coding4Evenodd();
	struct data_block_info encode(const char *buf, int size);
	int decode(int disk_id, char *buf, long long size, long long offset);
	int evenodd_recover_block(int disk_id, char *buf, long long size,
								long long offset, char*** pread_stripes, bool& in_buf);

	int evenodd_get_strip_size(){return strip_size;}

	virtual int recover(int failed_disk_id,char* newdevice);     
	virtual int recover_enable_node_encode(int failed_disk_id, char* newdevice, int* node_socketid_array);
	//recover the failed node using nor_multithreads
	virtual int recover_using_multi_threads_nor(int failed_disk_id, char* newdevice, int total_sor_threads);
	
	//recover the failed node using sor_multithreads
	virtual int recover_using_multi_threads_sor(int failed_disk_id, char* newdevice
									, int total_sor_threads, int number_sor_thread);
	virtual int recover_using_access_aggregation(int failed_disk_id, char* newdevice);
	
	virtual int recover_conventional(int failed_disk_id,char* newdevice);



	void print(string str); //show the matrix
	void random_data_element();
	void same_data_element(int e);
	void gen_p(); //generate p element(s)
	void gen_q(); //generate q elemnts(s)
	vector<int> repair(int id); //repair one disk(D or P or Q)
	void print_PAIR(const PAIR& e){
		printf("<%d, %d>", e.first, e.second);
	}
	void print_related_elems(const RELATED_ELEMS& re){
		RELATED_ELEMS:: const_iterator iter = re.begin();
		for(;iter != re.end(); iter++){
			print_PAIR(*iter);
			printf(",");
		}
	}
	void print_elem_hash();
	
	void elems_related_elems(int row, int col);
};

#endif
