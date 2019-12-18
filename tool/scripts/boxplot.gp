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

set xlabel "heap size [byte]"
set ylabel ylabel_title
set title "[".basebit."bit] benchmark : ".benchname.", threashold : ".threashold
if (label_max==10485760 && label_min==1310720){
    set xtics ("10485760" 1, "7864320" 2, "5242880" 3, "3932160" 4, "2621440" 5, "1966080" 6, "1310720" 7) rotate by -25
    xmax=7
} else {
    if (label_max==3932160 && label_min==491520){
        set xtics ("3932160" 1, "2621440" 2, "1966080" 3, "1310720" 4, "983040" 5, "655360" 6, "491520" 7) rotate by -25
        xmax=7
    } else {
        set xtics ("10485760" 1, "7864320" 2, "5242880" 3, "3932160" 4, "2621440" 5, "1966080" 6, "1310720" 7, "983040" 8, "655360" 9, "491520" 10) rotate by -25
        xmax=10
    }
}

# write to eps
set terminal eps
set output fileout

# plot
set style fill solid 0.25 border -1
set style boxplot nooutliers pointtype 7
# set style data boxplot
set style boxplot labels off
set boxwidth 1
set bars 0.5

# stats file1 using 2 nooutput

# plot for [i=0:STATS_blocks-1] file1 using (7*i):2 index i lt 1 title (i==0 ? 'mark sweep' : ''),\
#      for [i=0:STATS_blocks-1] file2 using (7*i+1):2 index i lt 2 title (i==0 ? 'mark compact' : ''),\
#      for [i=0:STATS_blocks-1] file3 using (7*i+2):2 index i lt 3 title (i==0 ? 'threaded compact' : ''),\
#      for [i=0:STATS_blocks-1] file4 using (7*i+3):2 index i lt 4 title (i==0 ? 'copy' : ''),\
#      for [i=0:STATS_blocks-1] file1 using (7*i+1.5):(-1):xticlabel(1) index i w l notitle

# 例外点を作らない
set style boxplot fraction 1

# stats file1 using 1 nooutput

print "plot to ".threashold."_".benchname

ymax=0;
ythreashold=10000

STATS_blocks=0
stats file1 nooutput
if (STATS_blocks > 0) {
    len = STATS_blocks-1
    array A[len]
    do for [i=1:len]{
        STATS_median=-1
        stats file1 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            A[i]=STATS_median
            if (A[i] > ymax && A[i] < ythreashold) { ymax = A[i] }
        }
    }
} else {
    array A[1]
}
STATS_blocks=0
stats file2 nooutput
if (STATS_blocks > 0) {
    len = STATS_blocks-1
    array B[len]
    do for [i=1:len]{
        STATS_median=-1
        stats file2 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            B[i]=STATS_median
            if (B[i] > ymax && B[i] < ythreashold) { ymax = B[i] }
        }
    }
} else {
    array B[1]
}
STATS_blocks=0
stats file3 nooutput
if (STATS_blocks > 0) {
    len = STATS_blocks-1
    array C[len]
    do for [i=1:len]{
        STATS_median=-1
        stats file3 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            C[i]=STATS_median
            if (C[i] > ymax && C[i] < ythreashold) { ymax = C[i] }
        }
    }
} else {
    array C[1]
}
STATS_blocks=0
stats file4 nooutput
if (STATS_blocks > 0) {
    len = STATS_blocks-1
    array D[len]
    do for [i=1:len]{
        STATS_median=-1
        stats file4 using 2 index i-1 nooutput
        if (STATS_median >= 0) {
            D[i]=STATS_median
            if (D[i] > ymax && D[i] < ythreashold) { ymax = D[i] }
        }
    }
} else {
    array D[1]
}

set autoscale xfix
set clip one
set clip two
set yrange [*:ymax*1.25]
set xrange [1-0.15/2-0.25:xmax+0.15/2+0.25]

# using (x座標):データの列:(箱の幅(0のときデフォルト値)):データ区分の列
plot file1 using (1-0.15/2-0.15):2:(0.1):1 with boxplot title 'mark sweep',\
     file2 using (1-0.15/2-0.00):2:(0.1):1 with boxplot title 'mark compact',\
     file3 using (1+0.15/2+0.00):2:(0.1):1 with boxplot title 'threaded compact',\
     file4 using (1+0.15/2+0.15):2:(0.1):1 with boxplot title 'copy',\
     A using ($1-0.15/2-0.15):2 with line notitle lc rgb "magenta",\
     B using ($1-0.15/2-0.00):2 with line notitle lc rgb "green",\
     C using ($1+0.15/2+0.00):2 with line notitle lc rgb "cyan",\
     D using ($1+0.15/2+0.15):2 with line notitle lc rgb "orange"
