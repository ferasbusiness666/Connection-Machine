import os
import pathlib

file_path = pathlib.Path(__file__).resolve()
os.chdir(file_path.parent)

file = "edge_detector.zig"

if file.endswith(".zig"):
    os.system(f"zig build-exe {file} -target wasm32-freestanding -fno-entry -rdynamic -O ReleaseSmall")
elif file.endswith(".c") or file.endswith(".cpp") or file.endswith(".cc"):
    os.system("emcc " + file + " -Os -s STANDALONE_WASM -s EXPORTED_FUNCTIONS=\"['_generateCircuit', '_getUUID', '_getName', '_getDefaultParameters']\" --no-entry -o " + file[:file.rfind(".")] + ".wasm")
else:
    raise Exception("File must be .zig, .c, .cpp, or .cc")

os.system("wasm-objdump -x " + file[:file.rfind(".")] + ".wasm")
