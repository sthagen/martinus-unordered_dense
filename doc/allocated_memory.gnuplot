#!/usr/bin/gnuplot

#set terminal pngcairo 
#set terminal pngcairo size 730,510 enhanced font 'Verdana,10'
set terminal pngcairo size 1440,1024 enhanced font 'Verdana,10'

# define axis
# remove border on top and right and set color to gray
set style line 11 lc rgb '#808080' lt 1
set border 3 back ls 11
set tics nomirror
# define grid
set style line 12 lc rgb '#808080' lt 0 lw 1
set grid back ls 12

# line styles
set style line 1 lt 1 lc rgb '#1B9E77' # dark teal
set style line 2 lt 1 lc rgb '#D95F02' # dark orange
set style line 3 lt 1 lc rgb '#7570B3' # dark lilac
set style line 4 lt 1 lc rgb '#E7298A' # dark magenta
set style line 5 lt 1 lc rgb '#66A61E' # dark lime green
set style line 6 lt 1 lc rgb '#E6AB02' # dark banana
set style line 7 lt 1 lc rgb '#A6761D' # dark tan
set style line 8 lt 1 lc rgb '#666666' # dark gray


set style line 101 lc rgb '#808080' lt 1 lw 1
set border 3 front ls 101
set tics nomirror out scale 0.75

# set key left top

set output 'allocated_memory.png'

set xlabel "Runtime [ms]"
set ylabel "Allocated memory [MB]"

set title "Allocated Memory when filling a map with 500M elements"

plot \
    'gottsdots_map.txt' using ($1*1e3):($2/1e6) w steps ls 1 lw 2 title "ankerl::unordered\\\_dense::map" , \
    'gottsdots_segmented_map.txt' using ($1*1e3):($2/1e6) w steps ls 2 lw 2 title "ankerl::unordered\\\_dense::segmented\\\_map" , \
    'gottsdots_boost_unordered_flat_map.txt' using ($1*1e3):($2/1e6) w steps ls 3 lw 2 title "boost::unordered\\\_flat\\\_map"
