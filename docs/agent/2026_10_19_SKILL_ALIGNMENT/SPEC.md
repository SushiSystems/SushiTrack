# Skill alignment

**Status:** Open — specified; the plan is in `PLAN.md`

Bring SushiTrack in line with every SushiSkills rule, so that every shared checker passes on the
tree and the layout, the test tree, the C++ style and the manual match the skills.

## Acceptance

`python tools/check_all.py .` prints nothing and exits 0, and `st build` followed by
`st test --label small --label medium` passes, on branch `feature/skill_alignment`.

## Owner decisions taken on 2026-10-19

| Question | Decision |
| --- | --- |
| Public headers | Move under `include/SushiTrack/<area>/`, one primary symbol per header |
| `tests/regression/` | Leaves the test tree for `applications/`; the benchmark data stays in git |
| `st test --suite` | Replaced by `--label small, medium, large, quarantined`, with no alias |
| `bindings/python/` | Stays where it is; its home is an open decision for SushiSkills |

## Out of scope

- The decisions `docs/design/REMAINING_WORK.md` leaves to the owner: the `v1.0.0` tag, the
  contributor agreement, the home of `bindings/`, the benchmark data leaving git.
- The defects in `docs/reference/KNOWN_ISSUES.md` that change tracker output or the C ABI.
