/*++

Module Name:

	IRPCollection.c

Abstract:

	This is the main module of the IRPCollection miniFilter driver.

Environment:

	Kernel mode

--*/

#include "IRPCollection.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 0;


#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

/*************************************************************************
	Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
IRPCollectionInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
IRPCollectionInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
IRPCollectionInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

NTSTATUS
IRPCollectionUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
IRPCollectionInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
IRPCollectionPreOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

VOID
IRPCollectionOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
);

FLT_POSTOP_CALLBACK_STATUS
IRPCollectionPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
IRPCollectionPreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

BOOLEAN
IRPCollectionDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
);

EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, IRPCollectionUnload)
#pragma alloc_text(PAGE, IRPCollectionInstanceQueryTeardown)
#pragma alloc_text(PAGE, IRPCollectionInstanceSetup)
#pragma alloc_text(PAGE, IRPCollectionInstanceTeardownStart)
#pragma alloc_text(PAGE, IRPCollectionInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

	//#if 0 // TODO - List all of the requests to filter.
		{ IRP_MJ_CREATE,
		  0,
		  IRPCollectionPreOperation,
		  IRPCollectionPostOperation },

		{ IRP_MJ_CREATE_NAMED_PIPE,
		  0,
		  IRPCollectionPreOperation,
		  IRPCollectionPostOperation },

		{ IRP_MJ_CLOSE,
		  0,
		  IRPCollectionPreOperation,
		  IRPCollectionPostOperation },

		{ IRP_MJ_READ,
		  0,
		  IRPCollectionPreOperation,
		  IRPCollectionPostOperation },

		{ IRP_MJ_WRITE,
		  0,
		  IRPCollectionPreOperation,
		  IRPCollectionPostOperation },

		  { IRP_MJ_QUERY_INFORMATION,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_SET_INFORMATION,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_QUERY_EA,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_SET_EA,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_FLUSH_BUFFERS,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_QUERY_VOLUME_INFORMATION,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_SET_VOLUME_INFORMATION,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_DIRECTORY_CONTROL,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_FILE_SYSTEM_CONTROL,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_DEVICE_CONTROL,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_INTERNAL_DEVICE_CONTROL,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_SHUTDOWN,
			0,
			IRPCollectionPreOperationNoPostOperation,
			NULL },                               //post operations not supported

		  { IRP_MJ_LOCK_CONTROL,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_CLEANUP,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_CREATE_MAILSLOT,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_QUERY_SECURITY,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_SET_SECURITY,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_QUERY_QUOTA,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_SET_QUOTA,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_PNP,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_RELEASE_FOR_MOD_WRITE,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_RELEASE_FOR_CC_FLUSH,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_NETWORK_QUERY_OPEN,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_MDL_READ,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_MDL_READ_COMPLETE,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_PREPARE_MDL_WRITE,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_MDL_WRITE_COMPLETE,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_VOLUME_MOUNT,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

		  { IRP_MJ_VOLUME_DISMOUNT,
			0,
			IRPCollectionPreOperation,
			IRPCollectionPostOperation },

			//#endif // TODO

				{ IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags

	NULL,                               //  Context
	Callbacks,                          //  Operation callbacks

	IRPCollectionUnload,                           //  MiniFilterUnload

	IRPCollectionInstanceSetup,                    //  InstanceSetup
	IRPCollectionInstanceQueryTeardown,            //  InstanceQueryTeardown
	IRPCollectionInstanceTeardownStart,            //  InstanceTeardownStart
	IRPCollectionInstanceTeardownComplete,         //  InstanceTeardownComplete

	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent

};



NTSTATUS
IRPCollectionInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
/*++

Routine Description:

	This routine is called whenever a new instance is created on a volume. This
	gives us a chance to decide if we need to attach to this volume or not.

	If this routine is not defined in the registration structure, automatic
	instances are always created.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Flags describing the reason for this attach request.

Return Value:

	STATUS_SUCCESS - attach
	STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionInstanceSetup: Entered\n"));

	return STATUS_SUCCESS;
}


NTSTATUS
IRPCollectionInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

	This is called when an instance is being manually deleted by a
	call to FltDetachVolume or FilterDetach thereby giving us a
	chance to fail that detach request.

	If this routine is not defined in the registration structure, explicit
	detach requests via FltDetachVolume or FilterDetach will always be
	failed.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Indicating where this detach request came from.

Return Value:

	Returns the status of this operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionInstanceQueryTeardown: Entered\n"));

	return STATUS_SUCCESS;
}


VOID
IRPCollectionInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

	This routine is called at the start of instance teardown.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Reason why this instance is being deleted.

Return Value:

	None.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionInstanceTeardownStart: Entered\n"));
}


VOID
IRPCollectionInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

	This routine is called at the end of instance teardown.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Reason why this instance is being deleted.

Return Value:

	None.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionInstanceTeardownComplete: Entered\n"));
}


/*************************************************************************
	MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

	This is the initialization routine for this miniFilter driver.  This
	registers with FltMgr and initializes all global data structures.

Arguments:

	DriverObject - Pointer to driver object created by the system to
		represent this driver.

	RegistryPath - Unicode string identifying where the parameters for this
		driver are located in the registry.

Return Value:

	Routine can return non success error codes.

--*/
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(RegistryPath);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!DriverEntry: Entered\n"));

	//
	//  Register with FltMgr to tell it our callback routines
	//

	status = FltRegisterFilter(DriverObject,
		&FilterRegistration,
		&gFilterHandle);

	FLT_ASSERT(NT_SUCCESS(status));

	if (NT_SUCCESS(status)) {

		//
		//  Start filtering i/o
		//

		status = FltStartFiltering(gFilterHandle);

		if (!NT_SUCCESS(status)) {

			FltUnregisterFilter(gFilterHandle);
		}
	}

	return status;
}

