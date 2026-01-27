#ifndef wasmCompiler_h
#define wasmCompiler_h

#include <wasmtime.hh>

class Environment;

class WasmCompiler {
public:
	static std::optional<wasmtime::Module> compileFile(const std::string& file);
};

#endif /* wasmCompiler_h */
