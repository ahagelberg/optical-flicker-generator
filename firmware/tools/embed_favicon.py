"""
Embed res/favicon.png at compile time. Generates include/favicon_data.h.
Run as PlatformIO pre script; if res/favicon.png is missing, embeds a minimal 1x1 PNG.
"""
import os
import base64

# Minimal 1x1 transparent PNG (fallback when res/favicon.png is missing)
FALLBACK_PNG_B64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=="

def main():
    try:
        Import("env")
        project_dir = env["PROJECT_DIR"]
    except NameError:
        project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    src_path = os.path.join(project_dir, "res", "favicon.png")
    out_path = os.path.join(project_dir, "include", "favicon_data.h")
    if os.path.isfile(src_path):
        with open(src_path, "rb") as f:
            data = f.read()
    else:
        data = base64.b64decode(FALLBACK_PNG_B64)
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    def byte_val(b):
        return b if isinstance(b, int) else ord(b)

    with open(out_path, "w") as f:
        f.write("/* Auto-generated from res/favicon.png - do not edit */\n\n")
        f.write("#ifndef FAVICON_DATA_H\n#define FAVICON_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write("static const unsigned int FAVICON_PNG_LEN = %u;\n\n" % len(data))
        f.write("static const uint8_t FAVICON_PNG[] = {\n")
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            f.write("  " + ", ".join("%3u" % byte_val(b) for b in chunk) + ",\n")
        f.write("};\n\n#endif\n")
    print("embed_favicon: %s -> include/favicon_data.h (%u bytes)" % (
        src_path if os.path.isfile(src_path) else "fallback", len(data)))

if __name__ == "__main__":
    main()
