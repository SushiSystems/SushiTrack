import os
from pathlib import Path

class PathManager:
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.paths = {}
        env_file = project_root / ".tools" / "paths.env"
        if env_file.exists():
            with open(env_file, "r") as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith("#") and "=" in line:
                        k, v = line.split("=", 1)
                        self.paths[k.strip()] = v.strip()
        
    def get(self, key: str, default: str) -> Path:
        return self.project_root / self.paths.get(key, default)

_manager = PathManager(Path(__file__).parent.parent)

DIR_TOOLS = _manager.get("DIR_TOOLS", ".tools")
DIR_BUILD = _manager.get("DIR_BUILD", "build")
DIR_DATA = _manager.get("DIR_DATA", "data")
DIR_3RDPARTY = _manager.get("DIR_3RDPARTY", "3rdparty")
DIR_TESTS = _manager.get("DIR_TESTS", "tests")
DIR_REGRESSION = _manager.get("DIR_REGRESSION", "tests/regression")
DIR_EVALUATOR = _manager.get("DIR_EVALUATOR", "tests/regression/evaluator")
DIR_INFERENCE = _manager.get("DIR_INFERENCE", "tests/regression/inference")
PROJECT_ROOT = _manager.project_root
