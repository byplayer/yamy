///////////////////////////////////////////////////////////////////////////////
// Driver for Madotsukai no Yu^utsu for Windows2000


#include <ntddk.h>
#include <ntddkbd.h>
#include <devioctl.h>

#pragma warning(3 : 4061 4100 4132 4701 4706)

#include "ioctl.h"
#include "keyque.c"

//#define USE_TOUCHPAD // very experimental!

#if DBG
// Enable debug logging only on checked build:
// We use macro to avoid function call overhead
// in non-logging case, and use double paren such
// as DEBUG_LOG((...)) because of va_list in macro.
#include "log.h"
#define DEBUG_LOG_INIT(x) mayuLogInit x
#define DEBUG_LOG_TERM(x) mayuLogTerm x
#define DEBUG_LOG(x) mayuLogEnque x
#define DEBUG_LOG_RETRIEVE(x) mayuLogDeque x
#else
#define DEBUG_LOG_INIT(x)
#define DEBUG_LOG_TERM(x)
#define DEBUG_LOG(x)
#define DEBUG_LOG_RETRIEVE(x) STATUS_INVALID_DEVICE_REQUEST
#endif

///////////////////////////////////////////////////////////////////////////////
// Device Extensions

struct _DetourDeviceExtension;
struct _FilterDeviceExtension;

typedef struct _DetourDeviceExtension {
	PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

	KSPIN_LOCK lock; // lock below datum
	PDEVICE_OBJECT filterDevObj;
	LONG isOpen;
	BOOLEAN wasCleanupInitiated; //
	PIRP irpq;
	KeyQue readQue; // when IRP_MJ_READ, the contents of readQue are returned
} DetourDeviceExtension;

typedef struct _FilterDeviceExtension {
	PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

	PDEVICE_OBJECT detourDevObj;
	PDEVICE_OBJECT kbdClassDevObj; // keyboard class device object

#ifdef USE_TOUCHPAD
	BOOLEAN isKeyboard;
#endif
	KSPIN_LOCK lock; // lock below datum
	PIRP irpq;
	KeyQue readQue; // when IRP_MJ_READ, the contents of readQue are returned
#ifdef USE_TOUCHPAD
	BOOLEAN isTouched;
#endif
} FilterDeviceExtension;

///////////////////////////////////////////////////////////////////////////////
// Protorypes (TODO)


NTSTATUS DriverEntry       (IN PDRIVER_OBJECT, IN PUNICODE_STRING);
NTSTATUS mayuAddDevice     (IN PDRIVER_OBJECT, IN PDEVICE_OBJECT);
VOID mayuUnloadDriver      (IN PDRIVER_OBJECT);
VOID mayuDetourReadCancel (IN PDEVICE_OBJECT, IN PIRP);

NTSTATUS filterGenericCompletion (IN PDEVICE_OBJECT, IN PIRP, IN PVOID);
NTSTATUS filterReadCompletion    (IN PDEVICE_OBJECT, IN PIRP, IN PVOID);

