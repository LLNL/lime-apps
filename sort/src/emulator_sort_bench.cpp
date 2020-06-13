

#include <iostream>
#include <algorithm>
#include <stdint.h>
#include <sys/time.h>

double get_wtime();


int main(int argc, char** argv) {

  if(argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <num 8byte ints>" <<std::endl;
    return -1;
  }

  int        num_ints = atoi(argv[1]);
  uint64_t*  data = NULL;

  //
  //  Allocate Data
  //
  data = (uint64_t*) malloc(num_ints * sizeof(uint64_t));
  if(data == NULL) {
    std::cerr << "Failed to allocate memory" << std::endl;
    exit(-1);
  }
  std::cout << "Allocated " << num_ints * sizeof(uint64_t) << " bytes." << std::endl;
  
  
  //
  // Init Data
  //
  double start_init = get_wtime();
  for(int i=0; i<num_ints; ++i) {
    data[i] = i;
  }
  double stop_init = get_wtime();
  std::cout << "Data Initialized: " << stop_init - start_init << " seconds." << std::endl;


  //
  // Scramble Data
  //
  double start_scramble = get_wtime();
  std::random_shuffle(data, data+num_ints);
  double stop_scramble = get_wtime();
  std::cout << "Data scrambled: " << stop_scramble - start_scramble << " seconds." << std::endl;


  //
  // Sort Data
  //
  double start_sort = get_wtime();
  std::sort(data, data+num_ints);
  double stop_sort = get_wtime();
  std::cout << "Data Sorted: " << stop_sort - start_sort << " seconds." << std::endl;


  //
  // Validate Data
  //
  double start_validate = get_wtime();
  for(int i=0; i<num_ints; ++i) {
    if(data[i] != i) {
      std::cerr << "Found an error!" << std::endl;
      exit(-1);
    }
  }
  double stop_validate = get_wtime();
  std::cout << "Data Validated: " << stop_validate - start_validate << " seconds." << std::endl;
  
  //
  // Free data
  //
  free(data);

  return 0;
}

double get_wtime() {
  struct timeval now;
  gettimeofday(&now, NULL);
  return double(now.tv_sec) + double(now.tv_usec)/1e6;
}



