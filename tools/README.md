# tools

The eleven checkers every Sushi Systems repository carries, the license block writer and their
tests. Every file here except this README and `settings.py` is a copy of `tools/` in SushiSkills,
made by `checking/copy_shared_tools.py`; a rule is fixed there first and copied here.

| Path | Checks | Takes |
| --- | --- | --- |
| `check_all.py` | Nothing itself; runs the eleven checkers and the checker tests, and prints only what failed | Repository root |
| `checking/checker.py` | Nothing itself; runs a checker's rule table and prints findings | |
| `checking/copy_shared_tools.py` | Nothing; copies the shared tools from a SushiSkills checkout, or lists the files that differ with `--report` | Repository root, `--from` |
| `checking/ignored_paths.py` | Nothing itself; names the files and folders git ignores, which no checker reads | |
| `settings.py` | Nothing; holds the settings of this repository | |
| `documentation/check_source_comments.py` | License block, file header, block ceiling, comment runs, separators, history words, `@date` | Files or folders |
| `documentation/check_docs_layout.py` | The `docs/` tree, required documents, names, work folders, status lines, links, reachability, archive candidates | Repository root |
| `documentation/check_changelog.py` | Changelog headings and entry shape | Repository root |
| `documentation/check_backlog.py` | The shape of `REMAINING_WORK.md` and `KNOWN_ISSUES.md` | Repository root |
| `documentation/check_design_format.py` | Design documents: the five sections in order, a status line of at most 160 characters, the phase table and its states, no commit hash, no line that dates built or merged work | Repository root |
| `documentation/check_docs_truth.py` | The manual against the source | Repository root, `--source` |
| `documentation/design_status.py` | Nothing itself; reads the status lines of design documents and work folders | |
| `layering/check_layering.py` | Declared tiers and the includes between modules | Repository root |
| `testing/check_test_mirror.py` | `tests/` mirrors the public headers | Repository root |
| `testing/check_test_sizes.py` | Every test names its size in its source | Repository root |
| `testing/check_probes.py` | Probes and scenario benchmarks live under `applications/` and stay out of the test runner | Repository root |
| `logs/check_logging.py` | Console writes outside the allowed paths, swallowed errors, log calls | Repository root |
| `licensing/write_license_block.py` | Nothing; writes the license block into every tracked source file, or lists the files that lack it with `--report` | Repository root, `--project` |
| `routing/summarize_dispatches.py` | Nothing; sums the dispatch lines of every `REPORT.md` per model and effort | Repository root |
| `tests/` | Unit tests for the checker rules; run `python -m unittest discover -s tools/tests` | |

Every checker takes `--report` and `--rule NAME`, exits 1 on findings, 0 when clean or
reporting, 2 on a bad path. Python 3.11, standard library only.

## Settings in this repository

| Setting | Value |
| --- | --- |
| `K_TIER_ORDER` | Empty. SushiTrack is one module, so `check_layering.py` has nothing to compare |
| `K_CONSOLE_PATHS` | The CLI console module and the programs under `applications/`, which write to the terminal by design |
| `K_CLOSED` | False; the licence lines are the PolyForm Noncommercial ones |
