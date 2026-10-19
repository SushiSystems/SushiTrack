# Estate audit: sushitrack

**Status:** Shipped — 2026-10-05. Read-only audit; nothing in the repository was changed.

One auditing agent per dimension read the tree and a second agent checked each finding against
the files. A finding marked *confirmed* was re-read by the reviewer; *unclear* means the reviewer
could not settle it; *found by review* means the first pass missed it and nobody re-checked it.
Refuted findings are listed and not counted. No build, test or project CLI was run.

| Dimension | High | Medium | Low | Refuted |
| --- | --- | --- | --- | --- |
| Licence | 3 | 11 | 3 | 1 |
| Command line | 3 | 16 | 5 | 0 |
| Documentation | 6 | 18 | 3 | 0 |
| Layout and hygiene | 3 | 11 | 7 | 0 |
| Code shape | 9 | 16 | 2 | 0 |

## Licence

### Facts

- Root D:/Projects/sushitrack/LICENSE: Apache License 2.0 full text, 190 lines; line 178 reads 'Copyright (c) 2026-present Mustafa Garip & Sushi Systems'. No root NOTICE, no COPYING, no docs/LICENSE.
- Other tracked licence files: third_party/bytetrack/LICENSE (MIT, 'Copyright (c) 2022 Vertical Beach (lp6m, medalotte)' and 'Copyright (c) 2021 Yifu Zhang') with NOTICE; third_party/ocsort/LICENSE (MIT, 'Copyright (c) 2023 Yueqi Guo') with NOTICE. Both NOTICE files name the upstream URL and local changes.
- Tracked files: 433. Outside third_party/ the code files are 43 .cpp, 27 .hpp, 10 .h, 50 .py, 5 CMakeLists.txt, 1 Dockerfile, 1 .sh, 1 .ps1, 1 .bat.
- Header variant A (C/C++, boxed 22-line block, full Apache-2.0 notice, 'Copyright (c) 2026-present Mustafa Garip & Sushi Systems'): 80 files = 43 .cpp + 27 .hpp + 10 .h; every own C/C++ file carries it. Examples: source/assignment_solvers.cpp, include/SushiTrack/config_loader.hpp, include/SushiTrack/sushitrack_c.h.
- Header variant B (Python, two '#' lines: copyright line + 'Licensed under the Apache License, Version 2.0. See LICENSE.'): 41 .py files. Examples: bindings/python/setup.py, bindings/python/sushitrack.py, cli/sushitrack_cli/cli.py.
- Header variant C (Python docstring '@copyright Copyright (c) 2026-present Mustafa Garip & Sushi Systems. All rights reserved.', no licence named): 2 files, tests/regression/inference/demo.py and tests/regression/inference/pipeline.py.
- No header at all, own files: 7 .py (cli/sushitrack_cli/__init__.py, __main__.py, console.py, services/__init__.py, cli/tests/conftest.py, test_console_lazy.py, test_service_enums.py); 5 CMakeLists.txt (root, tests/, tests/unit, tests/integration, tests/regression); Dockerfile; cli/misc/clean_preds.sh; cli/misc/clean_preds.bat; cli/misc/construct_trackeval_data.ps1.
- Tracked third_party C/C++ files: 25. 24 (bytetrack 12, ocsort 12) carry no header, as upstream; third_party/nlohmann/json.hpp carries 'SPDX-License-Identifier: MIT', 'SPDX-FileCopyrightText: 2013-2023 Niels Lohmann', version 3.11.3.
- Machine-readable: bindings/python/pyproject.toml line 11 'license = { text = "Apache-2.0" }' and line 19 classifier 'License :: OSI Approved :: Apache Software License' (package sushitrack 1.0.0); bindings/python/setup.py line 16 license="Apache-2.0".
- Machine-readable, absent: cli/pyproject.toml (sushitrack-cli 0.1.0) has no license field and no classifiers; CMakeLists.txt, Dockerfile (no LABEL), environment.yml, cli/sushistack.deps.toml and sushitrack.json declare no licence. No package.json, Doxyfile, sushi-module.toml or vcpkg.json is tracked.
- Prose: docs/README.md lines 1284-1298 reproduce the Apache-2.0 notice; lines 1300-1313 table the vendored code. docs/CONTRIBUTING.md line 81 requires 'the standard Apache-2.0 header'. docs/design/SUSHITRACK_MIGRATION.md lines 6, 13, 18, 28-29, 120-140 fix Apache-2.0 as the licence and header. Root README.md has no licence statement.
- Prose in agent records: docs/agent/2026_09_17_SUSHITRACK_MIGRATION/SPEC.md, PLAN.md and REPORT.md describe the move to Apache-2.0. docs/reference/CHANGELOG.md line 37: '2026-09-17 — core: Released initial Apache-2.0 open-source version of SushiTrack tracking engine.' under '## v1.0.0 — 2026-09-17'.
- Copyright holder and year: one form everywhere, 'Copyright (c) 2026-present Mustafa Garip & Sushi Systems' (122 header occurrences plus LICENSE and docs). @author is 'Mustafa Garip' in 82 files and 'Mustafa Garip (mustafagarip@sushisystems.io)' in 2.
- git shortlog -sne (HEAD and --all): one author, '79 Mustafa Garip <mustafa.garip3434@hotmail.com>'; committer identical. First commit 2026-05-04, last 2026-10-04. 48 commits carry a 'Co-Authored-By: Claude ...' trailer.
- Licence history: the initial commit d479857 (2026-05-04, project then named SushiTrack) added an empty LICENCE; eaa452c (2026-09-17) replaced it with Apache-2.0 LICENSE. No local git tags. Remote: https://github.com/SushiSystems/SushiTrack.git.
- Submodules (.gitmodules): third_party/eigen at 52f99a14 (gitlab libeigen; MPL-2.0 primary, plus Apache-2.0, BSD-3-Clause, MINPACK per its LICENSES/); third_party/TrackEval at 12c8791b (MIT, Copyright 2020 Jonathon Luiten); third_party/yolox at 6ddff482 (Apache-2.0, Copyright 2021-2022 Megvii Inc.). Each keeps its own LICENSE inside the submodule.
- Eigen use in own code: six includes, all '#include <Eigen/Dense>'; nothing from unsupported/. EIGEN_MPL2_ONLY is not defined anywhere outside the submodule.
- CMake: no FetchContent, ExternalProject or vcpkg manifest. add_subdirectory pulls third_party/bytetrack and third_party/ocsort (CMakeLists.txt lines 191, 194); find_package(GTest) in the three tests CMakeLists. cli/sushistack.deps.toml declares only googletest (BSD-3-Clause).
- Python dependencies: cli/pyproject.toml needs typer>=0.12, rich>=13.0, sushicore>=0.7.0, tomli, pytest (all MIT except sushicore; the installed sushicore 0.5.0 reports License-Expression Apache-2.0). environment.yml pins about 60 conda/pip packages including pytorch, torchvision, torchaudio, pytorch-cuda=12.1, opencv-python, pycocotools, sushicore==0.4.0.
- Dockerfile base image: nvidia/cuda:12.1.1-devel-ubuntu22.04, plus torch 2.5.1 cu121 wheels. No licence notice for any pip or conda dependency is kept in the tree; only the vendored and submodule code has one.
- Tracked benchmark data: 228 files under tests/regression/evaluator/data — 76 sequences, each det/det.txt, gt/gt.txt and seqinfo.ini: 40 DanceTrack, 28 MOT17 (DPM, FRCNN, SDP, YOLOX variants), 8 MOT20. No licence or README file sits in that folder.
- Untracked on disk (gitignored): third_party/video holds six MOT17/MOT20 clips (about 44 MB); third_party/weight holds five YOLOX checkpoints (about 2.2 GB).
- tools/documentation/check_source_comments.py locates the licence block by shape (boxed '/* ... */' lines) and does not match the licence text, so a new header text passes it as long as the box shape is kept.

### Corrections from review

- F1 says every commit up to '51f6bde/HEAD' is on the remote. 51f6bde is not an object in D:/Projects/sushitrack (it is the sushistack HEAD). The sushitrack HEAD is 87cfedd 'build(cli): require sushicore 0.7.0', and `git branch -vv` shows main one commit ahead of origin/main, so the last commit is not on the remote.
- F1's title states that v1.0.0 is published. The GitHub repository is PRIVATE (gh repo view: "visibility":"PRIVATE"), has no tags and no releases, and neither 'sushitrack' nor 'sushitrack-cli' exists on PyPI (HTTP 404 for both). The audit listed these as not checked but still rated the finding high and worded it as fact.
- F5 title 'Twelve own source files and five build files': the evidence lists 7 Python files plus 3 scripts (10) and 5 CMakeLists.txt plus the Dockerfile (6).
- Facts say third_party/weight holds five YOLOX checkpoints. It holds ten files: dancex.pth.tar, mot17l/m/s/x.pth.tar, mot20x.tar, yolox_l.pth, yolox_m.pth, yolox_s.pth, yolox_x.pth.
- not_checked says '159 .txt and 76 .ini tracked data files'. The data folder holds 152 .txt (76 det.txt + 76 gt.txt) and 76 .ini; 159 is every tracked .txt in the repository, including seven CMakeLists.txt.
- Facts say @author 'Mustafa Garip' appears in 82 files. `git grep -h '@author'` outside third_party, docs and tools gives 80 lines of 'Mustafa Garip' and 2 with the e-mail. The difference does not change any conclusion.
- Recomputed and correct: 433 tracked files; header counts 43 .cpp + 27 .hpp + 10 .h full Apache block, 41 .py short form, 2 .py 'All rights reserved', 7 .py none; 25 tracked third_party C/C++ files; 228 data files; one author with 79 commits; 48 Co-Authored-By trailers; no local tags.

### Findings

#### L1. [high] Tracked MOT17, MOT20 and DanceTrack annotation files carry third-party terms and no notice

Evidence: 228 tracked files under tests/regression/evaluator/data (76 sequences: 40 dancetrack*, 28 MOT17-*, 8 MOT20-*; gt/gt.txt, det/det.txt, seqinfo.ini). No LICENSE, NOTICE or README in that folder; docs/README.md lines 1300-1313 list code only. MOTChallenge publishes its data under CC BY-NC-SA 3.0 and DanceTrack under a non-commercial research licence (from upstream knowledge, not read in this tree).

Recommendation: Confirm each dataset's terms at the source, then either add a notice beside the data naming the source and licence, or stop redistributing it and fetch it at test time. The share-alike clause on the MOT ground truth is a term the repository's own licence cannot override; the data needs its own licence statement, separate from the code licence.

Review: confirmed. 228 tracked files under tests/regression/evaluator/data, 76 sequences, no LICENSE/NOTICE/README there; docs/README.md 1300-1313 lists code only. Dataset terms are still from memory, not from a file.

#### L2. [high] source/lapjv.cpp appears derived from the gatagat/lap LAPJV code but carries only the Sushi Systems Apache header

Evidence: source/lapjv.cpp uses the helper names _ccrrt_dense, _carr_dense, _find_dense, _scan_dense, find_path_dense and _ca_dense; include/SushiTrack/lapjv.hpp and source/assignment_solvers.cpp use lapjv_internal. The same seven names appear in third_party/bytetrack/src/lapjv.cpp, which ByteTrack took from gatagat/lap (BSD-2-Clause). Both own files open with 'Copyright (c) 2026-present Mustafa Garip & Sushi Systems' and no attribution. Line-level comparison was not done.

Recommendation: Compare source/lapjv.cpp with the upstream file. If it is a port, keep the upstream copyright and BSD/MIT notice in the file or in a root NOTICE; the owner cannot relicense that part as sole author, and the upstream notice must ship with it under any outbound licence.

Review: confirmed. Read source/lapjv.cpp against third_party/bytetrack/src/lapjv.cpp: same helper names, same LARGE constant, same loop bodies statement by statement (malloc replaced by std::vector, __restrict added). It is a port, not a name collision. No attribution in the file or its header.

#### L3. [high] Ported third-party code goes beyond lapjv.cpp

Evidence: source/rect.cpp is third_party/bytetrack/src/Rect.cpp function for function (same constructor initialiser `tlwh({x, y, width, height})`, same accessor order and bodies), under the Sushi Systems header only. tests/regression/inference/pipeline.py lines 167-190 (`preproc`) match third_party/yolox/yolox/data/data_augment.py lines 142-158 line for line; YOLOX is Apache-2.0, Copyright Megvii. The own file set mirrors ByteTrack-cpp (object.cpp, rect.cpp, kalman_filter.cpp, lapjv.cpp against Object.cpp, Rect.cpp, KalmanFilter.cpp, lapjv.cpp) and source/tracker.cpp keeps ByteTrack names (refind_tracklets, jointTracklets, track_thresh, match_thresh). I compared rect.cpp, lapjv.cpp and preproc by reading; object.cpp, kalman_filter.cpp, tracklet.cpp and tracker.cpp were not compared line by line. The audit put this under not_checked after a keyword search.

Recommendation: Before choosing the licensor wording, diff each own file in source/ and include/SushiTrack/ against its ByteTrack-cpp and OC_SORT_CPP counterpart and list the ported ones. Those files keep the upstream MIT (and, for lapjv, the gatagat/lap BSD-2-Clause) copyright line next to the new header, and pipeline.py keeps the Megvii Apache-2.0 notice for preproc. The new licence then covers the owner's changes only; the owner cannot present these files as sole-author work.

Review: found by review.

#### L4. [medium] Two Python files say 'All rights reserved', which contradicts LICENSE

Evidence: tests/regression/inference/demo.py line 6 and tests/regression/inference/pipeline.py line 6: '@copyright Copyright (c) 2026-present Mustafa Garip & Sushi Systems. All rights reserved.' Both also carry '@version v1.0.0' and an e-mail in @author, which the source-comments skill forbids, and lack the two-line licence header the other 41 Python files use.

Recommendation: Replace both docstring headers with the same header shape as the sibling Python files when the new header lands.

Review: confirmed. tests/regression/inference/demo.py and pipeline.py line 6 read 'All rights reserved.', with @version v1.0.0 and an e-mail in @author; `git grep -l 'All rights reserved'` returns exactly these two.

#### L5. [medium] Twelve own source files and five build files have no licence header

Evidence: Python: cli/sushitrack_cli/__init__.py, cli/sushitrack_cli/__main__.py, cli/sushitrack_cli/console.py, cli/sushitrack_cli/services/__init__.py, cli/tests/conftest.py, cli/tests/test_console_lazy.py, cli/tests/test_service_enums.py. Scripts: cli/misc/clean_preds.sh, cli/misc/clean_preds.bat, cli/misc/construct_trackeval_data.ps1. Build: CMakeLists.txt, tests/CMakeLists.txt, tests/unit/CMakeLists.txt, tests/integration/CMakeLists.txt, tests/regression/CMakeLists.txt, Dockerfile. docs/design/SUSHITRACK_MIGRATION.md line 120 says 'Every C, C++, Python, and shell file receives a standardized Apache-2.0 header'.

Recommendation: Add the new header to the seven Python files and three scripts in the relicensing change. Decide once, for all repositories, whether CMake files and Dockerfiles carry a header, and apply it the same way here.

Review: confirmed. The seven Python files, three scripts, five CMakeLists.txt and Dockerfile carry no header. The title's arithmetic is off: the list holds 10 source files and 6 build files, not 12 and 5. Only the Python files are a defect under source-comments; that skill does not cover CMake, Docker or shell.

#### L6. [medium] Licence declarations disagree between the two Python packages

Evidence: bindings/python/pyproject.toml lines 11 and 19 and bindings/python/setup.py line 16 declare Apache-2.0. cli/pyproject.toml declares no license and no classifiers, though every file in cli/sushitrack_cli that has a header says Apache-2.0. The classifier 'License :: OSI Approved :: Apache Software License' will be false under a non-commercial licence, which is not OSI-approved.

Recommendation: Under the new licence, give both pyproject files the same declaration: a 'license' expression using a LicenseRef or the chosen SPDX id, 'license-files', and no OSI classifier. Remove the duplicate metadata in setup.py or keep it identical.

Review: confirmed. bindings/python/pyproject.toml lines 11 and 19 and setup.py line 16 say Apache-2.0; cli/pyproject.toml has no license key and no classifiers. Dropping the duplicate setup.py metadata is a one-source fix.

#### L7. [medium] Two author e-mail addresses in package metadata, a third in git

Evidence: bindings/python/pyproject.toml line 13 and setup.py line 14: mustafagarip@sushisystems.io. cli/pyproject.toml line 9: garipm@mef.edu.tr. All 79 commits: mustafa.garip3434@hotmail.com. A university address on a package raises the question of whether the institution has any claim on work done under it.

Recommendation: Use one contact address in both packages. The owner should confirm that no employer or university agreement gives a third party rights in the code, since the relicensing rests on sole ownership.

Review: confirmed. mustafagarip@sushisystems.io in bindings/python/pyproject.toml line 13 and setup.py line 14; garipm@mef.edu.tr in cli/pyproject.toml line 9; all 79 commits use the hotmail address.

#### L8. [medium] The copyright holder is written as two parties, 'Mustafa Garip & Sushi Systems'

Evidence: LICENSE line 178 and 122 source headers: 'Copyright (c) 2026-present Mustafa Garip & Sushi Systems'. The form is consistent, but it names a person and an organisation as joint holders; a source-available licence needs one identifiable licensor who can grant commercial exceptions.

Recommendation: Decide who the licensor is (the person, or the company as a legal entity) before writing the new header, and use that single name in LICENSE and headers across all repositories.

Review: confirmed. LICENSE line 178 and 122 header lines read 'Copyright (c) 2026-present Mustafa Garip & Sushi Systems'. The form is what the source-comments template prescribes, so this is a decision for the owner, not a defect in the tree.

#### L9. [medium] Only one git author, but 48 commits record AI co-authorship and the pre-September history has an unclear origin

Evidence: git shortlog -sne: '79 Mustafa Garip <mustafa.garip3434@hotmail.com>', no other human contributor. 48 commit messages carry 'Co-Authored-By: Claude ...'. The project was named SushiTrack until 2026-09-17 (docs/design/SUSHITRACK_MIGRATION.md line 6; .gitignore line 4 '.sushitrack_env.json') and shipped with an empty LICENCE file from d479857.

Recommendation: No contributor consent is needed from the git record. The owner should confirm that SushiTrack was not written for or assigned to another party; that is the only ownership question the history leaves open.

Review: confirmed. git shortlog -sne HEAD and --all: 79 commits, one author, committer identical; 48 commits carry a Co-Authored-By trailer; docs/design/SUSHITRACK_MIGRATION.md line 5 names SushiTrack.

#### L10. [medium] Eigen is MPL-2.0, a file-level copyleft that keeps its own terms under any outbound licence

Evidence: third_party/eigen submodule at 52f99a14; third_party/eigen/COPYING.README: 'Eigen is primarily licensed under the Mozilla Public License 2.0'. Own code includes only <Eigen/Dense> (6 includes). EIGEN_MPL2_ONLY is not defined in CMakeLists.txt. MPL-2.0 is not in the accepted list of the dependencies skill (MIT, BSD, Apache-2.0, Zlib, ISC, public domain).

Recommendation: MPL-2.0 does not conflict with a non-commercial licence on the surrounding code, but Eigen files stay MPL-2.0 and the notice must say so. Define EIGEN_MPL2_ONLY so no non-MPL Eigen code can be pulled in, and record the owner's approval of MPL-2.0 since the skill's table does not cover it.

