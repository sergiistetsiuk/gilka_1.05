"""PlatformIO pre-build hook. Private values never appear in compiler flags or logs."""
from pathlib import Path
import sys

Import('env')
project = Path(env.subst('$PROJECT_DIR'))
sys.path.insert(0, str(project / 'scripts'))
from env_config import generate

folder = Path(env.subst('$BUILD_DIR')) / 'generated'
generate(project / '.env', folder / 'FirmwareDefaults.h')
env.Append(CPPPATH=[str(folder)])