NTSTATUS mayuGenericDispatch (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS detourCreate        (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS detourClose         (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS detourRead          (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS detourWrite         (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS detourCleanup       (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS detourDeviceControl (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS filterRead          (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS filterPassThrough   (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS detourPnP           (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS filterPnP           (IN PDEVICE_OBJECT, IN PIRP);
#ifndef MAYUD_NT4
NTSTATUS detourPower         (IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS filterPower         (IN PDEVICE_OBJECT, IN PIRP);
#endif // !MAYUD_NT4

BOOLEAN CancelKeyboardClassRead(IN PIRP, IN PDEVICE_OBJECT);
NTSTATUS readq(KeyQue*, PIRP);

#ifdef USE_TOUCHPAD
NTSTATUS filterTouchpadCompletion (IN PDEVICE_OBJECT, IN PIRP, IN PVOID);
NTSTATUS filterTouchpad      (IN PDEVICE_OBJECT, IN PIRP);
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( init, DriverEntry )
#endif // ALLOC_PRAGMA


///////////////////////////////////////////////////////////////////////////////
// Global Constants / Variables

BOOLEAN g_isPnP;
BOOLEAN g_isXp;
ULONG g_SpinLock_offset;
ULONG g_RequestIsPending_offset;

// Device names
#define UnicodeString(str) { sizeof(str) - sizeof(UNICODE_NULL),	\
			     sizeof(str) - sizeof(UNICODE_NULL), str }

static UNICODE_STRING MayuDetourDeviceName =
	UnicodeString(L"\\Device\\MayuDetour0");

static UNICODE_STRING MayuDetourWin32DeviceName =
	UnicodeString(L"\\DosDevices\\MayuDetour1");

static UNICODE_STRING KeyboardClassDeviceName =
	UnicodeString(DD_KEYBOARD_DEVICE_NAME_U L"0");

static UNICODE_STRING KeyboardClassDriverName =
	UnicodeString(L"\\Driver\\kbdclass");

#ifdef USE_TOUCHPAD
#define TOUCHPAD_PRESSURE_OFFSET 7
#endif

// Global Variables
PDRIVER_DISPATCH _IopInvalidDeviceRequest; // Default dispatch function


#define MAYUD_MODE L""
static UNICODE_STRING MayuDriverVersion =
	UnicodeString(L"$Revision: 1.27 $" MAYUD_MODE);


///////////////////////////////////////////////////////////////////////////////
// Entry / Unload

void DEBUG_LOGChain(PDRIVER_OBJECT driverObject)
{
	PDEVICE_OBJECT deviceObject = driverObject->DeviceObject;

	if (deviceObject) {
		while (deviceObject->NextDevice) {
			DEBUG_LOG(("%x->", deviceObject));
			deviceObject = deviceObject->NextDevice;
		}
		DEBUG_LOG(("%x", deviceObject));
	}
	return;
}

// initialize driver
NTSTATUS DriverEntry(IN PDRIVER_OBJECT driverObject,
					 IN PUNICODE_STRING registryPath)
{
	NTSTATUS status;
	BOOLEAN is_symbolicLinkCreated = FALSE;
	ULONG i;
	PDEVICE_OBJECT detourDevObj = NULL;
	DetourDeviceExtension *detourDevExt = NULL;
	ULONG start = 0;
	RTL_QUERY_REGISTRY_TABLE query[2];

	UNREFERENCED_PARAMETER(registryPath);

	DEBUG_LOG_INIT(("mayud: start logging"));

	// Environment specific initialize
	RtlZeroMemory(query, sizeof(query));
	query[0].Name = L"Start";
	query[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
	query[0].EntryContext = &start;
	RtlQueryRegistryValues(RTL_REGISTRY_SERVICES, L"mayud", query, NULL, NULL);
	if (start == 0x03) {
		g_isPnP = TRUE;
		DEBUG_LOG(("is PnP"));
	} else {
		g_isPnP = FALSE;
		DEBUG_LOG(("is not PnP"));
	}
#ifdef MAYUD_NT4
	g_isXp = FALSE;
	g_SpinLock_offset = 48;
	g_RequestIsPending_offset = 0;
#else /* !MAYUD_NT4 */
	if (IoIsWdmVersionAvailable(1, 0x20)) { // is WindowsXP
		DEBUG_LOG(("is WindowsXp"));
		g_isXp = TRUE;
		g_SpinLock_offset = 108;
		g_RequestIsPending_offset = 0;
	} else if (IoIsWdmVersionAvailable(1, 0x10)) { // is Windows2000
		DEBUG_LOG(("is Windows2000"));
		g_isXp =FALSE;
		g_SpinLock_offset = 116;
		g_RequestIsPending_offset = 48;
	} else { // Unknown version
		DEBUG_LOG(("unknown Windows"));
		status = STATUS_UNKNOWN_REVISION;
		goto error;
	}
#endif /* MAYUD_NT4 */

	// initialize global variables
	_IopInvalidDeviceRequest = driverObject->MajorFunction[IRP_MJ_CREATE];

	// set major functions
	driverObject->DriverUnload = mayuUnloadDriver;
	if (g_isPnP == TRUE) {
		driverObject->DriverExtension->AddDevice = mayuAddDevice;
	}
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
#ifdef MAYUD_NT4
		if (i != IRP_MJ_POWER)
#endif // MAYUD_NT4
			driverObject->MajorFunction[i] = mayuGenericDispatch;
	if (g_isPnP == TRUE) {
		driverObject->MajorFunction[IRP_MJ_PNP] = mayuGenericDispatch;
	}

	// create a device
	{
		// create detour device
		status = IoCreateDevice(driverObject, sizeof(DetourDeviceExtension),
								&MayuDetourDeviceName, FILE_DEVICE_KEYBOARD,
								0, FALSE, &detourDevObj);

		if (!NT_SUCCESS(status)) goto error;
		DEBUG_LOG(("create detour device: %x", detourDevObj));
		DEBUG_LOGChain(driverObject);
		detourDevObj->Flags |= DO_BUFFERED_IO;
#ifndef MAYUD_NT4
		detourDevObj->Flags |= DO_POWER_PAGABLE;
#endif // !MAYUD_NT4

		// initialize detour device extension
		detourDevExt = (DetourDeviceExtension*)detourDevObj->DeviceExtension;
		RtlZeroMemory(detourDevExt, sizeof(DetourDeviceExtension));
		detourDevExt->filterDevObj = NULL;

		KeInitializeSpinLock(&detourDevExt->lock);
		detourDevExt->isOpen = FALSE;
		detourDevExt->wasCleanupInitiated = FALSE;
		detourDevExt->irpq = NULL;
		status = KqInitialize(&detourDevExt->readQue);
		if (!NT_SUCCESS(status)) goto error;

		// create symbolic link for detour
		status =
			IoCreateSymbolicLink(&MayuDetourWin32DeviceName, &MayuDetourDeviceName);
		if (!NT_SUCCESS(status)) goto error;
		is_symbolicLinkCreated = TRUE;

		if (g_isPnP == FALSE)
			// attach filter device to keyboard class device
		{
			PFILE_OBJECT f;
			PDEVICE_OBJECT kbdClassDevObj;

			status = IoGetDeviceObjectPointer(&KeyboardClassDeviceName,
											  FILE_ALL_ACCESS, &f,
											  &kbdClassDevObj);
			if (!NT_SUCCESS(status)) goto error;
			ObDereferenceObject(f);
			status = mayuAddDevice(driverObject, kbdClassDevObj);

			// why cannot I do below ?
//      status = IoAttachDevice(filterDevObj, &KeyboardClassDeviceName,
//			      &filterDevExt->kbdClassDevObj);
			if (!NT_SUCCESS(status)) goto error;
		}

		// initialize Major Functions
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
			detourDevExt->MajorFunction[i] = _IopInvalidDeviceRequest;
		}

		detourDevExt->MajorFunction[IRP_MJ_READ] = detourRead;
		detourDevExt->MajorFunction[IRP_MJ_WRITE] = detourWrite;
		detourDevExt->MajorFunction[IRP_MJ_CREATE] = detourCreate;
		detourDevExt->MajorFunction[IRP_MJ_CLOSE] = detourClose;
		detourDevExt->MajorFunction[IRP_MJ_CLEANUP] = detourCleanup;
		detourDevExt->MajorFunction[IRP_MJ_DEVICE_CONTROL] = detourDeviceControl;

#ifndef MAYUD_NT4
		detourDevExt->MajorFunction[IRP_MJ_POWER] = detourPower;
#endif // !MAYUD_NT4
		if (g_isPnP == TRUE) {
			detourDevExt->MajorFunction[IRP_MJ_PNP] = detourPnP;
		}
	}
	detourDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;

error: {
		if (is_symbolicLinkCreated)
			IoDeleteSymbolicLink(&MayuDetourWin32DeviceName);
		if (detourDevObj) {
			KqFinalize(&detourDevExt->readQue);
			IoDeleteDevice(detourDevObj);
		}
	}
	return status;
}

NTSTATUS mayuAddDevice(IN PDRIVER_OBJECT driverObject,
					   IN PDEVICE_OBJECT kbdClassDevObj)
{
	NTSTATUS status;
	PDEVICE_OBJECT devObj;
	PDEVICE_OBJECT filterDevObj;
	PDEVICE_OBJECT attachedDevObj;
	DetourDeviceExtension *detourDevExt;
	FilterDeviceExtension *filterDevExt;
	ULONG i;

	DEBUG_LOG(("attach to device: %x", kbdClassDevObj));
	DEBUG_LOG(("type of device: %x", kbdClassDevObj->DeviceType));
	DEBUG_LOG(("name of driver: %T", &(kbdClassDevObj->DriverObject->DriverName)));

	// create filter device
	status = IoCreateDevice(driverObject, sizeof(FilterDeviceExtension),
							NULL, FILE_DEVICE_KEYBOARD,
							0, FALSE, &filterDevObj);
	DEBUG_LOG(("add filter device: %x", filterDevObj));
	DEBUG_LOGChain(driverObject);
	if (!NT_SUCCESS(status)) return status;
	filterDevObj->Flags |= DO_BUFFERED_IO;
#ifndef MAYUD_NT4
	filterDevObj->Flags |= DO_POWER_PAGABLE;
#endif // !MAYUD_NT4

	// initialize filter device extension
	filterDevExt = (FilterDeviceExtension*)filterDevObj->DeviceExtension;
	RtlZeroMemory(filterDevExt, sizeof(FilterDeviceExtension));

	KeInitializeSpinLock(&filterDevExt->lock);
	filterDevExt->irpq = NULL;
	status = KqInitialize(&filterDevExt->readQue);
	if (!NT_SUCCESS(status)) goto error;
#ifdef USE_TOUCHPAD
	filterDevExt->isKeyboard = FALSE;
	filterDevExt->isTouched = FALSE;
#endif

	attachedDevObj = kbdClassDevObj->AttachedDevice;
	while (attachedDevObj) {
		DEBUG_LOG(("attached to %T", &(attachedDevObj->DriverObject->DriverName)));
		DEBUG_LOG(("type of attched device: %x", attachedDevObj->DeviceType));
#ifdef USE_TOUCHPAD
		if (RtlCompareUnicodeString(&KeyboardClassDriverName, &attachedDevObj->DriverObject->DriverName, TRUE) == 0)
			filterDevExt->isKeyboard = TRUE;
#endif
		attachedDevObj = attachedDevObj->AttachedDevice;
	}

	devObj = filterDevObj->NextDevice;
	while (devObj->NextDevice) {
		devObj = devObj->NextDevice;
	}
	filterDevExt->detourDevObj = devObj;
	detourDevExt = (DetourDeviceExtension*)devObj->DeviceExtension;
	if (!detourDevExt->filterDevObj) {
		detourDevExt->filterDevObj = filterDevObj;
	}

	filterDevExt->kbdClassDevObj =
		IoAttachDeviceToDeviceStack(filterDevObj, kbdClassDevObj);
	if (!filterDevExt->kbdClassDevObj) goto error;

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		filterDevExt->MajorFunction[i] =
			(filterDevExt->kbdClassDevObj->DriverObject->MajorFunction[i]
			 == _IopInvalidDeviceRequest) ?
			_IopInvalidDeviceRequest : filterPassThrough;
	}
#ifdef USE_TOUCHPAD
	if (filterDevExt->isKeyboard == FALSE) {
		DEBUG_LOG(("filter read: GlidePoint"));
		filterDevObj->DeviceType = FILE_DEVICE_MOUSE;
		filterDevExt->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = filterTouchpad;
	} else
#endif
	{
		DEBUG_LOG(("filter read: Keyboard"));
		filterDevExt->MajorFunction[IRP_MJ_READ] = filterRead;
	}
#ifndef MAYUD_NT4
	filterDevExt->MajorFunction[IRP_MJ_POWER] = filterPower;
#endif // !MAYUD_NT4
	if (g_isPnP == TRUE) {
		filterDevExt->MajorFunction[IRP_MJ_PNP] = filterPnP;
	}
	filterDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;

error:
	DEBUG_LOG(("mayuAddDevice: error"));
	if (filterDevObj) {
		KqFinalize(&filterDevExt->readQue);
		IoDeleteDevice(filterDevObj);
	}
	return status;
}

BOOLEAN CancelKeyboardClassRead(PIRP cancelIrp, PDEVICE_OBJECT kbdClassDevObj)
{
	PVOID kbdClassDevExt;
	BOOLEAN isSafe;
	PKSPIN_LOCK SpinLock;
	KIRQL currentIrql;

	kbdClassDevExt = kbdClassDevObj->DeviceExtension;
	SpinLock = (PKSPIN_LOCK)((ULONG)kbdClassDevExt + g_SpinLock_offset);
	KeAcquireSpinLock(SpinLock, &currentIrql);
	if (g_isXp == TRUE) {
		isSafe = cancelIrp->CancelRoutine ? TRUE : FALSE;
	} else {
		isSafe = *(BOOLEAN*)((ULONG)kbdClassDevExt + g_RequestIsPending_offset);
	}
	if (isSafe == TRUE) {
		KeReleaseSpinLock(SpinLock, currentIrql);
		IoCancelIrp(cancelIrp);
	} else {
		DEBUG_LOG(("cancel irp not pending"));
		KeReleaseSpinLock(SpinLock, currentIrql);
	}
	return isSafe;
}

// unload driver
VOID mayuUnloadDriver(IN PDRIVER_OBJECT driverObject)
{
	KIRQL currentIrql;
	PIRP cancelIrp;
	PDEVICE_OBJECT devObj;
	DetourDeviceExtension *detourDevExt;

	// walk on device chain(the last one is detour device?)
	devObj = driverObject->DeviceObject;
	while (devObj->NextDevice) {
		FilterDeviceExtension *filterDevExt
		= (FilterDeviceExtension*)devObj->DeviceExtension;
		PDEVICE_OBJECT delObj;
		PDEVICE_OBJECT kbdClassDevObj;

		// detach
		IoDetachDevice(filterDevExt->kbdClassDevObj);
		// cancel filter IRP_MJ_READ
		KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);
		// TODO: at this point, the irp may be completed (but what can I do for it ?)
		// finalize read que
		KqFinalize(&filterDevExt->readQue);
		cancelIrp = filterDevExt->irpq;
		filterDevExt->irpq = NULL;
		kbdClassDevObj = filterDevExt->kbdClassDevObj;
		KeReleaseSpinLock(&filterDevExt->lock, currentIrql);
		if (cancelIrp) {
			while (CancelKeyboardClassRead(cancelIrp, kbdClassDevObj) != TRUE);
		}
		// delete device objects
		delObj= devObj;
		devObj = devObj->NextDevice;
		IoDeleteDevice(delObj);
	}

	detourDevExt = (DetourDeviceExtension*)devObj->DeviceExtension;
	// cancel filter IRP_MJ_READ
	KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
	// TODO: at this point, the irp may be completed (but what can I do for it ?)
	cancelIrp = detourDevExt->irpq;
	// finalize read que
	KqFinalize(&detourDevExt->readQue);
	KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
	if (cancelIrp)
		IoCancelIrp(cancelIrp);
	// delete device objects
	IoDeleteDevice(devObj);

	// delete symbolic link
	IoDeleteSymbolicLink(&MayuDetourWin32DeviceName);
	DEBUG_LOG_TERM(());
}


///////////////////////////////////////////////////////////////////////////////
// Cancel Functionss


// detour read cancel
VOID mayuDetourReadCancel(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	DetourDeviceExtension *devExt =
		(DetourDeviceExtension *)deviceObject->DeviceExtension;
	KIRQL currentIrql;

	KeAcquireSpinLock(&devExt->lock, &currentIrql);
	devExt->irpq = NULL;
	KeReleaseSpinLock(&devExt->lock, currentIrql);
	IoReleaseCancelSpinLock(irp->CancelIrql);
	DEBUG_LOG(("detourReadCancel:"));
#if 0
	KeAcquireSpinLock(&devExt->lock, &currentIrql);
	if (devExt->irpq && irp == deviceObject->CurrentIrp)
		// the current request is being cancelled
	{
		deviceObject->CurrentIrp = NULL;
		devExt->irpq = NULL;
		KeReleaseSpinLock(&devExt->lock, currentIrql);
		IoStartNextPacket(deviceObject, TRUE);
	} else {
		// Cancel a request in the device queue
		KIRQL cancelIrql;

		IoAcquireCancelSpinLock(&cancelIrql);
		KeRemoveEntryDeviceQueue(&deviceObject->DeviceQueue,
								 &irp->Tail.Overlay.DeviceQueueEntry);
		IoReleaseCancelSpinLock(cancelIrql);
		KeReleaseSpinLock(&devExt->lock, currentIrql);
	}
#endif

	irp->IoStatus.Status = STATUS_CANCELLED;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_KEYBOARD_INCREMENT);
}

///////////////////////////////////////////////////////////////////////////////
// Complete Functions


//
NTSTATUS filterGenericCompletion(IN PDEVICE_OBJECT deviceObject,
								 IN PIRP irp, IN PVOID context)
{
	UNREFERENCED_PARAMETER(deviceObject);
	UNREFERENCED_PARAMETER(context);

	if (irp->PendingReturned)
		IoMarkIrpPending(irp);
	return STATUS_SUCCESS;
}


//
NTSTATUS filterReadCompletion(IN PDEVICE_OBJECT deviceObject,
							  IN PIRP irp, IN PVOID context)
{
	NTSTATUS status;
	KIRQL currentIrql, cancelIrql;
	PIRP irpCancel = NULL;
	FilterDeviceExtension *filterDevExt =
		(FilterDeviceExtension*)deviceObject->DeviceExtension;
	PDEVICE_OBJECT detourDevObj = filterDevExt->detourDevObj;
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)detourDevObj->DeviceExtension;

	UNREFERENCED_PARAMETER(context);

	KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);
	filterDevExt->irpq = NULL;
	KeReleaseSpinLock(&filterDevExt->lock, currentIrql);
	if (irp->PendingReturned) {
		status = STATUS_PENDING;
		IoMarkIrpPending(irp);
	} else {
		status = STATUS_SUCCESS;
	}

	KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
	if (detourDevExt->isOpen && !detourDevExt->wasCleanupInitiated) {
		// if detour is opened, key datum are forwarded to detour
		if (irp->IoStatus.Status == STATUS_SUCCESS) {
			PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

			KqEnque(&detourDevExt->readQue,
					(KEYBOARD_INPUT_DATA *)irp->AssociatedIrp.SystemBuffer,
					irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA));

			irp->IoStatus.Status = STATUS_CANCELLED;
			irp->IoStatus.Information = 0;
			detourDevExt->filterDevObj = deviceObject;
		}

		IoAcquireCancelSpinLock(&cancelIrql);
		if (detourDevExt->irpq) {
			if (readq(&detourDevExt->readQue, detourDevExt->irpq) ==
					STATUS_SUCCESS) {
				IoSetCancelRoutine(detourDevExt->irpq, NULL);
				IoCompleteRequest(detourDevExt->irpq, IO_KEYBOARD_INCREMENT);
				detourDevExt->irpq = NULL;
			}
		}
		IoReleaseCancelSpinLock(cancelIrql);
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);

		KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);
		status = readq(&filterDevExt->readQue, irp);
		KeReleaseSpinLock(&filterDevExt->lock, currentIrql);
	} else {
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
	}

	if (status == STATUS_SUCCESS)
		irp->IoStatus.Status = STATUS_SUCCESS;
	return irp->IoStatus.Status;
}

NTSTATUS readq(KeyQue *readQue, PIRP irp)
{
	if (!KqIsEmpty(readQue)) {
		PIO_STACK_LOCATION irpSp;
		ULONG len;

		irpSp = IoGetCurrentIrpStackLocation(irp);
		len = KqDeque(readQue,
					  (KEYBOARD_INPUT_DATA *)irp->AssociatedIrp.SystemBuffer,
					  irpSp->Parameters.Read.Length / sizeof(KEYBOARD_INPUT_DATA));
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = len * sizeof(KEYBOARD_INPUT_DATA);
		irpSp->Parameters.Read.Length = irp->IoStatus.Information;
		return STATUS_SUCCESS;
	} else {
		irp->IoStatus.Status = STATUS_PENDING;
		irp->IoStatus.Information = 0;
		return STATUS_PENDING;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Dispatch Functions


// Generic Dispatcher
NTSTATUS mayuGenericDispatch(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

	if (deviceObject->NextDevice) {
		FilterDeviceExtension *filterDevExt =
			(FilterDeviceExtension *)deviceObject->DeviceExtension;

#ifdef USE_TOUCHPAD
		if (filterDevExt->isKeyboard == FALSE) {
			DEBUG_LOG(("MajorFunction: %x", irpSp->MajorFunction));
		}
#endif
		return filterDevExt->MajorFunction[irpSp->MajorFunction](deviceObject, irp);
	} else {
		DetourDeviceExtension *detourDevExt =
			(DetourDeviceExtension *)deviceObject->DeviceExtension;

		return detourDevExt->MajorFunction[irpSp->MajorFunction](deviceObject, irp);
	}
}


// detour IRP_MJ_CREATE
NTSTATUS detourCreate(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)deviceObject->DeviceExtension;

	if (1 < InterlockedIncrement(&detourDevExt->isOpen))
		// mayu detour device can be opend only once at a time
	{
		InterlockedDecrement(&detourDevExt->isOpen);
		irp->IoStatus.Status = STATUS_INTERNAL_ERROR;
	} else {
		PIRP irpCancel;
		KIRQL currentIrql;
		PDEVICE_OBJECT filterDevObj;

		KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
		detourDevExt->wasCleanupInitiated = FALSE;
		KqClear(&detourDevExt->readQue);
		filterDevObj = detourDevExt->filterDevObj;
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
		if (filterDevObj) {
			FilterDeviceExtension *filterDevExt =
				(FilterDeviceExtension*)filterDevObj->DeviceExtension;
			PDEVICE_OBJECT kbdClassDevObj;

			KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);
			irpCancel = filterDevExt->kbdClassDevObj->CurrentIrp;
			kbdClassDevObj = filterDevExt->kbdClassDevObj;
			KeReleaseSpinLock(&filterDevExt->lock, currentIrql);
			if (irpCancel) {
				CancelKeyboardClassRead(irpCancel, kbdClassDevObj);
			}
		}

		irp->IoStatus.Status = STATUS_SUCCESS;
	}
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}


// detour IRP_MJ_CLOSE
NTSTATUS detourClose(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)deviceObject->DeviceExtension;
	KIRQL currentIrql;

	KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
	InterlockedDecrement(&detourDevExt->isOpen);
	KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	DEBUG_LOG_TERM(());
	return STATUS_SUCCESS;
}


// detour IRP_MJ_READ
NTSTATUS detourRead(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)deviceObject->DeviceExtension;
	KIRQL currentIrql, cancelIrql;

	KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
	if (irpSp->Parameters.Read.Length == 0)
		status = STATUS_SUCCESS;
	else if (irpSp->Parameters.Read.Length % sizeof(KEYBOARD_INPUT_DATA))
		status = STATUS_BUFFER_TOO_SMALL;
	else
		status = readq(&detourDevExt->readQue, irp);
	if (status == STATUS_PENDING) {
		IoAcquireCancelSpinLock(&cancelIrql);
		IoMarkIrpPending(irp);
		detourDevExt->irpq = irp;
		IoSetCancelRoutine(irp, mayuDetourReadCancel);
		IoReleaseCancelSpinLock(cancelIrql);
	} else {
		IoCompleteRequest(irp, IO_KEYBOARD_INCREMENT);
	}
	KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
	return status;
}


