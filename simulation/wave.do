onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate -group {CPU board} /b68_cpu/B_A0
add wave -noupdate -group {CPU board} /b68_cpu/B_ACKn
add wave -noupdate -group {CPU board} -radix hexadecimal /b68_cpu/B_AD
add wave -noupdate -group {CPU board} /b68_cpu/B_ALEn
add wave -noupdate -group {CPU board} /b68_cpu/B_CEn
add wave -noupdate -group {CPU board} /b68_cpu/B_WEn
add wave -noupdate -group {CPU board} /b68_cpu/CPU_CLK
add wave -noupdate -group {CPU board} -radix hexadecimal /b68_cpu/CPU_A
add wave -noupdate -group {CPU board} /b68_cpu/CPU_FC
add wave -noupdate -group {CPU board} /b68_cpu/CPU_ASn
add wave -noupdate -group {CPU board} /b68_cpu/CPU_LDSn
add wave -noupdate -group {CPU board} /b68_cpu/CPU_UDSn
add wave -noupdate -group {CPU board} /b68_cpu/CPU_RWn
add wave -noupdate -group {CPU board} -radix hexadecimal /b68_cpu/CPU_D
add wave -noupdate -group {CPU board} /b68_cpu/CPU_DTACKn
add wave -noupdate -group {CPU board} /b68_cpu/CPU_BERRn
add wave -noupdate -group {CPU board} /b68_cpu/CPU_VPAn
add wave -noupdate -group {CPU board} /b68_cpu/CPU_IPLn
add wave -noupdate -group {CPU board} /b68_cpu/GCLK
add wave -noupdate -group {CPU board} /b68_cpu/G_CSn
add wave -noupdate -group {CPU board} /b68_cpu/G_DIR
add wave -noupdate -group {CPU board} /b68_cpu/G_OEn
add wave -noupdate -group {CPU board} /b68_cpu/G_OEn_HI
add wave -noupdate -group {CPU board} /b68_cpu/G_OEn_LO
add wave -noupdate -group {CPU board} /b68_cpu/IRQn
add wave -noupdate -group {CPU board} /b68_cpu/KB_CLK
add wave -noupdate -group {CPU board} /b68_cpu/KB_DAT
add wave -noupdate -group {CPU board} /b68_cpu/MS_CLK
add wave -noupdate -group {CPU board} /b68_cpu/MS_DAT
add wave -noupdate -group {CPU board} -radix hexadecimal /b68_cpu/M_A
add wave -noupdate -group {CPU board} /b68_cpu/M_RASn
add wave -noupdate -group {CPU board} /b68_cpu/M_CASLn
add wave -noupdate -group {CPU board} /b68_cpu/M_CASUn
add wave -noupdate -group {CPU board} /b68_cpu/M_WEn
add wave -noupdate -group {CPU board} /b68_cpu/P_RX
add wave -noupdate -group {CPU board} /b68_cpu/P_TX
add wave -noupdate -group {CPU board} /b68_cpu/RSTn
add wave -noupdate -group {CPU board} /b68_cpu/Y_wire
add wave -noupdate -group CPU/glue /b68_cpu/U4_glue/\\rf_cnt_rtl_0|ALT_INV_dffs\\
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_A0
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_ACKn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_AD
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_ALEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_CEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_WEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/DAC_REn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/DAC_WEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/FLEX_REn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/FLEX_WEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/G_DIR
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/G_OEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/HSYNC
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/IRQn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/L_A
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/L_AD
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/L_RSTn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/MA
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/MAD
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/MALE
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/MOEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/MSEL
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/MWEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/OPL2_CEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/OPL2_REn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/OPL2_WEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/OPL_MO
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/OPL_RSTn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/OPL_SH
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/PBLKn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/PCLK
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/PDAT
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/RGB
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/RSTn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/VB
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/VG
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/VR
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/VSYNC
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/clk25
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/clk3p58
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/vga_ena_wire
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {54475000 ps} 0}
quietly wave cursor active 1
configure wave -namecolwidth 150
configure wave -valuecolwidth 132
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ps
update
WaveRestoreZoom {92555 ns} {115655 ns}
