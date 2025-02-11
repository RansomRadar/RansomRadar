#pragma once


#include <fltKernel.h>
#include <dontuse.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <ntddk.h>

#define __HELPER_MAX_STRING_LENGTH 512
#define __HELPER_IRP_NOTHING 255
//#define __HELPER_MAX_LOG_LENGTH_FOR_ONCE 1024
typedef double __HELPER_F64;
typedef float __HELPER_F32;

UINT64 VALUE_TO_1_BIT_COUNT[256] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2,
3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3,
3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4,
5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5,
5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4,
5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6,
5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };

size_t GetFileName(wchar_t* fileName, PFLT_CALLBACK_DATA data);
VOID WriteToLog(PCFLT_RELATED_OBJECTS FltObjects, char* Buffer, UINT64 BufferSize);
NTSTATUS GetProcessImageName(wchar_t* buff);
NTSTATUS GetProcessBasicInfo(PROCESS_BASIC_INFORMATION* info);
void ProcessIRP(PFLT_CALLBACK_DATA data, PCFLT_RELATED_OBJECTS FltObjects);
__HELPER_F64 CalculateEntropyBasedOnBit(void* buffer, UINT64 size);
__HELPER_F64 CalculateEntropyBasedOnByte(void* buffer, UINT64 size);
UINT8 isTheIrpRenameFile(PFLT_CALLBACK_DATA Data);
UINT8 isTheIrpDeleteFile(PFLT_CALLBACK_DATA Data);
UINT64 getCurrentTime();
//UINT8 isTheIrpCreateFile(PFLT_CALLBACK_DATA Data);

// ZwQueryInformationProcess - dynamic loaded function which query info data about already opened processes 
typedef NTSTATUS(*QUERY_INFO_PROCESS) (
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

QUERY_INFO_PROCESS ZwQueryInformationProcess;

// external declarations (from rust code)
UINT64 my_add(UINT64 x, UINT64 y);
UINT8 my_string_test(char* src, UINT64 size, char* tar);
UINT8 __helper_add_record(UINT64 time, UINT8 major_opr_,
    UINT8 minor_opr_,
    wchar_t* process_name_,
    UINT64 process_name_size,
    UINT64 pid, UINT64 tid,
    UINT64 parent_pid,
    UINT64 buffer_length,
    DOUBLE entropy_bit_based,
    DOUBLE entropy_byte_based,
    wchar_t* file_name_,
    UINT64 file_name_size,
    UINT8 is_rename,
    UINT8 is_delete,
    UINT64 file_size,
    UINT64 offset);
UINT8 __helper_flush_and_copy_buf(char** buf, UINT64* size);
UINT8 __helper_output_header_and_copy_buf(char** buf, UINT64* size);
__HELPER_F64 __helper_log2(__HELPER_F64 p);
UINT8 __helper_convert_float_to_str(__HELPER_F64 f, INT8* s, UINT64 size);
void __helper_dealloc_all_resource();
void __helper_log_file_lock();
void __helper_log_file_unlock();
UINT8 __helper_is_blacklist(wchar_t* process_name_, UINT64 process_name_size);