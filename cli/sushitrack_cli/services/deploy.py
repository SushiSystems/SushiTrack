# deploy.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Assemble a ready-to-consume package under ``package/``.

Two flavours: ``cpp`` (headers + link/runtime library + a CMake example) and
``python`` (the ctypes binding with the native library bundled inside it). Both
are self-contained — the consumer needs neither this repo nor a build tree.
"""

import enum
import platform
import shutil
from importlib import resources

from .. import console
from ..config import IS_WINDOWS
from .artifacts import find_file, import_lib_name, shared_lib_name, sushilog_lib_name

KINDS = ("cpp", "python")


class DeployKind(str, enum.Enum):
    """The `deploy` command's consumer-flavour argument."""

    cpp = "cpp"
    python = "python"

#: The licence texts of the code inside the libraries, from the repository root to licenses/.
K_LICENCE_TEXTS = (
    ("third_party/bytetrack/LICENSE", "BYTETRACK_CPP_MIT.txt"),
    ("third_party/licenses/LAP_BSD_2_CLAUSE.txt", "LAP_BSD_2_CLAUSE.txt"),
    ("third_party/licenses/NLOHMANN_JSON_MIT.txt", "NLOHMANN_JSON_MIT.txt"),
    ("third_party/eigen/COPYING.MPL2", "EIGEN_MPL_2_0.txt"),
)


def _template(name):
    """Return the text of the deploy template *name*."""
    return resources.files("sushitrack_cli").joinpath("templates", name).read_text(encoding="utf-8")


def _copy_notices(paths, pkg_dir):
    """Copy LICENSE, NOTICE.md and the third-party licence texts; False after naming a missing one."""
    sources = [(paths.root / "LICENSE", pkg_dir / "LICENSE"),
               (paths.root / "NOTICE.md", pkg_dir / "NOTICE.md")]
    sources += [(paths.root / source, pkg_dir / "licenses" / target)
                for source, target in K_LICENCE_TEXTS]
    (pkg_dir / "licenses").mkdir(parents=True, exist_ok=True)
    for source, target in sources:
        if not source.is_file():
            console.error(f"{source} not found; a package without its notices may not ship.")
            return False
        shutil.copy2(source, target)
    return True


def _copy_config(paths, pkg_dir):
    config = getattr(paths, "config", paths.root / "sushitrack.json")
    if config.exists():
        shutil.copy2(config, pkg_dir / "sushitrack.json")


def _find_runtimes(paths):
    """Return SushiTrack's and SushiLog's runtime libraries, or None after naming the missing one."""
    found = []
    for name in (shared_lib_name(), sushilog_lib_name()):
        library = find_file(paths.build, name)
        if not library:
            console.error(f"{name} not found. Run `st build` first.")
            return None
        found.append(library)
    return found


def _deploy_cpp(paths, pkg_dir):
    runtimes = _find_runtimes(paths)
    if runtimes is None:
        return False
    runtime, sushilog = runtimes
    import_lib = find_file(paths.build, import_lib_name())
    if not import_lib:
        console.error(f"{import_lib_name()} not found. Run `st build` first.")
        return False

    include_dst = pkg_dir / "include" / "SushiTrack"
    include_dst.mkdir(parents=True, exist_ok=True)
    src_include = getattr(paths, "include", paths.root / "include" / "SushiTrack")
    if src_include.is_dir():
        for hdr in sorted(src_include.rglob("*")):
            if hdr.suffix in (".h", ".hpp"):
                target = include_dst / hdr.relative_to(src_include)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(hdr, target)

    (pkg_dir / "lib").mkdir(parents=True, exist_ok=True)
    shutil.copy2(import_lib, pkg_dir / "lib" / import_lib_name())
    if IS_WINDOWS:
        (pkg_dir / "bin").mkdir(parents=True, exist_ok=True)
        shutil.copy2(runtime, pkg_dir / "bin" / shared_lib_name())
        shutil.copy2(sushilog, pkg_dir / "bin" / sushilog_lib_name())
    else:
        shutil.copy2(sushilog, pkg_dir / "lib" / sushilog_lib_name())

    _copy_config(paths, pkg_dir)
    (pkg_dir / "example").mkdir(parents=True, exist_ok=True)
    (pkg_dir / "example" / "example.c").write_text(_template("example.c.in"), encoding="utf-8")
    (pkg_dir / "example" / "CMakeLists.txt").write_text(_template("CMakeLists.txt.in"),
                                                        encoding="utf-8")
    (pkg_dir / "README.md").write_text(
        _template("cpp_readme.md.in").format(import_lib=import_lib_name(),
                                             runtime_lib=shared_lib_name(),
                                             sushilog_lib=sushilog_lib_name(),
                                             platform=platform.system()), encoding="utf-8")
    return _copy_notices(paths, pkg_dir)


def _deploy_python(paths, pkg_dir):
    runtimes = _find_runtimes(paths)
    if runtimes is None:
        return False
    runtime, sushilog = runtimes

    binding = paths.root / "bindings" / "python" / "sushitrack.py"
    if not binding.exists():
        console.error(f"Python binding not found under {paths.root / 'bindings' / 'python'}")
        return False

    module_dir = pkg_dir / "sushitrack"
    module_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binding, module_dir / binding.name)
    shutil.copy2(runtime, module_dir / shared_lib_name())
    shutil.copy2(sushilog, module_dir / sushilog_lib_name())
    (module_dir / "__init__.py").write_text(
        f"from .{binding.stem} import *  # noqa: F401,F403\n", encoding="utf-8")

    _copy_config(paths, pkg_dir)
    (pkg_dir / "example.py").write_text(_template("example.py.in"), encoding="utf-8")
    (pkg_dir / "README.md").write_text(
        _template("python_readme.md.in").format(runtime_lib=shared_lib_name(),
                                                sushilog_lib=sushilog_lib_name(),
                                                platform=platform.system()), encoding="utf-8")
    return _copy_notices(paths, pkg_dir)


def deploy(paths, kind):
    """Assemble ``package/`` for *kind* ("cpp" or "python"). Returns an exit code."""
    if kind not in KINDS:
        console.error(f"Unknown deploy kind '{kind}'. Choose: {', '.join(KINDS)}.")
        return 2

    pkg_dir = paths.package
    console.header(f"Deploy ({kind})")
    if pkg_dir.exists():
        console.info(f"Clearing existing package directory: {pkg_dir}")
        shutil.rmtree(pkg_dir, ignore_errors=True)
    pkg_dir.mkdir(parents=True, exist_ok=True)

    console.info(f"Assembling for {platform.system()} -> {pkg_dir}")
    ok = _deploy_cpp(paths, pkg_dir) if kind == "cpp" \
        else _deploy_python(paths, pkg_dir)
    if not ok:
        console.error("Deploy failed.")
        return 1
    console.success(f"Package ready at {pkg_dir}")
    return 0