// detour IRP_MJ_WRITE
NTSTATUS detourWrite(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	ULONG len = irpSp->Parameters.Write.Length;
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)deviceObject->DeviceExtension;

	irp->IoStatus.Information = 0;
	if (len == 0)
		status = STATUS_SUCCESS;
	else if (len % sizeof(KEYBOARD_INPUT_DATA))
		status = STATUS_INVALID_PARAMETER;
	else {
		// write to filter que
		KIRQL cancelIrql, currentIrql;
		PIRP irpCancel;
		PDEVICE_OBJECT filterDevObj;

		KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
		filterDevObj = detourDevExt->filterDevObj;
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
		// enque filter que
		if (filterDevObj) {
			FilterDeviceExtension *filterDevExt =
				(FilterDeviceExtension*)filterDevObj->DeviceExtension;
			PDEVICE_OBJECT kbdClassDevObj;

			KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);

			len /= sizeof(KEYBOARD_INPUT_DATA);
			len = KqEnque(&filterDevExt->readQue,
						  (KEYBOARD_INPUT_DATA *)irp->AssociatedIrp.SystemBuffer,
						  len);
			irp->IoStatus.Information = len * sizeof(KEYBOARD_INPUT_DATA);
			irpSp->Parameters.Write.Length = irp->IoStatus.Information;
			// cancel filter irp
			irpCancel = filterDevExt->irpq;
			filterDevExt->irpq = NULL;
			kbdClassDevObj = filterDevExt->kbdClassDevObj;
			KeReleaseSpinLock(&filterDevExt->lock, currentIrql);
			if (irpCancel) {
				CancelKeyboardClassRead(irpCancel, kbdClassDevObj);
			}
			status = STATUS_SUCCESS;
		} else {
			irp->IoStatus.Information = 0;
			irpSp->Parameters.Write.Length = irp->IoStatus.Information;
			status = STATUS_CANCELLED;
		}
	}
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


