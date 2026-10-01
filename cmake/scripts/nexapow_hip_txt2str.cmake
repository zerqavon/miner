file(READ "${TXT2STR_ECC_HEADER}" eccHeader)
file(READ "${TXT2STR_ECC_SOURCE}" eccSource)
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../src/backend/opencl/cl/nexa/nexapow_g_table.cl" gTable)
file(READ "${TXT2STR_SOURCE_FILE}" kernelSource)
string(REPLACE "#include \"inc_ecc_secp256k1.h\"" "" eccSource "${eccSource}")
string(REPLACE "__kernel" "HIP_KERNEL_MARK" kernelSource "${kernelSource}")
string(REPLACE "__global" "" kernelSource "${kernelSource}")
string(REPLACE "__local" "__shared__" kernelSource "${kernelSource}")
string(REPLACE "__private" "" kernelSource "${kernelSource}")
string(REPLACE "__constant" "const" kernelSource "${kernelSource}")
string(REPLACE "HIP_KERNEL_MARK" "__global__" kernelSource "${kernelSource}")
string(REPLACE "get_global_id(0)" "(blockIdx.x * blockDim.x + threadIdx.x)" kernelSource "${kernelSource}")
string(REPLACE "get_local_id(0)" "threadIdx.x" kernelSource "${kernelSource}")
string(REPLACE "barrier(CLK_LOCAL_MEM_FENCE)" "__syncthreads()" kernelSource "${kernelSource}")
string(REPLACE "ulong" "unsigned long long" kernelSource "${kernelSource}")
string(REPLACE "static inline" "static __device__ __forceinline__" kernelSource "${kernelSource}")
string(REPLACE "__global__ void nexapow_sha_kernel" "extern \"C\" __global__ void nexapow_sha_kernel" kernelSource "${kernelSource}")
string(REPLACE "__constant u32 nexapow_g_table" "__device__ __constant__ u32 nexapow_g_table" gTable "${gTable}")
file(WRITE "${TXT2STR_HEADER_FILE}"
"static const char* ${TXT2STR_VARIABLE_NAME} = R\"delim(\n"
"typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64; typedef long long i64; typedef long long int64_t;\n"
"#define GLOBAL_AS\n#define PRIVATE_AS\n#define CONSTANT_AS const\n#define LOCAL_AS __shared__\n#define DECLSPEC static __device__ __forceinline__\n#define SECP256K1_TMPS_TYPE LOCAL_AS\n#define HAS_SUB 0\n#define HAS_SUBC 0\n"
"${eccHeader}\n${eccSource}\n${gTable}\n${kernelSource}\n"
")delim\";\n")
