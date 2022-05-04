add_llvm_library (LLVMGetTypePass MODULE
	/invariant-testing/lib/get_type_pass/get_type_pass.cpp

	PLUGIN_TOOL
	opt
	)