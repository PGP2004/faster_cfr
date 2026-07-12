#!/bin/bash

./build/bin/make_river_strengths
./build/bin/make_turn_cdfs
./build/bin/make_turn_clusters
./build/bin/make_flop_pdfs
./build/bin/make_flop_clusters

echo "All done!"