// detour IRP_MJ_CLEANUP
NTSTATUS detourCleanup(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	KIRQL currentIrql, cancelIrql;
	PIO_STACK_LOCATION irpSp;
	PIRP  currentIrp = NULL, irpCancel;
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)deviceObject->DeviceExtension;

	KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
	IoAcquireCancelSpinLock(&cancelIrql);
	irpSp = IoGetCurrentIrpStackLocation(irp);
	detourDevExt->wasCleanupInitiated = TRUE;

	// Complete all requests queued by this thread with STATUS_CANCELLED
	currentIrp = deviceObject->CurrentIrp;
	deviceObject->CurrentIrp = NULL;
	detourDevExt->irpq = NULL;

	while (currentIrp != NULL) {
		IoSetCancelRoutine(currentIrp, NULL);
		currentIrp->IoStatus.Status = STATUS_CANCELLED;
		currentIrp->IoStatus.Information = 0;

		IoReleaseCancelSpinLock(cancelIrql);
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
		IoCompleteRequest(currentIrp, IO_NO_INCREMENT);
		KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
		IoAcquireCancelSpinLock(&cancelIrql);

		// Dequeue the next packet (IRP) from the device work queue.
		{
			PKDEVICE_QUEUE_ENTRY packet =
				KeRemoveDeviceQueue(&deviceObject->DeviceQueue);
			currentIrp = packet ?
						 CONTAINING_RECORD(packet, IRP, Tail.Overlay.DeviceQueueEntry) : NULL;
		}
	}

	IoReleaseCancelSpinLock(cancelIrql);
	KeReleaseSpinLock(&detourDevExt->lock, currentIrql);

	// Complete the cleanup request with STATUS_SUCCESS.
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


