# compile_rust_min.py
import subprocess
import sys
import os

crate_dir = sys.argv[1]
release = "--release" in sys.argv
lto = "--lto" in sys.argv

if lto:
    with open(os.path.join(crate_dir, "Cargo.toml"), "a") as f:
        f.write("\n[profile.release]\nlto = true\n")

cmd = ["cargo", "build", "--target", "wasm32-unknown-unknown"]
if release:
    cmd.append("--release")

subprocess.run(cmd, cwd=crate_dir)

crate_name = "output"
with open(os.path.join(crate_dir, "Cargo.toml")) as f:
    for line in f:
        if line.strip().startswith("name"):
            crate_name = line.split("=")[1].strip().strip('"')
            break

print(os.path.join(
    crate_dir, "target", "wasm32-unknown-unknown",
    "release" if release else "debug", f"{crate_name}.wasm"
))