# Benchmarks: IMM run

The per-sequence results of the run with `kalman_type=2`; [Benchmarks](BENCHMARKS.md) holds the
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
                "kalman_type": 2
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
sushitrack     MOT17-02-DPM               18.3%   11.9%   28.3%   14.2%   20.5%    45      204    15689        99
bytetrack     MOT17-02-DPM               17.4%   13.6%   22.3%   14.6%   20.8%    49      521    15307        27
ocsort        MOT17-02-DPM               15.7%   11.6%   21.2%   13.6%   18.0%    49      217    15791        53

sushitrack     MOT17-02-FRCNN             36.1%   30.1%   43.7%   32.4%   41.9%    57      150    12352       179
bytetrack     MOT17-02-FRCNN             33.8%   29.7%   38.7%   32.7%   40.3%    47      202    12259        46
ocsort        MOT17-02-FRCNN             35.0%   29.8%   41.4%   31.8%   40.4%    60      187    12418       123

sushitrack     MOT17-02-SDP               33.5%   37.4%   30.2%   43.2%   42.4%   255      487     9814       381
bytetrack     MOT17-02-SDP               32.2%   37.6%   27.8%   43.0%   41.8%   213      582     9791        97
ocsort        MOT17-02-SDP               31.2%   32.6%   30.0%   38.4%   41.1%   173      292    10975       212

sushitrack     MOT17-02-YOLOX             53.8%   66.1%   44.1%   76.0%   61.2%   204      498     3764       397
bytetrack     MOT17-02-YOLOX             51.5%   65.9%   40.5%   75.5%   57.7%   214     1234     3111       215
ocsort        MOT17-02-YOLOX             48.2%   64.7%   36.3%   72.7%   53.1%   271     1169     3641       319

sushitrack     MOT17-04-DPM               32.3%   27.4%   38.0%   32.1%   41.2%   148      861    31302       292
bytetrack     MOT17-04-DPM               34.9%   30.8%   39.6%   34.5%   45.1%   128     2189    28820       200
ocsort        MOT17-04-DPM               28.4%   26.8%   30.3%   31.2%   36.0%   179     1111    31434       463

sushitrack     MOT17-04-FRCNN             53.3%   48.9%   58.2%   54.0%   61.5%    44       21    21802       316
bytetrack     MOT17-04-FRCNN             52.3%   48.5%   56.6%   54.2%   60.3%    52       61    21662        81
ocsort        MOT17-04-FRCNN             53.3%   48.5%   58.8%   53.9%   62.0%    41       32    21864       207

sushitrack     MOT17-04-SDP               64.0%   64.2%   63.9%   74.8%   76.0%   113       88    11788       429
bytetrack     MOT17-04-SDP               61.7%   65.0%   58.6%   74.8%   71.0%   121      121    11725       115
ocsort        MOT17-04-SDP               60.4%   60.9%   59.9%   72.1%   74.3%   131       64    13094       267

sushitrack     MOT17-04-YOLOX             85.6%   85.3%   85.9%   92.4%   92.9%    24      156     3450       501
bytetrack     MOT17-04-YOLOX             84.6%   84.8%   84.4%   92.3%   92.1%    31      278     3338       176
ocsort        MOT17-04-YOLOX             84.8%   84.5%   85.1%   91.5%   92.5%    25      327     3699       510

sushitrack     MOT17-05-DPM               27.9%   21.5%   36.4%   25.7%   34.8%    48      108     4982        62
bytetrack     MOT17-05-DPM               31.2%   24.6%   39.5%   29.2%   40.5%    47      216     4636        29
ocsort        MOT17-05-DPM               26.1%   19.7%   34.7%   23.7%   33.9%    43      104     5128        74

sushitrack     MOT17-05-FRCNN             46.6%   41.2%   52.8%   47.8%   59.0%    36       72     3501        79
bytetrack     MOT17-05-FRCNN             43.4%   40.4%   46.6%   48.2%   55.1%    66       82     3432        32
ocsort        MOT17-05-FRCNN             42.6%   38.5%   47.1%   44.9%   53.6%    29       71     3714        30

sushitrack     MOT17-05-SDP               50.1%   51.1%   49.6%   58.8%   61.4%   103       99     2650       108
bytetrack     MOT17-05-SDP               49.0%   50.8%   47.8%   58.0%   60.9%   156      147     2600        31
ocsort        MOT17-05-SDP               48.0%   45.5%   51.0%   51.8%   59.3%    86       89     3157       119

