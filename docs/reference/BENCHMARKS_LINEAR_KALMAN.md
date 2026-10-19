# Benchmarks: linear Kalman run

The per-sequence results of the run with `kalman_type=0`; [Benchmarks](BENCHMARKS.md) holds the
summary and the setup.

Configuration:

```json
{
    "sushitrack": {
        "basic": {
            "time_aware": 0,
            "frame_rate": 25,
            "track_buffer": 30,
            "track_thresh": 0.2,
            "high_thresh": 0.5,
            "match_thresh": 0.7,
            "enable_ocm": 1,
            "enable_oru": 1,
            "enable_reid": 0,
            "enable_tentative": 1,
            "enable_mahalanobis": 0
        },
        "advanced": {
            "coordinate_system": {
                "enable_xyah": 0,
                "iou_type": 0,
                "iou_plus_one": 0,
                "kalman_type": 0
            },
            "association": {
                "fuse_score": 1,
                "primary_assoc_mode": 1,
                "iou_match_thresh": 0.5,
                "unconfirmed_iou_thresh": 0.7,
                "duplicate_iou_thresh": 0.15
            },
            "reid_params": {
                "feature_momentum": 0.9,
                "reid_high_thresh": 0.3,
                "reid_high_iou_weight": 0.3,
                "reid_low_thresh": 0.5,
                "reid_low_iou_weight": 0.2,
                "reid_recovery_thresh": 0.4,
                "max_feature_history": 100,
                "reid_direct_assignment_weight": 1.0
            },
            "tentative_logic": {
                "tentative_match_thresh": 0.5,
                "tentative_confirm_hits": 3,
                "tentative_max_age_seconds": 0.08,
                "tentative_max_miss_frames": 2,
                "tentative_init_suppress_iou": 0.7,
                "tentative_fast_thresh": 0.7,
                "tentative_fast_hits": 1,
                "tentative_slow_thresh": 0.2,
                "tentative_slow_hits_offset": 2,
                "tentative_vslow_hits_offset": 4
            },
            "mahalanobis_gating": {
                "gating_thresh": 9.4877,
                "motion_cost_weight": 0.15,
                "max_area_ratio": 50.0
            },
            "ocm": {
                "ocm_vdc_weight": 0.1,
                "ocm_delta_t": 1,
                "ocm_min_velocity": 3.0,
                "ocm_max_age": 2,
                "ocm_maneuver_weight_scale": 0.15
            }
        }
    }
}
```

Per-sequence results:

