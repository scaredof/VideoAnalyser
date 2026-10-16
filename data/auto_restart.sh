#!/bin/bash
set -u
BASE_DIR=/home/ubuntu/BXC_VideoAnalyzer_v3.49
export LD_LIBRARY_PATH=/usr/local/openvino/3rdparty/tbb/lib:${LD_LIBRARY_PATH:-}
DATA_DIR=$BASE_DIR/data
LOG_DIR=$DATA_DIR/log
ANALYZER=$BASE_DIR/Analyzer/build/Analyzer
CONFIG=$BASE_DIR/config.json
ANALYZER_LOG=$LOG_DIR/analyzer.log
MAX_ANALYZER_BYTES=$((100 * 1024 * 1024))
MAX_ROTATED=5
mkdir -p $LOG_DIR
exec >/dev/null 2>&1
rotate_if_needed() {
    file=$1
    limit=$2
    base=$3
    if [ -f $file ] && [ $(stat -c %s $file 2>/dev/null || echo 0) -ge $limit ]; then
        cp -f $file ${base}.$(date +%Y%m%d-%H%M%S) && : > $file
        touch $file
        ls -1t ${base}.* 2>/dev/null | tail -n +$((MAX_ROTATED + 1)) | xargs -r rm -f
    fi
}
while true; do
    if ! pgrep -x Analyzer >/dev/null 2>&1; then
        rotate_if_needed $ANALYZER_LOG $MAX_ANALYZER_BYTES $LOG_DIR/analyzer.log
        echo "[$(date -Is)] Analyzer stopped; starting it again"
        $ANALYZER -f $CONFIG >>$ANALYZER_LOG 2>&1 &
    fi
    rotate_if_needed $ANALYZER_LOG $MAX_ANALYZER_BYTES $LOG_DIR/analyzer.log
    sleep 5
done