// detour IRP_MJ_DEVICE_CONTROL
NTSTATUS detourDeviceControl(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)deviceObject->DeviceExtension;

	irp->IoStatus.Information = 0;
	if (irpSp->Parameters.DeviceIoControl.IoControlCode != IOCTL_MAYU_GET_LOG) DEBUG_LOG(("DeviceIoControl: %x", irpSp->Parameters.DeviceIoControl.IoControlCode));
	status = STATUS_INVALID_DEVICE_REQUEST;
	switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_MAYU_DETOUR_CANCEL: {
		KIRQL currentIrql;
		PIRP irpCancel = NULL;

		KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
		if (detourDevExt->isOpen)
			irpCancel = detourDevExt->irpq;
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);

		if (irpCancel)
			IoCancelIrp(irpCancel);// at this point, the irpCancel may be completed
		status = STATUS_SUCCESS;
		break;
	}
	case IOCTL_MAYU_GET_VERSION: {
		if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
				MayuDriverVersion.Length) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlCopyMemory(irp->AssociatedIrp.SystemBuffer,
					  MayuDriverVersion.Buffer, MayuDriverVersion.Length);
		irp->IoStatus.Information = MayuDriverVersion.Length;
		DEBUG_LOG(("Version: %T", &MayuDriverVersion));
		status = STATUS_SUCCESS;
		break;
	}
	case IOCTL_MAYU_GET_LOG:
		status = DEBUG_LOG_RETRIEVE((irp));
		break;
	case IOCTL_MAYU_FORCE_KEYBOARD_INPUT: {
		KIRQL currentIrql, cancelIrql;

		// if detour is opened, key datum are forwarded to detour
		if (irpSp->Parameters.DeviceIoControl.InputBufferLength %
				sizeof(KEYBOARD_INPUT_DATA)) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
		KqEnque(&detourDevExt->readQue,
				(KEYBOARD_INPUT_DATA *)irp->AssociatedIrp.SystemBuffer,
				irpSp->Parameters.DeviceIoControl.InputBufferLength
				/ sizeof(KEYBOARD_INPUT_DATA));

		if (detourDevExt->irpq) {
			if (readq(&detourDevExt->readQue, detourDevExt->irpq) ==
					STATUS_SUCCESS) {
				IoAcquireCancelSpinLock(&cancelIrql);
				IoSetCancelRoutine(detourDevExt->irpq, NULL);
				IoReleaseCancelSpinLock(cancelIrql);
				IoCompleteRequest(detourDevExt->irpq, IO_KEYBOARD_INCREMENT);
				detourDevExt->irpq = NULL;
			}
		}
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
		status = STATUS_SUCCESS;
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	irp->IoStatus.Status = status;
	if (status != STATUS_PENDING)
		IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}


