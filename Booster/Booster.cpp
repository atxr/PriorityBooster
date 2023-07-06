// Booster.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <stdio.h>
#include "../PriorityBooster/PriorityBoosterCommon.h"

int Error(const char* message) {
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}

int main(int argc, const char* argv[])
{
	if (argc != 3) {
		printf("Usage: Booster <thread id> <priority>\n");
		return 1;
	}

	HANDLE hDevice = CreateFile(L"\\\\.\\PriorityBooster", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	ThreadData data = { atoi(argv[1]), atoi(argv[2]) };

	DWORD returned;
	BOOL success = DeviceIoControl(hDevice, IOCTL_PRIORITY_BOOSTER_SET_PRIORITY, &data, sizeof(data), nullptr, 0, &returned, nullptr);
	if (!success) {
		printf("Priority change failed");
		return 1;
	}

	CloseHandle(hDevice);
	return 0;

}