sushitrack     MOT17-05-YOLOX             68.0%   69.8%   66.5%   80.3%   81.2%    69      221     1073       130
bytetrack     MOT17-05-YOLOX             61.4%   68.1%   55.8%   78.6%   72.4%   190      538      751       109
ocsort        MOT17-05-YOLOX             61.0%   67.1%   55.8%   75.0%   71.9%   106      583     1043       185

sushitrack     MOT17-09-DPM               31.3%   34.5%   28.6%   43.1%   44.3%    68      166     2794       108
bytetrack     MOT17-09-DPM               32.3%   38.8%   27.2%   48.3%   43.7%    73      320     2359        27
ocsort        MOT17-09-DPM               30.9%   32.9%   29.0%   41.9%   44.4%    67      163     2865        64

sushitrack     MOT17-09-FRCNN             49.2%   50.2%   48.3%   54.3%   57.7%    24       10     2401        75
bytetrack     MOT17-09-FRCNN             44.0%   48.2%   40.2%   54.3%   55.0%    35       25     2374        18
ocsort        MOT17-09-FRCNN             48.5%   49.3%   47.7%   53.2%   58.4%    20       10     2462        35

sushitrack     MOT17-09-SDP               47.1%   55.3%   40.2%   62.6%   55.5%    50       21     1918       106
bytetrack     MOT17-09-SDP               44.2%   54.2%   36.2%   62.7%   53.4%    54       26     1904        25
ocsort        MOT17-09-SDP               44.5%   50.6%   39.1%   58.7%   54.5%    26       22     2152        46

sushitrack     MOT17-09-YOLOX             58.9%   70.9%   49.0%   81.4%   66.9%    33       35      922       113
bytetrack     MOT17-09-YOLOX             54.7%   69.9%   42.9%   81.1%   59.5%    46      180      779        27
ocsort        MOT17-09-YOLOX             54.7%   69.5%   43.2%   77.0%   60.9%    58      226      940        65

sushitrack     MOT17-10-DPM               24.1%   22.6%   25.7%   27.6%   29.2%    87      163     9050       120
bytetrack     MOT17-10-DPM               25.7%   24.9%   26.6%   30.4%   33.7%    74      284     8579        33
ocsort        MOT17-10-DPM               22.0%   20.6%   23.6%   25.5%   28.6%    65      146     9356        66

sushitrack     MOT17-10-FRCNN             40.7%   47.3%   35.3%   50.8%   44.9%   246      572     5502       253
bytetrack     MOT17-10-FRCNN             38.0%   47.0%   31.0%   49.5%   42.4%   311      819     5353       202
ocsort        MOT17-10-FRCNN             39.1%   46.1%   33.3%   49.6%   43.6%   204      518     5746       153

sushitrack     MOT17-10-SDP               44.5%   55.8%   35.7%   66.7%   53.0%   275      409     3596       250
bytetrack     MOT17-10-SDP               43.9%   55.8%   34.8%   65.1%   51.2%   395      541     3550        76
ocsort        MOT17-10-SDP               39.0%   52.4%   29.3%   62.5%   46.8%   277      354     4190       134

sushitrack     MOT17-10-YOLOX             56.1%   65.1%   48.7%   77.9%   65.5%   197      478     2161       255
bytetrack     MOT17-10-YOLOX             43.8%   65.2%   29.7%   75.9%   47.4%   433     1053     1614       211
ocsort        MOT17-10-YOLOX             48.0%   63.2%   36.9%   74.8%   54.8%   276     1026     1929       197

sushitrack     MOT17-11-DPM               39.5%   36.5%   42.7%   44.7%   51.1%    45      255     4917        95
bytetrack     MOT17-11-DPM               41.0%   38.4%   43.7%   46.2%   52.1%    35      417     4624        49
ocsort        MOT17-11-DPM               32.5%   35.6%   29.6%   44.0%   40.5%    76      159     5052       119

sushitrack     MOT17-11-FRCNN             55.7%   53.2%   58.3%   56.5%   61.8%    49      126     3930        88
bytetrack     MOT17-11-FRCNN             51.3%   52.6%   50.0%   56.9%   57.8%    53      168     3848        23
ocsort        MOT17-11-FRCNN             51.4%   52.0%   50.8%   55.1%   56.9%    41      139     4057        60

