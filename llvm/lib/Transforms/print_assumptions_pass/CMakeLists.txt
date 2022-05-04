add_llvm_library (LLVMPrintAssumptionsPass MODULE
	/invariant-testing/lib/print_assumptions_pass/print_assumptions_pass.cpp

	PLUGIN_TOOL
	opt
	)