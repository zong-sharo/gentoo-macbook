//
// standard includes
//
#include <cstdlib>
#include <cstdio>

#include <sys/io.h>
#include <strings.h>
#include <getopt.h>
#include <errno.h>

//
// interface implementation
//
struct InterfaceBase
{
	const size_t ioBase, ioLen;
	const int maxBrightness;

	inline InterfaceBase(const size_t ioBase, const size_t ioLen, const int maxBrightness)
		: ioBase(ioBase), ioLen(ioLen), maxBrightness(maxBrightness)
	{}

	virtual inline int init(void) const
	{
		return ioperm(ioBase, ioLen, 1);
	}

	virtual int getBrightness(void) const = 0;
	virtual int setBrightness(const int brightness) const = 0;
	virtual int stepBrightness(const int upDown) const = 0;
};

// Intel and Nvidia chipsets practically use the same interface
struct IntelNvidiaInterface : public InterfaceBase
{
	static const int BRIGHTNESS_DOWN = 0;
	static const int BRIGHTNESS_UP   = 1;

	inline IntelNvidiaInterface(const size_t ioBase)
		: InterfaceBase(ioBase, 2, 15)
	{}

	virtual inline int getBrightness(void) const
	{
		outb(0x03, ioBase + 1);
		outb(0xbf, ioBase);
		return inb(ioBase + 1) >> 4;
	}

	virtual inline int setBrightness(const int brightness) const
	{
		if ((brightness < 0) || (brightness > maxBrightness))
			return EINVAL;

		outb(0x04 | (brightness << 4), ioBase + 1);
		outb(0xbf, ioBase);
		return 0;
	}

	virtual inline int stepBrightness(const int upDown) const
	{
		outb(upDown, ioBase + 1);
		outb(0xbf, ioBase);
		return 0;
	}
};

// ... but the base address differs
static const IntelNvidiaInterface IntelInterface(0x0b2);
static const IntelNvidiaInterface NvidiaInterface(0x52e);

// GMUX interface is a bit different (but simpler)
struct GmuxInterface : public InterfaceBase
{
	inline GmuxInterface(void)
		: InterfaceBase(0x774, 4, 0x1af40)
	{}

	virtual inline int getBrightness(void) const
	{
		return inl(ioBase);
	}

	virtual inline int setBrightness(const int brightness) const
	{
		if ((brightness < 0) || (brightness > maxBrightness))
			return EINVAL;

		outl(brightness, ioBase);
		return 0;
	}

	virtual inline int stepBrightness(const int /* upDown */) const
	{
		return ENOTSUP;
	}
};

static const GmuxInterface GmuxInterface;

//
// command line options
//
static const char *programName = NULL;

static void printUsage(const int status)
{
	printf("%s:\n"
	       "  change display backlight on Intel-based Apple machines\n"
	       "\n"
	       "Usage:\n"
	       "  %s [--intel/--nvidia/--gmux] --get          OR\n"
	       "  %s [--intel/--nvidia/--gmux] --set <level>  OR\n"
	       "  %s [--intel/--nvidia/--gmux] --step-down    OR\n"
	       "  %s [--intel/--nvidia/--gmux] --step-up\n"
	       "\n"
	       "Options:\n"
	       "  -h, --help            display this help and exit\n"
	       "  -q, --quiet           do not inhibit usual output\n"
	       "\n"
	       "  -i, --intel           machine with Intel mainboard chipset\n"
	       "  -n, --nvidia          machine with Nvidia mainboard chipset\n"
	       "  -m, --gmux            machine with Core i5/i7 processor\n"
	       "\n"
	       "  -g, --get             display current brightness\n"
	       "  -s, --set <level>     set backlight to <level> (0-MAX)\n"
	       "      --step-down       decrease brightness\n"
	       "      --step-up         increase brightness\n",
	       programName, programName, programName, programName, programName);

	exit(status);
}

static const InterfaceBase* hwInterface = NULL;
static int doQuiet = 0, doGet = 0, doSetLevel = -1, doStep = -1;

static const char *shortOptions = "hqinmgs:";
static const struct option longOptions[] =
{
	{"help",        no_argument,        NULL,     'h'},
	{"quiet",       no_argument,        NULL,     'q'},

	{"intel",       no_argument,        NULL,     'i'},
	{"nvidia",      no_argument,        NULL,     'n'},
	{"gmux",        no_argument,        NULL,     'm'},

	{"get",         no_argument,        NULL,     'g'},
	{"set",         required_argument,  NULL,     's'},
	{"step-down",   no_argument,        &doStep,  IntelNvidiaInterface::BRIGHTNESS_DOWN},
	{"step-up",     no_argument,        &doStep,  IntelNvidiaInterface::BRIGHTNESS_UP},

	// end of list
	{NULL, 0, NULL, 0}
};

static int decodeSwitches(int argc, char *argv[])
{
	int c;
	while ((c = getopt_long(argc, argv, shortOptions, longOptions, NULL)) != EOF) {
		switch (c) {
			case 'h': printUsage(0);                   break;
			case 'q': doQuiet = 1;                     break;

			case 'i': hwInterface = &IntelInterface;   break;
			case 'n': hwInterface = &NvidiaInterface;  break;
			case 'm': hwInterface = &GmuxInterface;    break;

			case 'g': doGet = 1;                       break;
			case 's': {
				char *endp;
				doSetLevel = strtoul(optarg, &endp, 0);

				if (*endp) {
					fprintf(stderr, "%s: invalid number format: %s\n", programName, optarg);
					printUsage(EXIT_FAILURE);
				}
			} break;
		}
	}

	return optind;
}

//
// main routine
//
int main(int argc, char *argv[])
{
	//
	// parse command line arguments
	//
	programName = rindex(argv[0], '/');
	programName = programName ? programName + 1 : argv[0];
	decodeSwitches(argc, argv);

	//
	// map I/O ports
	//
	if (!hwInterface) {
		fprintf(stderr, "%s: no interface selected: use either of --intel, --nvidia, or --gmux\n", programName);
		return EXIT_FAILURE;
	}
	else if (hwInterface->init()) {
		perror("ioperm failed (you should be root)");
		return EXIT_FAILURE;
	}

	if (!doQuiet)
		printf("valid range: 0-%d (0x%x)\n", hwInterface->maxBrightness, hwInterface->maxBrightness);

	//
	// request to display current brightness
	//
	if (doGet)
		printf("%s%d\n", doQuiet ? "" : "current brightness: ", hwInterface->getBrightness());

	//
	// request to set new brightness
	//
	else if (doSetLevel >= 0) {
		if (!doQuiet)
			printf("setting brightness to: %d\n", doSetLevel);
		if (hwInterface->setBrightness(doSetLevel))
			fprintf(stderr, "%s: absolute change failed or not supported\n", programName);
		else if (!doQuiet)
			printf("current brightness is now: %d\n", hwInterface->getBrightness());
	}

	//
	// request to change incrementally
	//
	else if (doStep >= 0) {
		if (!doQuiet)
			printf("%s brightness\n", doStep ? "increasing" : "decreasing");
		if (hwInterface->stepBrightness(doStep))
			fprintf(stderr, "%s: relative change failed or not supported\n", programName);
		else if (!doQuiet)
			printf("current brightness is now: %d\n", hwInterface->getBrightness());
	}

	//
	// otherwise print usage info
	//
	else
		printUsage(0);

	return 0;
}

// ***** end of source ***** //

