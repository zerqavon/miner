include(CMakeParseArguments)

file(READ "${TXT2STR_ECC_HEADER}" eccHeader)
file(READ "${TXT2STR_ECC_SOURCE}" eccSource)
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../src/backend/opencl/cl/nexa/nexapow_g_table.cl" gTable)
file(READ "${TXT2STR_SOURCE_FILE}" kernelSource)
string(REPLACE "#include \"inc_ecc_secp256k1.h\"" "" eccSource "${eccSource}")

file(WRITE "${TXT2STR_HEADER_FILE}"
"static const char* ${TXT2STR_VARIABLE_NAME} = R\"delim(\n"
"typedef unsigned char u8;\n"
"typedef unsigned int u32;\n"
"typedef unsigned long u64;\n"
"typedef long i64;\n"
"typedef long long int64_t;\n"
"#define GLOBAL_AS __global\n"
"#define PRIVATE_AS __private\n"
"#define CONSTANT_AS __constant\n"
"#define DECLSPEC static inline\n"
"#define LOCAL_AS __local\n"
"#define SECP256K1_TMPS_TYPE LOCAL_AS\n"
"#define HAS_SUB 0\n#define HAS_SUBC 0\n"
"${eccHeader}\n${eccSource}\n${gTable}\n${kernelSource}\n"
")delim\";\n")
