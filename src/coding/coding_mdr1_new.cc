#include "coding_mdr1_new.hh"
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
#include <math.h>

#include <iostream>
#include <vector>
#include <map>
#include <set>
using namespace std;

//NCFS context state
extern struct ncfs_state* NCFS_DATA;
extern FileSystemLayer* fileSystemLayer;
extern CacheLayer* cacheLayer;
extern DiskusageReport* diskusageLayer;


map<int, > func(int k){
	if(k = 1){

	}
}