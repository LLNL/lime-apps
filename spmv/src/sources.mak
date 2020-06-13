TARGET = spmv
VERSION = 1.1
SRC += ../src $(addprefix ../src/,bebop_util matrix_generator spmvbench) $(SHARED)
ifneq ($(filter %SERVER,$(DEFS)),)
  MODULES += server
else
  MODULES += spmv
  MODULES += block_smvm_code smvm_benchmark smvm_timing_results smvm_timing_run smvm_verify_result timing
  MODULES += bcoo_matrix bcsr_matrix create_rand
  MODULES += __complex mt19937ar random_number smvm_malloc smvm_util sort_joint_arrays
endif
ifneq ($(filter $(NEED_STREAM),$(DEFS)),)
  DEFS += -DUSE_LSU
  MODULES += lsu_cmd
endif
ifeq ($(ARG),1)
  RUN_ARGS = -c -s23 -n34 -v15
else
  RUN_ARGS = -c -s18 -n34 -v15
endif
# CXXFLAGS += -std=c++11
# LDFLAGS += 
# LDLIBS += -lstdc++