sushitrack     MOT17-11-SDP               59.0%   61.6%   56.7%   69.6%   66.8%    72      221     2571       117
bytetrack     MOT17-11-SDP               55.2%   61.1%   50.0%   69.1%   63.8%   116      255     2547        49
ocsort        MOT17-11-SDP               51.1%   57.5%   45.4%   65.8%   59.5%    83      190     2958        57

sushitrack     MOT17-11-YOLOX             76.3%   79.9%   73.1%   90.3%   84.8%    37      235      645       129
bytetrack     MOT17-11-YOLOX             72.2%   79.0%   66.4%   90.3%   79.4%    58      414      445       126
ocsort        MOT17-11-YOLOX             70.2%   78.1%   63.4%   88.2%   77.1%    64      488      565       163

sushitrack     MOT17-13-DPM               15.7%    7.9%   31.2%    9.5%   16.6%    31       67    10434        49
bytetrack     MOT17-13-DPM               16.5%    9.4%   29.1%   11.1%   17.8%    72       96    10178        16
ocsort        MOT17-13-DPM               15.2%    8.2%   28.6%   10.2%   17.4%    21       66    10370        23

sushitrack     MOT17-13-FRCNN             45.1%   43.4%   47.3%   46.5%   52.8%   230      570     5424       197
bytetrack     MOT17-13-FRCNN             36.4%   42.8%   31.6%   42.8%   40.3%   564      851     5245        67
ocsort        MOT17-13-FRCNN             44.3%   42.1%   47.1%   45.4%   52.3%   167      517     5677       154

sushitrack     MOT17-13-SDP               49.0%   41.5%   58.1%   47.9%   58.2%   147      526     5398       187
bytetrack     MOT17-13-SDP               42.6%   41.4%   44.3%   45.3%   49.6%   457      653     5261       227
ocsort        MOT17-13-SDP               45.0%   39.8%   51.2%   46.8%   54.4%   119      457     5617       113

sushitrack     MOT17-13-YOLOX             65.1%   70.1%   60.6%   83.7%   76.1%    87      221     1594       215
bytetrack     MOT17-13-YOLOX             55.6%   68.4%   45.7%   78.1%   64.3%   683      533     1335       172
ocsort        MOT17-13-YOLOX             65.0%   70.2%   60.4%   83.6%   75.9%    79      445     1381       155

sushitrack     MOT20-01                   39.7%   29.8%   52.8%   31.2%   41.0%    19       23    13623       203
bytetrack     MOT20-01                   37.4%   27.5%   51.0%   29.3%   38.4%    17       20    14018        60
ocsort        MOT20-01                   29.1%   16.4%   51.6%   16.9%   27.1%    19        0    16488        77

sushitrack     MOT20-01-YOLOX             62.1%   67.7%   57.4%   79.1%   73.4%   104      232     3814       513
bytetrack     MOT20-01-YOLOX             59.2%   67.4%   52.2%   79.4%   68.6%   159      438     3496       146
ocsort        MOT20-01-YOLOX             60.3%   65.7%   55.7%   76.3%   71.2%   109      580     4024       827

sushitrack     MOT20-02                   34.3%   29.4%   39.9%   30.8%   38.3%   142      198   106683       240
bytetrack     MOT20-02                   31.3%   27.8%   35.4%   29.5%   35.0%   160      195   108705       130
ocsort        MOT20-02                   25.5%   18.6%   35.1%   19.3%   27.5%   131        0   124794       194

sushitrack     MOT20-02-YOLOX             56.7%   67.4%   47.9%   79.4%   63.9%   723     1523    29610       641
bytetrack     MOT20-02-YOLOX             52.3%   66.6%   41.3%   79.4%   58.5%   898     2624    28378       424
ocsort        MOT20-02-YOLOX             55.2%   65.1%   47.0%   76.5%   63.0%   739     4031    31582       881

sushitrack     MOT20-03                   13.9%    4.8%   40.3%    5.1%    9.4%     6        5   297542       132
bytetrack     MOT20-03                   12.3%    3.8%   40.4%    4.1%    7.7%     4       14   300846       130
ocsort        MOT20-03                    1.4%    0.3%    7.2%    0.3%    0.4%    13        0   312800        25

