// ConsoleApplication1.cpp : Defines the entry point for the console application.
//


#include <iostream>
#include "getopt2.h"

#include "..\include\canapi4.h"
#include "utils.h"
#include "params.h"
#include "registry.h"


#define DRIVERNAME	"pcan_usb"

class ConsoleApp {
private:
	getopt_t	getopt;

	HCANCLIENT	hClient;
	HCANNET	hNet;

public:
	can_device_t	can_device; // USB?, PCI?, LAN? ,...
	HCANHW	hHw; // handle of hardware to test

	void ConsoleApp::CommandlineError(void);
	void ConsoleApp::CommandlineOptionError(void);
	void HelpAndExit(void);
	void printDeviceName(FILE *stream);
	void ExecuteOptions(int argc, char **argv);
};






void ConsoleApp::HelpAndExit()
{
	fprintf(stdout, "CantestIot - Commandline version of PEAK CANTEST for Win10 IoT\n");
	fprintf(stdout, "\nVersion: " __DATE__ " " __TIME__ "\n");
	fprintf(stdout, "\nCommand line summary:\n\n");
	// getop must be intialized to print the syntax
	getopt_help(&getopt, stdout, 96, 10, "cantestiot");
	exit(1);
}


// show error for one option
void ConsoleApp::CommandlineError()
{
	fprintf(stdout, "\nError while parsing commandline:\n");
	fprintf(stdout, "  %s\n", getopt.curerrortext);
	fprintf(stdout, "\nUse cantestiot /? for help.\n");
	exit(1);
}

// parameter wrong for currently parsed option
void ConsoleApp::CommandlineOptionError()
{
	fprintf(stdout, "\nError while parsing commandline option:\n");
	fprintf(stdout, "  %s\nSyntax:  ", getopt.curerrortext);
	getopt_help_option(&getopt, stdout, 96, 10);
	fprintf(stdout, "\nUse cantestiot /? for help.\n");
	exit(1);
}


void ConsoleApp::printDeviceName(FILE *stream) {
	fprintf(stdout, "Active device: %s\n", candevice2text(can_device));
}



