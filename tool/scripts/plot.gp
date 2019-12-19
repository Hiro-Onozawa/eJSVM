# indir : 入力データファイル格納ディレクトリ
# outdir : 出力グラフファイル格納ディレクトリ
# benchname : ベンチマーク名
# threashold : スレッショルド
# basebit : 実行環境のビット幅
# ylabel_title : y軸の表記
# label_max : x軸のメモリの表記の最大値
# label_min : x軸のメモリの表記の最小値

file1=indir."/mark_sweep_".threashold."_".benchname.".txt"
file2=indir."/mark_compact_".threashold."_".benchname.".txt"
file3=indir."/threaded_compact_".threashold."_".benchname.".txt"
file4=indir."/copy_".threashold."_".benchname.".txt"
fileout=outdir."/".threashold."_".benchname.".eps"
tableout=outdir."/".threashold."_".benchname."_values.txt"

set xlabel "heap size [KiB]"
set ylabel ylabel_title
set title "[".basebit."bit] benchmark : ".benchname.", threashold : ".threashold
if (label_max==10485760 && label_min==1310720){
    xmax=7
    set xtics ("10240" 1, "7680" 2, "5120" 3, "3840" 4, "2560" 5, "1920" 6, "1280" 7)
    array sizes[xmax] = ["10240", "7680", "5120", "3840", "2560", "1920", "1280"]
} else {
    if (label_max==3932160 && label_min==491520){
        xmax=7
        set xtics ("3840" 1, "2560" 2, "1920" 3, "1280" 4, "960" 5, "640" 6, "480" 7)
        array sizes[xmax] = ["3840", "2560", "1920", "1280", "960", "640", "480"]
    } else {
        xmax=10
        set xtics ("10240" 1, "7680" 2, "5120" 3, "3840" 4, "2560" 5, "1920" 6, "1280" 7, "960" 8, "640" 9, "480" 10)
        array sizes[xmax] = ["10240", "7680", "5120", "3840", "2560", "1920", "1280", "960", "640", "480"]
    }
}

# write to eps
set terminal eps
set output fileout

# plot
print "plot to ".threashold."_".benchname

ymax=0;
ythreashold=10000

STATS_blocks=0
array A[xmax]
stats file1 nooutput
if (STATS_blocks > 0) {
    do for [i=1:xmax]{
        STATS_median=-1
        stats file1 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            A[i]=STATS_median
            if (A[i] > ymax && A[i] < ythreashold) { ymax = A[i] }
        }
    }
}
STATS_blocks=0
array B[xmax]
stats file2 nooutput
if (STATS_blocks > 0) {
    do for [i=1:xmax]{
        STATS_median=-1
        stats file2 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            B[i]=STATS_median
            if (B[i] > ymax && B[i] < ythreashold) { ymax = B[i] }
        }
    }
}
STATS_blocks=0
array C[xmax]
stats file3 nooutput
if (STATS_blocks > 0) {
    do for [i=1:xmax]{
        STATS_median=-1
        stats file3 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            C[i]=STATS_median
            if (C[i] > ymax && C[i] < ythreashold) { ymax = C[i] }
        }
    }
}
STATS_blocks=0
array D[xmax]
stats file4 nooutput
if (STATS_blocks > 0) {
    do for [i=1:xmax]{
        STATS_median=-1
        stats file4 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            D[i]=STATS_median
            if (D[i] > ymax && D[i] < ythreashold) { ymax = D[i] }
        }
    }
}

set autoscale xfix
set clip one
set clip two
set yrange [*:ymax*1.25]
set xrange [1:xmax]

# using (x座標):データの列:(箱の幅(0のときデフォルト値)):データ区分の列
plot A using 1:2 with linespoints pt 1 ps 0.75 title "mark sweep",\
     B using 1:2 with linespoints pt 2 ps 0.75 title "mark compact",\
     C using 1:2 with linespoints pt 3 ps 0.75 title "threaded compact",\
     D using 1:2 with linespoints pt 4 ps 0.75 title "copy"


# write table to txt
set print tableout
print "Heap Size \\[KiB\\] & mark sweep & mark compact & threaded compact & copy \\\\ \\hline \\\\ \\hline"
do for [i=1:xmax]{
    print sizes[i], " & ", A[i], " & ", B[i], " & ", C[i], " & ", D[i], " \\\\"
}
print "\\hline"
set print "-"