Review: confirmed. third_party/eigen/COPYING.README states MPL-2.0 primary; six own files include <Eigen/Dense>; EIGEN_MPL2_ONLY has no match outside the submodule; MPL-2.0 is absent from the dependencies skill table. docs/README.md line 1311 already records Eigen as MPL-2.0.

#### L11. [medium] Third-party notices are incomplete for a distribution

Evidence: No root NOTICE or THIRD_PARTY file. third_party/nlohmann/ holds only json.hpp, with no LICENSE file and no README giving source URL and version. third_party/bytetrack and third_party/ocsort carry LICENSE and NOTICE but not the README.md the dependencies skill requires. The submodule folder names in .gitmodules still read '3rdparty/TrackEval', '3rdparty/eigen', '3rdparty/yolox'. cli/sushitrack_cli/services/deploy.py builds deploy packages; whether they bundle any licence file was not read.

Recommendation: Add one third-party notice at the root listing every vendored and submodule component with licence and copyright line, add the MIT text beside json.hpp, and make the deploy packages include LICENSE plus that notice.

Review: confirmed. third_party/nlohmann holds only json.hpp; bytetrack and ocsort have LICENSE and NOTICE but no README.md; .gitmodules section names read 3rdparty/...; no root NOTICE. I read cli/sushitrack_cli/services/deploy.py: it copies headers, libraries and sushitrack.json and no licence file (see missed).

#### L12. [medium] Docker image and conda environment pull NVIDIA components under proprietary terms

Evidence: Dockerfile line 1 'FROM nvidia/cuda:12.1.1-devel-ubuntu22.04' (NVIDIA Deep Learning Container licence, CUDA EULA); line 31 torch 2.5.1 cu121 wheels bundle CUDA and cuDNN libraries; environment.yml lists 'pytorch-cuda=12.1' from the nvidia channel. These are vendor SDK terms, which the dependencies skill's table rejects.

Recommendation: The repository does not redistribute these binaries, so the source licence is unaffected, but do not publish a built image. State in the docs that the image and the CUDA stack are used under NVIDIA's terms. They are needed only for the YOLOX inference path under tests/regression/inference.

Review: confirmed. Dockerfile line 1 is nvidia/cuda:12.1.1-devel-ubuntu22.04 and line 31 installs torch 2.5.1 from the cu121 index; environment.yml lists the nvidia channel and pytorch-cuda=12.1.

#### L13. [medium] Deploy packages ship binaries and headers with no licence file

