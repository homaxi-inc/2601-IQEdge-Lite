# Pre-build hook (platformio.ini). Loads .env for logging only.
# IQWATCH_API_KEY in .env is for tools/aws-verify on the host — NOT compiled into firmware.
Import("env")

from os.path import isfile, join


def _load_dotenv(path):
    out = {}
    if not isfile(path):
        return out
    with open(path, encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if "=" not in line:
                continue
            key, val = line.split("=", 1)
            out[key.strip()] = val.strip().strip('"').strip("'")
    return out


project_dir = env.subst("$PROJECT_DIR")
dotenv = _load_dotenv(join(project_dir, ".env"))
if dotenv.get("IQWATCH_API_KEY"):
    print("[inject_env] IQWATCH_API_KEY in .env (host verify tools only; firmware uses MQTT only)")
else:
    print("[inject_env] IQWATCH_API_KEY unset (.env optional for tools/aws-verify)")