#ifndef MAYUD_NT4
// detour IRP_MJ_POWER
NTSTATUS detourPower(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	UNREFERENCED_PARAMETER(deviceObject);

	PoStartNextPowerIrp(irp);
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
#endif // !MAYUD_NT4


// filter IRP_MJ_READ
NTSTATUS filterRead(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	FilterDeviceExtension *filterDevExt =
		(FilterDeviceExtension*)deviceObject->DeviceExtension;
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)filterDevExt->detourDevObj->DeviceExtension;
	KIRQL currentIrql;

	KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
	if (detourDevExt->isOpen && !detourDevExt->wasCleanupInitiated)
		// read from que
	{
		ULONG len = irpSp->Parameters.Read.Length;

		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
		irp->IoStatus.Information = 0;
		KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);
		if (len == 0)
			status = STATUS_SUCCESS;
		else if (len % sizeof(KEYBOARD_INPUT_DATA))
			status = STATUS_BUFFER_TOO_SMALL;
		else
			status = readq(&filterDevExt->readQue, irp);
		KeReleaseSpinLock(&filterDevExt->lock, currentIrql);
		if (status != STATUS_PENDING) {
			irp->IoStatus.Status = status;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}
	} else {
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
	}
	KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);
	filterDevExt->irpq = irp;
	KeReleaseSpinLock(&filterDevExt->lock, currentIrql);

	*IoGetNextIrpStackLocation(irp) = *irpSp;
	IoSetCompletionRoutine(irp, filterReadCompletion, NULL, TRUE, TRUE, TRUE);
	return IoCallDriver(filterDevExt->kbdClassDevObj, irp);
}