void ConsoleApp::ExecuteOptions(int argc, char **argv)
{
	int	res;
	bool	any = 0;
	static char buff[GETOPT_MAX_LINELEN + 1];

	// define commandline syntax
	getopt_init(&getopt, /*ignore_case*/1);
	/*
	getopt_def(&getopt, NULL, NULL, "clinearg1,clinearg2", "optclinearg",
		"dummy arguments for testing getopt2.\nmulti\nline!",
		"abc,123", "set arg1 to \"abc\", arg2 to \"123\".",
		"\"abcd, efg\", 123, 789", "set arg1 to \"abc, efg\", arg2 to \"123\", optarg to \"789\"."
		);
	*/
	getopt_def(&getopt, "?", "help", NULL, NULL, "Print help.",
		NULL, NULL, NULL, NULL);

	sprintf(buff, "Select device driver.\n"
		"name = one of %s.  Default = USB.", candevices_txt);
	getopt_def(&getopt, "dev", "device", "devicename", NULL, buff,
		"usb", "select PCAN_USB driver", NULL, NULL);
	getopt_def(&getopt, "ld", "listdrivers", NULL, NULL, "List installed PCAN drivers.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "lh", "listhardware", NULL, NULL, "List available CAN controller hardware.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "ln", "listnets", NULL, NULL, "List registered nets.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "lc", "listclients", NULL, NULL, "List registered clients.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "gdp", "getdriverparams", "param", NULL, "List parameter(s) of driver.\n"
		"param = \"*\" | <nr> | <name> .",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "ghp", "gethwparams", "handle,param", NULL, "List parameter(s) of hardware(s).\n"
		"handle = \"*\" | <nr> ; param = \"*\" | <nr> | <name>.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "gnp", "getnetparams", "handle,param",
		NULL, "List parameter(s) of net(s).\n"
		"handle = \"*\" | <nr> ; param = \"*\" | <nr> | <name>.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "gcp", "getclientparams", "handle,param",
		NULL, "List parameter(s) of client(s).\n"
		"handle = \"*\" | <nr> ; param = \"*\" | <nr> | <name>.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "sdp", "setdriverparams", "param,val", NULL, "Set parameter of driver.\n"
		"param = <nr> | <name> .",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "shp", "sethwparams", "handle,param,val", NULL, "Set a parameter of a single hardware.\n"
		"handle = <nr> ; param = <nr> | <name>.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "snp", "setnetparams", "handle,param,val", NULL, "Set a parameter of a single net.\n"
		"handle = <nr> ; param = <nr> | <name>.",
		NULL, NULL, NULL, NULL);
	getopt_def(&getopt, "scp", "setclientparams", "handle,param,val", NULL, "Set a parameter of a single client(s).\n"
		"handle = <nr> ; param = <nr> | <name>.",
		NULL, NULL, NULL, NULL);

	getopt_def(&getopt, "gr", "getregistry", "keyvalname", NULL, "Get a values of driver registry.\n"
		"keyvalname = * | name .",
		"*", "show all values in HKLM\\System\\CurrentControlSet\\Services\\<driver>",
		"failonentry", "Show the \"Parameters\\FailOnEntry\" value");
	getopt_def(&getopt, "sr", "setregistry", "keyvalname,val", NULL, "Set a value in the driver registry.\n"
		"keyvalname = name ; val = string or DWORD value",
		"traceevents0 0x06", "Sets \"DriverParams\\TraceEvents0\", selects mask with trace events to DEVICE|DRIVER (see trace.h)",
		"Net33 \"{name = TestNet1M-2M,hHw=16,controllernr=0},{f_core=20000000,nom_brp=5,nom_tseg1=2,nom_tseg2=1,nom_sjw=1,data_brp=2,data_tseg1=3,data_tseg2=1,data_sjw=1}\"",
		"defines an CAN-FD net with 1M nominal and 2M data bitrate.");
	getopt_def(&getopt, "dr", "deleteregistry", "keyvalname", NULL, "Deletes a value from the driver registry.",
		"isrtimeout", "Deletes the \"DriverParams\\ISRtimeout\" entry",
		NULL, NULL);

	if (argc < 2)
		HelpAndExit(); // at least 1 required

	res = getopt_first(&getopt, argc, argv);
	while (res > 0) {
		char buff1[GETOPT_MAX_LINELEN + 1];
		char buff2[GETOPT_MAX_LINELEN + 1];
		char buff3[GETOPT_MAX_LINELEN + 1];
		//printf("\n");

		any = 1; // an option was found. if invalid: getopt made error
		if (getopt_isoption(&getopt, "help")) {
			HelpAndExit();
		}
		else if (getopt_isoption(&getopt, "device")) {
			if (getopt_arg_s(&getopt, "devicename", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			can_device = text2candevice(buff1);
			if (can_device == pcan_unknown)
				CommandlineOptionError();
		}
		else if (getopt_isoption(&getopt, "listdrivers")) {
			printDeviceName(stdout);
			listDrivers(stdout);
		}
		else if (getopt_isoption(&getopt, "listhardware")) {
			printDeviceName(stdout);
			listHardware(stdout, can_device);
		}
		else if (getopt_isoption(&getopt, "listnets")) {
			printDeviceName(stdout);
			listNets(stdout, can_device);
		}
		else if (getopt_isoption(&getopt, "listclients")) {
			printDeviceName(stdout);
			listClients(stdout, can_device);
		}
		else if (getopt_isoption(&getopt, "getdriverparams")) {
			if (getopt_arg_s(&getopt, "param", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			printObjectParams(stdout, can_device, CAN_PARAM_OBJCLASS_DRIVER, NULL, buff1);
		}
		else if (getopt_isoption(&getopt, "gethwparams")) {
			if (getopt_arg_s(&getopt, "handle", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "param", buff2, sizeof(buff2)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			printObjectParams(stdout, can_device, CAN_PARAM_OBJCLASS_HARDWARE, buff1, buff2);
		}
		else if (getopt_isoption(&getopt, "getnetparams")) {
			if (getopt_arg_s(&getopt, "handle", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "param", buff2, sizeof(buff2)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			printObjectParams(stdout, can_device, CAN_PARAM_OBJCLASS_NET, buff1, buff2);
		}
		else if (getopt_isoption(&getopt, "getclientparams")) {
			if (getopt_arg_s(&getopt, "handle", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "param", buff2, sizeof(buff2)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			printObjectParams(stdout, can_device, CAN_PARAM_OBJCLASS_CLIENT, buff1, buff2);
		}
		else if (getopt_isoption(&getopt, "setdriverparams")) {
			if (getopt_arg_s(&getopt, "param", buff2, sizeof(buff2)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "val", buff3, sizeof(buff3)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			setObjectParams(stdout, can_device, CAN_PARAM_OBJCLASS_DRIVER, NULL, buff2, buff3);
		}
		else if (getopt_isoption(&getopt, "sethwparams")) {
			if (getopt_arg_s(&getopt, "handle", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "param", buff2, sizeof(buff2)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "val", buff3, sizeof(buff3)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			setObjectParams(stdout, can_device, CAN_PARAM_OBJCLASS_HARDWARE, buff1, buff2, buff3);
		}
		else if (getopt_isoption(&getopt, "setnetparams")) {
			if (getopt_arg_s(&getopt, "handle", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "param", buff2, sizeof(buff2)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "val", buff3, sizeof(buff3)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			setObjectParams(stdout, can_device, CAN_PARAM_OBJCLASS_NET, buff1, buff2, buff3);
		}
		else if (getopt_isoption(&getopt, "setclientparams")) {
			if (getopt_arg_s(&getopt, "handle", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "param", buff2, sizeof(buff2)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "val", buff3, sizeof(buff3)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			setObjectParams(stdout, can_device, CAN_PARAM_OBJCLASS_CLIENT, buff1, buff2, buff3);
		}
		else if (getopt_isoption(&getopt, "getregistry")) {
			if (getopt_arg_s(&getopt, "keyvalname", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			printRegistryValues(stdout, can_device, buff1);
		}
		else if (getopt_isoption(&getopt, "setregistry")) {
			if (getopt_arg_s(&getopt, "keyvalname", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			if (getopt_arg_s(&getopt, "val", buff2, sizeof(buff2)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			setRegistryValue(stdout, can_device, buff1, buff2);
		}
		else if (getopt_isoption(&getopt, "deleteregistry")) {
			if (getopt_arg_s(&getopt, "keyvalname", buff1, sizeof(buff1)) < 0)
				CommandlineOptionError();
			printDeviceName(stdout);
			setRegistryValue(stdout, can_device, buff1, NULL);
		}
		//		else if (getopt_isoption(&getopt, NULL)) {
		//			// rest of command line
		//		} 

		res = getopt_next(&getopt);
	}

	if (res == GETOPT_STATUS_MINARGCOUNT || res == GETOPT_STATUS_MAXARGCOUNT)
		// known option, but wrong number of arguments
		CommandlineOptionError();
	else if (res < 0)
		CommandlineError();

	if (!any) {
		fprintf(stdout, "No operation performed!\n");
	}

	// all OK: continue
}



int main(int argc, char **argv)
{
	printf("CantestIoT build " __DATE__ " " __TIME__ "\n");

	ConsoleApp *consoleApp = new ConsoleApp();
	consoleApp->can_device = pcan_usb; // fix
	consoleApp->hHw = 16; // fix;
	consoleApp->ExecuteOptions(argc, argv);

	exit(0);
}
