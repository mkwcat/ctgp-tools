#pragma once

bool DVDLowRead(void *destinationAddress, int length, int offset, void (*busyStateCallback)());