```text
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
----------------------------------------------------------------------------------------------------------------
sushitrack     MOT17-02-DPM               19.2%   12.2%   30.2%   14.3%   21.0%    39      204    15677        19
bytetrack     MOT17-02-DPM               17.4%   13.6%   22.3%   14.6%   20.8%    49      521    15307        27
ocsort        MOT17-02-DPM               15.7%   11.6%   21.2%   13.6%   18.0%    49      217    15791        93

sushitrack     MOT17-02-FRCNN             34.8%   29.9%   40.8%   32.4%   39.7%    57      149    12361        32
bytetrack     MOT17-02-FRCNN             33.8%   29.7%   38.7%   32.7%   40.3%    47      202    12259       109
ocsort        MOT17-02-FRCNN             35.0%   29.8%   41.4%   31.8%   40.4%    60      187    12418       185

sushitrack     MOT17-02-SDP               36.3%   37.6%   35.2%   43.3%   46.0%   222      476     9843        87
bytetrack     MOT17-02-SDP               32.2%   37.6%   27.8%   43.0%   41.8%   213      582     9791       112
ocsort        MOT17-02-SDP               31.2%   32.6%   30.0%   38.4%   41.1%   173      292    10975       216

sushitrack     MOT17-02-YOLOX             53.0%   66.2%   42.8%   75.7%   59.0%   205      515     3799        74
bytetrack     MOT17-02-YOLOX             51.5%   65.9%   40.5%   75.5%   57.7%   214     1234     3111       143
ocsort        MOT17-02-YOLOX             48.2%   64.7%   36.3%   72.7%   53.1%   271     1169     3641       337

sushitrack     MOT17-04-DPM               31.9%   27.2%   37.4%   31.8%   40.5%   148      870    31436        49
bytetrack     MOT17-04-DPM               34.9%   30.8%   39.6%   34.5%   45.1%   128     2189    28820       139
ocsort        MOT17-04-DPM               28.4%   26.8%   30.3%   31.2%   36.0%   179     1111    31434       341

sushitrack     MOT17-04-FRCNN             53.3%   48.7%   58.6%   54.0%   62.0%    43       21    21795        77
bytetrack     MOT17-04-FRCNN             52.3%   48.5%   56.6%   54.2%   60.3%    52       61    21662        80
ocsort        MOT17-04-FRCNN             53.3%   48.5%   58.8%   53.9%   62.0%    41       32    21864       193

sushitrack     MOT17-04-SDP               63.1%   64.5%   61.7%   74.7%   74.2%   135       78    11842       116
bytetrack     MOT17-04-SDP               61.7%   65.0%   58.6%   74.8%   71.0%   121      121    11725       189
ocsort        MOT17-04-SDP               60.4%   60.9%   59.9%   72.1%   74.3%   131       64    13094       350

sushitrack     MOT17-04-YOLOX             85.5%   85.3%   85.7%   92.3%   92.8%    28      163     3448       133
bytetrack     MOT17-04-YOLOX             84.6%   84.8%   84.4%   92.3%   92.1%    31      278     3338       427
ocsort        MOT17-04-YOLOX             84.8%   84.5%   85.1%   91.5%   92.5%    25      327     3699       609

sushitrack     MOT17-05-DPM               26.9%   21.4%   33.9%   25.4%   33.5%    55      115     4992         8
bytetrack     MOT17-05-DPM               31.2%   24.6%   39.5%   29.2%   40.5%    47      216     4636        14
ocsort        MOT17-05-DPM               26.1%   19.7%   34.7%   23.7%   33.9%    43      104     5128        70

sushitrack     MOT17-05-FRCNN             44.9%   41.0%   49.3%   47.8%   57.2%    38       72     3499        12
bytetrack     MOT17-05-FRCNN             43.4%   40.4%   46.6%   48.2%   55.1%    66       82     3432        43
ocsort        MOT17-05-FRCNN             42.6%   38.5%   47.1%   44.9%   53.6%    29       71     3714        40

sushitrack     MOT17-05-SDP               50.1%   50.9%   49.7%   58.6%   62.3%   102      102     2660        14
bytetrack     MOT17-05-SDP               49.0%   50.8%   47.8%   58.0%   60.9%   156      147     2600        52
ocsort        MOT17-05-SDP               48.0%   45.5%   51.0%   51.8%   59.3%    86       89     3157       117

sushitrack     MOT17-05-YOLOX             67.6%   69.5%   66.2%   79.6%   81.3%    83      241     1086        23
bytetrack     MOT17-05-YOLOX             61.4%   68.1%   55.8%   78.6%   72.4%   190      538      751       111
ocsort        MOT17-05-YOLOX             61.0%   67.1%   55.8%   75.0%   71.9%   106      583     1043       204

sushitrack     MOT17-09-DPM               36.1%   34.9%   37.5%   43.1%   50.2%    67      166     2797        24
bytetrack     MOT17-09-DPM               32.3%   38.8%   27.2%   48.3%   43.7%    73      320     2359        94
ocsort        MOT17-09-DPM               30.9%   32.9%   29.0%   41.9%   44.4%    67      163     2865       135

sushitrack     MOT17-09-FRCNN             50.7%   50.6%   50.9%   54.5%   61.0%    16       11     2394        11
bytetrack     MOT17-09-FRCNN             44.0%   48.2%   40.2%   54.3%   55.0%    35       25     2374        18
ocsort        MOT17-09-FRCNN             48.5%   49.3%   47.7%   53.2%   58.4%    20       10     2462        52

sushitrack     MOT17-09-SDP               52.7%   55.4%   50.2%   63.3%   65.0%    22       17     1914        18
bytetrack     MOT17-09-SDP               44.2%   54.2%   36.2%   62.7%   53.4%    54       26     1904        73
ocsort        MOT17-09-SDP               44.5%   50.6%   39.1%   58.7%   54.5%    26       22     2152       126

sushitrack     MOT17-09-YOLOX             61.8%   72.4%   52.7%   81.3%   69.3%    35       48      914        22
bytetrack     MOT17-09-YOLOX             54.7%   69.9%   42.9%   81.1%   59.5%    46      180      779       104
ocsort        MOT17-09-YOLOX             54.7%   69.5%   43.2%   77.0%   60.9%    58      226      940       186

sushitrack     MOT17-10-DPM               21.5%   22.5%   20.7%   27.5%   25.7%    83      146     9079        22
bytetrack     MOT17-10-DPM               25.7%   24.9%   26.6%   30.4%   33.7%    74      284     8579        32
ocsort        MOT17-10-DPM               22.0%   20.6%   23.6%   25.5%   28.6%    65      146     9356        63

sushitrack     MOT17-10-FRCNN             40.5%   47.3%   34.9%   50.9%   45.1%   247      562     5494        35
bytetrack     MOT17-10-FRCNN             38.0%   47.0%   31.0%   49.5%   42.4%   311      819     5353       108
ocsort        MOT17-10-FRCNN             39.1%   46.1%   33.3%   49.6%   43.6%   204      518     5746       200

sushitrack     MOT17-10-SDP               41.4%   56.2%   30.8%   66.1%   48.3%   331      412     3606        46
bytetrack     MOT17-10-SDP               43.9%   55.8%   34.8%   65.1%   51.2%   395      541     3550        88
ocsort        MOT17-10-SDP               39.0%   52.4%   29.3%   62.5%   46.8%   277      354     4190       144

sushitrack     MOT17-10-YOLOX             50.7%   64.5%   40.1%   76.9%   57.0%   242      481     2248        46
bytetrack     MOT17-10-YOLOX             43.8%   65.2%   29.7%   75.9%   47.4%   433     1053     1614        82
ocsort        MOT17-10-YOLOX             48.0%   63.2%   36.9%   74.8%   54.8%   276     1026     1929       294

sushitrack     MOT17-11-DPM               39.9%   36.8%   43.2%   44.6%   51.2%    50      242     4938        18
bytetrack     MOT17-11-DPM               41.0%   38.4%   43.7%   46.2%   52.1%    35      417     4624        65
ocsort        MOT17-11-DPM               32.5%   35.6%   29.6%   44.0%   40.5%    76      159     5052        99

sushitrack     MOT17-11-FRCNN             55.1%   53.0%   57.3%   56.5%   61.4%    39      129     3934        13
bytetrack     MOT17-11-FRCNN             51.3%   52.6%   50.0%   56.9%   57.8%    53      168     3848        27
ocsort        MOT17-11-FRCNN             51.4%   52.0%   50.8%   55.1%   56.9%    41      139     4057        73

sushitrack     MOT17-11-SDP               58.0%   61.0%   55.3%   69.4%   65.5%    73      222     2595        20
bytetrack     MOT17-11-SDP               55.2%   61.1%   50.0%   69.1%   63.8%   116      255     2547        34
ocsort        MOT17-11-SDP               51.1%   57.5%   45.4%   65.8%   59.5%    83      190     2958        81

sushitrack     MOT17-11-YOLOX             73.9%   78.9%   69.3%   89.8%   81.5%    54      249      662        22
bytetrack     MOT17-11-YOLOX             72.2%   79.0%   66.4%   90.3%   79.4%    58      414      445        35
ocsort        MOT17-11-YOLOX             70.2%   78.1%   63.4%   88.2%   77.1%    64      488      565        73

sushitrack     MOT17-13-DPM               15.7%    7.8%   31.9%    9.3%   16.4%    32       69    10455         7
bytetrack     MOT17-13-DPM               16.5%    9.4%   29.1%   11.1%   17.8%    72       96    10178        17
ocsort        MOT17-13-DPM               15.2%    8.2%   28.6%   10.2%   17.4%    21       66    10370        23

sushitrack     MOT17-13-FRCNN             44.5%   42.9%   46.6%   46.4%   53.1%   210      575     5457        29
bytetrack     MOT17-13-FRCNN             36.4%   42.8%   31.6%   42.8%   40.3%   564      851     5245        67
ocsort        MOT17-13-FRCNN             44.3%   42.1%   47.1%   45.4%   52.3%   167      517     5677       134

sushitrack     MOT17-13-SDP               48.6%   41.5%   57.2%   48.3%   59.1%   110      524     5389        25
bytetrack     MOT17-13-SDP               42.6%   41.4%   44.3%   45.3%   49.6%   457      653     5261        83
ocsort        MOT17-13-SDP               45.0%   39.8%   51.2%   46.8%   54.4%   119      457     5617        92

sushitrack     MOT17-13-YOLOX             64.4%   69.6%   59.8%   83.4%   75.9%    83      233     1617        35
bytetrack     MOT17-13-YOLOX             55.6%   68.4%   45.7%   78.1%   64.3%   683      533     1335        75
ocsort        MOT17-13-YOLOX             65.0%   70.2%   60.4%   83.6%   75.9%    79      445     1381       132

sushitrack     MOT20-01                   40.5%   29.9%   55.0%   31.2%   42.0%    19       23    13620        37
bytetrack     MOT20-01                   37.4%   27.5%   51.0%   29.3%   38.4%    17       20    14018        61
ocsort        MOT20-01                   29.1%   16.4%   51.6%   16.9%   27.1%    19        0    16488        73

sushitrack     MOT20-01-YOLOX             63.7%   67.7%   60.3%   79.0%   75.6%    99      243     3828       131
bytetrack     MOT20-01-YOLOX             59.2%   67.4%   52.2%   79.4%   68.6%   159      438     3496       146
ocsort        MOT20-01-YOLOX             60.3%   65.7%   55.7%   76.3%   71.2%   109      580     4024       350

sushitrack     MOT20-02                   33.9%   29.4%   39.1%   30.8%   38.3%   146      198   106701        47
bytetrack     MOT20-02                   31.3%   27.8%   35.4%   29.5%   35.0%   160      195   108705       140
ocsort        MOT20-02                   25.5%   18.6%   35.1%   19.3%   27.5%   131        0   124794       187

sushitrack     MOT20-02-YOLOX             56.0%   67.3%   46.8%   79.3%   63.3%   760     1544    29679       147
bytetrack     MOT20-02-YOLOX             52.3%   66.6%   41.3%   79.4%   58.5%   898     2624    28378       238
ocsort        MOT20-02-YOLOX             55.2%   65.1%   47.0%   76.5%   63.0%   739     4031    31582       747

sushitrack     MOT20-03                   13.8%    4.8%   39.9%    5.1%    9.4%     7        5   297505        70
bytetrack     MOT20-03                   12.3%    3.8%   40.4%    4.1%    7.7%     4       14   300846        69
ocsort        MOT20-03                    1.4%    0.3%    7.2%    0.3%    0.4%    13        0   312800        12

sushitrack     MOT20-03-YOLOX             74.4%   76.1%   72.7%   89.9%   88.8%   389     3117    28020       663
bytetrack     MOT20-03-YOLOX             71.2%   74.7%   67.9%   89.5%   85.7%   501     6265    26109      1129
ocsort        MOT20-03-YOLOX             73.2%   75.1%   71.4%   88.5%   87.9%   440     6797    28758      2468

sushitrack     MOT20-05                   11.7%    4.0%   34.9%    4.2%    7.6%    24        9   618876        83
bytetrack     MOT20-05                   11.2%    3.5%   36.0%    3.8%    6.8%    21        7   621934       166
ocsort        MOT20-05                    1.4%    0.3%    7.6%    0.3%    0.5%    23        1   644558        27

sushitrack     MOT20-05-YOLOX             64.7%   72.0%   58.3%   85.0%   75.6%  1460     8601    86941      1363
bytetrack     MOT20-05-YOLOX             59.9%   70.8%   50.8%   84.5%   69.9%  1920    17280    81000      2100
ocsort        MOT20-05-YOLOX             63.1%   70.4%   56.6%   82.8%   74.3%  1498    17815    91581      5585

sushitrack     dancetrack0001             59.4%   68.3%   51.7%   88.5%   63.5%    20      276      271        13
bytetrack     dancetrack0001             47.1%   66.2%   33.5%   83.1%   49.3%    20      527      282        21
ocsort        dancetrack0001             46.3%   55.5%   38.6%   46.3%   45.3%    22     2250      364        71

sushitrack     dancetrack0002             58.0%   86.1%   39.1%   93.8%   57.9%    61        6      505        13
bytetrack     dancetrack0002             38.0%   76.1%   19.0%   92.8%   39.3%    89       51      529        19
ocsort        dancetrack0002             57.6%   85.2%   38.9%   92.1%   57.0%    45        6      680        34

sushitrack     dancetrack0006             38.9%   69.0%   21.9%   81.0%   38.6%    85      353     1571        15
bytetrack     dancetrack0006             30.7%   63.1%   15.0%   79.6%   33.8%   105      475     1574        21
ocsort        dancetrack0006             37.8%   65.8%   21.7%   77.2%   43.8%    69      436     1907        59

sushitrack     dancetrack0008             41.3%   74.7%   22.8%   87.5%   39.5%    89       11      755        12
bytetrack     dancetrack0008             31.8%   66.8%   15.2%   84.3%   34.1%    93      129      856        19
ocsort        dancetrack0008             36.9%   72.1%   18.9%   84.0%   33.5%    68       56      970        40

sushitrack     dancetrack0012             33.0%   62.7%   17.5%   74.0%   33.6%   203       81     3107        19
bytetrack     dancetrack0012             27.3%   58.1%   12.9%   74.2%   28.0%   198      133     3036        26
ocsort        dancetrack0012             31.0%   59.9%   16.1%   70.7%   31.5%   155       82     3585        69

sushitrack     dancetrack0015             70.3%   86.1%   57.5%   96.9%   74.0%     7        9      319        15
bytetrack     dancetrack0015             60.1%   80.2%   45.1%   96.6%   68.8%    20       23      318        21
ocsort        dancetrack0015             78.2%   85.3%   71.6%   95.4%   87.7%     7       11      477        46

sushitrack     dancetrack0016             40.5%   82.9%   19.8%   93.7%   35.7%    46      104      648         9
bytetrack     dancetrack0016             27.9%   76.4%   10.2%   93.3%   26.1%    85      128      636        15
ocsort        dancetrack0016             47.8%   83.5%   27.4%   91.7%   46.7%    39       82      931        43

sushitrack     dancetrack0020             41.4%   58.1%   29.8%   72.3%   44.6%   335      448     4799        80
bytetrack     dancetrack0020             40.6%   57.1%   29.4%   72.4%   45.2%   302      796     4461       259
ocsort        dancetrack0020             42.6%   57.4%   32.0%   69.9%   47.6%   298      556     5214       359

sushitrack     dancetrack0023             33.9%   65.4%   17.6%   80.2%   35.9%   162      137     2282        15
bytetrack     dancetrack0023             35.9%   59.6%   21.7%   79.3%   39.9%   123      366     2204        23
ocsort        dancetrack0023             36.5%   61.6%   21.7%   76.0%   39.7%   128      447     2551        59

sushitrack     dancetrack0024             71.8%   81.9%   63.0%   94.5%   83.3%     9       21      219         9
bytetrack     dancetrack0024             55.1%   70.2%   43.3%   90.8%   75.0%    13      115      288        13
ocsort        dancetrack0024             67.7%   76.0%   60.3%   93.3%   79.6%    11       14      277        29

sushitrack     dancetrack0027             45.7%   59.2%   35.4%   71.2%   53.8%    49       44      898        14
bytetrack     dancetrack0027             42.7%   59.8%   30.5%   76.0%   51.9%    45       53      729        20
ocsort        dancetrack0027             45.4%   60.4%   34.3%   71.7%   52.3%    39       58      877        50

sushitrack     dancetrack0029             52.6%   72.6%   38.2%   89.9%   56.2%    47       47      702        11
bytetrack     dancetrack0029             41.3%   67.9%   25.3%   89.5%   48.1%    44      115      666        16
ocsort        dancetrack0029             41.7%   72.0%   24.2%   85.7%   40.6%    46       77     1007        60

sushitrack     dancetrack0032             38.0%   59.8%   24.2%   70.6%   40.5%    39       42      906         9
bytetrack     dancetrack0032             31.6%   56.8%   17.7%   71.1%   35.3%    44       55      871        11
ocsort        dancetrack0032             36.6%   60.7%   22.0%   70.8%   38.5%    35       44      901        27

sushitrack     dancetrack0033             33.2%   62.7%   17.5%   75.2%   36.3%   132      316     1099        14
bytetrack     dancetrack0033             26.0%   58.6%   11.5%   73.8%   29.5%   134      414     1089        20
ocsort        dancetrack0033             33.8%   61.3%   18.7%   69.0%   38.0%   108      533     1292        52

sushitrack     dancetrack0037             34.3%   73.2%   16.1%   85.2%   31.6%    88      174      970        15
bytetrack     dancetrack0037             29.8%   66.3%   13.4%   82.8%   28.4%    93      297     1035        49
ocsort        dancetrack0037             35.0%   71.2%   17.2%   82.8%   34.9%    74      208     1149        41

sushitrack     dancetrack0039             46.7%   84.2%   25.9%   94.8%   45.4%    27       18      272         8
bytetrack     dancetrack0039             51.2%   78.1%   33.6%   94.2%   54.2%    33       47      275        12
ocsort        dancetrack0039             60.1%   84.0%   43.0%   93.3%   59.4%    23       19      365        27

sushitrack     dancetrack0044             21.3%   59.1%    7.7%   70.7%   18.6%   404      583     2936        25
bytetrack     dancetrack0044             22.4%   56.8%    8.9%   68.2%   23.4%   572     1438     2254        44
ocsort        dancetrack0044             21.3%   55.0%    8.3%   64.4%   19.8%   354     1170     3244       111

sushitrack     dancetrack0045             32.7%   65.8%   16.3%   81.5%   33.1%   231      110     2589        25
bytetrack     dancetrack0045             30.7%   61.8%   15.3%   81.1%   34.1%   218      368     2407        36
ocsort        dancetrack0045             31.8%   63.1%   16.0%   77.3%   35.7%   185      139     3270        97

sushitrack     dancetrack0049             38.9%   70.3%   21.6%   83.7%   37.9%   111      137     1301        14
bytetrack     dancetrack0049             28.3%   63.0%   12.8%   80.9%   28.3%   108      369     1339        21
ocsort        dancetrack0049             34.0%   62.9%   18.4%   70.6%   34.4%    85     1060     1652        65

sushitrack     dancetrack0051             37.2%   70.8%   19.6%   82.2%   36.7%   104       16     1802        18
bytetrack     dancetrack0051             32.4%   63.8%   16.5%   80.9%   34.6%   118      112     1834        58
ocsort        dancetrack0051             41.8%   67.6%   25.9%   78.4%   41.0%    92       23     2215       117

sushitrack     dancetrack0052             47.5%   79.1%   28.5%   91.1%   47.3%    32       16      351         6
bytetrack     dancetrack0052             41.5%   71.9%   24.0%   90.2%   47.0%    32       83      322         9
ocsort        dancetrack0052             51.7%   77.9%   34.3%   89.3%   55.4%    19       36      423        21

sushitrack     dancetrack0053             43.1%   68.8%   27.1%   82.1%   44.3%    62      489      516        10
bytetrack     dancetrack0053             22.6%   57.1%    9.0%   68.6%   20.8%    95     1001      776        16
ocsort        dancetrack0053             32.4%   65.4%   16.1%   72.6%   31.2%    55      979      595        36

sushitrack     dancetrack0055             36.2%   73.6%   17.8%   87.4%   38.0%    77      157      498         9
bytetrack     dancetrack0055             23.9%   60.7%    9.5%   75.0%   22.9%    84      637      733        14
ocsort        dancetrack0055             37.2%   68.4%   20.3%   79.2%   34.3%    52      556      603        33

sushitrack     dancetrack0057             51.3%   73.0%   36.0%   87.9%   58.3%    36       76      252         8
bytetrack     dancetrack0057             40.4%   64.9%   25.2%   83.0%   54.5%    37      162      311        12
ocsort        dancetrack0057             49.2%   73.0%   33.2%   85.6%   58.2%    19       86      328        24

sushitrack     dancetrack0061             42.7%   76.1%   24.0%   93.6%   42.7%    34       48      300         9
bytetrack     dancetrack0061             31.9%   67.3%   15.1%   88.3%   36.2%    24      245      432        12
ocsort        dancetrack0061             53.3%   74.5%   38.1%   89.3%   59.8%    30      228      387        24

sushitrack     dancetrack0062             36.0%   75.9%   17.1%   90.5%   35.1%    64       35      495        10
bytetrack     dancetrack0062             32.3%   64.8%   16.1%   81.9%   34.0%    62      334      733        13
ocsort        dancetrack0062             39.8%   75.3%   21.0%   89.1%   40.7%    41       44      596        26

sushitrack     dancetrack0066             40.9%   78.4%   21.3%   92.0%   40.8%    44       23      412         8
bytetrack     dancetrack0066             32.6%   67.5%   15.8%   87.5%   42.6%    60      173      516        13
ocsort        dancetrack0066             44.7%   76.0%   26.3%   89.2%   46.1%    26       37      584        28

sushitrack     dancetrack0068             26.8%   69.6%   10.4%   82.9%   21.4%   119       48      857        10
bytetrack     dancetrack0068             24.5%   57.3%   10.5%   74.9%   25.6%   147      330     1028        28
ocsort        dancetrack0068             29.0%   65.8%   12.8%   75.6%   26.0%    94      288     1081        46

sushitrack     dancetrack0069             64.8%   83.6%   50.2%   97.1%   65.6%    15        8      216         9
bytetrack     dancetrack0069             47.9%   78.3%   29.3%   96.7%   55.0%    18       34      218        13
ocsort        dancetrack0069             60.8%   87.5%   42.2%   96.1%   64.4%    10       18      298        30

sushitrack     dancetrack0072             31.1%   61.1%   15.8%   67.7%   29.2%    78     1082      748        11
bytetrack     dancetrack0072             21.1%   50.1%    8.9%   53.0%   22.0%    96     1682     1000        18
ocsort        dancetrack0072             33.9%   55.2%   20.8%   56.0%   35.0%    53     1714      830        43

sushitrack     dancetrack0074             49.2%   76.1%   31.8%   89.0%   49.5%    32      117      432         8
bytetrack     dancetrack0074             42.7%   70.2%   26.0%   87.7%   48.3%    25      203      425        12
ocsort        dancetrack0074             49.9%   75.9%   32.8%   87.4%   49.9%    21      131      517        27

sushitrack     dancetrack0075             41.8%   69.9%   25.0%   80.9%   42.6%    30       80      928        10
bytetrack     dancetrack0075             48.7%   68.2%   34.7%   81.3%   56.1%    17      142      860        15
ocsort        dancetrack0075             56.5%   71.1%   44.9%   81.1%   67.0%    19      119      888        31

sushitrack     dancetrack0080             73.7%   80.7%   67.3%   94.9%   84.2%    34       25      559        18
bytetrack     dancetrack0080             64.0%   70.3%   58.2%   92.1%   83.1%    28      251      675        25
ocsort        dancetrack0080             74.2%   80.5%   68.5%   93.6%   86.8%    26      126      620        59

sushitrack     dancetrack0082             60.5%   79.7%   46.0%   94.1%   65.5%    80       15      639        41
bytetrack     dancetrack0082             52.8%   73.5%   38.0%   93.8%   61.2%   108       69      593        56
ocsort        dancetrack0082             64.4%   78.8%   52.7%   92.3%   71.2%    77       38      839       116

sushitrack     dancetrack0083             51.7%   79.4%   33.8%   95.4%   52.0%    84       65      540        56
bytetrack     dancetrack0083             48.5%   68.4%   34.5%   86.6%   55.7%    67      816     1120        70
ocsort        dancetrack0083             49.8%   78.5%   31.6%   93.5%   51.1%    84      130      762       197

sushitrack     dancetrack0086             42.5%   64.5%   28.0%   76.4%   46.3%   101      814     1322        35
bytetrack     dancetrack0086             39.7%   59.1%   26.7%   73.8%   47.7%    80     1149     1256        81
ocsort        dancetrack0086             44.8%   58.2%   34.6%   64.6%   50.7%    87     1666     1606       125

sushitrack     dancetrack0087             41.3%   65.9%   26.1%   81.4%   45.3%   133      111     1748        19
bytetrack     dancetrack0087             35.3%   58.7%   21.3%   76.7%   39.6%   132      493     1864        28
ocsort        dancetrack0087             38.9%   63.7%   23.9%   79.1%   42.2%   107      128     1998        78

sushitrack     dancetrack0096             58.7%   68.3%   50.5%   82.3%   67.5%   149      631     2018        64
bytetrack     dancetrack0096             52.3%   58.8%   46.6%   77.3%   66.0%   157     1357     2077        83
ocsort        dancetrack0096             56.3%   62.1%   51.2%   72.0%   65.4%   138     1694     2588       307

sushitrack     dancetrack0098             39.7%   74.8%   21.1%   87.3%   37.0%    68       77      924        12
bytetrack     dancetrack0098             33.0%   68.0%   16.1%   85.9%   33.8%    82      179      925        20
ocsort        dancetrack0098             42.0%   73.5%   24.1%   83.7%   42.2%    65      103     1202        66

sushitrack     dancetrack0099             47.0%   68.4%   32.3%   81.7%   49.2%    47      365      722        24
bytetrack     dancetrack0099             48.0%   65.9%   35.1%   79.6%   54.4%    55      562      648        85
ocsort        dancetrack0099             50.4%   66.4%   38.4%   76.8%   56.6%    43      584      812        63

----------------------------------------------------------------------------------------------------------------
MOT17
----------------------------------------------------------------------------------------------------------------
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
sushitrack     MOT17                      47.2%   48.2%   47.5%   55.0%   55.5%  2849     7092   185931        37
bytetrack     MOT17                      44.7%   48.5%   42.4%   55.1%   52.5%  4773    12806   177427        87
ocsort        MOT17                      44.1%   46.4%   43.2%   53.0%   52.2%  2836     9172   191275       167
----------------------------------------------------------------------------------------------------------------
MOT20
----------------------------------------------------------------------------------------------------------------
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
sushitrack     MOT20                      44.9%   43.9%   50.9%   50.6%   50.1%  2904    13740  1185170       318
bytetrack     MOT20                      41.9%   42.7%   46.9%   49.9%   46.3%  3680    26843  1184486       506
ocsort        MOT20                      38.7%   39.0%   41.5%   45.1%   44.0%  2972    29224  1254585      1181
----------------------------------------------------------------------------------------------------------------
DanceTrack
----------------------------------------------------------------------------------------------------------------
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
sushitrack     DanceTrack                 44.9%   72.0%   29.3%   85.1%   46.5%  3568     7215    42428        18
bytetrack     DanceTrack                 37.9%   65.2%   23.3%   82.0%   42.8%  3863    15913    43195        34
ocsort        DanceTrack                 45.6%   69.7%   31.1%   80.2%   48.5%  2949    15976    50485        71
----------------------------------------------------------------------------------------------------------------
OVERALL
----------------------------------------------------------------------------------------------------------------
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
sushitrack     OVERALL                    45.7%   60.3%   38.3%   70.4%   50.2%  9321    28047  1413529        57
bytetrack     OVERALL                    40.8%   56.7%   32.8%   68.7%   46.8% 12316    55562  1405108       103
ocsort        OVERALL                    44.3%   57.9%   36.7%   66.5%   49.4%  8757    54372  1496345       223
----------------------------------------------------------------------------------------------------------------
```