// pass throught irp to keyboard class driver
NTSTATUS filterPassThrough(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	FilterDeviceExtension *filterDevExt =
		(FilterDeviceExtension*)deviceObject->DeviceExtension;

	*IoGetNextIrpStackLocation(irp) = *IoGetCurrentIrpStackLocation(irp);
	IoSetCompletionRoutine(irp, filterGenericCompletion,
						   NULL, TRUE, TRUE, TRUE);
	return IoCallDriver(filterDevExt->kbdClassDevObj, irp);
}


#ifndef MAYUD_NT4
// filter IRP_MJ_POWER
NTSTATUS filterPower(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	FilterDeviceExtension *filterDevExt =
		(FilterDeviceExtension*)deviceObject->DeviceExtension;

	PoStartNextPowerIrp(irp);
	IoSkipCurrentIrpStackLocation(irp);
	return PoCallDriver(filterDevExt->kbdClassDevObj, irp);
}
#endif // !MAYUD_NT4
NTSTATUS filterPnP(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	FilterDeviceExtension *filterDevExt =
		(FilterDeviceExtension*)deviceObject->DeviceExtension;
	DetourDeviceExtension *detourDevExt =
		(DetourDeviceExtension*)filterDevExt->detourDevObj->DeviceExtension;
	KIRQL currentIrql;
	NTSTATUS status;
	ULONG minor;
	PIRP cancelIrp;
	PDRIVER_OBJECT driverObject = deviceObject->DriverObject;

	minor = irpSp->MinorFunction;
	IoSkipCurrentIrpStackLocation(irp);
	status = IoCallDriver(filterDevExt->kbdClassDevObj, irp);
	DEBUG_LOG(("filterPnP: minor=%d(%x)", minor, minor));
	switch (minor) {
	case IRP_MN_SURPRISE_REMOVAL:
	case IRP_MN_REMOVE_DEVICE:
		KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
		if (detourDevExt->filterDevObj == deviceObject) {
			PDEVICE_OBJECT devObj = deviceObject->DriverObject->DeviceObject;

			DEBUG_LOG(("filterPnP: current filter(%x) was removed", deviceObject));
			detourDevExt->filterDevObj = NULL;
			while (devObj->NextDevice) {
				if (devObj != deviceObject) {
					detourDevExt->filterDevObj = devObj;
					break;
				}
				devObj = devObj->NextDevice;
			}
			DEBUG_LOG(("filterPnP: current filter was changed to %x", detourDevExt->filterDevObj));
		}
		KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
		// detach
		IoDetachDevice(filterDevExt->kbdClassDevObj);

		KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);
		// TODO: at this point, the irp may be completed (but what can I do for it ?)
		cancelIrp = filterDevExt->irpq;
		KqFinalize(&filterDevExt->readQue);

		KeReleaseSpinLock(&filterDevExt->lock, currentIrql);
		if (cancelIrp) {
			IoCancelIrp(cancelIrp);
		}
		IoDeleteDevice(deviceObject);
		DEBUG_LOG(("delete filter device: %x", deviceObject));
		DEBUG_LOGChain(driverObject);
		break;
	default:
		break;
	}
	return status;
}


