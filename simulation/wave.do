onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /b68_cpu/B_A0
add wave -noupdate /b68_cpu/B_ACKn
add wave -noupdate -radix hexadecimal /b68_cpu/B_AD
add wave -noupdate /b68_cpu/B_ALEn
add wave -noupdate -expand /b68_cpu/B_CEn
add wave -noupdate /b68_cpu/B_WEn
add wave -noupdate -radix hexadecimal /b68_cpu/CPU_A
add wave -noupdate /b68_cpu/CPU_ASn
add wave -noupdate /b68_cpu/CPU_BERRn
add wave -noupdate /b68_cpu/CPU_CLK
add wave -noupdate -radix hexadecimal /b68_cpu/CPU_D
add wave -noupdate /b68_cpu/CPU_DTACKn
add wave -noupdate /b68_cpu/CPU_FC
add wave -noupdate /b68_cpu/CPU_IPLn
add wave -noupdate /b68_cpu/CPU_LDSn
add wave -noupdate /b68_cpu/CPU_RWn
add wave -noupdate /b68_cpu/CPU_UDSn
add wave -noupdate /b68_cpu/CPU_VPAn
add wave -noupdate /b68_cpu/GCLK
add wave -noupdate /b68_cpu/G_CSn
add wave -noupdate /b68_cpu/G_DIR
add wave -noupdate /b68_cpu/G_OEn
add wave -noupdate /b68_cpu/G_OEn_HI
add wave -noupdate /b68_cpu/G_OEn_LO
add wave -noupdate /b68_cpu/IRQn
add wave -noupdate /b68_cpu/KB_CLK
add wave -noupdate /b68_cpu/KB_DAT
add wave -noupdate /b68_cpu/MS_CLK
add wave -noupdate /b68_cpu/MS_DAT
add wave -noupdate -radix hexadecimal /b68_cpu/M_A
add wave -noupdate /b68_cpu/M_RASn
add wave -noupdate /b68_cpu/M_CASLn
add wave -noupdate /b68_cpu/M_CASUn
add wave -noupdate /b68_cpu/M_WEn
add wave -noupdate /b68_cpu/P_RX
add wave -noupdate /b68_cpu/P_TX
add wave -noupdate /b68_cpu/RSTn
add wave -noupdate /b68_cpu/Y_wire
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {3919141 ps} 0}
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
WaveRestoreZoom {0 ps} {7350 ns}
