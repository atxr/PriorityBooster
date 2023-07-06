#include <ntifs.h>
#include <ntddk.h>
#include "PriorityBoosterCommon.h"

void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS PriorityBoosterDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

extern "C" NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = PriorityBoosterUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBoosterDeviceControl;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\PriorityBooster");

	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status;
	}

	UNICODE_STRING symlink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");
	status = IoCreateSymbolicLink(&symlink, &devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symlink (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	KdPrint(("PriorityBooster loaded\n"));
	return STATUS_SUCCESS;
}

void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symlink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");
	IoDeleteSymbolicLink(&symlink);
	IoDeleteDevice(DriverObject->DeviceObject);

	KdPrint(("PriorityBooster unloaded\n"));
}

_Use_decl_annotations_
NTSTATUS PriorityBoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS PriorityBoosterDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
		if (!data) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (data->Priority < 1 || data->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PETHREAD Thread;
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);
		if (!NT_SUCCESS(status))
			break;

		KeSetPriorityThread((PKTHREAD)Thread, data->Priority);
		ObDereferenceObject(Thread);

		KdPrint(("Thread priority changed to %d for thread %u\n", data->Priority, data->ThreadId));
		break;
	}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