sushitrack     MOT20-03-YOLOX             73.8%   76.2%   71.5%   89.9%   87.7%   409     3109    28047      1852
bytetrack     MOT20-03-YOLOX             71.2%   74.7%   67.9%   89.5%   85.7%   501     6265    26109      1193
ocsort        MOT20-03-YOLOX             73.2%   75.1%   71.4%   88.5%   87.9%   440     6797    28758      2816

sushitrack     MOT20-05                   11.8%    4.0%   35.5%    4.2%    7.7%    20        9   618871       179
bytetrack     MOT20-05                   11.2%    3.5%   36.0%    3.8%    6.8%    21        7   621934       503
ocsort        MOT20-05                    1.4%    0.3%    7.6%    0.3%    0.5%    23        1   644558        18

sushitrack     MOT20-05-YOLOX             64.3%   71.8%   57.6%   85.0%   75.0%  1514     8744    86803      3043
bytetrack     MOT20-05-YOLOX             59.9%   70.8%   50.8%   84.5%   69.9%  1920    17280    81000      2018
ocsort        MOT20-05-YOLOX             63.1%   70.4%   56.6%   82.8%   74.3%  1498    17815    91581      6038

sushitrack     dancetrack0001             60.8%   75.6%   48.9%   88.3%   64.2%    16      285      272        84
bytetrack     dancetrack0001             47.1%   66.2%   33.5%   83.1%   49.3%    20      527      282        30
ocsort        dancetrack0001             46.3%   55.5%   38.6%   46.3%   45.3%    22     2250      364        77

sushitrack     dancetrack0002             61.6%   83.5%   45.5%   93.8%   62.9%    60        8      503        88
bytetrack     dancetrack0002             38.0%   76.1%   19.0%   92.8%   39.3%    89       51      529        22
ocsort        dancetrack0002             57.6%   85.2%   38.9%   92.1%   57.0%    45        6      680        40

sushitrack     dancetrack0006             37.2%   68.7%   20.1%   81.0%   40.2%    83      356     1566        97
bytetrack     dancetrack0006             30.7%   63.1%   15.0%   79.6%   33.8%   105      475     1574        21
ocsort        dancetrack0006             37.8%   65.8%   21.7%   77.2%   43.8%    69      436     1907       114

sushitrack     dancetrack0008             36.2%   75.2%   17.5%   87.8%   35.6%    79       13      746        91
bytetrack     dancetrack0008             31.8%   66.8%   15.2%   84.3%   34.1%    93      129      856        49
ocsort        dancetrack0008             36.9%   72.1%   18.9%   84.0%   33.5%    68       56      970       113

sushitrack     dancetrack0012             31.9%   62.5%   16.3%   74.3%   33.6%   187       82     3083       124
bytetrack     dancetrack0012             27.3%   58.1%   12.9%   74.2%   28.0%   198      133     3036        27
ocsort        dancetrack0012             31.0%   59.9%   16.1%   70.7%   31.5%   155       82     3585        73

sushitrack     dancetrack0015             71.3%   85.0%   59.9%   96.8%   75.7%    11       11      324        93
bytetrack     dancetrack0015             60.1%   80.2%   45.1%   96.6%   68.8%    20       23      318        50
ocsort        dancetrack0015             78.2%   85.3%   71.6%   95.4%   87.7%     7       11      477        89

sushitrack     dancetrack0016             37.7%   85.2%   16.7%   93.9%   32.0%    42       96      631        66
bytetrack     dancetrack0016             27.9%   76.4%   10.2%   93.3%   26.1%    85      128      636        39
ocsort        dancetrack0016             47.8%   83.5%   27.4%   91.7%   46.7%    39       82      931        63

sushitrack     dancetrack0020             41.9%   59.4%   29.9%   72.6%   46.0%   322      455     4751       440
bytetrack     dancetrack0020             40.6%   57.1%   29.4%   72.4%   45.2%   302      796     4461       331
ocsort        dancetrack0020             42.6%   57.4%   32.0%   69.9%   47.6%   298      556     5214       405

sushitrack     dancetrack0023             34.0%   65.8%   17.6%   80.5%   34.5%   136      131     2268        96
bytetrack     dancetrack0023             35.9%   59.6%   21.7%   79.3%   39.9%   123      366     2204        23
ocsort        dancetrack0023             36.5%   61.6%   21.7%   76.0%   39.7%   128      447     2551        97