Evidence: cli/sushitrack_cli/services/deploy.py lines 171-233 copy the public headers, the import library, the runtime library, the Python binding and sushitrack.json into the package directory. No line copies LICENSE or any third-party notice (`grep -n -i 'licen|notice'` matches only the file's own header). The library compiles in third_party/nlohmann/json.hpp (MIT) and the ported ByteTrack code, whose licences require the notice to travel with copies. The audit named this file in F11 and left it unread.

Recommendation: Give deploy.py one function that copies LICENSE and the third-party notice into every package, called from both the C++ and the Python package builders, so the two package shapes stay the same.

Review: found by review.

#### L14. [medium] The relicensing can be clean, and the audit's cut-over advice would give that away

Evidence: The repository is private, untagged, unreleased and absent from PyPI (see wrong_facts). F1 recommends treating eaa452c..HEAD as permanently Apache-2.0 and announcing a cut-over version. With no public recipient, no Apache-2.0 grant has been made to anyone outside. docs/reference/CHANGELOG.md line 37 ('Released initial Apache-2.0 open-source version') describes a release that did not happen publicly.

Recommendation: Change the licence before the repository is made public or any package is uploaded, and the Apache-2.0 period never becomes a public grant. The owner should confirm that no copy was handed to a third party (collaborators on the private repo, a shared Docker image, a sent archive). Decide whether the first public version keeps the 1.0.0 number; the changelog line stays as history under the documentation rules.

Review: found by review.

#### L15. [low] YOLOX checkpoints and MOT video clips sit under third_party/ without a licence record

Evidence: D:/Projects/sushitrack/third_party/weight (dancex.pth.tar, mot17l/m/s/x.pth.tar, about 2.2 GB) and third_party/video (MOT17-04.mp4, MOT17-10.webm, MOT17-11.webm, MOT17-13.webm, MOT20-01.webm, MOT20-05.mp4). Both are gitignored, so they are not distributed. The mot17*/dancex weights are ByteTrack/OC-SORT model-zoo files trained on MOT data.

Recommendation: Keep them untracked. If documentation tells users where to download them, name the source and its terms there.

Review: confirmed. Both folders exist and git status is clean, so they are ignored. third_party/weight holds ten files, not the five listed: also mot20x.tar and yolox_l/m/s/x.pth.

#### L16. [low] Every prose statement of the licence names Apache-2.0 and will go stale together

Evidence: docs/README.md lines 1284-1298; docs/CONTRIBUTING.md line 81; docs/design/SUSHITRACK_MIGRATION.md lines 6, 13, 18, 28, 120-140. Root README.md states no licence at all. The agent records under docs/agent/2026_09_17_SUSHITRACK_MIGRATION/ also name Apache-2.0 but are execution history.

Recommendation: Rewrite the docs/README.md and CONTRIBUTING.md statements in the relicensing commit, add a licence line to the root README.md, and record the change in a new design document; leave the agent records and the v1.0.0 changelog line as they are, since they describe what was true then.

Review: confirmed. docs/README.md 1284-1298, docs/CONTRIBUTING.md line 81 and docs/design/SUSHITRACK_MIGRATION.md lines 6 and 120 read as cited; root README.md has no licence match.

#### L17. [low] sushicore dependency pins disagree and its licence will change with the same programme

Evidence: cli/pyproject.toml line 22 'sushicore>=0.7.0'; environment.yml 'sushicore==0.4.0'; the interpreter on this machine has sushicore 0.5.0 with License-Expression Apache-2.0.

Recommendation: Align the two pins. When sushicore is relicensed, the CLI inherits a non-commercial dependency; the versions already on PyPI stay Apache-2.0.

Review: confirmed. cli/pyproject.toml line 22 'sushicore>=0.7.0', environment.yml line 65 'sushicore==0.4.0'. The installed-version claim (0.5.0) was not rechecked. sushicore exists on PyPI (HTTP 200), so its published versions are the ones that stay Apache-2.0.

### Refuted

- v1.0.0 is already published under Apache-2.0 and that grant cannot be withdrawn: The changelog and LICENSE evidence exists, but the publication premise fails. `gh repo view SushiSystems/SushiTrack` returns visibility PRIVATE, with no tags and no releases; pypi.org/pypi/sushitrack and /sushitrack-cli both return 404. Nothing under Apache-2.0 has reached the public, so there is no grant to withdraw. The cited HEAD 51f6bde is not an object in this repo; HEAD is 87cfedd and is one commit ahead of origin/main.

### Not checked

- Whether the GitHub repository SushiSystems/SushiTrack is public, and whether it has remote tags or GitHub releases (no network call made; no local tags exist).
- Whether 'sushitrack' or 'sushitrack-cli' is published on PyPI (not queried).
- Line-by-line comparison of source/lapjv.cpp with third_party/bytetrack/src/lapjv.cpp or gatagat/lap; F3 rests on seven shared helper names only.
- Whether other own files (kalman_filter, tracker association, OCM/ORU logic) are ports of ByteTrack or OC-SORT code; only a keyword search was run.
- The exact licence text of the MOT17, MOT20 and DanceTrack datasets; the terms named in F2 come from memory of the upstream sites, not from a file in the tree.
- Licences of the roughly 60 conda and pip packages in environment.yml, one by one; none is vendored.
- Contents of cli/sushitrack_cli/services/deploy.py: whether deploy packages bundle LICENSE or third-party notices.
- Per-file licences inside the eigen, TrackEval and yolox submodules beyond their top-level licence files.
- Files under build/, .pytest_cache/ and the three untracked cmake_test_discovery_*.json files at the root.
- The full text of docs/ pages other than the lines returned by the licence keyword search.
- The 159 .txt and 76 .ini tracked data files were counted, not opened.

## Command line

### Facts

- Entry points: console scripts `st` and `sushitrack`, both -> `sushitrack_cli.cli:entry` (D:/Projects/sushitrack/cli/pyproject.toml:30-32); `python -m sushitrack_cli` via `__main__.py`. `entry()` sets the program name from argv[0] and turns a bare invocation into `--help` with exit 0.
- Package: `sushitrack-cli` version 0.1.0 in cli/pyproject.toml:7, while `sushitrack_cli/__init__.py:3` says `__version__ = "2.0.0"`. requires-python >=3.10. bindings/python is a separate ctypes package (`sushitrack` 1.0.0, setup.py) with no console script.
- Framework: Typer (`typer>=0.12`), `rich>=13.0`, `sushicore>=0.7.0` (sushicore's own pyproject is at 0.7.0). App: `typer.Typer(name="st", cls=help_group(console.current), no_args_is_help=True, add_completion=False, rich_markup_mode="rich")`.
- Command tree, panel Project: `build` (--type/-t, --no-test, --clean, --jobs/-j, --deploy, -D); `test` (--suite/-s, --filter/-f, --repeat/-r, --jobs/-j); `run [TARGET]` (--select/-s, args after --); `clean`; `deploy {cpp|python}`; `docker build` (--no-cache, extra args), `docker run` (--no-gpu, command after --).
- Command tree, panel Benchmark: `eval` (--scenario, --tracker, --compare, --eval-only, --trackeval, --export, --private, --public); `infer` (--source, --tracker, --name/-n, --ckpt/-c, --device, --conf, --nms, --tsize H W, --num-classes, --legacy, --stress, --no-save, --save_name, --mot-label, --no-screen, --reid, --reid-model, extra args).
- Command tree, panel Diagnostics: `status`, `config`, `paths`, `env` (--all/-a). From sushicore with no panel given (default panel): `setup` (--dry-run, --yes, --toolchain, --no-gpu), `doctor` (--for), `link` (--workspace), `unlink` (--workspace).
- From sushicore: `register_provision_commands`/`ModuleProvision` (setup, doctor, link, unlink), `typer_help.help_group` (help page), `cli_console.LazyConsole`, `module_config.ModuleConfig`, `profile.ModuleProfile`, `config_base.load_tool_config`, `provision.config.ProvisionSettings`, `proc.Runner`, `cmake_driver.CMakeDriver`, `cmake_cache`, `build_env`, `discovery.ExecutableIndex`, `provision.checks`/`doctor`, `provision.probe.find_vcvars`, `workspace.read_toml`.
- Local: the sectioned `Config` loader with its own `ENV_OVERRIDES`, `_merge_platform`, `_coerce` (config.py); `Paths`; `find_file`/`find_binary` (services/artifacts.py); `status`, `config_show`, `paths_show`, `env_dump` (services/diag.py); build stamp logic; deploy, docker, eval, infer services; `st_checks` (eigen, docker, yolox, torch, torchvision, cv2, pycocotools, TrackEval, datasets, numpy, scipy).
- Help panels: three constants K_PROJECT / K_BENCHMARK / K_DIAGNOSTICS in cli.py:36-38; `docker` sub-app sits under Project. Every local command has a docstring and an `epilog` of example lines. The page is drawn by sushicore's HelpPage, falling back to Typer's help on error.
- Output: Rich through sushicore's Console (header, info, warn, error, success, table, command). No `--json`, no `--describe`, no machine mode: `LazyConsole.machine` is never set. `st eval` prints captured child stdout/stderr with bare `print` on failure (label.py:104-105), and the evaluator's `print_trackeval_table` prints on its own.
- Version flag: none. No `--version` option or callback exists in cli.py; `__version__` is not read anywhere under cli/.
- Exit codes: child exit code passed through for build/test/run/docker/infer; 1 for a missing tree, binary, script or failed deploy/eval; 2 for usage errors raised by services (`--private` with `--public`, missing `--source`, unknown doctor group, unknown deploy kind); 127 for a missing tool (process.py:13); 130 on Ctrl+C through the Runner; `doctor` returns 1 when a required check fails; bare `st` returns 0.
- Error reporting: services call `console.error(...)` and return a code. Three paths bypass the console and raise `SystemExit(<message>)`: outside a checkout (sushicore `find_project_root`), unknown build type (build.py:44), unknown tracker (label.py:26); Python prints the message to stderr unstyled and exits 1.
- Colour: `[cli] color = auto|always|never` in cli/config.toml, `ST_COLOR` / `ST_THEME` / `ST_ICONS` env overrides, and `NO_COLOR` honoured in sushicore/config.py:112. There is no `--no-color` command-line flag.
- Help wording: imperative, sentence case, ending in a period ("Configure and build the project.", "Remove the build tree and the assembled package directory."). The diagnostics commands use "Show" once and "Print" three times. Root help uses an em dash: "SushiTrack developer CLI — build, test, evaluate, run and Docker."
- Platform handling: `IS_WINDOWS` in config.py:46 picks `.exe`, `sushitrack.dll` vs `libsushitrack.so`, and the vcvars snapshot. Generator defaults to Ninja on Windows. `[platform.<os>]` overlays are supported in config.toml.

### Corrections from review

- F8 says `st status` and `st paths` exist in no sibling. `se` has `status` (D:/Projects/sushiengine/cli/sushiengine/cli.py:861) and `where` (:873).
- F18 says no service file except artifacts.py has `from __future__ import annotations`. services/checks.py:5 has it as well: 2 of 11 service modules carry it, 9 do not (the list of nine in the finding is correct).
- F17 says sushicore's `unlink` has no example. commands.py:317 with :255 gives it the epilog `st unlink`.
- F13 title says `doctor --for test` has no check. sushicore provision/checks.py:192 registers `ctest` in group `test`; only st's own list (services/checks.py:49-64) lacks one. fragment_check is fixed to group `build` (checks.py:131) and skips fragments with no check_cmd (:115-116), so GoogleTest is never checked.
- F6 cites sushicore/diag.py:42-145 for the Diagnostics class. The class spans 42-176; env_dump starts at 145.
- F5 says `except Exception` at evaluate.py:61 violates the Python style rule. The rule forbids bare `except:` and `except Exception: pass`; line 62 logs and returns 1.
- F1 counts seven cut comments and lists seven; services/tests.py:13 is an eighth.
- F2 says two version numbers. The repository has three: CLI package 0.1.0 (cli/pyproject.toml:7), `__version__` 2.0.0 (__init__.py:3), library and binding 1.0.0 (CMakeLists.txt:2, bindings/python/setup.py:11).
- Facts recomputed and found correct: sushicore requirement >=0.7.0 against sushicore pyproject 0.7.0; the command tree and options in cli.py (16 local commands plus setup/doctor/link/unlink); NO_COLOR at sushicore/config.py:112; `st infer` flags match demo.py:39-59 one for one.

### Findings

#### C1. [high] Truncated comments and a corrupted C example inside the deploy template

Evidence: D:/Projects/sushitrack/cli/sushitrack_cli/services/deploy.py:35 reads `# include <stdio.h> include "SushiTrack/sushitrack_c.h"` inside the `_CPP_EXAMPLE` string, so the `example/example.c` that `st deploy cpp` writes does not compile (the second `#include` lost its `#` and its line break). deploy.py:123, inside `_PY_EXAMPLE`, ends mid-word: `...package directory and disco`. The same cut at about 80 characters appears in plain comments: services/build.py:28 (`rather than building th`), build.py:35 (`A single-config gene`), services/diag.py:17 (`the ones that act`), services/evaluate.py:11 (`everything el`), evaluate.py:25 (`TrackEval); name`), config.py:98 (unclosed parenthesis).

Recommendation: Restore both generated templates from git history (two separate `#include` lines; a whole comment in the Python example) and add a test that compiles or at least parses the emitted example. Rewrite or delete the seven cut comments; whatever tool collapsed them must not touch string literals.

Review: confirmed. Read services/deploy.py:35 and :123, build.py:28 and :35, diag.py:17, evaluate.py:11 and :25, config.py:98, tests.py:13; every quoted cut is there. deploy.py:35 drops the sushitrack_c.h include, so example.c fails on the undeclared sushitrack_tracker_t. tests.py:13 (`The single categories anchor`) is an eighth cut the audit did not list.

#### C2. [high] No `--version` flag, and two different version numbers

Evidence: cli/sushitrack_cli/cli.py declares no `--version` option or callback (grep for `version` under cli/sushitrack_cli finds only `__init__.py:3`). `__init__.py:3` says `__version__ = "2.0.0"`; cli/pyproject.toml:7 says `version = "0.1.0"`. The sibling `se` has an eager `--version` (D:/Projects/sushiengine/cli/sushiengine/cli.py:103-114, with its own `version.py`).

Recommendation: Give every Sushi CLI one `--version` built in sushicore (program, package version, sushicore version, project root) and register it the way `register_provision_commands` is registered. Keep one version source for st: read it from package metadata and delete the stale constant.

Review: confirmed. cli.py has no --version; __init__.py:3 says 2.0.0, cli/pyproject.toml:7 says 0.1.0; se has the eager flag at sushiengine/cli/sushiengine/cli.py:103-115. The library is a third number: CMakeLists.txt:2 and bindings/python/setup.py:11 both say 1.0.0. Recommendation is brick-shaped but is a sushicore public-interface decision for the owner.

#### C3. [high] Installation instructions contradict each other and the code about sushicore

Evidence: D:/Projects/sushitrack/docs/README.md:303-308 tells the user to `pipx inject sushitrack-cli --editable /path/to/sushistack/sushicore` because "`sushicore` is not published to any index — it ships inside the `sushistack` repository". sushicore is its own repository at D:/Projects/sushicore, and docs/design/CLI_TYPER_MIGRATION.md:48 states "PyPI carries `sushicore` 0.4.0". D:/Projects/sushitrack/environment.yml:65 pins `sushicore==0.4.0` while cli/pyproject.toml:20 requires `sushicore>=0.7.0`, so the documented conda environment cannot satisfy the CLI. The Dockerfile (line 39) runs `pip3 install /workspace/sushitrack/cli` with no sushicore source configured.

Recommendation: State one install path that is true today (where sushicore 0.7.0 comes from), raise the environment.yml pin to match pyproject, and make the Dockerfile install sushicore the same way.

Review: confirmed. docs/README.md:303-308, docs/design/CLI_TYPER_MIGRATION.md:48, environment.yml:65 (sushicore==0.4.0), cli/pyproject.toml:20 (>=0.7.0) and Dockerfile:39 all read as quoted. Whether 0.7.0 is on an index was not checked by me either.

#### C4. [medium] Three user errors skip the console and exit through bare SystemExit

Evidence: services/build.py:44 `raise SystemExit(f"Unknown build type ...")` (reachable from a bad `[build] type` in config.toml or `ST_BUILD_TYPE`, in both `st build` and `st test` via tests.py:38); services/label.py:26 `raise SystemExit(f"Unknown tracker ...")` for `st eval --tracker x` and `st infer --tracker x`; sushicore module_config.py:75 for any command run outside a checkout. Each prints an unstyled line with exit 1, while every other usage error in this CLI goes through `console.error` and returns 2 (label.py:115-116, infer.py:33-34). A service raising `SystemExit` also breaks the rule that a service returns a code.

Recommendation: Declare `--tracker` as an Enum like `--type` and `--suite` so Typer rejects it with exit 2. Have services raise a module error and let `entry()` catch it, print one `console.error` line and return 2. Settle the same policy in sushicore for the not-in-a-project case so all seven CLIs match.

Review: confirmed. build.py:44, label.py:26, tests.py:38, sushicore module_config.py:75 confirmed. Two corrections: entry() returns the message string, not an int, on these paths (cli.py:395-396, against its own docstring); and the Enum recommendation for --tracker contradicts the recorded decision in CLI_TYPER_MIGRATION.md:39 without saying so. The 'raise a module error, catch in entry()' half is the brick-shaped fix.

#### C5. [medium] Unhandled exceptions that reach the user as a traceback

Evidence: `entry()` (cli.py:393-397) catches only `SystemExit`. (1) services/build.py:85 and :130 read and write the stamp file, and build.py:123 `mkdir`, with no OSError handling. (2) services/deploy.py:192-237 runs `shutil.copy2`/`write_text` unguarded; a locked `sushitrack.dll` on Windows raises PermissionError. (3) services/evaluate.py:68-70 calls `print_table` and `export_results` outside the `try` that ends at line 63, and lines 96-107 and 144 copy files unguarded. (4) services/evaluate.py:57 computes `1_000_000.0 / micros`; a `0 us/frame` PERF line gives ZeroDivisionError, though that one is inside the broad `except Exception` at line 61, which itself violates the Python style rule. (5) config.py:230 `read_toml` on a malformed config.local.toml: not verified whether sushicore converts the TOMLDecodeError.

Recommendation: Add one boundary in `entry()` that catches the CLI's base error and OSError, prints one line and returns 1; wrap the file operations in deploy and evaluate in the module's own error; replace `except Exception` with the specific errors the evaluator raises.

Review: confirmed. entry() catches only SystemExit (cli.py:393-397); unguarded file operations at build.py:85/123/130, deploy.py:187-237, evaluate.py:68-70, 96-107, 144 are as cited. Item (5) is resolved: sushicore workspace.py:42-47 calls tomllib.load with no handler, so a malformed config.local.toml is a traceback. Overstated: evaluate.py:61 logs and returns 1, which the style rule allows (it forbids bare except and `except Exception: pass`). SOLID: catching bare OSError in entry() is the blanket half of the recommendation; wrapping file operations in the module's own error is the brick.

#### C6. [medium] `config`, `env`, executable lookup and env-token list duplicate what sushicore provides

Evidence: sushicore/diag.py:42-145 has a `Diagnostics` class with `config_show` and `env_dump`; st reimplements both in services/diag.py:113-160. services/diag.py:18-21 keeps its own `_ENV_OF_INTEREST` tuple while `PROFILE.env_of_interest=("EIGEN", "SUSHITRACK")` (config.py:31) and sushicore's `ModuleProfile.env_tokens()` (profile.py:105-113) are never read, so that profile field is dead. config.py:115-130 keeps a local `ENV_OVERRIDES` table, `_merge_platform` and `_coerce` beside sushicore's `load_tool_config`. services/artifacts.py:37-61 walks the build tree by hand next to `sushicore.discovery.ExecutableIndex` (line 19). `Appearance` and `load_appearance` (config.py:49-57, 238-260) have no caller under cli/sushitrack_cli.

Recommendation: Owner decision on the boundary: either sushicore's `Diagnostics` learns a sectioned config, or st's config flattens. Then drop the local `config`/`env` renderers, read env tokens from the profile, and delete `Appearance`/`load_appearance` if no test needs them.

Review: confirmed. sushicore diag.py has Diagnostics.config_show (111) and env_dump (145; the class runs 42-176, not 42-145); st's copies are services/diag.py:113-160. env_tokens() is read only by sushicore diag.py:159, so PROFILE.env_of_interest is dead in st. Appearance/load_appearance have no caller in cli/ or cli/tests. The local diag is a recorded deliberate deviation (CLI_TYPER_MIGRATION.md:54), and the docstring line that section asked for is absent from diag.py:3.

#### C7. [medium] Shared provision commands land in an unnamed help panel

Evidence: cli.py:363-369 builds `ModuleProvision(...)` without `panel=`, so `setup`, `doctor`, `link`, `unlink` are registered with `rich_help_panel=None` (sushicore provision/commands.py:92, :256) and fall outside the Project / Benchmark / Diagnostics panels. cli/tests/test_help_screen.py:12-16 lists only the three panels and line 92-95 checks merely that the four names appear somewhere.

Recommendation: Fix the panel name for the provision commands once across all CLIs (in sushicore's default or in each `ModuleProvision`), and assert it in the help-screen test.

Review: confirmed. cli.py:363-369 passes no panel; commands.py:92 and :256 as cited. st is the only outlier: sa (cli.py:256), sb (:175), sr (:288) pass K_DIAGNOSTICS, sd (:175) and se (:984) pass K_ENVIRONMENT. The fix is one argument in st, plus the test assertion; no sushicore change is needed.

#### C8. [medium] Command and option names differ from the sibling CLIs

Evidence: `st infer` vs `sa inference` (D:/Projects/sushiai/cli/sushiai/cli.py:204). `st deploy` vs `sr package` (D:/Projects/sushiruntime/cli/sushiruntime/cli.py:174) for assembling a redistributable tree. `st status` and `st paths` exist in no sibling (sa, sb, sd, sr have `config` and `env` only). Siblings sa, sb, sd, sr all have `doxygen`; st has none. `sr build` offers `--force` and `sr test`/`run`/`env` offer `--asan`; st has neither. `st run --select/-s` and `st test --suite/-s` give `-s` two meanings; `sr run` uses `--sort` (sushiruntime cli.py:156).

Recommendation: Write the shared command vocabulary down once (build, test, run, clean, config, env, doctor, setup, link, unlink, docker, plus agreed names for package/deploy, infer/inference, status, paths) and rename against it under the api-stability rules. This is a public-surface decision for the owner.

Review: confirmed. sa `inference` (cli.py:204), sr `package` (:174), sr --force/--asan/--sort and doxygen in sa/sb/sd/sr all check out. One claim is wrong: `st status` does exist in a sibling, se (sushiengine cli.py:861), and se `where` (:873) is the counterpart of `st paths`. Missed in the same vein: the picker flag has three spellings (st --select, sr/sa/sb --sort, se --pick with --sort deprecated at :252-255), and se calls the sub-app `container` with `docker` a hidden alias (:136, :145).

#### C9. [medium] Option naming inconsistent inside `st infer` and across commands

Evidence: cli.py:306 `--save_name` uses an underscore while every other long option uses hyphens (`--no-save`, `--mot-label`, `--reid-model`, `--num-classes`). cli.py:295 `--device` is a free string documented as "cpu or gpu." and cli.py:243-245, 290-292 `--tracker` is a free string documented as three choices, while `--type`, `--suite` and `--deploy` are Enums. cli.py:288-289 declares `--source` as optional with help "(required)" and the check happens later in infer.py:32-34, after `demo.py` existence is tested at :29-31. `--private`/`--public` are two booleans checked for conflict in the service (label.py:114-116) instead of one choice.

Recommendation: Rename to `--save-name` (keep the old spelling as a deprecated alias for one release), make `--device` and `--tracker` Enums, mark `--source` required in Typer, and replace the two detection flags with one `--detections {all,private,public}` or keep them and reject the pair in the command layer.

Review: confirmed. cli.py:306 --save_name, :295 --device, :243/:290 --tracker, :288-289 --source, label.py:114-116 all as cited. --save_name mirrors demo.py:43; infer.py:55 already maps it, so the rename touches only cli.py. The --tracker Enum again runs against CLI_TYPER_MIGRATION.md:39.

#### C10. [medium] No machine-readable output and no `--describe`

Evidence: sushicore's `LazyConsole` has a `machine` switch that selects the JSON renderer (D:/Projects/sushicore/sushicore/cli_console.py:39-49); nothing in cli/sushitrack_cli sets it, and no command declares `--json` or `--describe`. services/diag.py:53, 60, 123, 131, 133, 156 build Rich markup strings and call `console.console.print` directly, and services/runner.py:22, 27 uses Rich markup and a raw `input()`, which would bypass a JSON renderer. services/label.py:104-105 uses bare `print`.

Recommendation: Decide in sushicore whether every CLI gets a global `--json` (and `--describe` if hub needs it), then route st's status/config/paths/env rows and prompts through the Console API instead of raw Rich and `print`/`input` (sushicore's Console already lists `prompt`).

Review: confirmed. cli_console.py:39-49 and the raw Rich/print/input sites (diag.py:53, 60, 123, 131, 133, 156; runner.py:22, 27; label.py:104-105) are as cited; no --json or --describe in cli.py.

#### C11. [medium] Documentation names files, variables and a status that the code no longer has

Evidence: docs/README.md:412 lists `ST_PROJECT_ROOT`; no such variable is read in cli/sushitrack_cli or in D:/Projects/sushicore/sushicore (grep: no matches). docs/README.md:1227 lists a `discovery` service; the file is `services/artifacts.py`, and `checks.py` is missing from that row. docs/README.md:966 says the PERF line is read by `cli/sushitrack_cli/services/tests.py`; the parser is `services/label.py:69-88`. docs/README.md:328 and :1233 say `st` is symlinked into `/usr/local/bin` and installed with pipx; Dockerfile:35-39 says there is no symlink step and uses `pip3 install`. docs/README.md:268-270 lists `docs/ARCHITECTURE.md`, `docs/CHANGELOG.md`, `docs/benchmarks/PRIVATE.md`; the files are `docs/architecture/OVERVIEW.md` and `docs/reference/CHANGELOG.md`. docs/README.md:1228 omits the `[tool]` section present in cli/config.toml:39-45.

Recommendation: Correct each line in the same change as the CLI cleanup, and move the CLI reference out of the 1200-line docs/README.md into docs/guides/ so one page tracks the command surface.

Review: confirmed. docs/README.md:412, :966, :1227, :1228, :328, :1233, :268-270 read as quoted; no PROJECT_ROOT in sushicore; docs/ holds only architecture/OVERVIEW.md and reference/{BENCHMARKS,CHANGELOG}.md. The :1227 row also omits `artifacts`. docs/guides/ does not exist yet, so the recommendation creates it.

#### C12. [medium] Design document still open for a migration that has landed, with stale targets

Evidence: docs/design/CLI_TYPER_MIGRATION.md:3 and docs/design/README.md:8 say "Open — phase 0 of 1", yet cli/sushitrack_cli/cli.py is the Typer layer the document specifies and `app.py` is gone (only a stale `__pycache__/app.cpython-313.pyc` remains). The same document, line 48, sets the floor at `sushicore>=0.4.0`; pyproject.toml:20 is at `>=0.7.0`. Line 24 names `services/discovery.py`, which no longer exists.

Recommendation: Mark the document done and move it to docs/archive/ per the documentation rules, or update its status line and the version it names.

Review: confirmed. CLI_TYPER_MIGRATION.md:3 and design/README.md:8 say 'Open - phase 0 of 1'; cli.py is the Typer layer; app.py survives only as an untracked .pyc (git ls-files shows no pycache). Lines 24 and 48 are stale as cited.

#### C13. [medium] `doctor --for test` is advertised but st registers no test check, and googletest's default is described two ways

Evidence: sushicore doctor.py:11 `GROUPS = ("build", "test", "infer", "eval")` and docs/README.md:402 list a `test` group; services/checks.py:49-64 registers st checks only under `build`, `infer` and `eval`. cli/sushistack.deps.toml:25 says the test targets are "Off the build by default (BUILD_UNIT_TEST=OFF and friends)" while cli.py:68-70 and build.py:94 turn all three test options ON unless `--no-test` is passed, so a missing GoogleTest fails the default `st build` yet is declared `required = false`. Whether sushicore's `standard_checks` or `fragment_check` put anything in the `test` group was not read.

Recommendation: Move the googletest fragment check into the `test` group (or add an st check there) and reword the fragment description to match the CLI default; decide whether `required` should be true given `st build` builds tests by default.

Review: confirmed. checks.py:49-64 has no `test` entry and sushistack.deps.toml:25 contradicts build.py:94 as cited. Two corrections: `doctor --for test` is not empty, because sushicore standard_checks puts ctest there (provision/checks.py:192); and the stated fix is not available in st, since fragment_check hard-codes group `build` (checks.py:131). Worse than reported: the googletest fragment has no check_cmd, and fragment_check skips such entries (checks.py:115-116), so doctor never looks for GoogleTest at all.

#### C14. [medium] A malformed config file is a traceback on every command

Evidence: D:/Projects/sushicore/sushicore/workspace.py:42-47 `read_toml` calls `tomllib.load` with no handler; D:/Projects/sushitrack/cli/sushitrack_cli/config.py:230 and :252 call it for config.toml and config.local.toml, and `context()` (config.py:314-318) runs at the top of every command. The audit left this as 'not verified' in F5 and not_checked.

Recommendation: Catch TOMLDecodeError in sushicore's `read_toml` and re-raise the module's own config error naming the file and line; `entry()` prints it as one line. One fix covers all seven CLIs.

Review: found by review.

#### C15. [medium] Commands report success or exit 0 when nothing was done

Evidence: services/label.py:120-122: `st eval` with no matching sequence (a typo in --scenario, an empty data root) warns and returns 0. services/build.py:150-158: `st clean` calls `shutil.rmtree(..., ignore_errors=True)` and then prints `Removed: build` without checking; the same silent rmtree precedes configure at build.py:112 and :122 and deploy at deploy.py:251, so a locked file on Windows leaves a half-deleted tree and a success line.

Recommendation: Return non-zero from `st eval` when a --scenario filter matches nothing. After each rmtree, check the directory is gone and report the path that survived; drop ignore_errors in favour of an error the service raises.

Review: found by review.

#### C16. [medium] `st doctor` never checks GoogleTest, which the default `st build` needs

Evidence: cli/sushistack.deps.toml:24-28 declares googletest with no `check_cmd`; sushicore provision/checks.py:115-116 skips any fragment entry without one, and services/checks.py:49-64 adds no GoogleTest check. build.py:94 turns the three test options ON by default. docs/README.md:402 presents doctor as the readiness check per group.

Recommendation: Give the fragment a check_cmd or add an st check for GoogleTest in the `test` group, required when tests are built by default. This replaces F13's 'move the fragment check' recommendation, which sushicore's fixed `build` group does not allow.

Review: found by review.

#### C17. [medium] Services write to the terminal and return codes; the style rule forbids both

Evidence: python-code-style: 'A service never imports the command framework or writes to the terminal' and 'The entry point catches the module base error, prints one line, exits non-zero.' Every service imports `console` and prints (build.py:12, deploy.py:14, diag.py:10, docker.py:8, evaluate.py:8, infer.py:12, label.py:7, runner.py:5, tests.py:9), holds module-level singletons (build.py:51 `DRIVER`, process.py:29 `RUNNER`, artifacts.py:19 `EXECUTABLES`), and no module base error exists anywhere under cli/sushitrack_cli. F18 names layout and annotations only; F4 and F5 treat the symptoms.

Recommendation: Owner decision, since the sibling CLIs share the shape: either the skill is amended to allow a console passed in through the constructor, or services become classes that take the console and runner as dependencies, raise a `SushiTrackCliError`, and leave printing to the command layer. F4, F5 and F10 all reduce to this one change.

Review: found by review.

#### C18. [medium] Shared `setup` help offers SYCL toolchain options that SushiTrack does not use

Evidence: sushicore provision/commands.py:261-272 gives `st setup` the example `st setup --toolchain adaptivecpp  # Also install a second SYCL toolchain`, `--yes` for 'the LLVM-download prompt' and `--no-gpu` for 'the toolkit for this machine's GPU'. cli/sushistack.deps.toml:5-10 states SushiTrack consumes none of the shared SYCL toolchain. `--no-gpu` also means something else in `st docker run` (cli.py:213-215). docs/README.md:401 documents only `--dry-run`.

Recommendation: Let ModuleProvision declare whether the module has toolchains (it already has `uses_base`), and register the toolchain options and examples only then. Document the options st keeps in the CLI reference.

Review: found by review.

#### C19. [medium] CLI reference omits options and whole commands that exist

Evidence: docs/README.md:386-557 has sections for build, test, eval, infer, clean and docker only. `st run --select/-s` (cli.py:150-152), `st env --all/-a` (cli.py:354-356), `st setup --yes/--toolchain/--no-gpu`, and the exit-code contract (2 usage, 127 missing tool, 130 interrupt) appear nowhere in the file (grep for `--select`, `env --all`, `--toolchain`, `--yes` returns no match). docs/guides/ does not exist, so there is no CLI guide page at all.

Recommendation: When the reference moves to docs/guides/, generate or hand-check one section per command against cli.py, including run, deploy, status, config, paths, env and the four provision commands, and state the exit codes once.

Review: found by review.

#### C20. [low] Hard-coded paths and layout assumptions

Evidence: services/deploy.py:220 hard-codes `paths.root / "bindings" / "python" / "sushitrack.py"` while every other directory comes from `[paths]`. services/evaluate.py:113 hard-codes `paths.third_party / "TrackEval"` and line 93 falls back to MOT17 for any non-MOT20 name, so DanceTrack sequences are staged as MOT17. services/evaluate.py:28-29 tells the user to run `conda env create -f environment.yml`. services/artifacts.py:15 omits `MinSizeRel` from `_CONFIG_DIRS` though `--type minsizerel` is offered. config.py:272 fixes `cli` as `root / "cli"`. cli/misc/clean_preds.sh:12 and clean_preds.bat:8 point at `cli/config.json`; the file is `cli/config.toml`, and both comments say "three levels up" while the script goes two.

Recommendation: Add `bindings` to `[paths]`, add `MinSizeRel` to the search list, report an explicit error for a sequence that belongs to neither benchmark, and fix the two script comments.

Review: confirmed. deploy.py:220, evaluate.py:93 and :113, artifacts.py:15, config.py:272, clean_preds.sh:8-12 and clean_preds.bat:4-8 as cited. The MinSizeRel omission costs only speed: find_file falls back to rglob (artifacts.py:60).

#### C21. [low] Platform assumptions: no macOS library name, Docker run always interactive

Evidence: services/artifacts.py:27-34 returns `libsushitrack.so` for every non-Windows platform, and services/diag.py:91-92 does the same for `libtracker_bridge.so`; on macOS the files are `.dylib`, while config.py:135 and sushicore accept `darwin` as a platform. services/docker.py:32 always passes `-it`, so `st docker run -- <cmd>` fails without a TTY (CI, pipes), although docs/README.md:328 presents it as usable non-interactively. services/checks.py:14 suggests `winget install Docker.DockerDesktop` with no check that winget exists.

Recommendation: Either declare macOS unsupported in the profile and docs, or add the `.dylib` branch in one place. Pass `-it` only when stdin is a terminal.

Review: confirmed. artifacts.py:27-34, diag.py:90-92, docker.py:32, checks.py:14 as cited. docs/README.md:328 speaks of a raw `docker run ... st build`, not `st docker run`; the page that shows the non-interactive form is :556.

#### C22. [low] Dead and unreachable code in the command layer

Evidence: cli.py:400-401 `if __name__ == "__main__": app()` bypasses `entry()` and its exit-code handling; `__main__.py` already covers `python -m`. services/deploy.py:243-245 rejects an unknown kind that Typer's `DeployKind` Enum has already rejected; services/build.py:16, 39-45 `normalize_build_type` repeats the `BuildType` Enum for the command-line path. services/docker.py:29 `run(..., forwarded=())` takes a parameter cli.py:223-224 never passes. services/tests.py:20 `SUITES` duplicates the `Suite` Enum and is unused in the CLI. cli.py:319 packs options into `types.SimpleNamespace` because the infer service still has an argparse-shaped signature.

Recommendation: Remove the `__main__` guard from cli.py, the unused `forwarded` parameter and `SUITES`, and give `infer_svc.run` keyword parameters like the other services.

Review: confirmed. cli.py:400-401, deploy.py:243-245, docker.py:29, tests.py:20, cli.py:319 as cited. SUITES, KINDS and BUILD_TYPES are pinned to the Enums by cli/tests/test_service_enums.py:13-21, and CLI_TYPER_MIGRATION.md:40 keeps them on purpose, so removing SUITES also edits that test. normalize_build_type is live for the config path (build.py:107, tests.py:38).

#### C23. [low] Help text gaps and wording

Evidence: cli.py:346 `paths` help says "(replaces cli/paths.env)", a file that no longer exists, which is history in user-facing text. cli.py:45 root help uses an em dash and lists "build, test, evaluate, run and Docker" while the command is `eval`. cli.py:182-186 `docker` sub-app has no epilog examples though every other entry does. sushicore's `unlink` (provision/commands.py:317) has no example. cli.py:147-149 `run` help says "exact, then substring match" without saying what happens on several matches. `--type` help (cli.py:66) repeats the Enum choices Typer already prints.

Recommendation: Drop the history clause, name the commands as typed, add the missing epilogs, and pick one verb for the read-only commands (Show or Print) across all CLIs.

Review: confirmed. cli.py:345, :45, :182-186, :147-149, :66 as cited. One sub-claim is wrong: `unlink` does carry an example, because `@command("unlink")` at commands.py:317 passes "unlink" as the example and :255 renders it as `st unlink`.

#### C24. [low] CLI package departs from the Sushi Python layout and header rules

Evidence: `cli/sushitrack_cli/__main__.py`, `__init__.py` and `console.py` carry no license header, and no service file except artifacts.py has `from __future__ import annotations` (build.py, deploy.py, diag.py, docker.py, evaluate.py, infer.py, label.py, runner.py, tests.py). cli.py:52-54, 179-181, 227-229, 328-330 use separator comment lines and cli.py:380-392 has two multi-line `//`-style paragraphs, both forbidden in source. Service signatures are unannotated (`def build(cfg, paths, build_type=None, ...)`). Constants are `UPPER_SNAKE` without the `K_` prefix except the three panel names. The package sits at `cli/sushitrack_cli/` with `tests/` flat, not `source/<package>/` and `tests/{unit,integration,...}`.

Recommendation: Bring the package to the python-code-style and source-comments rules in the same pass as the license-header change, since every file is touched then anyway.

Review: confirmed. Headers, separators (cli.py:52-54, 179-181, 227-229, 328-330), comment paragraphs (:380-392), unannotated services and the flat layout are as cited. The __future__ sentence is wrong: checks.py:5 has it too, so two service files carry it and nine do not. SOLID/commit shape: folding the style pass into the licence-header commit mixes two changes.

### Not checked

- Nothing was executed: no `st` command, no pytest, no build. Every claim about runtime behaviour (exit codes, tracebacks, help layout) comes from reading source.
- sushicore internals beyond provision/commands.py, typer_help.py, module_config.py, cli_console.py, profile.py and the symbol list of diag.py: `proc.Runner`, `cmake_driver`, `build_env`, `config.py`/`console.py` (colour resolution, JSON renderer), `workspace.read_toml` error handling, `standard_checks` and `fragment_check` group assignment.
- Whether a malformed `cli/config.local.toml` produces a traceback (depends on `sushicore.workspace.read_toml`, not read).
- The cli/tests suite other than test_help_screen.py (test_config, test_process, test_env, test_checks, test_plain_runs, test_run_label_deploy, test_build_service, test_test_service, test_diag_service, test_artifacts, test_label_env, test_console_lazy, test_service_enums, conftest).
- bindings/python/sushitrack.py and bindings/python/README.md content; only setup.py name and version were read. It has no console script.
- tests/regression/inference/demo.py and the evaluator (trackeval_runner.py, report_generator.py): whether `st infer`'s flags and defaults still match demo.py's argparse was not compared.
- cli/misc/construct_trackeval_data.ps1 and clean_preds.bat beyond the grep hits.
- docs/README.md outside lines 262-561 and the grep hits; docs/architecture/OVERVIEW.md, docs/reference/BENCHMARKS.md, docs/reference/CHANGELOG.md, docs/CONTRIBUTING.md in full, root README.md beyond its three CLI lines, and the docs/agent reports.
- Sibling CLIs were sampled by command name and option name only (sa, sb, sd, sr, and `se --version`); hub's and se's full command trees and their help wording were not compared line by line.
- Whether sushicore 0.7.0 is actually published to an index (F3 reports the contradiction between documents, not which one is right).
- Git history for when the truncated comments in F1 were introduced.

## Documentation

### Facts

- Present: README.md (11 lines), docs/README.md (1313 lines), docs/CONTRIBUTING.md (194 lines), docs/architecture/OVERVIEW.md (640 lines), docs/reference/CHANGELOG.md, docs/reference/BENCHMARKS.md, docs/design/README.md, docs/design/SUSHITRACK_MIGRATION.md, docs/design/CLI_TYPER_MIGRATION.md.
- Missing: docs/DOCUMENTATION_STYLE_GUIDE.md, docs/getting_started/, docs/modules/, docs/guides/, docs/reference/GLOSSARY.md, docs/reference/KNOWN_ISSUES.md, docs/design/REMAINING_WORK.md, docs/archive/.
- docs/agent/ holds one conforming work folder (2026_09_17_SUSHITRACK_MIGRATION with SPEC.md, PLAN.md, REPORT.md) plus docs/agent/plans/ and docs/agent/reports/ with one file each. The global CLAUDE.md names specs|plans|reports; the documentation skill allows only dated work folders. docs/agent/specs/ does not exist.
- Module READMEs: bindings/python/README.md and tools/README.md exist. cli/, include/SushiTrack/, source/, tests/ and its three suites have none. third_party/bytetrack and third_party/ocsort carry LICENSE and NOTICE but no README.
- No docs/superpowers, docs/slop, duplicated CHANGELOG or duplicated LICENSE exists under docs/. One LICENSE at the root (Apache-2.0, 10778 bytes); docs/README.md lines 1284-1298 restate the Apache notice inline.
- Root also carries AGENTS.md (41 lines) and CLAUDE.md (56 lines). Three ignored cmake_test_discovery_*.json files sit in the working tree root; they are untracked.
- No .github/ folder, so no workflow documentation to check.
- Versions in the tree: CMakeLists.txt project VERSION 1.0.0, bindings/python/pyproject.toml 1.0.0, cli/pyproject.toml 0.1.0 with sushicore>=0.7.0, environment.yml pins sushicore==0.4.0. `git tag` prints nothing, so the changelog heading `## v1.0.0 — 2026-09-17` has no tag behind it.
- Headers are snake_case .hpp under include/SushiTrack/ (tracker.hpp, tracker_config.hpp, ...), with sushitrack_c.h in the same folder; sources are snake_case .cpp under source/.
- The st CLI is a Typer app in cli/sushitrack_cli/cli.py with flat commands plus a docker sub-app; cli/st.py, cli/sushitrack_cli/app.py and services/discovery.py do not exist.
- docs/design/README.md maps two topics to two documents; both documents have a status line on line 3 (after a blank line), not on line 2 as the skill words it.
- docs/reference/CHANGELOG.md has 22 entries in Unreleased and 5 under v1.0.0; one line exceeds 240 characters (line 21, 423 characters).

### Corrections from review

- Changelog: Unreleased holds 21 entries, not 22 (v1.0.0 holds 5, correct).
- docs/README.md Project Structure tree lists 24 PascalCase .h files (lines 159-183), not 26; types.hpp on line 160 is already correct.
- F5 counts eight dead Contents anchors; there are nine. Line 41 `#docker-build--docker-run` does not match the heading `### st docker build / st docker run` at line 548.
- F14 says CHANGELOG.md line 23 has no parenthesised where-list; it ends with (`Dockerfile`, `README.md`). The entries without one are lines 6, 31 and 33, plus 37-41.
- F14 gives line 16 as 215 characters; it is 214.
- F22 leaves the status-line position open; check_docs_layout.py:127-135 matches the first content line under the title, so line 3 after a blank line passes.
- F7 calls label.py and infer.py siblings of process.py; they are in cli/sushitrack_cli/services/, and the omission is in the services comment at docs/README.md:283-284.

### Findings

#### D1. [high] docs/README.md is the whole manual in one 1313-line file, not an index

Evidence: D:/Projects/sushitrack/docs/README.md, 1313 lines. It contains Architecture (l.60), Features (l.80), Requirements (l.128), Installation (l.293), Build (l.332), CLI Reference (l.386), C API (l.561), Python Binding (l.617), Deploy Packages (l.651), Configuration Parameters (l.828), Test Suites (l.929), Ultralytics Integration (l.1036), Benchmark Results (l.1237), License (l.1284). docs/getting_started/, docs/guides/ and docs/modules/ do not exist.

Recommendation: Split by the placement questions: installation and build into docs/getting_started/, Ultralytics integration and deploy packages into docs/guides/, CLI reference, C API, configuration parameters into docs/reference/, binding facts into bindings/python/README.md, CLI internals into a new cli/README.md. Leave docs/README.md as a link index.

Review: confirmed. docs/README.md is 1313 lines with the listed H2 sections at the cited lines; docs/getting_started, guides and modules are absent. The split recommendation follows the placement questions.

#### D2. [high] docs/README.md cites header and source paths from before the snake_case rename

Evidence: docs/README.md:62 `source/SushiTrackC.cpp`, `include/SushiTrack/Tracker.h` (actual: source/sushitrack_c.cpp, include/SushiTrack/tracker.hpp). Lines 563, 818, 830, 1095 `include/sushitrack_c.h` (actual: include/SushiTrack/sushitrack_c.h). Line 797 `source/ConfigLoader.cpp`, line 830 `include/SushiTrack/TrackerConfig.h`, line 875 `include/SushiTrack/KalmanFilter.h`. The Project Structure tree at lines 156-183 lists 26 PascalCase .h files (Tracker.h, Tracklet.h, IMMFilter.h, ...) and `include/sushitrack_c.h`; none exist.

Recommendation: Replace every cited path with the current snake_case .hpp/.cpp name and regenerate the tree block from `git ls-files`.

Review: confirmed. Lines 62, 563, 797, 818, 830, 875, 1095 cite the old names; include/SushiTrack holds only snake_case .hpp plus sushitrack_c.h. The tree block lists 24 PascalCase .h files, not 26.

#### D3. [high] docs/architecture/OVERVIEW.md cites the same pre-rename paths

Evidence: D:/Projects/sushitrack/docs/architecture/OVERVIEW.md lines 9 and 541 `include/SushiTrack/TrackerConfig.h`; 142 `Object.h`; 146 `Rect.h`; 150 and 446 `Tracklet.h`; 190 `source/KalmanFilter.cpp`; 195 `KalmanFilter.h`; 218 `source/IMMFilter.cpp`; 289 `KinematicState.h`; 302 `source/DistanceStrategies.cpp`; 433 `source/AssignmentSolvers.cpp`. All missing from the tree.

Recommendation: Update each to the snake_case file (tracker_config.hpp, object.hpp, rect.hpp, tracklet.hpp, kalman_filter.cpp, imm_filter.cpp, kinematic_state.hpp, distance_strategies.cpp, assignment_solvers.cpp).

Review: confirmed. All cited OVERVIEW.md lines carry pre-rename paths. The list is incomplete: line 77 `source/Tracker.cpp`, line 433 `source/LAPJV.cpp` and line 546 `ConfigLoader.cpp` are stale too.

#### D4. [high] AGENTS.md tells agents to run a CLI script that does not exist

Evidence: D:/Projects/sushitrack/AGENTS.md lines 18-20 `python cli/st.py build|test|run`, line 23 "Only `cli/st.py` configures the required runtime environment". cli/st.py is not in the tree; the CLI is the pipx-installed `st` entry point (cli/pyproject.toml:31). CLAUDE.md:48 and README.md:6-8 already say `st build`. The changelog line 23 records the same fix for README and Dockerfile but AGENTS.md was left.

Recommendation: Rewrite section 3 of AGENTS.md to `st build`, `st test`, `st run <target>` installed with pipx from cli/.

Review: confirmed. AGENTS.md:18-20 and :23 name cli/st.py; cli/ has no st.py; cli/pyproject.toml:31-32 defines the st and sushitrack entry points.

#### D5. [high] docs/README.md table of contents links to seven CLI sections that do not exist

Evidence: docs/README.md:34-40 link `#project-build`, `#project-test-all`, `#project-test-unit`, `#project-test-integration`, `#project-test-label`, `#project-test-inference`, `#project-clean`. The actual headings are `### st build` (l.415), `st test` (l.426), `st eval` (l.461), `st infer` (l.495), `st clean` (l.539). Line 29 links `#cli-installer-powershell--bash`; the heading at l.295 is `### CLI (pipx)`. Line 672 still says "a plain `project build`".

Recommendation: Rebuild the contents list from the current headings and replace `project build` with `st build` at line 672.

Review: confirmed. Contents lines 34-40 and 29 point at headings that do not exist; line 672 says `project build`. Line 41 `#docker-build--docker-run` is broken as well (heading at l.548 is `st docker build` / `st docker run`), so nine anchors are dead, not eight.

#### D6. [high] Installation section contradicts the package metadata and the changelog on where sushicore comes from

Evidence: docs/README.md:304-308: `pipx inject sushitrack-cli --editable /path/to/sushistack/sushicore` and "`sushicore` is not published to any index — it ships inside the `sushistack` repository — so the second step is required". cli/pyproject.toml:20 declares `sushicore>=0.7.0` as a normal dependency, and docs/reference/CHANGELOG.md:29 says "Took `sushicore` from PyPI instead of a sibling checkout". docs/README.md:141 also states `sushicore==0.4.0` (matching environment.yml:65) while the CLI requires 0.7.0.

Recommendation: Delete the inject step and the unpublished claim. Raise the environment.yml pin as a code fix, then restate the version at line 141.

Review: confirmed. docs/README.md:304-308 keeps the inject step and the 'not published' claim; cli/pyproject.toml:20 requires sushicore>=0.7.0; CHANGELOG.md:29 says PyPI; README:141 and environment.yml:65 say 0.4.0. Raising the environment.yml pin is a build-configuration edit and needs the owner.

#### D7. [medium] docs/README.md describes its own docs/ folder and the CLI package wrongly

Evidence: docs/README.md:268-270 list `docs/ARCHITECTURE.md`, `docs/CHANGELOG.md`, `docs/benchmarks/PRIVATE.md`; actual files are docs/architecture/OVERVIEW.md, docs/reference/CHANGELOG.md, docs/reference/BENCHMARKS.md. Line 1227 lists a `discovery` service; docs/reference/CHANGELOG.md:6 records its removal and cli/sushitrack_cli/services/ holds artifacts.py and checks.py instead. Lines 277-284 omit process.py's siblings label.py, infer.py. Lines 275 and 410 cite `cli/config.local.toml` as if present (it is an untracked optional file, stated only at l.275).

Recommendation: Correct the docs/ subtree and the services list to match the tree.

Review: confirmed. Lines 268-270 and 1227 are wrong as stated. The services comment at l.283-284 omits label and infer; the audit's phrase "process.py's siblings" is muddled, they live in services/. The config.local.toml point is weak: l.275 marks it as not tracked.

#### D8. [medium] No backlog file; deferred work sits in the architecture page

Evidence: docs/design/REMAINING_WORK.md does not exist. D:/Projects/sushitrack/docs/architecture/OVERVIEW.md:625-640 `## Future work` lists three deferred changes (within-pool duplicate suppression, IMM mixture-covariance gating, lazy coordinated-turn update). 36 unchecked `- [ ]` boxes remain in docs/agent/2026_09_17_SUSHITRACK_MIGRATION/PLAN.md and 38 in docs/agent/plans/2026-09-22-cli-typer-migration.md, with zero checked in either.

Recommendation: Create docs/design/REMAINING_WORK.md, move the three Future work items into it, and remove the section from OVERVIEW.md.

Review: confirmed. No docs/design/REMAINING_WORK.md; OVERVIEW.md:625-640 holds three deferred items; unchecked boxes are 36 and 38 with none checked.

#### D9. [medium] Both design documents show Open status for work the changelog and reports say has landed

Evidence: docs/design/SUSHITRACK_MIGRATION.md:3 `Open — phase 1 of 4`, while docs/agent/2026_09_17_SUSHITRACK_MIGRATION/REPORT.md:4 says the migration "has been successfully executed" and CHANGELOG.md:35-41 records v1.0.0. docs/design/CLI_TYPER_MIGRATION.md:3 `Open — phase 0 of 1`, while CHANGELOG.md:25 says "Rebuilt the `st` command layer on Typer" and cli/sushitrack_cli/cli.py:42 is a typer.Typer app. docs/design/README.md:7-8 repeats both stale statuses.

Recommendation: Set both to `**Status:** Shipped` on line 2 and update the map; they become archive candidates 90 days after their last commit.

Review: confirmed. Both status lines and the map at docs/design/README.md:7-8 say Open; cli.py:42 is a typer.Typer app and CHANGELOG.md:25 and :37-41 record both pieces of work as done.

#### D10. [medium] Design documents describe a layout and files that were never the final state

Evidence: docs/design/SUSHITRACK_MIGRATION.md:55-60 and 180-182 list `cli/config.json`, `cli/st.py`, `st.bat`, `st.sh`, `sushitrack.bat`; lines 216-217 say to build with `python cli/st.py build`. The tree has cli/config.toml and no script. docs/design/CLI_TYPER_MIGRATION.md:7 and 15 cite `cli/sushitrack_cli/app.py` (deleted) and line 24 lists a `discovery` service (deleted).

Recommendation: When marking them shipped, either correct the paths or archive the documents as written; do not leave them in docs/design/ as live intent.

Review: confirmed. SUSHITRACK_MIGRATION.md:54-60, 180-182, 212, 216-217 and CLI_TYPER_MIGRATION.md:7, 15, 24 cite files absent from the tree (cli/config.json, st.py, st.bat, app.py, discovery.py).

#### D11. [medium] Agent work is stored in two shapes, and the Typer work has no folder

Evidence: docs/agent/plans/2026-09-22-cli-typer-migration.md (1077 lines) and docs/agent/reports/2026-09-22-wave-1-task-7f.md (94 lines) are loose files with lower-case hyphenated names. The documentation skill requires `docs/agent/<YYYY_MM_DD>_<WORK_NAME>/` with only SPEC.md, PLAN.md, REPORT.md and UPPER_SNAKE_CASE names; the sibling folder 2026_09_17_SUSHITRACK_MIGRATION follows it. The global CLAUDE.md tree says `docs/agent/specs|plans|reports`, which conflicts with the skill.

Recommendation: Have the owner pick one shape. If the skill wins, move the plan to docs/agent/2026_09_22_CLI_TYPER_MIGRATION/PLAN.md and the report to a 2026_09_22 work folder as REPORT.md.

Review: confirmed. Both loose files exist with hyphenated lower-case names; check_docs_layout.py K_WORK_FOLDER and K_WORK_FILES enforce the dated-folder shape. The report (wave 1 task 7f, sushicore from PyPI) is a different piece of work from the Typer plan, so it needs its own folder; the audit's recommendation already allows for that.

#### D12. [medium] Documents not reachable from docs/README.md

Evidence: docs/README.md links only architecture/OVERVIEW.md (l.104, l.919) and reference/BENCHMARKS.md (l.1242). It has no link to docs/CONTRIBUTING.md (named only inside the tree block at l.267), docs/reference/CHANGELOG.md, docs/design/README.md, either design document, any file under docs/agent/, bindings/python/README.md or tools/README.md.

Recommendation: Add an index section linking every document under docs/ and both module READMEs.

Review: confirmed. The only relative links in docs/README.md are at lines 104, 919 and 1242 (OVERVIEW twice, BENCHMARKS once).

#### D13. [medium] docs/reference/BENCHMARKS.md is a raw dump with no title, no prose and no Markdown structure

Evidence: D:/Projects/sushitrack/docs/reference/BENCHMARKS.md, 806 lines. Line 1 is `{`; lines 1-68 are an unfenced JSON config; from line 69 an unfenced fixed-width table. `grep '^#'` returns no heading. docs/README.md:1242 presents it as the recorded internal run.

Recommendation: Add a title, the date, commit and machine of the run, and fence the config and tables as code blocks.

Review: confirmed. BENCHMARKS.md line 1 is `{`, the table starts near line 70, and there are zero heading lines in 806 lines.

#### D14. [medium] Changelog entries break the entry rules

Evidence: docs/reference/CHANGELOG.md:21 is 423 characters, two clauses joined by a semicolon, and cites seven places (limit five). Line 16 cites `[tool]`, `ToolSettings` and two files in a 215-character line with a why-like tail. Lines 23 and 31 give no parenthesised where-list in the required form; line 6 has no parenthesised place. Lines 37-41 (v1.0.0) cite no file or symbol at all. Blank lines separate entries 17-33, and line 17 uses scope `evaluator/inference`, which is two scopes.

Recommendation: Split line 21 into a build entry and a fix entry, add places to the v1.0.0 entries, and remove the blank lines between entries.

Review: confirmed. Line 21 is 423 characters with seven cited places and a semicolon. One sub-claim is wrong: line 23 does end with a parenthesised list (`Dockerfile`, `README.md`). Lines 6, 31 and 33 have none (33 was not named). Line 16 is 214 characters, not 215.

#### D15. [medium] Changelog release heading has no tag, and component versions disagree

Evidence: docs/reference/CHANGELOG.md:35 `## v1.0.0 — 2026-09-17`; `git tag` in D:/Projects/sushitrack prints nothing. The skill says the heading is the tag. CMakeLists.txt:2 and bindings/python/pyproject.toml:7 say 1.0.0; cli/pyproject.toml:7 says 0.1.0. docs/architecture/OVERVIEW.md:7-8 says values match "the code as of v1.0.0" with 22 unreleased entries since.

Recommendation: Tag v1.0.0 at the release commit or rename the section, and state in the manual which version number governs the repository.

Review: confirmed. `git tag` prints nothing; CMakeLists.txt:2 and bindings/python/pyproject.toml:7 say 1.0.0, cli/pyproject.toml:7 says 0.1.0. Creating the tag is a release action for the owner, not a documentation fix.

#### D16. [medium] docs/CONTRIBUTING.md contradicts the source-comment rule and omits the documentation rules

Evidence: D:/Projects/sushitrack/docs/CONTRIBUTING.md:51 "Write comments only when the **why** is non-obvious ... Do not describe what the code does." The global rule is the reverse: a comment says what a thing does, and the reason lives in a design document. The file has no section on what must be documented, where a document goes, or the changelog entry (its `## Documentation` at l.75 covers only Doxygen headers). Lines 180-194 give commit prefixes without the scope form the changelog uses (`cli:`, `build:`).

Recommendation: Replace the Comments section with a pointer to the source-comments rule, and add the placement questions, changelog rule and same-commit rule.

Review: confirmed. CONTRIBUTING.md:51 tells authors to comment the why and not the what, the reverse of the global rule; the file has no placement, changelog or same-commit section; line 194 lists prefixes without scopes.

#### D17. [medium] Root README.md is too thin to be a front door

Evidence: D:/Projects/sushitrack/README.md is 11 lines: one sentence, three commands, one link. It names no license, no supported platforms, no prerequisite (sushicore, conda environment, submodules listed in .gitmodules), no minimal C or Python usage, and no status. `pipx install cli/` followed by `st build` fails on a fresh clone without the submodules and toolchain the manual describes at docs/README.md:128-143.

Recommendation: Add what the library does in two or three sentences, the license line, prerequisites with `git submodule update --init`, a five-line usage example, and links to the manual sections.

Review: confirmed. README.md is 11 lines: one sentence, three commands, one link. .gitmodules lists three submodules the README never mentions.

#### D18. [medium] Required reference and style files are absent

Evidence: Not in the tree: docs/DOCUMENTATION_STYLE_GUIDE.md, docs/reference/GLOSSARY.md, docs/reference/KNOWN_ISSUES.md, docs/archive/. The manual uses undefined terms a glossary would hold (OCM, ORU, NSA, IMM, VDC, tentative, lost buffer) at docs/README.md:80-124.

Recommendation: Create the four as part of the skeleton build-out, after owner approval as the global rule requires.

Review: confirmed. The four paths are absent; `find docs -type d` shows only agent, architecture, design, reference.

#### D19. [medium] Module READMEs missing for cli/ and tests/; the bindings README is thin and its build claim is unverified

Evidence: No cli/README.md, tests/README.md, include/SushiTrack/README.md or source/README.md. CLI internals are documented in docs/README.md:1218-1232 and test suite facts at 929-1000 instead. bindings/python/README.md is 22 lines; line 8 says `pip install .` without saying the native library must be built first or how the binding finds it (docs/README.md:748 `How discovery works` holds that fact).

Recommendation: Add cli/README.md and tests/README.md from the manual sections that describe them, and move library discovery into the bindings README.

Review: unclear. The missing files are real, but tools/documentation/check_docs_layout.py:164-172 only requires READMEs under modules/<tier>/<module>/, and this is a single-module repository; whether cli/, tests/, source/ and include/ count as modules is an owner decision. The bindings README part is confirmed: `pip install .` with no word on building the library or SUSHITRACK_LIB (sushitrack.py:204-209).

#### D20. [medium] The manual describes a Docker install the Dockerfile does not perform

Evidence: D:/Projects/sushitrack/docs/README.md:1233 says the Dockerfile 'installs the CLI with pipx, symlinking `st` into `/usr/local/bin`', and line 326 repeats the symlink. D:/Projects/sushitrack/Dockerfile:35-39 runs `pip3 install --no-cache-dir /workspace/sushitrack/cli` and its comment says 'No separate symlink step'. docs/README.md:326 also says the Dockerfile pins the same wheels as environment.yml, but the Dockerfile pin list (lines 19-31) has no sushicore while environment.yml:65 pins sushicore==0.4.0.

Recommendation: Rewrite both sentences from the Dockerfile as it stands: pip install of cli/, entry points on PATH, sushicore resolved through cli/pyproject.toml.

Review: found by review.

#### D21. [medium] OVERVIEW.md stale paths go beyond the list in F3

Evidence: D:/Projects/sushitrack/docs/architecture/OVERVIEW.md:77 `source/Tracker.cpp`, :433 `source/LAPJV.cpp`, :546 `ConfigLoader.cpp`. The tree has source/tracker.cpp, source/lapjv.cpp, source/config_loader.cpp. A fix that follows F3's list alone leaves three dead paths.

Recommendation: Fix every path in the file by a scan against `git ls-files`, not from the F3 list.

Review: found by review.

#### D22. [medium] tools/README.md is the sushiskills reference text, false in this repository

Evidence: D:/Projects/sushitrack/tools/README.md:3-4 says 'Reference implementations of the four checkers every Sushi Systems repository carries. A repository copies this folder whole; a rule is fixed here first and copied out.' In sushitrack the folder is the copy, not the reference. Line 19-20 says check_layering.py does nothing until K_TIER_ORDER is set, without saying whether this repository sets it.

Recommendation: State that the folder is a copy of the sushiskills reference, name the source, and record this repository's K_ settings.

Review: found by review.

#### D23. [medium] Agent instruction files disagree with each other and with the tree

Evidence: D:/Projects/sushitrack/AGENTS.md:36 calls tests/ 'Unit, integration, and benchmark test suites'; the tree has tests/unit, tests/integration, tests/regression. D:/Projects/sushitrack/CLAUDE.md:46-48 gives the `st` pipx form while AGENTS.md:18-23 gives `python cli/st.py`. CLAUDE.md:52 tells agents to record progress in 'the corresponding docs/agent/ work directory', which the loose docs/agent/plans and docs/agent/reports files do not follow. The same directives are written twice, in two files.

Recommendation: Keep one of the two files as the source and have the other point to it; the audit's F4 fixes only the command block.

Review: found by review.

#### D24. [medium] bindings/python/README.md and docs/CONTRIBUTING.md show different constructor forms with no explanation

Evidence: D:/Projects/sushitrack/bindings/python/README.md:16 uses `Tracker(config_path="sushitrack.json")`; D:/Projects/sushitrack/docs/CONTRIBUTING.md:133 uses `Tracker(config='auto')`. bindings/python/sushitrack.py:320-327 accepts lib_path, max_tracks, config_path, config and keyword overrides; neither document says what `config` versus `config_path` does or that SUSHITRACK_LIB (sushitrack.py:209) selects the library.

Recommendation: Document the constructor arguments and the library lookup order once, in bindings/python/README.md, and link to it from the manual and CONTRIBUTING.

Review: found by review.

#### D25. [low] Broken relative link and stale commands inside the agent work folder

Evidence: docs/agent/2026_09_17_SUSHITRACK_MIGRATION/PLAN.md:62 links `[docs/README.md](docs/README.md)`, which resolves to docs/agent/2026_09_17_SUSHITRACK_MIGRATION/docs/README.md (missing); it sits inside a quoted README template. SPEC.md:12 and REPORT.md:26 cite `python cli/st.py` and `cli/sushitrack.py`, neither of which exists.

Recommendation: Put the template in a fenced code block so the link is not live. Leave the historical commands if the folder is archived as written.

Review: confirmed. PLAN.md:62 link is as quoted. The template is already inside a ```markdown fence, but the inner ```bash fence closes it early, so the link renders live; the fix is a four-backtick outer fence, not 'add a fence'.

#### D26. [low] Prose that breaks the humanizer register

Evidence: docs/agent/2026_09_17_SUSHITRACK_MIGRATION/REPORT.md:4 "successfully executed with zero compromises ... 100% test pass rate"; heading `## 2. Key Accomplishments`; line 36 totals 542 while lines 33-35 sum to 548. docs/README.md has 88 lines containing an em dash and bold-lead paragraphs throughout Features (l.82-124). CLAUDE.md:56 ends with "love you boss, kolay gelsin.", a non-English line in an English artifact. SPEC.md:7 "Absolutely NO".

Recommendation: Rewrite REPORT.md as plain facts with the corrected test total, cut the em-dash density when the manual is split, and remove the CLAUDE.md sign-off.

Review: confirmed. REPORT.md:4, :6 and :33-36 as quoted (289+31+228 = 548, file says 542); 88 em-dash lines in docs/README.md; CLAUDE.md:56 as quoted; SPEC.md:7 as quoted.

#### D27. [low] Status line position and CONTRIBUTING license statement

Evidence: docs/design/SUSHITRACK_MIGRATION.md and docs/design/CLI_TYPER_MIGRATION.md carry the status on line 3 after a blank line; the skill says the second line. docs/CONTRIBUTING.md:81 hard-codes "the standard Apache-2.0 header" and docs/README.md:1284-1298 reproduce the Apache notice, both of which change if the owner replaces the license.

Recommendation: Confirm whether check_docs_layout.py accepts line 3. Reduce the license text in the manual to one line that links to LICENSE, so a license change touches one place.

Review: confirmed. Status is on line 3 in both documents. check_docs_layout.py:133 tests the first content line after the title, so line 3 passes the checker; that half is a non-issue. CONTRIBUTING.md:81 and docs/README.md:1284-1298 hard-code Apache-2.0 as stated.

### Not checked

- The technical content of docs/README.md lines 590-1236 (C API signatures, status codes, parameter defaults, deploy layouts, Ultralytics adapter code) was not compared against include/SushiTrack/sushitrack_c.h, tracker_config.hpp or cli/sushitrack_cli/services/deploy.py; only headings, links and cited paths were checked.
- docs/architecture/OVERVIEW.md was checked for cited paths and its Future work section only; its descriptions of the pipeline, IMM and lifecycle were not verified against source/.
- CLI flag tables in docs/README.md:415-560 were not compared flag by flag with cli/sushitrack_cli/cli.py; only command names were confirmed.
- Benchmark numbers in docs/README.md:1237-1282 were not reconciled with docs/reference/BENCHMARKS.md.
- docs/agent/plans/2026-09-22-cli-typer-migration.md (1077 lines) and docs/agent/2026_09_17_SUSHITRACK_MIGRATION/PLAN.md (285 lines) were read only at their heads; only one broken link was machine-checked.
- External URLs (GitHub, sushisystems.io, vendored upstreams) were not fetched.
- In-page anchors other than the Contents list of docs/README.md were not resolved one by one.
- tools/documentation/check_docs_layout.py and check_changelog.py were not run, as the task forbids running project tools; their verdicts may differ from this manual measurement.
- Third-party documentation under third_party/ (submodules TrackEval, eigen, yolox; vendored bytetrack, ocsort NOTICE files) was not reviewed.
- Whether README.md's `pipx install cli/` works on a clean machine was not tested.
- Humanizer review was sampled (REPORT.md, SPEC.md, CLAUDE.md, the Features section); the whole manual was not read sentence by sentence.

## Layout and hygiene

### Facts

- Root (tracked): .dockerignore, .gitattributes, .gitignore, .gitmodules, AGENTS.md, CLAUDE.md, CMakeLists.txt, Dockerfile, LICENSE (Apache 2.0), README.md, environment.yml (conda env), sushitrack.json (runtime tracker config), bindings/, cli/, docs/, include/, source/, tests/, third_party/, tools/.
- Root (untracked, ignored): build/, .pytest_cache/, cmake_test_discovery_a0675c2597.json (7.5 KB), cmake_test_discovery_a8348403ea.json (143 KB), cmake_test_discovery_d595c1816c.json (60 KB).
- 433 files tracked: tests 282, cli 41, third_party 34, include 28, docs 13, source 12, tools 7, bindings 4, root 12.
- Single-module layout: include/SushiTrack/*.hpp (28) and source/*.cpp (12), flat, no <area> subfolder; tests/{unit,integration,regression}.
- Version declarations: CMakeLists.txt:2 `project(sushitrack VERSION 1.0.0)`; cli/pyproject.toml:7 `0.1.0`; cli/sushitrack_cli/__init__.py:3 `__version__ = "2.0.0"`; bindings/python/pyproject.toml:7, setup.py:11 and sushitrack.py:21 all `1.0.0`; changelog has `## Unreleased` and `## v1.0.0 — 2026-09-17`. They do not agree (see F5).
- Git tags: none (`git tag -l` prints nothing), although the changelog has a v1.0.0 section.
- Remote: origin https://github.com/SushiSystems/SushiTrack.git; single branch main, 79 commits, main is ahead of origin/main by 1 (87cfedd, 2026-10-04, unpushed).
- git status: working tree clean, no modified and no untracked-unignored files. Everything untracked is ignored: build/, pytest caches, __pycache__, cli/sushitrack_cli.egg-info/, the three cmake_test_discovery JSONs, evaluator data pred/ and img1/ folders.
- CI workflows: none. There is no .github/ directory. Siblings sushicore and sushistack each carry .github/workflows/ci.yml and release.yml.
- tools/: README.md, common/checker.py (shared rule-table runner), documentation/check_source_comments.py, documentation/check_docs_layout.py, documentation/check_changelog.py, layering/check_layering.py. All four mandatory checkers are present under the mandated names.
- .gitignore covers: cli/config.local.{json,toml}, .sushitrack_env.json, build/ bin/ lib/ out/, package/, evaluator data pred/ and img1/, CMake generated files incl. compile_commands.json and cmake_test_discovery_*.json, Visual Studio files, .vscode/ .idea/, Python caches/venvs/.env/*.pth/*.tar/*.egg-info, *.mp4 *.webm, .DS_Store, Thumbs.db, docs/html/, output/.
- Submodules (3): third_party/TrackEval, third_party/eigen, third_party/yolox. third_party/bytetrack, ocsort and nlohmann are vendored in-tree.
- Tracked payload at HEAD is 236.5 MB, of which tests/regression/evaluator/data is 234.2 MB in 228 files (152 .txt, 76 .ini). .git packs are 100.84 MiB. No tracked image, model weight, archive or executable.
- docs/ present: README.md, CONTRIBUTING.md, architecture/OVERVIEW.md, design/{README,CLI_TYPER_MIGRATION,SUSHITRACK_MIGRATION}.md, reference/{CHANGELOG,BENCHMARKS}.md, agent/{plans,reports,2026_09_17_SUSHITRACK_MIGRATION}.
- Sibling D:/Projects/obsolete_sushitrack still exists beside this repository, with the pre-migration 3rdparty/ layout.

### Corrections from review

- F10 calls the migration finished and its design document closed. docs/design/SUSHITRACK_MIGRATION.md line 3 and docs/design/README.md both read `Status: Open — phase 1 of 4`, so under the placement rule it stays in docs/design/; moving it to docs/archive/ is wrong until the owner closes the remaining phases. Only the dated agent folder needs refiling.
- F12 says `18 files in tests/unit`. There are 18 .cpp files and 19 tracked files (CMakeLists.txt included). The conclusion does not change.
- F9 lists imgui.ini and *.log as rules the repository misses. Nothing in sushitrack writes either; the real gaps are .sushitrack_env.json, build-asan/, .worktrees/, worktrees/ and .superpowers/.
- F3 states the cause (`discovery output is being emitted with the source root as working directory`) without evidence. It is correct, and the evidence is tests/unit/CMakeLists.txt:63, tests/integration/CMakeLists.txt:48 and tests/regression/CMakeLists.txt:93, each `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}`.
- All recomputed numbers match the audit: 433 tracked files, 79 commits, per-folder counts (tests 282, cli 41, third_party 34, include 28, docs 13, source 12, tools 7, bindings 4, root 12), 234,179,854 / 236,533,508 bytes, 228 data files, 22 duplicate blobs, six version declarations, no tags, three submodules.

### Findings

#### L1. [high] No CI at all

Evidence: D:/Projects/sushitrack/.github does not exist (`ls .github` -> No such file or directory). D:/Projects/sushitrack/CLAUDE.md lists `continuous-integration` as an enforced skill. Siblings carry D:/Projects/sushicore/.github/workflows/{ci.yml,release.yml} and D:/Projects/sushistack/.github/workflows/{ci.yml,release.yml}.

Recommendation: Add a push/PR workflow and a tag workflow that run the four tools/ checkers and `st build` / `st test` on Windows and Linux through the CLI only. Settle the file names stack-wide: the skill says push.yml and release.yml, the siblings use ci.yml and release.yml.

Review: confirmed. D:/Projects/sushitrack has no .github/ (ls -la of root); CLAUDE.md lists continuous-integration; sushicore and sushistack each hold .github/workflows/ci.yml and release.yml. The ci.yml vs push.yml naming conflict with the skill is real.

#### L2. [high] 234 MB of MOT benchmark data tracked in git, with duplicates

Evidence: `git ls-tree -r -l HEAD tests/regression/evaluator/data` sums to 234,179,854 bytes over 228 files, out of 236,533,508 tracked in total. Largest: tests/regression/evaluator/data/MOT20-05-YOLOX/det/det.txt 30.4 MB; MOT20-05-YOLOX/gt/gt.txt and MOT20-05/gt/gt.txt are both 26,217,027 bytes. 22 blob hashes occur more than once under that folder (each gt.txt is stored for the plain and the -YOLOX sequence). .gitattributes has no LFS rule.

Recommendation: Owner decision: move the MOT17/MOT20/DanceTrack ground truth and detections out of the repository (fetched on demand by `st eval`, or Git LFS) and keep only a small fixture under tests/regression/fixtures/. Check the MOTChallenge and DanceTrack redistribution terms before the public release, since this data ships under the repository's licence file.

Review: confirmed. Recomputed: 234,179,854 bytes in 228 files (152 txt, 76 ini) of 236,533,508 total; 22 duplicated blob hashes; MOT20-05 gt.txt stored twice under one hash at 26,217,027 bytes; .gitattributes has no LFS rule.

#### L3. [high] Version numbers disagree and no tag exists

Evidence: D:/Projects/sushitrack/CMakeLists.txt:2 `VERSION 1.0.0`; cli/pyproject.toml:7 `version = "0.1.0"`; cli/sushitrack_cli/__init__.py:3 `__version__ = "2.0.0"`; bindings/python/pyproject.toml:7, bindings/python/setup.py:11 and bindings/python/sushitrack.py:21 `1.0.0`. docs/reference/CHANGELOG.md:35 `## v1.0.0 — 2026-09-17`, yet `git tag -l` is empty. No generated version.hpp: grep for a version symbol in include/SushiTrack finds only licence text.

Recommendation: Make CMakeLists.txt the single source; generate version.hpp and the bindings version from it; drop the hand-written `__version__ = "2.0.0"` in the CLI or read it from package metadata. The CLI disagrees with itself (0.1.0 vs 2.0.0). Tag v1.0.0 on the commit the changelog names or correct the heading; v1.0.0 is an owner decision under the versioning skill.

Review: confirmed. All six version lines match as cited; changelog line 35 is `## v1.0.0 — 2026-09-17`; git tag -l is empty; no version symbol under include/SushiTrack. Recommendation is brick-shaped (one source, generated copies).

#### L4. [medium] CMake test-discovery JSON written into the repository root

Evidence: D:/Projects/sushitrack/cmake_test_discovery_a0675c2597.json (7,529 B), cmake_test_discovery_a8348403ea.json (143,505 B), cmake_test_discovery_d595c1816c.json (60,110 B), all dated Sep 24 09:27. They are ignored (.gitignore line `cmake_test_discovery_*.json`) and excluded in .dockerignore, so the ignore rule hides the symptom rather than fixing it.

Recommendation: Delete the three files and make gtest discovery write under build/ (the discovery output is being emitted with the source root as working directory); then the ignore rule and the .dockerignore line can go.

Review: confirmed. Three cmake_test_discovery_*.json in root with the stated sizes; .gitignore:26 and .dockerignore:18 ignore them. Cause verified: gtest_discover_tests uses WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} at tests/unit/CMakeLists.txt:63, tests/integration/CMakeLists.txt:48, tests/regression/CMakeLists.txt:93. The audit did not cite these lines; the fix has to keep the tests' run directory while moving discovery output, so it is not a one-line change.

#### L5. [medium] Root holds entries the layout does not allow, and lacks ones it requires

Evidence: Not in the allowed root table of the repository-layout skill: D:/Projects/sushitrack/bindings/ (top-level folder), Dockerfile, .dockerignore, environment.yml, sushitrack.json. Required but missing: .clang-format, .editorconfig, cmake/. D:/Projects/sushitrack/CMakeLists.txt is 10,222 bytes and carries compiler-flag and export logic (e.g. the C4251 block near line 62) where the skill allows only project(), options and add_subdirectory.

Recommendation: Ask the owner where bindings/, the Docker files, environment.yml and the default sushitrack.json belong (the skill says a new top-level folder is not allowed). Add .clang-format and .editorconfig, and move the CMake logic into cmake/ functions.

Review: confirmed. Root holds bindings/, Dockerfile, .dockerignore, environment.yml, sushitrack.json, none in the skill's root table; .clang-format, .editorconfig and cmake/ are absent. CMakeLists.txt is 10,222 bytes with flags (47-84), Eigen lookup (89-100) and target definitions (105-185); the C4251 comment is at 55-65.

#### L6. [medium] sushicore pin differs between the CLI, the conda env and the manual

Evidence: D:/Projects/sushitrack/cli/pyproject.toml requires `sushicore>=0.7.0` (commit 87cfedd); D:/Projects/sushitrack/environment.yml:65 pins `sushicore==0.4.0`; D:/Projects/sushitrack/docs/README.md:141 calls environment.yml the source of truth and lists `sushicore==0.4.0`. Dockerfile has no sushicore line and relies on the CLI install.

Recommendation: Raise environment.yml and docs/README.md to the version the CLI requires; an environment built from environment.yml cannot satisfy the CLI today.

Review: confirmed. cli/pyproject.toml:20 `sushicore>=0.7.0`, environment.yml:65 `sushicore==0.4.0`, docs/README.md:141 lists 0.4.0 and names environment.yml the source of truth; Dockerfile has no sushicore line.

#### L7. [medium] Stale compiled leftovers of deleted tools under tools/

Evidence: D:/Projects/sushitrack/tools/__pycache__/ holds logger.cpython-310.pyc, logger.cpython-311.pyc, logger.cpython-313.pyc, paths_manager.cpython-311.pyc, paths_manager.cpython-313.pyc. No tools/logger.py or tools/paths_manager.py exists in the tree or in `git ls-files tools`. Ignored by `__pycache__/`, so invisible to git status.

Recommendation: Delete tools/__pycache__/. These are bytecode of pre-migration modules and still import if the folder is on sys.path.

Review: confirmed. tools/__pycache__ holds the five .pyc files named (dated Apr-May); git ls-files tools has no logger.py or paths_manager.py.

#### L8. [medium] .gitignore misses files the repository produces and carries rules for things it never has

Evidence: D:/Projects/sushitrack/.gitignore has no rule for `.sushitrack_env.json` (cli/sushitrack_cli/env.py:17), build-asan/, .superpowers/, .worktrees/ or worktrees/ (the last three are listed in .dockerignore, so they are expected to appear), imgui.ini, or *.log. It ignores `*.db`, `*.tar`, `*.pth` globally; `*.pth` and `*.tar` would also silently hide a deliberately added weight or fixture.

Recommendation: Add the missing rules and align .gitignore with .dockerignore. .dockerignore also names `Dockerfile.cli`, which does not exist.

Review: confirmed. .gitignore has no rule for .sushitrack_env.json, build-asan/, .worktrees/, worktrees/ or .superpowers/; *.db (35), *.pth (53), *.tar (54) are global; .dockerignore:26 names Dockerfile.cli, which does not exist. The imgui.ini and *.log items are padding: nothing in this repository produces them.

#### L9. [medium] Documentation skeleton is incomplete

Evidence: Missing under D:/Projects/sushitrack/docs: DOCUMENTATION_STYLE_GUIDE.md, getting_started/, modules/, guides/, archive/, agent/specs/, reference/GLOSSARY.md, reference/KNOWN_ISSUES.md, design/REMAINING_WORK.md. Finished migration work sits in docs/agent/2026_09_17_SUSHITRACK_MIGRATION/{PLAN,REPORT,SPEC}.md, a dated folder beside the specs|plans|reports split, and its design twin docs/design/SUSHITRACK_MIGRATION.md is still in design/.

Recommendation: Build the missing skeleton as its own approved task; file the migration SPEC/PLAN/REPORT under agent/specs|plans|reports or archive them, and move a closed design document to docs/archive/.

Review: confirmed. git ls-files docs shows 13 files and none of the nine missing entries; the dated folder docs/agent/2026_09_17_SUSHITRACK_MIGRATION/ exists. One part is wrong: the migration design is not closed, see wrong_facts.

#### L10. [medium] docs/README.md project-structure tree describes a layout that no longer exists

Evidence: D:/Projects/sushitrack/docs/README.md lines 150-175 list `include/sushitrack_c.h`, `SushiTrack/Tracker.h`, `TrackerConfig.h`, `Tracklet.h`, `IMMFilter.h`, `KalmanFilter.h` and so on. The tracked files are include/SushiTrack/sushitrack_c.h, tracker.hpp, tracker_config.hpp, tracklet.hpp, imm_filter.hpp, kalman_filter.hpp. The Dockerfile line there says `alias setup`, which the Dockerfile no longer does.

Recommendation: Regenerate the tree from `git ls-files`; every cited path must resolve.

Review: confirmed. docs/README.md:150-175 lists include/sushitrack_c.h, Tracker.h, TrackerConfig.h and other PascalCase .h names; tracked headers are include/SushiTrack/*.hpp in snake_case plus sushitrack_c.h inside SushiTrack/. Line 152 says `alias setup`; the Dockerfile only pip-installs cli/.

#### L11. [medium] AGENTS.md tells agents to run a CLI entry point that does not exist

Evidence: D:/Projects/sushitrack/AGENTS.md:18-20 and :23 name `python cli/st.py build|test|run` and say only `cli/st.py` configures the environment. cli/st.py is absent (`ls cli` shows config.toml, misc, pyproject.toml, sushistack.deps.toml, sushitrack_cli, tests), and docs/reference/CHANGELOG.md records on 2026-09-22 that cli/st.sh and cli/st.py no longer exist. CLAUDE.md gives the pipx-installed `st` instead, so the two agent files disagree. The audit listed AGENTS.md under not_checked.

Recommendation: Rewrite AGENTS.md section 3 to the pipx-installed `st` commands CLAUDE.md names, and correct section 5, which describes tests/ as `unit, integration, and benchmark` while the tree has unit, integration and regression.

Review: found by review.

#### L12. [medium] Unreleased section meets the release-proposal threshold and nobody tracks the next version

Evidence: D:/Projects/sushitrack/docs/reference/CHANGELOG.md lines 3-33 hold 22 entries under `## Unreleased`, above the 15-entry trigger in versioning-and-release, and the history carries a breaking commit (57bedc8 `refactor(tracker)!: rebrand core library, c abi, and test suite to sushitrack`). The audit reported the version disagreement (F5) but not that a release proposal is due, nor that with no tag the `since the last tag` computation has no base.

Recommendation: After F5 fixes the source of truth, the owner decides whether v1.0.0 was ever released; then compute the next version from the 22 entries and propose it. Until a tag exists the changelog heading `v1.0.0` names a release that git cannot show.

Review: found by review.

#### L13. [medium] Public headers are flat, without the <area> level the single-module layout prescribes

Evidence: All 28 headers sit directly in D:/Projects/sushitrack/include/SushiTrack/ and all 12 sources in source/ (git ls-files include source). repository-layout gives `include/<Project>/<area>/<name>.hpp` and `source/<area>/<name>.cpp`. The audit recorded this as a fact and raised no finding. Several headers also break one-symbol-per-file by plural name: assignment_solvers.hpp, distance_strategies.hpp, timing_policies.hpp, kalman_types.hpp. The C header is sushitrack_c.h while every other header is .hpp.

Recommendation: Ask the owner before moving anything: the include paths are public interface. If approved, group by responsibility (filters, association, timing, C API) in one change with the include paths updated, and split the plural headers one symbol per file.

Review: found by review.

#### L14. [medium] Docker image cannot follow the CLI's sushicore requirement by the route the manual describes

Evidence: D:/Projects/sushitrack/Dockerfile:39 runs `pip3 install /workspace/sushitrack/cli`, which needs `sushicore>=0.7.0` (cli/pyproject.toml:20) from an index. docs/README.md:304-307 says sushicore is not published to any index and must be injected from a sushistack checkout, while the changelog entry of 2026-09-22 says sushicore is taken from PyPI. One of the two statements is false; F6 covers the 0.4.0 pin but not this contradiction. I did not query PyPI.

Recommendation: Confirm whether sushicore 0.7.0 is on PyPI. If it is, delete the `pipx inject` paragraph in docs/README.md; if not, the Dockerfile and `pipx install cli/` in README.md both fail on a clean machine and need the documented source for sushicore.

Review: found by review.

#### L15. [low] Leftover SushiTrack and 3rdparty names in tracked configuration

Evidence: D:/Projects/sushitrack/.gitignore:4 `.sushitrack_env.json` while the CLI writes `.sushitrack_env.json` (cli/sushitrack_cli/env.py:17), which is therefore not ignored; .gitignore:12 comment says `ds build --deploy`. D:/Projects/sushitrack/.gitmodules:1,4,7 name the submodules `3rdparty/TrackEval`, `3rdparty/eigen`, `3rdparty/yolox` while their paths are third_party/. cli/sushitrack_cli/config.py:123 `ST_3RDPARTY_DIR`; cli/sushitrack_cli/services/evaluate.py:112 docstring says `3rdparty/TrackEval`; docs/README.md:151 says `3rdparty subdirs`.

Recommendation: Replace the ignore rule with `.sushitrack_env.json`, fix the `ds` comment, rename the submodule sections to third_party/..., and correct the two prose references. The environment variable name is a public CLI surface: ask before renaming it.

Review: confirmed. .gitignore:4 and :12, .gitmodules:1,4,7, config.py:123, evaluate.py:112, docs/README.md:151 and env.py:17 all read as cited; git check-ignore on .sushitrack_env.json matches no rule. I did not trace which directory the cache file is written to.

#### L16. [low] Test tree departs from naming and fixture rules

Evidence: PascalCase file names against the snake_case rule: D:/Projects/sushitrack/tests/unit/TestTracker.cpp (18 files in tests/unit), tests/integration/TestFullStack.cpp, Helpers.h, Main.cpp, tests/regression/MOTRunner.h, TrackerRegistry.cpp, ScenarioList.h; headers use .h while the library uses .hpp. Loose configs at D:/Projects/sushitrack/tests/bytetrack.json and tests/ocsort.json instead of a fixtures folder. Python applications live inside the test tree: tests/regression/evaluator/{run_eval.py,data_loader.py,report_generator.py,trackeval_runner.py} and tests/regression/inference/{demo.py,pipeline.py,tracker_bridge.py}. No tests/common/.

Recommendation: Rename to snake_case .cpp/.hpp, move the two JSON files and the data under tests/<kind>/fixtures/, and ask the owner whether the evaluator and inference pipelines belong under applications/ or the CLI package.

Review: confirmed. tests/unit holds 18 PascalCase .cpp files (19 files with CMakeLists.txt); tests/integration and tests/regression use PascalCase .h/.cpp; tests/bytetrack.json and tests/ocsort.json are loose; evaluator and inference Python files are tracked under tests/regression; no tests/common/.

#### L17. [low] A second script folder inside cli/

Evidence: D:/Projects/sushitrack/cli/misc/clean_preds.bat, clean_preds.sh and construct_trackeval_data.ps1 are standalone shell scripts beside the CLI package. The repository-layout skill lists `a second CLI or script folder` as a mistake: everything runnable goes through cli/ commands or tools/.

Recommendation: Fold them into `st` subcommands (one implementation instead of a .bat/.sh pair) or delete them if `st eval` already covers the job.

Review: confirmed. cli/misc/clean_preds.bat, clean_preds.sh and construct_trackeval_data.ps1 are tracked. Folding a .bat/.sh pair into one command is a brick-shaped fix.

#### L18. [low] Ignored build residue in the working tree

Evidence: D:/Projects/sushitrack/cli/sushitrack_cli.egg-info/ (Sep 22), D:/Projects/sushitrack/.pytest_cache/, cli/.pytest_cache/, bindings/python/__pycache__/, tests/regression/evaluator/__pycache__/, build/. All ignored, none tracked. README.md installs with `pipx install cli/`, so the egg-info comes from an editable or in-tree install.

Recommendation: Remove them before packaging or a Docker build; .dockerignore does not exclude *.egg-info, so `COPY .` in the Dockerfile ships it into the image.

Review: confirmed. git status --ignored lists cli/sushitrack_cli.egg-info/, both .pytest_cache/ folders, the __pycache__ folders and build/; .dockerignore has no *.egg-info rule and Dockerfile:33 is `COPY .`.

#### L19. [low] Orphaned pack indexes in .git

Evidence: `git count-objects -vH` reports `garbage: 5`, `size-garbage: 33.04 KiB` with five `warning: no corresponding .pack` lines for D:/Projects/sushitrack/.git/objects/pack/pack-1d61e042....idx, pack-690c14a1....idx, pack-8fb62bbd....idx, pack-d03ad401....idx and pack-e2f48149....idx.

Recommendation: Run `git fsck` and then `git gc` once the owner agrees; confirm first that no object is missing, since an .idx without its .pack can mean an interrupted repack.

Review: confirmed. git count-objects -vH prints five `no corresponding .pack` warnings, garbage: 5, size-garbage: 33.04 KiB, size-pack: 100.84 MiB.

#### L20. [low] Repository and product naming is split across four spellings

Evidence: Remote is https://github.com/SushiSystems/SushiTrack.git (PascalCase), local folder D:/Projects/sushitrack, CMake project `sushitrack` (CMakeLists.txt:2), include root include/SushiTrack/, PyPI name `sushitrack-cli` with scripts `st` and `sushitrack` (cli/pyproject.toml). The name matches the product; only the casing differs. D:/Projects/obsolete_sushitrack is a stale sibling checkout with the old 3rdparty/ layout.

Recommendation: Fix one casing rule for remotes across the stack (compare the sibling remotes in the cross-repo pass) and ask the owner whether obsolete_sushitrack can be deleted.

Review: confirmed. Remote is SushiSystems/SushiTrack.git; CMake project `sushitrack`; include/SushiTrack/; package sushitrack-cli with scripts st and sushitrack; D:/Projects/obsolete_sushitrack exists with 3rdparty/, src/, test/. Low severity is right: only casing differs.

#### L21. [low] Unpushed commit on main and a personal sign-off line in CLAUDE.md

Evidence: `git status -sb` -> `## main...origin/main [ahead 1]`; the commit is 87cfedd `build(cli): require sushicore 0.7.0` (2026-10-04). D:/Projects/sushitrack/CLAUDE.md ends with `love you boss, kolay gelsin.`, a non-English line in a tracked project file of a repository about to go public.

Recommendation: Push 87cfedd when the owner says so; remove the closing line from CLAUDE.md.

Review: confirmed. git status -sb gives `main...origin/main [ahead 1]`, HEAD is 87cfedd dated 2026-10-04; CLAUDE.md ends with `love you boss, kolay gelsin.`

### Not checked

- Contents of build/ and whether anything in it is worth keeping; size of build/ and of the ignored evaluator pred/ and img1/ folders (a `du` over them exceeded the 120 s timeout).
- Whether the four tools/ checkers pass: no checker was run (task is read-only and forbids running project tools), and their K_ constants, including K_TIER_ORDER in tools/layering/check_layering.py, were not read.
- Whether tools/ is byte-identical to the reference copy in sushiskills.
- AGENTS.md content and whether it matches CLAUDE.md.
- third_party/ licence files and the pinned submodule commits (eigen sits on a nightly, TrackEval on master head) beyond listing them.
- Git history for large or binary blobs that were once tracked and later removed; only HEAD was measured.
- cli/sushistack.deps.toml and cli/config.toml contents; sushi-module.toml does not exist in this repository and no package.json exists.
- Layout comparison against siblings other than sushicore and sushistack (sushiai, sushiblas, sushidsp, sushiengine and the rest were not opened).
- Line-ending state of tracked files against .gitattributes.
- Per-file check of the 13 docs/ files for status lines and links; only presence was checked.
- Redistribution terms of the MOT17, MOT20 and DanceTrack files tracked under tests/regression/evaluator/data.

## Code shape

### Facts

- Layout is single-module: one C++ library (include/SushiTrack + source/), no modules/<tier>/ tree. tools/layering/check_layering.py:20 has K_TIER_ORDER = () so the layering checker does nothing here.
- Units and dependency direction: core C++ lib (include/, source/) -> Eigen, nlohmann json; C ABI (source/sushitrack_c.cpp, source/config_loader.cpp) sits on top of the core; bindings/python -> C ABI via ctypes; tests/unit and tests/integration -> core and C ABI; tests/regression C++ -> core + third_party/bytetrack + third_party/ocsort; cli/ -> sushicore, drives cmake/ctest and the Python under tests/regression; tests/regression Python -> cli (sushitrack_cli.console) and bindings/python.
- Inside the core the include graph is acyclic at header level except that logger.hpp, types.hpp and config_loader.hpp include the C ABI header sushitrack_c.h, so the foundation depends on the outermost surface.
- Languages (tracked, counted by extension): 55 .cpp, 34 .hpp, 16 .h, 50 .py, plus 1 .ps1, 1 .sh, 1 .bat. Tracked data: 159 .txt and 76 .ini, almost all under tests/regression/evaluator (233 files).
- Approximate tracked files/lines per unit (cpp/hpp/h/py/scripts): include 28 / 3818; source 12 / 5808; bindings 2 / 489; cli/sushitrack_cli 19 / 2091; cli/tests 15 / 1072; cli/misc 3 / 142; tests/unit 18 / 7460; tests/integration 12 / 2079; tests/regression 18 / 3427; tools 6 / 816. Total about 27060 lines outside third_party.
- Twenty largest source files (lines): source/distance_strategies.cpp 1191; tests/unit/TestDistanceStrategies.cpp 1045; source/tracker.cpp 970; source/imm_filter.cpp 621; source/tracklet.cpp 604; tests/unit/TestTracklet.cpp 586; tests/unit/TestIntegration.cpp 585; tests/unit/TestKalmanFilter.cpp 563; include/SushiTrack/distance_strategies.hpp 563; source/kalman_filter.cpp 545; source/sushitrack_c.cpp 533; tests/unit/TestBoundaryConditions.cpp 515; tests/regression/inference/pipeline.py 498; tests/unit/TestConfiguration.cpp 486; tests/unit/TestCAPIErrors.cpp 471; bindings/python/sushitrack.py 470; tests/unit/TestTracker.cpp 465; tests/regression/evaluator/trackeval_runner.py 441; tests/unit/TestTrackerTentative.cpp 440; include/SushiTrack/tracklet.hpp 406.
- Test layout: GoogleTest. tests/unit (17 test files + UnitTest.cpp main), tests/integration (10 test files + Main.cpp + Helpers.h), tests/regression (one RegressionTest.cpp with adapters for SushiTrack, ByteTrack, OCSORT, plus Python evaluator/ and inference/), cli/tests (14 pytest files + conftest.py). No tests/golden, no tests/benchmark.
- No tests at all: bindings/python (sushitrack.py, setup.py), tools/ (4 checkers + common/checker.py), tests/regression Python (evaluator/*.py, inference/*.py), cli/misc scripts. In C++, no test names IMMFilter, lapjv directly, KalmanFilterRegistry, GenericKalmanFactory, loadConfigFromJson or resolveConfigPath (grep over tests/unit, tests/integration, tests/regression C++ returned 0 files for each).
- Every tracked C++ and Python file has an @file block or a module docstring; no TODO/FIXME/HACK/XXX and no @date in project source (only inside the checker's own regex). No stacked // runs in source/ or include/ apart from sushitrack_c.h:36.
- Module verdicts. Core C++ library (include/ + source/): needs restructuring. C ABI + config loader (sushitrack_c.cpp, config_loader.cpp): needs restructuring. bindings/python: needs targeted fixes. cli/sushitrack_cli: needs targeted fixes. cli/tests: sound. tests/unit and tests/integration: needs targeted fixes. tests/regression (C++ harness + Python): needs restructuring. tools/: sound in shape, but unconfigured and untested (targeted fixes).
- Three untracked cmake_test_discovery_*.json files sit in the repository root (not in git ls-files); git status at the start was clean, so they are ignored build debris.

### Corrections from review

- F8 says the Mahalanobis gate block is copied three times. getGateSqDistances with the 1e4f sentinel appears at source/distance_strategies.cpp:855, :915, :1126 and :1169 (two matrix forms, two single-pair forms), plus the in_gate variant near :737-754: four copies plus one variant.
- F6 places the raw `new` at source/sushitrack_c.cpp:398; it is at :402. The function starts at :208, not :201.
- F18 says fourteen commands live in cli/sushitrack_cli/cli.py. The file carries 13 command decorators (lines 55, 99, 137, 160, 167, 190, 206, 230, 278, 331, 337, 343, 349); further commands come from sushicore's register_provision_commands.
- F19 says the future import is missing from 9 of 11 files under cli/sushitrack_cli/services. The folder has 12 tracked files and 10 lack it (the nine named plus services/__init__.py); only artifacts.py and checks.py carry it. It also says 'all seven' regression Python files; there are eight (evaluator/__init__.py, data_loader.py, report_generator.py, run_eval.py, trackeval_runner.py, inference/demo.py, pipeline.py, tracker_bridge.py).
- F20 lists six Python files without a licence line. Seven tracked files have no licence or copyright text in their first 12 lines: cli/sushitrack_cli/__init__.py, __main__.py, console.py, services/__init__.py, and also cli/tests/conftest.py, cli/tests/test_console_lazy.py, cli/tests/test_service_enums.py, which the audit omits. tests/regression/inference/pipeline.py and demo.py are not licence-less: they carry '@copyright ... All rights reserved', which contradicts the Apache header on the other 121 source files.
- F22 calls sushitrack_create_tracker near-dead. It is the entry point the deploy templates and the migration design document tell users to call (cli/sushitrack_cli/services/deploy.py:39 and :109, docs/design/SUSHITRACK_MIGRATION.md:168).
- F16 cites tracker_bridge.py:11-15; the sys.path edit is at :14-15. It omits tests/regression/inference/demo.py:22-24 and tests/regression/evaluator/trackeval_runner.py:14-15, which do the same.
- Recomputed and correct: extension counts (159 txt, 76 ini, 55 cpp, 50 py, 34 hpp, 16 h), all ten per-unit file and line counts, the 27060 total, the twenty largest files, and 233 tracked files under tests/regression/evaluator.

### Findings

#### C1. [high] Logger breaks the logging shape: no categories, printf format, macros that collide with C enum names

Evidence: include/SushiTrack/logger.hpp:78-81 defines SUSHITRACK_LOG_DEBUG/INFO/WARN/ERROR(...) as printf-style variadics with no category argument; the same identifiers are enumerators in include/SushiTrack/sushitrack_c.h:68 (SUSHITRACK_LOG_ERROR = 3) and SUSHITRACK_RUNTIME_ERROR is both a status enumerator (sushitrack_c.h:56) and a throwing macro (logger.hpp:86). Categories are faked as string prefixes ('[Tracker]', '[Solver]', '[Distance]') at every call site, e.g. source/tracker.cpp:99, source/assignment_solvers.cpp:94. No <module>_log_category.hpp exists. No TRACE or FATAL level, no compile-time cut: arguments are always evaluated. Logger is a static-only class over a file-level global (source/logger.cpp:40).

Recommendation: Rebuild the logger as the foundation brick the logging skill describes: a category constant per area, positional {} formatter, compile-time minimum level, and macro names that do not reuse the C ABI enumerators. This is a public-surface change, so the owner decides the shape first.

Review: confirmed. include/SushiTrack/logger.hpp:78-86 and sushitrack_c.h:56,68 read as cited: printf variadics, no category, macro names equal to the C enumerators, no TRACE/FATAL, no compile-time cut; source/logger.cpp:40 holds the global callback.

#### C2. [high] Exception text passed as a printf format string

Evidence: source/sushitrack_c.cpp:425 `SUSHITRACK_LOG_ERROR(e.what());` and include/SushiTrack/kalman_filter_registry.hpp:87 `SUSHITRACK_LOG_ERROR(("[KalmanFilterRegistry] Unregistered Kalman type requested: " + std::to_string(type)).c_str());`. Both hand a runtime string to Logger::log as the format argument (source/logger.cpp:80 runs vsnprintf on it). A '%' in an exception message reads garbage varargs. The second also concatenates at the call site, which the logging skill forbids. The first also names no operation or consequence.

Recommendation: Pass a literal format with the message as an argument in both places; state the failed operation and what the caller receives.

Review: confirmed. source/sushitrack_c.cpp:425 passes e.what() as the format; kalman_filter_registry.hpp:87 passes a concatenated c_str(); logger.cpp:80 runs vsnprintf on it.

#### C3. [high] Per-element log calls inside the cost-matrix hot loops

Evidence: source/distance_strategies.cpp:751, :863 and :1134 log once per gated (track, detection) pair inside the i/j loops. source/tracker.cpp logs per track per frame at :343, :415, :482, :544, :605, :669, :672, :699, :723, :762, :770, :782 and at INFO per expired track at :818. Because logger.hpp has no compile-time cut, the (unsigned long long) casts and the call are paid in release builds.

Recommendation: Replace per-pair and per-track lines with counters reported once per update, or move them to a TRACE level that compiles out.

Review: confirmed. distance_strategies.cpp:751, :863, :1134 log inside the j loop. The tracker.cpp lines exist, but most fire on a state transition, not every track every frame; :762 is the real per-track-per-frame one. With no callback set logV returns early, so the release cost is the call and argument evaluation.

#### C4. [high] Third-party Eigen types leak through the public headers

Evidence: include/SushiTrack/rect.hpp:32, include/SushiTrack/kalman_types.hpp:32 and include/SushiTrack/kalman_filter.hpp:34 include <Eigen/Dense>; KalmanTypes::StateMean/StateCov/DetectBox appear in the pure interfaces i_state_predictor.hpp and i_gate_validator.hpp (e.g. i_gate_validator.hpp:112-113) and in tracklet.hpp:302-303; tracklet.hpp:69 exposes EIGEN_MAKE_ALIGNED_OPERATOR_NEW. Any consumer of the C++ headers must have Eigen at the same version and alignment settings.

Recommendation: Decide with the owner whether the C++ headers are a public surface. If they are, hide Eigen behind project-owned value types; if only the C ABI is public, move the C++ headers out of include/ so the leak stops being public.

Review: confirmed. <Eigen/Dense> at rect.hpp:32, kalman_types.hpp:32, kalman_filter.hpp:34; EIGEN_MAKE_ALIGNED_OPERATOR_NEW at tracklet.hpp:69; object.hpp:63 and imm_filter.hpp:109-139 leak it too.

#### C5. [high] The parameter list is written out in five places that every new parameter must edit

Evidence: One field, tentative_slow_hits_offset, appears at include/SushiTrack/sushitrack_c.h:183 (C struct), include/SushiTrack/tracker_config.hpp:206 (C++ struct with its own default), source/sushitrack_c.cpp:163 (default again) and :244 (field-by-field copy, lines 206-262), source/config_loader.cpp:149 (JSON overlay) and bindings/python/sushitrack.py:171 (ctypes mirror). Defaults are stated twice (tracker_config.hpp and sushitrack_c.cpp:~130-185) and can drift.

Recommendation: Describe the parameters once (a field table or X-macro owned by one header) and derive the defaults, the C-to-C++ copy and the JSON overlay from it; generate or verify the Python ctypes mirror against it in a test.

Review: confirmed. grep for tentative_slow_hits_offset gives exactly the six cited sites plus its use at tracker.cpp:576. SOLID check on the fix: a single field table is brick-shaped; an X-macro inside the public C header would make the ABI struct unreadable, so the table should live in a private source header.

#### C6. [high] sushitrack_create_ex is a 200-line composition root with integer kind chains

Evidence: source/sushitrack_c.cpp:201-427 does parameter translation, gate-validator selection, an if/else-if chain over config.iou_type == 1/2/3 (:299-318), a four-way branch over enable_mahalanobis x enable_reid that wires eleven calculator classes (:331-381), decorator stacking (:383-396) and handle allocation with a raw `new` (:398). Kalman kinds already use a registry (kalman_filter_registry.hpp:51) but IoU kinds do not, so siblings are shaped differently and each new IoU variant edits this function. kalman_type, iou_type and primary_assoc_mode are bare ints (tracker_config.hpp:75).

Recommendation: Split into a params-to-config translator, a metric-pipeline builder and the C handle wrapper. Give IoU variants the same registration shape as Kalman filters, and replace the int kinds with sized enum classes.

Review: confirmed. sushitrack_create_ex spans source/sushitrack_c.cpp:208-427 (try at 214), iou_type chain at :299-318, raw new at :402 (audit says :398). The eleven-class four-way branch was not re-counted.

#### C7. [high] Tracklet carries six responsibilities and also stands in for a detection

Evidence: include/SushiTrack/tracklet.hpp:66-406: lifecycle state machine (activate/confirm/markAsLost, source/tracklet.cpp:111-387), Kalman mean/covariance (:302-303), appearance feature gallery (updateFeatures, getMaxFeatureSimilarity, feature_history_), OCM observation velocity (obs_history_, recordObservation at tracklet.cpp:460), ORU checkpoint replay (captureCheckpoint, recoveryReupdate at :505-548), IMM regime classification (refreshRegime :345) and a mutex (state_mutex_). Tracker passes detections as Tracklets too: det_high and det_low are std::vector<TrackletPtr> in include/SushiTrack/tracker.hpp matchHighConfidence/matchLowConfidence, and update(const Tracklet& new_track, ...) at tracklet.cpp:273 takes a detection dressed as a track.

Recommendation: Separate a Detection value type from Tracklet, and lift the feature gallery, observation-momentum state and ORU checkpoint into their own bricks that Tracklet composes. The module-by-module wave should start here.

Review: confirmed. tracklet.hpp:66-406 and tracklet.cpp:128-273 show the lifecycle, checkpoint, observation and regime calls in one class; tracker.hpp:139-203 passes det_high/det_low as std::vector<TrackletPtr>; update(const Tracklet&) at tracklet.cpp:273.

#### C8. [high] distance_strategies holds eleven classes in one header and one 1191-line source, with the gating block copied three times

Evidence: include/SushiTrack/distance_strategies.hpp declares IoU, DIoU, GIoU, SIoU, Cosine, ReIDFirst, PureReID, Fused, FuseScoreDecorator, ObservationMomentumDecorator and GatedIoU calculators (lines 46-519), against the one-primary-type-per-header rule. source/distance_strategies.cpp repeats the Mahalanobis gate (getGateSqDistances / sentinel 1e4f / identical log line) at :850-866 and :1120-1136 and a third variant at :737-754; the four IoU calculate() bodies (:75, :164, :291, :408) share the same loop skeleton. The sentinel 1e4f is a repeated magic number.

Recommendation: One calculator per file; extract the gate pass and the pairwise-loop skeleton into shared bricks; name the sentinel as a K_ constant.

Review: confirmed. Eleven classes at distance_strategies.hpp:46-519. getGateSqDistances is called at :855, :915, :1126, :1169, so the gate block exists four times, not three. 1e4f appears on about 45 lines.

#### C9. [high] C ABI update and query calls report failure as 'zero tracks', and no entry point catches non-std exceptions

Evidence: source/sushitrack_c.cpp:465-467 returns 0 for a null handle, null buffer or negative count with no log; :490-494 and :517-521 catch std::exception, log, and return 0, which a caller cannot tell from an empty frame. sushitrack_update and sushitrack_get_tracklets return int, so sushitrack_status_t never reaches the caller on the hot path. Every extern "C" function catches only std::bad_alloc and std::exception (:418-427, :490, :517); there is no catch-all, so anything else unwinds through the C boundary. F14 covers only the create and config paths.

Recommendation: Give the per-frame calls a status return (count through an out-parameter, as sushitrack_create_ex already does for the handle) and convert every exception to a status at the boundary, including a final catch-all that logs and returns SUSHITRACK_RUNTIME_ERROR. This changes the public C surface, so the owner decides.

Review: found by review.

#### C10. [medium] KinematicState policy is spread over four switches in three files

Evidence: source/distance_strategies.cpp:57 (gate scale and ReID shift per state), source/distance_strategies.cpp:1003 (OCM weight per state), source/tracker.cpp:797 (lost-buffer scale per state), plus IMM mode switches at source/imm_filter.cpp:127 and :204. A new kinematic state or IMM mode must be added to each, and three of them hide the omission behind `default:`.

Recommendation: Hold the per-state modifiers in one table keyed by KinematicState, owned beside include/SushiTrack/kinematic_state.hpp, and have the three consumers read it.

Review: confirmed. switch at distance_strategies.cpp:57 and :1003, tracker.cpp:797, imm_filter.cpp:127 and :204; default: at :63, :1007, :803.

#### C11. [medium] Tracker is a nine-stage pipeline in one class with seven-parameter in/out stage methods

Evidence: include/SushiTrack/tracker.hpp: constructor takes eight arguments, five of them ICostCalculator pointers for named stages; private stage methods matchHighConfidence, matchLowConfidence, matchLostRecovery, matchTentative (seven vector<TrackletPtr>& parameters), matchUnconfirmed, cleanupLost, finalizeLists, plus set utilities jointTracklets/subTracklets/removeDuplicateTracklets. source/tracker.cpp is 970 lines. `mutable` scratch members (current_visit_id_, main_cost_matrix_, iou_cost_matrix_) let const methods mutate state. Tracker also constructs its own timing policy (tracker.cpp includes timing_policies.hpp at :35) rather than receiving it.

Recommendation: Model each association stage as an object behind one stage interface and let Tracker run a list of them; move the tracklet-set utilities to free functions; inject the timing policy.

Review: confirmed. tracker.hpp:120-123 mutable scratch members, stage methods at :139-203; tracker.cpp:60-61 builds WallClockTimingPolicy or FrameTimingPolicy itself. The stage-interface fix is brick-shaped.

#### C12. [medium] Core foundation depends on the C ABI header, and hidden singletons replace injected dependencies

Evidence: include/SushiTrack/logger.hpp:32, include/SushiTrack/types.hpp:32 and include/SushiTrack/config_loader.hpp:32 include <SushiTrack/sushitrack_c.h>; Logger's API is typed in sushitrack_log_level_t (logger.hpp:59). Singletons: KalmanFilterRegistry::getInstance() (kalman_filter_registry.hpp:58) filled by static-init registrars with magic ints at source/kalman_filter.cpp:42-43 and source/imm_filter.cpp:40; NullGateValidator::getInstance() (i_gate_validator.hpp:99); global g_log_callback (source/logger.cpp:40). Static registrars in a static library can be dropped by the linker.

Recommendation: Give the core its own level and status enums and let the C ABI convert at its boundary. Pass a registry or factory in through constructors instead of reaching for a global.

Review: confirmed. logger.hpp:32 and types.hpp:32 include sushitrack_c.h; getInstance at kalman_filter_registry.hpp:58 and i_gate_validator.hpp:99; registrars at kalman_filter.cpp:42-43 and imm_filter.cpp:40. config_loader.hpp:32 was not opened.

#### C13. [medium] Interfaces and implementations share headers; one filter subclasses another concrete filter

Evidence: include/SushiTrack/i_kalman_factory.hpp:41 and :59 hold IKalmanFactory and GenericKalmanFactory, whose definition lives in source/kalman_filter.cpp:~540; include/SushiTrack/i_gate_validator.hpp:42 and :92 hold IGateValidator and NullGateValidator; timing_policies.hpp:42,75 holds two policies; kalman_filter_registry.hpp:51,101 is fully header-implemented and logs from a header. NSAKalmanFilter derives from the concrete KalmanFilter (nsa_kalman_filter.hpp:39) and both are registered from kalman_filter.cpp; there is no source/nsa_kalman_filter.cpp. Only the timing policies are marked final.

Recommendation: One type per header with its own source file; mark leaf classes final; decide whether NSA is a decoration of KalmanFilter rather than a subclass.

Review: confirmed. i_kalman_factory.hpp:59, i_gate_validator.hpp:92, timing_policies.hpp:42,75 (the only final classes), nsa_kalman_filter.hpp:39 derives from KalmanFilter; source/ has no nsa_kalman_filter.cpp.

#### C14. [medium] C++ naming and class shape diverge from cpp-code-style across the whole core

Evidence: camelCase methods throughout (getTracks, getGlobalFrameId, predictTracks in tracker.hpp; setCallback, logV in logger.hpp:51-64, beside snake_case throw_runtime_error at :72); I-prefixed interfaces in seven i_*.hpp files; constants kDefaultConfigName, kMaxSearchDepth (source/config_loader.cpp:46-47) and kRegimeStationaryProb (tracklet.hpp) instead of K_UPPER_SNAKE; unsized `enum class TrackState` (tracklet.hpp:50) beside the sized KinematicState (kinematic_state.hpp:42); access specifiers indented in tracker.hpp/cost_matrix.hpp:44 but flush in logger.hpp and kalman_filter_registry.hpp; no [[nodiscard]] on Tracker::update or getTracks; test files are PascalCase (tests/unit/TestTracker.cpp) and regression headers use .h.

Recommendation: Treat the rename as one API-stability decision for the owner (it breaks the C++ surface), then apply it mechanically in one wave with clang-format.

Review: confirmed. kDefaultConfigName/kMaxSearchDepth at config_loader.cpp:46-47, unsized TrackState at tracklet.hpp:50, flush access specifiers in logger.hpp:46, camelCase beside throw_runtime_error. Test file names and [[nodiscard]] were not rechecked.

#### C15. [medium] Configuration loading hides failures

Evidence: source/config_loader.cpp:282-283 logs WARN and returns SUSHITRACK_SUCCESS when the resolved file cannot be opened; sushitrack_create_from_config at :~303-308 discards the status of sushitrack_load_params_from_json and creates a tracker from defaults even after a parse error; getInt/getFloat (:49-77) return the default silently for a wrongly typed value, and getInt casts any double to T. loadConfigFromJson returns bool (include/SushiTrack/config_loader.hpp:52) where the style asks for a status enum. sushitrack_create returns nullptr with the status dropped (source/sushitrack_c.cpp:193-196).

Recommendation: Return a status naming the reason (not found, unreadable, parse error, wrong type) and forward it; make a malformed explicit path a failure rather than a silent default.

Review: confirmed. config_loader.cpp:281-285 returns SUCCESS on an unopenable file; :303-309 drops the status of sushitrack_load_params_from_json; getInt at :49-66 casts any number through double; sushitrack_create at sushitrack_c.cpp:190-200 drops the status.

#### C16. [medium] Swallowed exceptions in test and harness code

Evidence: tests/regression/TestInfra.h:174 `catch (...) { break; }` drops a malformed detection row silently; tests/regression/TestInfra.h:209-224 parse_json prints to std::cerr and returns an empty object, so a broken config runs the regression with defaults; tests/unit/TestConfiguration.cpp:172-175 `catch (...) { return Json::object(); }`. Python: cli/sushitrack_cli/services/evaluate.py:61 `except Exception as exc`, tests/regression/inference/demo.py:129 and tests/regression/inference/pipeline.py:313 catch bare Exception.

Recommendation: Fail the test or the run on unreadable input; catch the specific exception types and re-raise as the module's own error.

Review: confirmed. catch (...) at TestInfra.h:174 and :221, TestConfiguration.cpp:172; except Exception at evaluate.py:61, demo.py:129, pipeline.py:313. No other match in tracked files.

#### C17. [medium] CLI and regression Python import each other through sys.path edits

Evidence: cli/sushitrack_cli/services/evaluate.py:18-19 inserts tests/regression/evaluator into sys.path and imports it; in the other direction tests/regression/evaluator/report_generator.py:12-15, run_eval.py:13-18 and tests/regression/inference/pipeline.py:19-24 insert cli/ into sys.path and import sushitrack_cli.console (pipeline.py:19 calls it 'the same hack'); tests/regression/inference/tracker_bridge.py:11-15 does the same for bindings/python. cli/sushitrack_cli/config.py:84-86 hard-codes the tests/regression paths.

Recommendation: Break the cycle: the evaluator and inference code are tools, not tests, and want their own package that the CLI depends on one way, with output passed in rather than imported from the CLI.

Review: confirmed. sys.path edits at evaluate.py:18-19, report_generator.py:12-13, run_eval.py:13-16, pipeline.py:19-23, tracker_bridge.py:14-15, demo.py:22-24; config.py:84-86 hard-codes the tests/regression paths. The audit missed demo.py and trackeval_runner.py:14-15.

#### C18. [medium] tests/regression mixes a regression suite, a benchmark harness, an evaluator and a demo, with the tracker list kept in three places

Evidence: tests/regression holds RegressionTest.cpp plus header-only adapters (ByteTrackAdapter.h, OCSORTAdapter.h, SushiTrackAdapter.h, MOTRunner.h, TestInfra.h), a C shim exported for Python (tests/regression/TrackerRegistry.cpp:40-90), evaluator/ (233 tracked files, mostly data) and inference/demo.py. Tracker names are listed in tests/regression/TrackerRegistry.cpp:37-40 (with 'SushiTrack' and 'sushitrack' both registered), cli/sushitrack_cli/services/label.py:15-19 and tests/regression/inference/demo.py:39; tracker_bridge.py:45 branches on the string 'sushitrack'. cli/sushitrack_cli/services/label.py:90-106 parses PERF lines from this binary, so the suite is a measurement, which the testing skill places under tests/benchmark and never as pass/fail. Tests are not named after a defect.

Recommendation: Split by purpose: benchmark harness under tests/benchmark, evaluator and inference as a tool package, and keep tests/regression for defect-named tests. Keep one source for the tracker list.

Review: confirmed. TrackerRegistry.cpp:37-40 registers SushiTrack twice; label.py:15-19 repeats the list; RegressionTest.cpp has one TEST_P (LabelInference) and label.py:90-106 parses its PERF output.

#### C19. [medium] CLI services print, exit and share a module-level runner; there is no commands/ layer

Evidence: All fourteen commands live in cli/sushitrack_cli/cli.py (401 lines, :55-360) instead of commands/. Every service imports the console: build.py:12, deploy.py:14, diag.py:10, docker.py:8, evaluate.py:8, infer.py:12, label.py:7, runner.py:5, tests.py:9; label.py:104-105 calls print() directly. Services raise SystemExit (services/build.py:44, services/label.py:26) and return int exit codes rather than raising a module error; no module base exception exists. cli/sushitrack_cli/process.py:29 `RUNNER = make_runner(console)` is a module-level singleton imported by seven services. cli.py:33-35 imports enums from three services, and services/build.py:19 `BuildType(str, enum.Enum)` uses lower-case members.

Recommendation: Compare with the other Sushi CLIs before changing: add a commands/ package, pass an output port and the runner into services, and raise a StError base class that the entry point turns into one line and an exit code.

Review: confirmed. Nine services import console; SystemExit at build.py:44 and label.py:26; print at label.py:104-105; RUNNER at process.py:29. cli.py carries 13 command decorators, not 14 (see wrong_facts).

#### C20. [medium] Python style gaps: missing future import, missing annotations, non-K_ constants, no logging

Evidence: `from __future__ import annotations` is missing from 9 of 11 files under cli/sushitrack_cli/services/ (build.py, deploy.py, diag.py, docker.py, evaluate.py, infer.py, label.py, runner.py, tests.py) and from all seven tests/regression Python files. Unannotated signatures: cli/sushitrack_cli/process.py:16 `make_runner(output)`, config.py:151 `legacy_tool_keys(document)`, services/build.py:39, services/label.py:22. Constants without K_: PROFILE (config.py:26), CONFIG_NAMES, IS_WINDOWS (config.py:42-44), MISSING_EXIT_CODE, RUNNER (process.py:13,29), LIBRARY_TARGET, TEST_TARGETS, STAMP_NAME (services/build.py:27-35), TRACKERS (services/label.py:15), while cli.py:36-38 uses K_. No K_LOGGER or logging.getLogger anywhere in cli/, bindings/ or tests/regression. bindings/python/sushitrack.py:79 SushiTrackError derives from RuntimeError and the file holds eleven classes.

Recommendation: A mechanical pass per file; the module wave can take cli/ and bindings/ together.

Review: confirmed. The future import is missing in the nine named files and in services/__init__.py, and in eight regression Python files. No getLogger or K_LOGGER in any tracked Python. Constant names at process.py:13,29 seen; the rest not reopened.

#### C21. [medium] Source-comment rule breaks: separators, non-Doxygen docstrings, missing licence lines, missing @brief

Evidence: Separator lines in cli/sushitrack_cli/cli.py:52-54, :179-181, :227-229, :328-330. tests/regression/inference/pipeline.py:1-7 opens with a Doxygen-tagged docstring carrying @version and @copyright, no licence line, and uses @brief/@param inside Python docstrings (:45-56); demo.py also lacks the licence line, as do cli/sushitrack_cli/__init__.py, __main__.py, console.py and services/__init__.py. Paragraph comments: cli/sushitrack_cli/console.py:28-29, pipeline.py:19-20, console.py's 12-line module docstring (limit six). Missing @brief: getters at include/SushiTrack/tracklet.hpp:81-92, private members at :301-317 and helpers updateRect/updateRectXyah/normalizeL2; struct fields at source/sushitrack_c.cpp:50-53 and source/distance_strategies.cpp:50-52; free function stateAssocMods at :55. Briefs that are not one verb-first sentence: tracker.hpp class block, tracklet.hpp immActive ('True when...'), and history wording at tests/unit/TestCAPIErrors.cpp:123 ('no longer a misuse') and cli/tests/test_console_lazy.py:3 ('used to build'). Design reasoning sits in comments at tracklet.hpp (kRegimeStationarySpeedFrac, frames_since_observation_). .cpp files repeat the header's Doxygen block for each definition (source/logger.cpp:42-45, :52-56).

Recommendation: Run tools/documentation/check_source_comments.py over include, source, cli, bindings and tests through the project's door and fix what it reports; the missing-@brief cases need a manual pass because the checker does not cover them (tools/README.md 'Not checked').

Review: confirmed. Eight separator lines in cli.py at the cited positions; console.py has a 12-line docstring; tracklet.hpp:81-92 getters and :301-317 members have no @brief; logger.cpp:42-56 repeats header blocks. Licence part is incomplete and misworded, see wrong_facts.

#### C22. [medium] Coverage gaps in the core and no tests for bindings, tools or the regression Python

Evidence: grep for IMMFilter, imm_filter, lapjv, KalmanFilterRegistry, GenericKalmanFactory, loadConfigFromJson and resolveConfigPath over tests/unit/*.cpp, tests/integration/* and tests/regression/*.{h,cpp} returns no file, while source/imm_filter.cpp is 621 lines and source/lapjv.cpp 370. IMM is reached only through the C API with kalman_type set in config-driven tests. bindings/python/ has no test file; tools/ has none; tests/regression/evaluator and inference have none. Unit tests are per theme, not per source file: TestIntegration.cpp (585 lines) sits in tests/unit, and TestBoundaryConditions, TestTrackerBuffers, TestReID map to no single unit. cli/sushitrack_cli/services/docker.py, evaluate.py and infer.py have no test_<name>.py of their own in cli/tests.

Recommendation: Add direct unit tests for IMMFilter, lapjv and the config loader; add a ctypes layout test for the bindings against sushitrack_c.h; move TestIntegration.cpp to tests/integration.

Review: confirmed. Recomputed: IMMFilter, imm_filter, lapjv, KalmanFilterRegistry, GenericKalmanFactory, loadConfigFromJson, resolveConfigPath each match 0 test files. The loader is reached through the C entry points: sushitrack_load_params_from_json is named in 2 test files and kalman_type in 8.

#### C23. [medium] The core signals failure by throwing through its public C++ surface, and the logger owns the throw

Evidence: include/SushiTrack/logger.hpp:72 and :86 put throw_runtime_error inside Logger (source/logger.cpp:110 throws std::runtime_error), so one class both records and raises. It is used for internal invariant breaks at source/lapjv.cpp:321, :325, :333 and source/assignment_solvers.cpp:226, :248; source/lapjv.cpp:351 throws std::invalid_argument for a null workspace. cpp-code-style asks for an assert macro on a broken precondition and a status enum for an external failure, and forbids exceptions crossing a module's public surface. The repository has no SUSHITRACK_ASSERT.

Recommendation: Take the throw out of Logger. Add an assert/verify macro for the lapjv invariants and return a solver status from IAssignmentSolver for the numerically unstable case, which Tracker then handles.

Review: found by review.

#### C24. [medium] Tracklet exposes a public mutable scratch field that Tracker writes, next to a mutex that guards only some methods

Evidence: include/SushiTrack/tracklet.hpp:298 declares `mutable uint64_t visit_id_ = 0;` in the public section with a private-member name; source/tracker.cpp:890, :897, :916 write it from Tracker's set utilities. state_mutex_ (tracklet.hpp:403) is locked at tracklet.cpp:187, :231, :278, :315, :325, :600 while the inline getters at tracklet.hpp:81-92 read the same fields unlocked, so the locking promises nothing a caller can rely on. F7 names the mutex but not the leak of Tracker's bookkeeping into Tracklet.

Recommendation: Keep the visit marks in Tracker (a set or an index-keyed vector) so Tracklet carries no field for another class's algorithm, then decide with the owner whether Tracklet is meant to be thread-safe; if not, remove the mutex.

Review: found by review.

#### C25. [medium] Two source files carry a proprietary copyright line that contradicts the Apache header everywhere else

Evidence: tests/regression/inference/pipeline.py:6 and tests/regression/inference/demo.py:6 say '@copyright ... All rights reserved'; the other 121 tracked source files carry the Apache block. The audit records these two only as 'no licence line'. Three test files with no header at all (cli/tests/conftest.py, test_console_lazy.py, test_service_enums.py) are missing from its list.

Recommendation: Put all nine files on the list for the licence-header wave so the replacement is applied to every file, and remove the 'All rights reserved' line in the same change.

Review: found by review.

#### C26. [low] Dead or near-dead code and duplicate entry points

Evidence: include/SushiTrack/types.hpp:36-38 (Status, LogLevel, LogCallback aliases) is included only by tests/unit/TestLogger.cpp:32. sushitrack_create_tracker (source/config_loader.cpp:315-318) only forwards to sushitrack_create_from_config. bindings/python/sushitrack.py:247-305 resolves alternate PascalCase symbol names (sushitrack_GetDefaultParams, sushitrack_CreateEx, sushitrack_Destroy) that the C header does not export. tests/regression/TrackerRegistry.cpp:38-39 registers the same adapter under two spellings. cli/sushitrack_cli/config.py:147-162 carries a legacy-key migration path. cli/misc holds three loose scripts (clean_preds.bat, clean_preds.sh, construct_trackeval_data.ps1) outside the CLI. TrackerConfig comment at include/SushiTrack/tracker_config.hpp:75 documents an int mode as '0: Age-cascade legacy'.

Recommendation: Confirm each with the owner before removal, since deleting a public C symbol or header is an API-stability decision.

Review: confirmed. types.hpp is included only by tests/unit/TestLogger.cpp:32; config_loader.cpp:317-320 forwards. sushitrack_create_tracker is not dead: cli/sushitrack_cli/services/deploy.py:39,109 and docs/design/SUSHITRACK_MIGRATION.md:168 present it as the main entry, so the duplicate to question is sushitrack_create_from_config. sushitrack_destroy_tracker (sushitrack_c.cpp:445) is a second forwarding pair the audit left out.

#### C27. [low] The layering checker is unconfigured, so nothing enforces dependency direction here

Evidence: tools/layering/check_layering.py:20 `K_TIER_ORDER: tuple[str, ...] = ()`; collect() at :57-62 only yields a tree when root/modules exists, which this repository does not have, and tools/README.md states Python imports across modules are not checked. The cycles in F11 and F16 are therefore invisible to the tooling.

Recommendation: Either declare the single-module case explicitly in the checker's settings or extend it to cover include/ and the Python import direction once the owner has fixed the boundaries.

Review: confirmed. tools/layering/check_layering.py:20 K_TIER_ORDER = () and collect() at :57-62 only yields when root/modules exists.

### Not checked

- Line-by-line reading of source/tracker.cpp, source/imm_filter.cpp, source/kalman_filter.cpp, source/lapjv.cpp, source/assignment_solvers.cpp, source/rect.cpp, source/object.cpp: only headers, greps and excerpts were read.
- include/SushiTrack/cost_matrix.hpp, lapjv.hpp, imm_filter.hpp, nsa_kalman_filter.hpp, object.hpp, rect.hpp, kinematic_state.hpp, timing_policies.hpp, sushitrack_export.hpp: only their type declarations and includes were listed.
- tests/unit and tests/integration bodies: test names, determinism and tolerance use were checked only by grep (no rand/clock use and no bare float == found by the patterns used); test quality was not reviewed.
- tests/regression adapters (ByteTrackAdapter.h, OCSORTAdapter.h, SushiTrackAdapter.h, MOTRunner.h, ScenarioList.h, ITrackerAdapter.h, TrackerRegistry.h) and the Python evaluator (data_loader.py, report_generator.py, trackeval_runner.py, run_eval.py) were not read beyond their imports.
- cli/sushitrack_cli/env.py, services/artifacts.py, checks.py, diag.py, docker.py, infer.py, runner.py, tests.py and the body of services/deploy.py: only imports and excerpts.
- cli/tests contents: file list only.
- tools/ checkers: only tools/README.md and the head of check_layering.py were read; no checker was run (read-only task, no project tooling executed).
- bindings/python/setup.py and the Tracker class body in bindings/python/sushitrack.py after line 330.
- CMakeLists.txt files, Dockerfile, .clang-format presence, CI workflows and platform-portability (export macros, __restrict use in tracker.hpp).
- third_party/ (bytetrack, ocsort, eigen, yolox, TrackEval, nlohmann) and the licence/NOTICE side of vendoring.
- A complete count of symbols missing @brief: sampled in tracklet.hpp, sushitrack_c.cpp and distance_strategies.cpp only.
- Thread-safety of Tracklet::state_mutex_ and the mutable scratch matrices in Tracker.
- One of my survey commands (a recursive grep over cli/ and tests/regression directories) timed out and was replaced by a tracked-files-only version; no result from the timed-out run is used.
