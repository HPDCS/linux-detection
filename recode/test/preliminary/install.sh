	#!/bin/zsh

declare -a files=("system/octave-benchmark" "pts/swet" "system/openscad" "system/inkscape" "pts/jgfxbat" "pts/powertop-wakeups" "pts/mafft" "pts/neat" "pts/comd-cl" "pts/aircrack-ng" "pts/srslte" "pts/system-libjpeg" "pts/system-decompress-tiff" "pts/system-libxml2" "pts/opendwarfs" )

for i in "${files[@]}"
do
	phoronix-test-suite install "$i"
done
