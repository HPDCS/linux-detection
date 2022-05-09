#!/bin/zsh

#  "pts/systester" requires qt
declare -a files=("pts/dcraw" "pts/stream" "pts/tscp" "pts/render-bench" "pts/m-queens" "pts/scimark2" "pts/fio" "pts/sqlite" "pts/jxrendermark" "pts/aio-stress" "pts/build-mplayer" "pts/crafty" "pts/t-test1" "pts/core-latency" "pts/n-queens" "pts/ipc-benchmark" "pts/cpuminer-opt" "pts/ctx-clock" "pts/fhourstones")
for i in "${files[@]}"
do
	phoronix-test-suite install "$i"
done