#ifdef USE_TOUCHPAD
//
NTSTATUS filterTouchpadCompletion(IN PDEVICE_OBJECT deviceObject,
								  IN PIRP irp, IN PVOID context)
{
	KIRQL currentIrql;
	FilterDeviceExtension *filterDevExt =
		(FilterDeviceExtension*)deviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	//PIO_STACK_LOCATION irpSp = IoGetNextIrpStackLocation(irp);
	UCHAR *data = irp->UserBuffer;
	UCHAR pressure;

	UNREFERENCED_PARAMETER(context);

	if (irp->PendingReturned)
		IoMarkIrpPending(irp);

	if (data)
		pressure = data[TOUCHPAD_PRESSURE_OFFSET];
	else
		pressure = 0;

	if (data) {
		ULONG *p = (ULONG*)data;
		//DEBUG_LOG(("UserBuffer: %2x %2x %2x %2x", data[4], data[5], data[6], data[7]));
		//DEBUG_LOG(("UserBuffer: %x %x %x %x %x %x %x %x", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
	}
	KeAcquireSpinLock(&filterDevExt->lock, &currentIrql);
	if (filterDevExt->isTouched == FALSE && pressure) {
		KIRQL currentIrql, cancelIrql;
		PDEVICE_OBJECT detourDevObj = filterDevExt->detourDevObj;
		DetourDeviceExtension *detourDevExt =
			(DetourDeviceExtension*)detourDevObj->DeviceExtension;
		if (detourDevExt->isOpen) {
			KEYBOARD_INPUT_DATA PadKey = {0, TOUCHPAD_SCANCODE, 0, 0, 0};
			KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
			// if detour is opened, key datum are forwarded to detour
			KqEnque(&detourDevExt->readQue, &PadKey, 1);
			detourDevExt->filterDevObj = deviceObject;

			if (detourDevExt->irpq) {
				if (readq(&detourDevExt->readQue, detourDevExt->irpq) ==
						STATUS_SUCCESS) {
					IoAcquireCancelSpinLock(&cancelIrql);
					IoSetCancelRoutine(detourDevExt->irpq, NULL);
					IoReleaseCancelSpinLock(cancelIrql);
					IoCompleteRequest(detourDevExt->irpq, IO_KEYBOARD_INCREMENT);
					detourDevExt->irpq = NULL;
				}
			}
			KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
		}
		filterDevExt->isTouched = TRUE;
	} else {
		if (filterDevExt->isTouched == TRUE && pressure == 0) {
			KIRQL currentIrql, cancelIrql;
			PDEVICE_OBJECT detourDevObj = filterDevExt->detourDevObj;
			DetourDeviceExtension *detourDevExt =
				(DetourDeviceExtension*)detourDevObj->DeviceExtension;
			if (detourDevExt->isOpen) {
				KEYBOARD_INPUT_DATA PadKey = {0, TOUCHPAD_SCANCODE, 1, 0, 0};
				KeAcquireSpinLock(&detourDevExt->lock, &currentIrql);
				// if detour is opened, key datum are forwarded to detour
				KqEnque(&detourDevExt->readQue, &PadKey, 1);
				detourDevExt->filterDevObj = deviceObject;

				if (detourDevExt->irpq) {
					if (readq(&detourDevExt->readQue, detourDevExt->irpq) ==
							STATUS_SUCCESS) {
						IoAcquireCancelSpinLock(&cancelIrql);
						IoSetCancelRoutine(detourDevExt->irpq, NULL);
						IoReleaseCancelSpinLock(cancelIrql);
						IoCompleteRequest(detourDevExt->irpq, IO_KEYBOARD_INCREMENT);
						detourDevExt->irpq = NULL;
					}
				}
				KeReleaseSpinLock(&detourDevExt->lock, currentIrql);
			}
			filterDevExt->isTouched = FALSE;
		}
	}
	//DEBUG_LOG(("touchpad pressed: out=%u in=%u code=%u status=%x SystemBuffer=%x UserBuffer=%x", irpSp->Parameters.DeviceIoControl.OutputBufferLength, irpSp->Parameters.DeviceIoControl.InputBufferLength, irpSp->Parameters.DeviceIoControl.IoControlCode, irp->IoStatus.Status, irp->AssociatedIrp.SystemBuffer, irp->UserBuffer));
	KeReleaseSpinLock(&filterDevExt->lock, currentIrql);
	return STATUS_SUCCESS;
}


// filter touchpad input
NTSTATUS filterTouchpad(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	FilterDeviceExtension *filterDevExt =
		(FilterDeviceExtension*)deviceObject->DeviceExtension;

	*IoGetNextIrpStackLocation(irp) = *IoGetCurrentIrpStackLocation(irp);
	IoSetCompletionRoutine(irp, filterTouchpadCompletion,
						   NULL, TRUE, TRUE, TRUE);
	return IoCallDriver(filterDevExt->kbdClassDevObj, irp);
}
#endif


NTSTATUS detourPnP(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	UNREFERENCED_PARAMETER(deviceObject);

	IoSkipCurrentIrpStackLocation(irp);
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
