add_llvm_library (LLVMAddAssumptionsPass MODULE
	../../../../../invariant-testing/lib/add_assumptions_pass/add_assumptions_pass.cpp

	PLUGIN_TOOL
	opt
	)
