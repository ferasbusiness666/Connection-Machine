import pathlib
import json
import hashlib
import os
import colorama
colorama.init()

script_dir = pathlib.Path(__file__).parent.resolve()
build_cache_path = script_dir/"build_cache.json"
if not build_cache_path.exists():
    with open(build_cache_path, "w") as f:
        json.dump({}, f)
with open(build_cache_path, "r") as f:
    build_cache = json.load(f)


def cache_key(file_path: pathlib.Path) -> str:
    """Store cache paths relative to script so build_cache is portable."""
    try:
        return file_path.resolve().relative_to(script_dir).as_posix()
    except ValueError:
        return file_path.resolve().as_posix()


# Ensure existing cache entries get normalized to relative paths when possible.
normalized_cache = {}
cache_normalized = False
for path_str, hash_val in build_cache.items():
    normalized_key = cache_key(pathlib.Path(path_str))
    if normalized_key != path_str:
        cache_normalized = True
    normalized_cache[normalized_key] = hash_val
build_cache = normalized_cache

folders = [
    "cpp",
    "zig",
    # "rust",
]

target_directory = (script_dir/"wasm").resolve()

# walk through each folder and build the wasm files
folder_paths = [(script_dir/folder).resolve() for folder in folders]
file_paths = []
for folder_path in folder_paths:
    for file_group in folder_path.walk():
        for filename in file_group[2]:
            file = file_group[0] / pathlib.Path(filename)
            if file.suffix not in [".zig", ".rs", ".c", ".cpp", ".cc"]:
                continue
            if file.stem == "cm":
                continue
            file_paths.append(file)

build_cache_updated = False
for file in file_paths:
    file_hash = hashlib.md5(file.read_bytes()).hexdigest()
    file_cache_key = cache_key(file)
    if file_cache_key in build_cache and build_cache[file_cache_key] == file_hash:
        print(colorama.Fore.YELLOW + "Skipping" + colorama.Style.RESET_ALL, file)
        continue
    print(colorama.Fore.GREEN + "Building" + colorama.Style.RESET_ALL, file)
    os.chdir(file.parent)
    res = 0
    if file.suffix == ".zig":
        res = os.system(f"zig build-exe {file} -target wasm32-freestanding -fno-entry -rdynamic -O ReleaseSmall -femit-bin={target_directory / pathlib.Path(file.stem + '.wasm')}")
    elif file.suffix == ".rs":
        continue  # Rust builds are currently disabled
        res = os.system(f"rustc {file} -target wasm32-unknown-unknown -O -o {target_directory / pathlib.Path(file.stem + '.wasm')}")
    elif file.suffix in [".cc", ".c", ".cpp"]:
        res = os.system(f"emcc {file} -Os -flto -s STANDALONE_WASM -s EXPORTED_FUNCTIONS=\"['_generateCircuit', '_getUUID', '_getName', '_getDefaultParameters']\" --no-entry -o {target_directory / pathlib.Path(file.stem + '.wasm')}")
    else:
        raise Exception("File must be .zig, .rx, .c, .cpp, or .cc")
    if res != 0:
        continue
    build_cache[file_cache_key] = file_hash
    build_cache_updated = True

if build_cache_updated or cache_normalized:
    with open(build_cache_path, "w") as f:
        json.dump(build_cache, f, indent=4)