sushitrack     dancetrack0024             65.0%   75.6%   55.9%   94.5%   77.6%    12       21      213        67
bytetrack     dancetrack0024             55.1%   70.2%   43.3%   90.8%   75.0%    13      115      288        37
ocsort        dancetrack0024             67.7%   76.0%   60.3%   93.3%   79.6%    11       14      277        70

sushitrack     dancetrack0027             41.2%   59.9%   28.4%   71.7%   47.0%    45       42      889        97
bytetrack     dancetrack0027             42.7%   59.8%   30.5%   76.0%   51.9%    45       53      729        57
ocsort        dancetrack0027             45.4%   60.4%   34.3%   71.7%   52.3%    39       58      877       100

sushitrack     dancetrack0029             38.8%   74.5%   20.2%   90.0%   34.9%    47       44      697        67
bytetrack     dancetrack0029             41.3%   67.9%   25.3%   89.5%   48.1%    44      115      666        46
ocsort        dancetrack0029             41.7%   72.0%   24.2%   85.7%   40.6%    46       77     1007        63

sushitrack     dancetrack0032             35.8%   60.7%   21.1%   70.6%   36.3%    36       47      906        53
bytetrack     dancetrack0032             31.6%   56.8%   17.7%   71.1%   35.3%    44       55      871        13
ocsort        dancetrack0032             36.6%   60.7%   22.0%   70.8%   38.5%    35       44      901        82

sushitrack     dancetrack0033             33.3%   63.0%   17.7%   75.5%   37.8%   117      317     1097        99
bytetrack     dancetrack0033             26.0%   58.6%   11.5%   73.8%   29.5%   134      414     1089        20
ocsort        dancetrack0033             33.8%   61.3%   18.7%   69.0%   38.0%   108      533     1292        55

sushitrack     dancetrack0037             35.2%   73.6%   16.9%   85.3%   33.9%    80      177      965        79
bytetrack     dancetrack0037             29.8%   66.3%   13.4%   82.8%   28.4%    93      297     1035        49
ocsort        dancetrack0037             35.0%   71.2%   17.2%   82.8%   34.9%    74      208     1149        71

sushitrack     dancetrack0039             47.0%   84.7%   26.2%   94.8%   43.0%    27       17      273        53
bytetrack     dancetrack0039             51.2%   78.1%   33.6%   94.2%   54.2%    33       47      275        17
ocsort        dancetrack0039             60.1%   84.0%   43.0%   93.3%   59.4%    23       19      365        75

sushitrack     dancetrack0044             23.5%   59.0%    9.4%   71.2%   22.6%   378      606     2874       178
bytetrack     dancetrack0044             22.4%   56.8%    8.9%   68.2%   23.4%   572     1438     2254        90
ocsort        dancetrack0044             21.3%   55.0%    8.3%   64.4%   19.8%   354     1170     3244       167

sushitrack     dancetrack0045             35.3%   65.3%   19.2%   81.7%   36.5%   220      105     2564       152
bytetrack     dancetrack0045             30.7%   61.8%   15.3%   81.1%   34.1%   218      368     2407        36
ocsort        dancetrack0045             31.8%   63.1%   16.0%   77.3%   35.7%   185      139     3270       134

sushitrack     dancetrack0049             39.3%   70.6%   21.9%   83.8%   39.8%    93      152     1299        87
bytetrack     dancetrack0049             28.3%   63.0%   12.8%   80.9%   28.3%   108      369     1339        22
ocsort        dancetrack0049             34.0%   62.9%   18.4%   70.6%   34.4%    85     1060     1652        61

sushitrack     dancetrack0051             44.5%   67.9%   29.3%   82.3%   45.9%    99       18     1796        96
bytetrack     dancetrack0051             32.4%   63.8%   16.5%   80.9%   34.6%   118      112     1834        23
ocsort        dancetrack0051             41.8%   67.6%   25.9%   78.4%   41.0%    92       23     2215        59

sushitrack     dancetrack0052             46.6%   79.1%   27.5%   90.7%   46.5%    27       34      353        46
bytetrack     dancetrack0052             41.5%   71.9%   24.0%   90.2%   47.0%    32       83      322        15
ocsort        dancetrack0052             51.7%   77.9%   34.3%   89.3%   55.4%    19       36      423        58

