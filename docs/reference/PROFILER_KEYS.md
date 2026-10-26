# Profiler keys

The names SushiTrack records under. They are a contract with the analyses: the record stores
the names, so a renamed key shows as missing and not as a wrong number. Each constant is declared
in `include/SushiTrack/profiling/profiler_keys.hpp`; this page lists them.

## Timeline and groups

| Name | Kind | Meaning |
| --- | --- | --- |
| `sushitrack.frame` | Timeline | The host marks it once per frame, before `Tracker::update`; one period is one call |
| `Track` | Group | The frame, its lifecycle stages and the output |
| `Associate` | Group | The matching stages, the solver and the cost matrices |
| `Filter` | Group | Predicting the tracks |
| `Detect` | Group | Splitting the detections into pools |

## Zones

| Name | Level | Group | Where |
| --- | --- | --- | --- |
| `update` | 1 | Track | One call of `Tracker::update` |
| `split` | 1 | Detect | Splitting the detections into the high and low pools |
| `predict` | 1 | Filter | Predicting every pooled track |
| `match_high` | 1 | Associate | The high-confidence stage |
| `match_low` | 1 | Associate | The low-confidence stage |
| `lost_recovery` | 1 | Associate | Recovery by appearance; present with ReID only |
| `tentative` | 1 | Associate | The tentative stage; present with `enable_tentative` |
| `unconfirmed` | 1 | Associate | The unconfirmed stage; present without `enable_tentative` |
| `cleanup_lost` | 1 | Track | Removing lost tracks past their buffer |
| `finalize` | 1 | Track | Rebuilding the pools and dropping duplicates |
| `output` | 1 | Track | Sorting the tracks and copying them into the caller's buffer |
| `solve` | 2 | Associate | One assignment solve; mode 0 can run it many times a frame |
| `cost_matrix` | 2 | Associate | One cost matrix fill |

## Counters

Each is folded once per period. `Set` keeps the last value of the frame and `Add` sums the
frame's samples.

| Name | Level | Op | Unit | Meaning |
| --- | --- | --- | --- | --- |
| `detections_in` | 1 | Set | detections | Detections handed to the frame |
| `matches_high` | 1 | Set | tracks | Tracks matched, updated or recovered by the high stage |
| `tracks_newly_lost` | 1 | Set | tracks | Tracks the low stage marked lost |
| `tracks_recovered` | 1 | Set | tracks | Lost tracks matched again |
| `tentative_confirmed` | 1 | Set | tracks | Tracks the tentative or unconfirmed stage added to the activated set |
| `alive_tracked` | 1 | Set | tracks | Tracks in the Tracked state when the frame ends |
| `alive_lost` | 1 | Set | tracks | Tracks in the Lost state when the frame ends |
| `tracks_written` | 1 | Set | tracks | Tracks copied to the caller's buffer |
| `tracks_dropped` | 1 | Set | tracks | Tracks that did not fit the caller's buffer |
| `detections_high` | 2 | Set | detections | Detections at or above `track_thresh` |
| `detections_low` | 2 | Set | detections | Detections below `track_thresh` |
| `tracks_pool` | 2 | Set | tracks | Tracks in the pool the high stage matches |
| `tracks_tentative_in` | 2 | Set | tracks | Tentative tracks entering the frame |
| `tracks_non_active` | 2 | Set | tracks | Tracks awaiting confirmation without the tentative stage |
| `matches_low` | 2 | Set | tracks | Tracks matched by the low stage |
| `detections_low_remaining` | 2 | Set | detections | Low-pool detections no track took |
| `tentative_removed` | 2 | Set | tracks | Tracks removed by the tentative or unconfirmed stage |
| `tracks_started` | 2 | Set | tracks | Tracks started by the frame |
| `alive_tentative` | 2 | Set | tracks | Tracks in the Tentative state when the frame ends |
| `removed_this_frame` | 2 | Set | tracks | Tracks removed by the frame |
| `cost_cells` | 2 | Add | cells | Cost matrix cells filled by the frame's stages |

## Instants

All are level 2.

| Name | Meaning |
| --- | --- |
| `track_lost` | The low stage marked a track lost |
| `track_confirmed` | A tentative or unconfirmed track became confirmed |
| `track_removed` | A track was removed: aged out, expired or dropped as a duplicate |
| `track_recovered` | A lost track matched a detection again |

## Environment facts

Set by `register_profiler_parts` in the record's `environment`; none is a build fact, so
`sp record compare` accepts records that differ in them.

| Name | Holds |
| --- | --- |
| `sushitrack.library` | `SushiTrack` |
| `sushitrack.version` | The version the build declared, or `unknown` |
| `sushitrack.commit` | The commit the build declared, or `unknown` |
| `sushitrack.date` | The UTC date and time the session started, `YYYY-MM-DDTHH:MM:SSZ` |
| `sushitrack.profile_level` | The profile level compiled in |
| `mot.tracker` | The tracker a `mot_engine` record was made with |
| `mot.sequence` | The sequence it was made over |
