TARGET = spmv
VERSION = 1.1
SRC = ../src $(addprefix ../src/,bebop_util matrix_generator spmvbench) ../../shared
ifneq (,$(filter %SERVER,$(DEFS)))
  DEFS += -DUSE_LSU
  MODULES = server
else
  MODULES = spmv
  MODULES += block_smvm_code smvm_benchmark smvm_timing_results smvm_timing_run smvm_verify_result timing
  MODULES += bcoo_matrix bcsr_matrix create_rand
  MODULES += __complex mt19937ar random_number smvm_malloc smvm_util sort_joint_arrays
endif
ifneq (,$(filter %CLIENT %SERVER %USE_LSU,$(DEFS)))
  DEFS += -DUSE_SP -DUSE_OCM
  MODULES += lsu_cmd aport stream
endif
RUN_ARGS = -c -s18 -n34 -v15
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++