sushitrack     dancetrack0053             35.4%   69.9%   18.0%   81.9%   37.0%    71      500      506        68
bytetrack     dancetrack0053             22.6%   57.1%    9.0%   68.6%   20.8%    95     1001      776        22
ocsort        dancetrack0053             32.4%   65.4%   16.1%   72.6%   31.2%    55      979      595        42

sushitrack     dancetrack0055             38.5%   72.6%   20.4%   88.0%   36.3%    68      153      479        60
bytetrack     dancetrack0055             23.9%   60.7%    9.5%   75.0%   22.9%    84      637      733        30
ocsort        dancetrack0055             37.2%   68.4%   20.3%   79.2%   34.3%    52      556      603        69

sushitrack     dancetrack0057             51.3%   73.2%   36.0%   88.4%   62.8%    29       75      245        61
bytetrack     dancetrack0057             40.4%   64.9%   25.2%   83.0%   54.5%    37      162      311        19
ocsort        dancetrack0057             49.2%   73.0%   33.2%   85.6%   58.2%    19       86      328        64

sushitrack     dancetrack0061             37.2%   75.5%   18.4%   94.1%   39.1%    32       42      283        54
bytetrack     dancetrack0061             31.9%   67.3%   15.1%   88.3%   36.2%    24      245      432        36
ocsort        dancetrack0061             53.3%   74.5%   38.1%   89.3%   59.8%    30      228      387        65

sushitrack     dancetrack0062             36.6%   76.8%   17.4%   91.0%   32.9%    52       28      483        56
bytetrack     dancetrack0062             32.3%   64.8%   16.1%   81.9%   34.0%    62      334      733        28
ocsort        dancetrack0062             39.8%   75.3%   21.0%   89.1%   40.7%    41       44      596        65

sushitrack     dancetrack0066             46.2%   76.2%   28.0%   92.2%   51.1%    44       23      401        57
bytetrack     dancetrack0066             32.6%   67.5%   15.8%   87.5%   42.6%    60      173      516        23
ocsort        dancetrack0066             44.7%   76.0%   26.3%   89.2%   46.1%    26       37      584        72

sushitrack     dancetrack0068             29.6%   68.4%   12.8%   83.5%   27.4%    99       43      851        58
bytetrack     dancetrack0068             24.5%   57.3%   10.5%   74.9%   25.6%   147      330     1028        28
ocsort        dancetrack0068             29.0%   65.8%   12.8%   75.6%   26.0%    94      288     1081        79

sushitrack     dancetrack0069             66.8%   87.7%   50.8%   97.2%   68.9%    10        7      217        63
bytetrack     dancetrack0069             47.9%   78.3%   29.3%   96.7%   55.0%    18       34      218        34
ocsort        dancetrack0069             60.8%   87.5%   42.2%   96.1%   64.4%    10       18      298        60

sushitrack     dancetrack0072             34.1%   60.2%   19.4%   67.9%   35.9%    69     1080      744        75
bytetrack     dancetrack0072             21.1%   50.1%    8.9%   53.0%   22.0%    96     1682     1000        19
ocsort        dancetrack0072             33.9%   55.2%   20.8%   56.0%   35.0%    53     1714      830        48

sushitrack     dancetrack0074             50.3%   77.0%   32.9%   89.0%   50.8%    24      126      433        58
bytetrack     dancetrack0074             42.7%   70.2%   26.0%   87.7%   48.3%    25      203      425        12
ocsort        dancetrack0074             49.9%   75.9%   32.8%   87.4%   49.9%    21      131      517        29

sushitrack     dancetrack0075             47.3%   68.1%   33.0%   80.7%   51.5%    28       85      937        67
bytetrack     dancetrack0075             48.7%   68.2%   34.7%   81.3%   56.1%    17      142      860        15
ocsort        dancetrack0075             56.5%   71.1%   44.9%   81.1%   67.0%    19      119      888        33

sushitrack     dancetrack0080             71.8%   79.9%   64.5%   94.8%   81.5%    36       28      561       112
bytetrack     dancetrack0080             64.0%   70.3%   58.2%   92.1%   83.1%    28      251      675        77
ocsort        dancetrack0080             74.2%   80.5%   68.5%   93.6%   86.8%    26      126      620       149

sushitrack     dancetrack0082             67.6%   79.9%   57.1%   94.3%   74.0%    63       16      630       243
bytetrack     dancetrack0082             52.8%   73.5%   38.0%   93.8%   61.2%   108       69      593        57
ocsort        dancetrack0082             64.4%   78.8%   52.7%   92.3%   71.2%    77       38      839       129

