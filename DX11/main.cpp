#include "stdafx.h"
#include "systemclass.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	SystemClass* System;
	bool result;

	// create the system object.
	System = new SystemClass;
	if(!System)
	{
		return -1;
	}

	// Initialized and run the system object
	result = System->Initialize();
	if(result)
	{
		System->Run();
	}

	// shutdown and release the system object
	System->Shutdown();
	delete System;
	System = nullptr;

	return 0;
}