transcript quietly

# global options
set opt_fast_start 1

# parse arguments
set n $argc

while { $n > 0 } {
    # extract argument value
    regexp -line -- {=(.+)} $1 full val
    
    switch -regexp $1 {
	fast-start=[01] {
	    set opt_fast_start $val
	}
	default {
	    echo "error: $1: bad argument"
	    pause
	}
    }

    shift

    incr n -1
}

# setup generic
if { $opt_fast_start == 1 } {
    set g_fast_start "-gFAST_START=true"
} else {
    set g_fast_start "-gFAST_START=false"
}

vsim -sdftyp /b68_cpu/U4_glue=../boards/b68k-cpu/glue/simulation/modelsim/glue_vhd.sdo -sdftyp /b68_cpu/av_board_gen/av_board/U6_flex=../boards/b68k-av/flex/simulation/modelsim/flex_vhd.sdo -sdftyp /b68_cpu/av_board_gen/av_board/U2_avmgr=../boards/b68k-av/avmgr/simulation/modelsim/avmgr_vhd.sdo $g_fast_start work.b68_cpu

# waveform
do wave.do

if { $opt_fast_start == 1 } {
    # reduce bootstrap DMA size to 16 bytes thus startup time
    #  1- enforce counter value
    force -freeze sim:/b68_cpu/U4_glue/\\rf_cnt_rtl_0|dffs\\ 000001000 0
    #  2- run until first rf_cnt value change reset time
    run 1.5 us
    #  3- unforce
    noforce sim:/b68_cpu/U4_glue/\\rf_cnt_rtl_0|dffs\\
} else {
    run 1.5 us
}	


#run 50 us
