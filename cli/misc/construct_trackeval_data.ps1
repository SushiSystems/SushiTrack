$script_dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$base_dir = Resolve-Path "$script_dir/../.."
$test_data = "$base_dir/tests/regression/evaluator/data"
$te_base = "$base_dir/third_party/TrackEval"
$te_data = "$te_base/data"

Write-Host "Constructing TrackEval data structure..."

# Create base directories
$directories = @(
    "$te_data/gt/mot_challenge/MOT17-train",
    "$te_data/gt/mot_challenge/MOT20-train",
    "$te_data/trackers/mot_challenge/MOT17-train",
    "$te_data/trackers/mot_challenge/MOT20-train"
)

foreach ($dir in $directories) {
    if (!(Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
}

# 1. MOT17 Ground Truth (Linking SDP versions as representative)
$mot17_ids = @("02", "04", "05", "09", "10", "11", "13")
foreach ($id in $mot17_ids) {
    $src = "$test_data/MOT17-$id-SDP"
    $dest = "$te_data/gt/mot_challenge/MOT17-train/MOT17-$id"
    if (Test-Path $src) {
        if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }
        New-Item -ItemType Junction -Path $dest -Target $src -Force | Out-Null
        Write-Host "Linked MOT17-$id Ground Truth"
    }
}

# 2. MOT20 Ground Truth
$mot20_ids = @("01", "02", "03", "05")
foreach ($id in $mot20_ids) {
    $src = "$test_data/MOT20-$id"
    $dest = "$te_data/gt/mot_challenge/MOT20-train/MOT20-$id"
    if (Test-Path $src) {
        if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }
        New-Item -ItemType Junction -Path $dest -Target $src -Force | Out-Null
        Write-Host "Linked MOT20-$id Ground Truth"
    }
}


# 3. Trackers (sushitrack, bytetrack, ocsort)
$trackers = @("sushitrack", "bytetrack", "ocsort")

foreach ($tracker in $trackers) {
    $t_mot17_dir = "$te_data/trackers/mot_challenge/MOT17-train/$tracker/data"
    $t_mot20_dir = "$te_data/trackers/mot_challenge/MOT20-train/$tracker/data"
    
    New-Item -ItemType Directory -Path $t_mot17_dir -Force | Out-Null
    New-Item -ItemType Directory -Path $t_mot20_dir -Force | Out-Null

    # Copy MOT17 predictions
    foreach ($id in $mot17_ids) {
        foreach ($suffix in @("SDP", "DPM", "FRCNN")) {
            $src = "$test_data/MOT17-$id-$suffix/pred/$tracker/MOT17-$id-$suffix.txt"
            if (Test-Path $src) {
                Copy-Item $src -Destination "$t_mot17_dir/MOT17-$id.txt" -Force
                Write-Host "Copied $tracker MOT17-$id prediction (from $suffix)"
                break
            }
        }
    }


    # Copy MOT20 predictions
    foreach ($id in $mot20_ids) {
        $src = "$test_data/MOT20-$id/pred/$tracker/MOT20-$id.txt"
        if (Test-Path $src) {
            Copy-Item $src -Destination "$t_mot20_dir/MOT20-$id.txt" -Force
            Write-Host "Copied $tracker MOT20-$id prediction"
        }
    }
}

Write-Host "TrackEval structure construction complete."
