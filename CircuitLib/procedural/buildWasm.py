import os
import pathlib

file_path = (pathlib.Path(__file__).parent/"wasm").resolve()
os.chdir(file_path)

file = ".." / pathlib.Path("cpp/box.cpp")
# file = ".." / pathlib.Path("zig/edge_detector.zig")
# file = ".." / pathlib.Path("rust/edge_detector.rs")

if file.suffix == ".zig":
    os.system(f"zig build-exe {file} -target wasm32-freestanding -fno-entry -rdynamic -O ReleaseSmall")
elif file.suffix == ".rs":
    os.system(f"rustc {file} -target wasm32-unknown-unknown -O -o {file.stem}.wasm")
elif file.suffix in [".cc", ".c", ".cpp"]:
    os.system(f"emcc {file} -Os -flto -s STANDALONE_WASM -s EXPORTED_FUNCTIONS=\"['_generateCircuit', '_getUUID', '_getName', '_getDefaultParameters']\" --no-entry -o {file.stem}.wasm")
else:
    raise Exception("File must be .zig, .rx, .c, .cpp, or .cc")

# os.system("wasm-objdump -x " + str(file_path / pathlib.Path(file.name[:file.name.rfind(".")])) + ".wasm")
