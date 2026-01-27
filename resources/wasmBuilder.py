import pathlib
import sys
import os

def build(source: pathlib.Path, target: pathlib.Path):
    assert (source.suffix in [".zig", ".rs", ".c", ".cpp", ".cc"])
    assert (target.suffix == ".wasm")
    os.chdir(source.parent)
    res = 0
    if source.suffix == ".zig":
        res = os.system(f"zig build-exe {source} -target wasm32-freestanding -fno-entry -rdynamic -O ReleaseSmall -femit-bin={target}")
    elif source.suffix == ".rs":
        return False # Rust builds are currently disabled
        res = os.system(f"rustc {source} -target wasm32-unknown-unknown -O -o {target}")
    elif source.suffix in [".cc", ".c", ".cpp"]:
        res = os.system(f"emcc {source} -Os -flto -s STANDALONE_WASM -s EXPORTED_FUNCTIONS=\"['_generateCircuit', '_getUUID', '_getName', '_getDefaultParameters']\" --no-entry -o {target}")
    return res == 0

build(sys.argv[1], sys.argv[2])
