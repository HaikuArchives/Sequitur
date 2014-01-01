
	ARPROOT		= ..
	
#	specify the name of the project
	IMG_NAME	= ArpLegato

	ADDON_PATH	= Filters

	RSRCS		:= ArpLegato
	
#	specify any additional beos shared libraries needed.
	ADD_BESHLIBS	= 

#	specify additional non-beos shared libraries needed.
	ADD_SHLIBS		= $(CPPLIB) AmKernel

#	specify any additional beos static libraries needed.
	ADD_BESTLIBS	= 

#	specify additional non-beos static libraries needed.
	ADD_STLIBS		=
	
#	specify any additional system include paths.
	SYSTEM_INCLUDES	=
	SYSLOCAL_INCLUDES	=

#	specify any additional local include paths.
	LOCAL_INCLUDES	=

include ../makefiles/add-on