NTSTATUS
IRPCollectionUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
/*++

Routine Description:

	This is the unload routine for this miniFilter driver. This is called
	when the minifilter is about to be unloaded. We can fail this unload
	request if this is not a mandatory unload indicated by the Flags
	parameter.

Arguments:

	Flags - Indicating if this is a mandatory unload.

Return Value:

	Returns STATUS_SUCCESS.

--*/
{
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionUnload: Entered\n"));

	FltUnregisterFilter(gFilterHandle);

	return STATUS_SUCCESS;
}


/*************************************************************************
	MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
IRPCollectionPreOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
/*++

Routine Description:

	This routine is a pre-operation dispatch routine for this miniFilter.

	This is non-pageable because it could be called on the paging path

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The context for the completion routine for this
		operation.

Return Value:

	The return value is the status of the operation.

--*/
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionPreOperation: Entered\n"));

	//
	//  See if this is an operation we would like the operation status
	//  for.  If so request it.
	//
	//  NOTE: most filters do NOT need to do this.  You only need to make
	//        this call if, for example, you need to know if the oplock was
	//        actually granted.
	//

	if (IRPCollectionDoRequestOperationStatus(Data)) {

		status = FltRequestOperationStatusCallback(Data,
			IRPCollectionOperationStatusCallback,
			(PVOID)(++OperationStatusCtx));
		if (!NT_SUCCESS(status)) {

			PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
				("IRPCollection!IRPCollectionPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
					status));
		}
	}

	// This template code does not do anything with the callbackData, but
	// rather returns FLT_PREOP_SUCCESS_WITH_CALLBACK.
	// This passes the request down to the next miniFilter in the chain.

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}



