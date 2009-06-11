///////////////////////////////////////////////////////////////////////////////
// Rescue driver for Madotsukai no Yu^utsu for Windows2000/XP


#include <ntddk.h>

#pragma warning(3 : 4061 4100 4132 4701 4706)

///////////////////////////////////////////////////////////////////////////////
// Protorypes


NTSTATUS DriverEntry       (IN PDRIVER_OBJECT, IN PUNICODE_STRING);
NTSTATUS mayuAddDevice     (IN PDRIVER_OBJECT, IN PDEVICE_OBJECT);
VOID mayuUnloadDriver      (IN PDRIVER_OBJECT);
NTSTATUS mayuGenericDispatch (IN PDEVICE_OBJECT, IN PIRP);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( init, DriverEntry )
#endif // ALLOC_PRAGMA

///////////////////////////////////////////////////////////////////////////////
// Entry / Unload


// initialize driver
NTSTATUS DriverEntry(IN PDRIVER_OBJECT driverObject,
		     IN PUNICODE_STRING registryPath)
{
  ULONG i;
  UNREFERENCED_PARAMETER(registryPath);

  // set major functions
  driverObject->DriverUnload = mayuUnloadDriver;
  driverObject->DriverExtension->AddDevice = mayuAddDevice;
  for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
    driverObject->MajorFunction[i] = mayuGenericDispatch;
  driverObject->MajorFunction[IRP_MJ_PNP] = mayuGenericDispatch;
  return STATUS_SUCCESS;
}

NTSTATUS mayuAddDevice(IN PDRIVER_OBJECT driverObject,
		       IN PDEVICE_OBJECT kbdClassDevObj)
{
  UNREFERENCED_PARAMETER(driverObject);
  UNREFERENCED_PARAMETER(kbdClassDevObj);
  return STATUS_SUCCESS;
}

// unload driver
VOID mayuUnloadDriver(IN PDRIVER_OBJECT driverObject)
{
  UNREFERENCED_PARAMETER(driverObject);
}


///////////////////////////////////////////////////////////////////////////////
// Dispatch Functions


// Generic Dispatcher
NTSTATUS mayuGenericDispatch(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
  UNREFERENCED_PARAMETER(deviceObject);
  UNREFERENCED_PARAMETER(irp);
  return STATUS_SUCCESS;
}