sushitrack     dancetrack0083             57.1%   79.8%   40.9%   95.7%   60.7%    60       64      523       305
bytetrack     dancetrack0083             48.5%   68.4%   34.5%   86.6%   55.7%    67      816     1120        88
ocsort        dancetrack0083             49.8%   78.5%   31.6%   93.5%   51.1%    84      130      762       329

sushitrack     dancetrack0086             39.8%   64.4%   24.6%   76.5%   41.0%    96      843     1294       215
bytetrack     dancetrack0086             39.7%   59.1%   26.7%   73.8%   47.7%    80     1149     1256       141
ocsort        dancetrack0086             44.8%   58.2%   34.6%   64.6%   50.7%    87     1666     1606       325

sushitrack     dancetrack0087             40.5%   65.0%   25.4%   81.5%   43.6%   134      119     1724       122
bytetrack     dancetrack0087             35.3%   58.7%   21.3%   76.7%   39.6%   132      493     1864        61
ocsort        dancetrack0087             38.9%   63.7%   23.9%   79.1%   42.2%   107      128     1998       160

sushitrack     dancetrack0096             62.3%   68.8%   56.6%   82.6%   73.7%   116      628     2002       346
bytetrack     dancetrack0096             52.3%   58.8%   46.6%   77.3%   66.0%   157     1357     2077       237
ocsort        dancetrack0096             56.3%   62.1%   51.2%   72.0%   65.4%   138     1694     2588       283

sushitrack     dancetrack0098             42.3%   75.4%   23.9%   87.5%   41.7%    58       76      917        78
bytetrack     dancetrack0098             33.0%   68.0%   16.1%   85.9%   33.8%    82      179      925        19
ocsort        dancetrack0098             42.0%   73.5%   24.1%   83.7%   42.2%    65      103     1202        48

sushitrack     dancetrack0099             46.9%   69.2%   31.8%   82.0%   50.8%    44      373      698       121
bytetrack     dancetrack0099             48.0%   65.9%   35.1%   79.6%   54.4%    55      562      648        63
ocsort        dancetrack0099             50.4%   66.4%   38.4%   76.8%   56.6%    43      584      812        69

----------------------------------------------------------------------------------------------------------------
MOT17
----------------------------------------------------------------------------------------------------------------
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
sushitrack     MOT17                      47.4%   48.2%   47.8%   55.2%   55.7%  2824     7040   185424       190
bytetrack     MOT17                      44.7%   48.5%   42.4%   55.1%   52.5%  4773    12806   177427        89
ocsort        MOT17                      44.1%   46.4%   43.2%   53.0%   52.2%  2836     9172   191275       149
----------------------------------------------------------------------------------------------------------------
MOT20
----------------------------------------------------------------------------------------------------------------
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
sushitrack     MOT20                      44.6%   43.9%   50.4%   50.6%   49.5%  2937    13843  1184993       850
bytetrack     MOT20                      41.9%   42.7%   46.9%   49.9%   46.3%  3680    26843  1184486       575
ocsort        MOT20                      38.7%   39.0%   41.5%   45.1%   44.0%  2972    29224  1254585      1360
----------------------------------------------------------------------------------------------------------------
DanceTrack
----------------------------------------------------------------------------------------------------------------
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
sushitrack     DanceTrack                 45.0%   72.1%   29.5%   85.2%   47.2%  3250     7326    41998       112
bytetrack     DanceTrack                 37.9%   65.2%   23.3%   82.0%   42.8%  3863    15913    43195        51
ocsort        DanceTrack                 45.6%   69.7%   31.1%   80.2%   48.5%  2949    15976    50485       105
----------------------------------------------------------------------------------------------------------------
OVERALL
----------------------------------------------------------------------------------------------------------------
Tracker       Scenario                    HOTA    DetA    AssA    MOTA    IDF1  IDSW       FP       FN   Lat(us)
sushitrack     OVERALL                    45.8%   60.3%   38.4%   70.5%   50.6%  9011    28209  1412415       218
bytetrack     OVERALL                    40.8%   56.7%   32.8%   68.7%   46.8% 12316    55562  1405108       120
ocsort        OVERALL                    44.3%   57.9%   36.7%   66.5%   49.4%  8757    54372  1496345       253
---------------------------------------------------------------------------------------------------------------
```