VOID
IRPCollectionOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
)
/*++

Routine Description:

	This routine is called when the given operation returns from the call
	to IoCallDriver.  This is useful for operations where STATUS_PENDING
	means the operation was successfully queued.  This is useful for OpLocks
	and directory change notification operations.

	This callback is called in the context of the originating thread and will
	never be called at DPC level.  The file object has been correctly
	referenced so that you can access it.  It will be automatically
	dereferenced upon return.

	This is non-pageable because it could be called on the paging path

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	RequesterContext - The context for the completion routine for this
		operation.

	OperationStatus -

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionOperationStatusCallback: Entered\n"));

	PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
		("IRPCollection!IRPCollectionOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
			OperationStatus,
			RequesterContext,
			ParameterSnapshot->MajorFunction,
			ParameterSnapshot->MinorFunction,
			FltGetIrpName(ParameterSnapshot->MajorFunction)));
}


FLT_POSTOP_CALLBACK_STATUS
IRPCollectionPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
/*++

Routine Description:

	This routine is the post-operation completion routine for this
	miniFilter.

	This is non-pageable because it may be called at DPC level.

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The completion context set in the pre-operation routine.

	Flags - Denotes whether the completion is successful or is being drained.

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionPostOperation: Entered\n"));

	ProcessIRP(Data, FltObjects);

	return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
IRPCollectionPreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
/*++

Routine Description:

	This routine is a pre-operation dispatch routine for this miniFilter.

	This is non-pageable because it could be called on the paging path

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The context for the completion routine for this
		operation.

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("IRPCollection!IRPCollectionPreOperationNoPostOperation: Entered\n"));

	// This template code does not do anything with the callbackData, but
	// rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
	// This passes the request down to the next miniFilter in the chain.
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
IRPCollectionDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
)
/*++

Routine Description:

	This identifies those operations we want the operation status for.  These
	are typically operations that return STATUS_PENDING as a normal completion
	status.

Arguments:

Return Value:

	TRUE - If we want the operation status
	FALSE - If we don't

--*/
{
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

	//
	//  return boolean state based on which operations we are interested in
	//

	return (BOOLEAN)

		//
		//  Check for oplock operations
		//

		(((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
			((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
				(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK) ||
				(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
				(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

			||

			//
			//    Check for directy change notification
			//

			((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
				(iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
			);
}


size_t GetFileName(wchar_t* fileName, PFLT_CALLBACK_DATA data) {
	PFLT_FILE_NAME_INFORMATION nameInfo;
	NTSTATUS status;
	size_t size = 0;
	if (data->Iopb->TargetFileObject == NULL || data->Iopb->TargetInstance == NULL)
	{
		fileName[0] = '\0\0';
		return 0;
	}
	//DbgPrint("[hello] before\n");
	status = FltGetFileNameInformation(data,
		FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
		//FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
		&nameInfo);
	//DbgPrint("[hello] after\n");
	if (!NT_SUCCESS(status) || nameInfo == NULL) {
		return 0;
	}
	/*status = FltParseFileNameInformation(nameInfo);
	if (!NT_SUCCESS(status) || nameInfo == NULL) {
		return 0;
	}*/

	UNICODE_STRING usName = nameInfo->Name;
	size = usName.Length / sizeof(wchar_t);
	wchar_t* src = usName.Buffer;
	for (int i = 0; i < size; i++) {
		fileName[i] = *(src + i);
	}
	fileName[size] = '\0\0';
	//FltReleaseFileNameInformation(nameInfo);
	return size;
}

VOID WriteToLog(PCFLT_RELATED_OBJECTS FltObjects, char* Buffer, UINT64 BufferSize) {

	UNICODE_STRING     OutPathW;
	OBJECT_ATTRIBUTES  objAttr;

	RtlInitUnicodeString(&OutPathW, L"\\DosDevices\\C:\\WINDOWS\\log.txt");

	InitializeObjectAttributes(&objAttr, &OutPathW,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL, NULL);



	//// Do not try to perform any file operations at higher IRQL levels.
	//// Instead, you may use a work item or a system worker thread to perform file operations.

	if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
			("IRQL != PASSIVE_LEVEL\n"));
		return;
	}

	HANDLE   fileHandle;
	NTSTATUS status;
	IO_STATUS_BLOCK    ioStatusBlock;

	status = FltCreateFile(
		FltObjects->Filter,
		FltObjects->Instance,
		&fileHandle,
		FILE_APPEND_DATA | SYNCHRONIZE,
		&objAttr, &ioStatusBlock, NULL,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL, 0, 0);

	if (status == STATUS_OBJECT_NAME_COLLISION ||
		status == STATUS_DELETE_PENDING) {
		return;
	}
	if (!NT_SUCCESS(status)) {
		return;
	}

	PFILE_OBJECT fileObject = NULL;
	status = ObReferenceObjectByHandle(fileHandle, 0, NULL, KernelMode,
		&fileObject,
		NULL);
	if (!NT_SUCCESS(status)) {
		goto End;
	}

	LARGE_INTEGER ByteOffset;
	ByteOffset.HighPart = -1;
	ByteOffset.LowPart = FILE_WRITE_TO_END_OF_FILE;
	status = FltWriteFile(FltObjects->Instance, fileObject, &ByteOffset, (ULONG)BufferSize,
		Buffer, 0, NULL, NULL, NULL);
	if (!NT_SUCCESS(status)) {
		goto End;
	}
	
End:
	if (fileObject) {
		ObDereferenceObject(fileObject);
	}
	if (fileHandle) {
		FltClose(fileHandle);
	}
}

void InitializeZwQueryInformationProcess() {
	if (NULL == ZwQueryInformationProcess) {
		UNICODE_STRING routineName;
		RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");
		ZwQueryInformationProcess =
			(QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&routineName);
		if (NULL == ZwQueryInformationProcess) {
			DbgPrint("Cannot resolve ZwQueryInformationProcess\n");
		}
	}
}

NTSTATUS GetProcessImageName(wchar_t* buff)
{
	NTSTATUS status;

	wchar_t procNameBuf_[__HELPER_MAX_STRING_LENGTH] = { 0 };
	UNICODE_STRING procNameStr;
	RtlInitUnicodeString(&procNameStr, procNameBuf_);

	InitializeZwQueryInformationProcess();
	status = ZwQueryInformationProcess(NtCurrentProcess(),
		ProcessImageFileName,
		&procNameStr,
		__HELPER_MAX_STRING_LENGTH,
		NULL);

	int i = 0;
	for (i = 0; i < procNameStr.Length / sizeof(wchar_t); i++) {
		buff[i] = procNameStr.Buffer[i];
	}

	return status;
}

NTSTATUS GetProcessBasicInfo(PROCESS_BASIC_INFORMATION* info) {
	InitializeZwQueryInformationProcess();
	return ZwQueryInformationProcess(NtCurrentProcess(), ProcessBasicInformation, info, sizeof(PROCESS_BASIC_INFORMATION), NULL);
}

__HELPER_F64 CalculateEntropyBasedOnBit(void* buffer, UINT64 size) {
	//if (buffer == NULL) {
	//	DbgPrint("%lld\n", size);
	//}
	//return 0.0;

	UINT64 one_bit_count = 0;
	UINT64 zero_bit_count = 0;
	UINT8* p = (UINT8*)buffer;
	for (UINT64 i = 0; i < size; i++) {
		UINT64 tmp = VALUE_TO_1_BIT_COUNT[p[i]];
		one_bit_count += tmp;
		zero_bit_count += 8 - tmp;
	}
	__HELPER_F64 p_0 = ((__HELPER_F64)zero_bit_count) / (8 * size);
	__HELPER_F64 p_1 = ((__HELPER_F64)one_bit_count) / (8 * size);
	if (p_0 == 0 || p_1 == 0) {
		return 0.0;
	}
	__HELPER_F64 entropy = -(p_0 * __helper_log2(p_0) + p_1 * __helper_log2(p_1));
	return entropy;
}

__HELPER_F64 CalculateEntropyBasedOnByte(void* buffer, UINT64 size) {
	//if (buffer == NULL) {
	//	DbgPrint("%lld\n", size);
	//}
	//return 0.0;
	UINT64 count[256] = { 0 };
	UINT8* p = (UINT8*)buffer;
	for (UINT64 i = 0; i < size; i++) {
		count[p[i]] += 1;
	}
	__HELPER_F64 entropy = 0.0;
	for (int i = 0; i < 256; i++) {
		if (count[i] == 0) {
			continue;
		}
		__HELPER_F64 probability = ((__HELPER_F64)count[i]) / ((__HELPER_F64)size);
		entropy += -(probability * __helper_log2(probability));
	}
	return entropy;
}

void ProcessIRP(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects) {
	if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
			("IRQL != PASSIVE_LEVEL\n"));
		return;
	}
	if (Data->RequestorMode == KernelMode)
		return;
	if (IoGetTopLevelIrp() != NULL) {
		return;
	}

	NTSTATUS st;

	// get file name
	wchar_t fileNameBuf[__HELPER_MAX_STRING_LENGTH];
	UINT64 fileNameSize = 0;
	//fileNameBuf[0] = '1';
	fileNameSize = GetFileName(fileNameBuf, Data);
	if (fileNameSize == 0) {
		return;
	}
	
	// get proc name
	wchar_t procNameBuf[__HELPER_MAX_STRING_LENGTH] = { 0 };
	/*procNameStr.Buffer = procNameBuf_;
	procNameStr.Length = 0;
	procNameStr.MaximumLength = sizeof(procNameBuf_);*/
	st = GetProcessImageName(procNameBuf);
	if (!NT_SUCCESS(st)) {
		DbgPrint("[hello fail]: Failed to get proc name: %#010x\n", st);
		return;
	}
	UINT64 procNameSize = wcslen(procNameBuf);
	if (procNameSize == 0) {
		return;
	}
	if (__helper_is_blacklist(procNameBuf, procNameSize)) {
		//DbgPrint("[hello error] %ws is in blacklist\n", procNameBuf);
		return;
	}
	else {
		//DbgPrint("[hello ok] %ws is not in blacklist", procNameBuf);
	}


	//DbgPrint("[hello]: %ls %lld\n", procNameBuf, procNameSize);

	// get system time
	UINT64 time = getCurrentTime();

	// get pid, parentid
	UINT64 pid, parentId;
	PROCESS_BASIC_INFORMATION procInfo;
	st = GetProcessBasicInfo(&procInfo);
	if (!NT_SUCCESS(st)) {
		DbgPrint("[hello fail]: Failed to get proc name: %#010x\n", st);
		return;
	}
	pid = procInfo.UniqueProcessId;
	parentId = procInfo.InheritedFromUniqueProcessId;
	
	// get tid
	UINT64 tid = (UINT64)PsGetCurrentThreadId();
	
	//DbgPrint("[hello] %lld %lld %lld\n", pid, tid, parentId);

	UINT64 bufferLength = 0;
	__HELPER_F64 entropy_bit_based = 0.0;
	__HELPER_F64 entropy_byte_based = 0.0;

	LARGE_INTEGER file_size;
	file_size.QuadPart = 0;

	LARGE_INTEGER offset;
	offset.QuadPart = 0;

	switch (Data->Iopb->MajorFunction)
	{
		case IRP_MJ_WRITE:
		{
			PVOID buffer = NULL;
			if (Data->Iopb->Parameters.Read.MdlAddress != NULL) { //there's mdl buffer, we use it
				buffer = MmGetSystemAddressForMdlSafe(Data->Iopb->Parameters.Read.MdlAddress,
					NormalPagePriority | MdlMappingNoExecute);
			}
			else {
				buffer = Data->Iopb->Parameters.Write.WriteBuffer;
			}
			if (buffer == NULL || Data->Iopb->Parameters.Write.Length == 0) {
				break;
			}
			bufferLength = Data->Iopb->Parameters.Write.Length;
			entropy_bit_based = CalculateEntropyBasedOnBit(buffer, bufferLength);
			entropy_byte_based = CalculateEntropyBasedOnByte(buffer, bufferLength);
			offset = Data->Iopb->Parameters.Write.ByteOffset;
			// get file size
			if (Data->Iopb->TargetFileObject != NULL) {
				FsRtlGetFileSize(Data->Iopb->TargetFileObject, &file_size);
			}
			break;
		}
		case IRP_MJ_READ:
		{
			PVOID buffer = NULL;
			if (Data->Iopb->Parameters.Read.MdlAddress != NULL) { //there's mdl buffer, we use it
				buffer = MmGetSystemAddressForMdlSafe(Data->Iopb->Parameters.Read.MdlAddress,
					NormalPagePriority | MdlMappingNoExecute);
			}
			else {
				buffer = Data->Iopb->Parameters.Read.ReadBuffer;
			}
			if (buffer == NULL || Data->Iopb->Parameters.Read.Length == 0) {
				break;
			}
			bufferLength = Data->Iopb->Parameters.Read.Length;
			entropy_bit_based = CalculateEntropyBasedOnBit(buffer, bufferLength);
			entropy_byte_based = CalculateEntropyBasedOnByte(buffer, bufferLength);
			offset = Data->Iopb->Parameters.Read.ByteOffset;
			// get file size
			if (Data->Iopb->TargetFileObject != NULL) {
				FsRtlGetFileSize(Data->Iopb->TargetFileObject, &file_size);
			}
			break;
		}
		default:
			break;
	}

	//DbgPrint("[hello]: %llu %d %d %lld %ls %llu %llu %llu %llu %s %ls\n", time, Data->Iopb->MajorFunction,
	//	Data->Iopb->MinorFunction, procNameSize, procNameBuf,
	//	pid, tid, parentId,
	//	bufferLength, entropyBuf, fileNameBuf);

	UINT8 is_rename = isTheIrpRenameFile(Data);
	UINT8 is_delete = isTheIrpDeleteFile(Data);


	UINT8 needFlush = __helper_add_record(time, Data->Iopb->MajorFunction,
		Data->Iopb->MinorFunction, procNameBuf, procNameSize,
		pid, tid, parentId,
		bufferLength, entropy_bit_based, entropy_byte_based, fileNameBuf, fileNameSize, is_rename, is_delete, file_size.QuadPart, offset.QuadPart);
	if (needFlush) {
		__helper_log_file_lock();
		DbgPrint("[hello]: time to flush\n");
		char* logBuf = NULL;
		UINT64 size = 0;
		UINT8 copiedFlag = __helper_flush_and_copy_buf(&logBuf, &size);
		if (copiedFlag == 1) {
			WriteToLog(FltObjects, logBuf, size);
			DbgPrint("[hello]: flush is ok: %lld\n", size);
		}
		else if (copiedFlag == 255) {
			DbgPrint("[hello]: flush fails: copy flag 255 %lld\n", size);
		}
		__helper_log_file_unlock();
	}
}

// ref: https://stackoverflow.com/questions/61858298/minifilter-missing-deleted-files-in-particular-scenarios-windows-10-1903
UINT8 isTheIrpDeleteFile(PFLT_CALLBACK_DATA Data) {
	if (Data->Iopb->MajorFunction == IRP_MJ_CREATE) {
		if (Data->Iopb->Parameters.Create.Options & (FILE_DELETE_ON_CLOSE)) {
			return 1;
		}
	}
	else if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION) {
		switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass) {
		case FileDispositionInformationEx:
		case FileDispositionInformation:
			// deleting a file
			if (((FILE_DISPOSITION_INFORMATION*)Data->Iopb->Parameters.SetFileInformation.InfoBuffer)->DeleteFile) {
				return 1;
			}
			break;
		}
	}
	return 0;
}

UINT64 getCurrentTime() {
	UINT64 time = 0;
	LARGE_INTEGER timeLarge;
	KeQuerySystemTimePrecise(&timeLarge);
	time = timeLarge.QuadPart;
	return time;
}

UINT8 isTheIrpRenameFile(PFLT_CALLBACK_DATA Data) {
	if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION) {
		switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass) {
		case FileRenameInformationEx:
		case FileRenameInformation:
			// rename a file
			return 1;
		}
	}
	return 0;
}

//UINT8 isTheIrpCreateFile(PFLT_CALLBACK_DATA Data) {
//	if (Data->Iopb->MajorFunction == IRP_MJ_CREATE) {
//		if (Data->Iopb->Parameters.Create.Options & (FILE_SUPERSEDE | FILE_CREATE | FILE_OPEN_IF | FILE_OVERWRITE_IF) != 0) {
//
//		}
//	}
//	return 0;
//}
