#include "stdafx.h"

// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that you can declare multiple components within one DLL.
DECLARE_COMPONENT_VERSION(
	// component name
	COMPONENTNAME,
	// component version
	COMPONENTVERSION,
	// about text, use \n to separate multiple lines
	// If you don't want any about text, set this parameter to NULL.
	COMPONENTNAME
	" for foobar2000 v1.x\n"
	"\n"
	"contributors:\n"
	"Sami Salonen"
	"\n\n"
	COMPONENTNAME
);


// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_playlisthistory.dll");
