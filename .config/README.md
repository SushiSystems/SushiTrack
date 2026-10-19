# .config

Configuration a tool can be pointed at, kept out of the repository root.

| File | Who reads it | The flag that points at it | The call site that passes it |
| --- | --- | --- | --- |
| `docker/Dockerfile` | docker | `docker build -f` | `cli/sushitrack_cli/services/docker.py`, in `build()` |
| `docker/Dockerfile.dockerignore` | docker with BuildKit | found beside the Dockerfile `-f` names | the same call |
| `conda/environment.yml` | conda | `conda env create -f` | `docs/getting_started/INSTALL.md`, typed by the user |

Adding a file here means adding a row.
