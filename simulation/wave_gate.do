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
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_ALEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_CEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_WEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/B_ACKn
add wave -noupdate -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/B_AD
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/DAC_REn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/DAC_WEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/FLEX_REn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/FLEX_WEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/G_DIR
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/G_OEn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/VSYNC
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/HSYNC
add wave -noupdate -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/VB
add wave -noupdate -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/VG
add wave -noupdate -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/VR
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/IRQn
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/L_A
add wave -noupdate -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/L_AD
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/L_RSTn
add wave -noupdate -group {AV board} -radix hexadecimal -childformat {{/b68_cpu/av_board_gen/av_board/MA(16) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(15) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(14) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(13) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(12) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(11) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(10) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(9) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(8) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(7) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(6) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(5) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(4) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(3) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(2) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(1) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(0) -radix hexadecimal}} -subitemconfig {/b68_cpu/av_board_gen/av_board/MA(16) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(15) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(14) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(13) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(12) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(11) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(10) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(9) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(8) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(7) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(6) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(5) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(4) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(3) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(2) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(1) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(0) {-height 17 -radix hexadecimal}} /b68_cpu/av_board_gen/av_board/MA
add wave -noupdate -group {AV board} -radix hexadecimal -childformat {{/b68_cpu/av_board_gen/av_board/MAD(15) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(14) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(13) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(12) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(11) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(10) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(9) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(8) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(7) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(6) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(5) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(4) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(3) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(2) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(1) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(0) -radix hexadecimal}} -subitemconfig {/b68_cpu/av_board_gen/av_board/MAD(15) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(14) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(13) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(12) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(11) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(10) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(9) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(8) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(7) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(6) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(5) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(4) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(3) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(2) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(1) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(0) {-height 17 -radix hexadecimal}} /b68_cpu/av_board_gen/av_board/MAD
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
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/clk25
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/clk3p58
add wave -noupdate -group {AV board} /b68_cpu/av_board_gen/av_board/vga_ena_wire
add wave -noupdate -expand -group flex /b68_cpu/av_board_gen/av_board/L_RSTn
add wave -noupdate -expand -group flex /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/MA0
add wave -noupdate -expand -group flex -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/MAD
add wave -noupdate -expand -group flex /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/MALE
add wave -noupdate -expand -group flex /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/MOEn
add wave -noupdate -expand -group flex /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/MSEL
add wave -noupdate -expand -group flex /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/MSEL_r
add wave -noupdate -expand -group flex /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/MWEn
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/CLK25
add wave -noupdate -expand -group flex -expand -group GPU -label LWE /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\g_lwe~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label BUSY /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\gpu_i|BUSY~combout\\
add wave -noupdate -expand -group flex -expand -group GPU -label ACK /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\m_g_ack~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label RDACK /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\m_g_rack~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label WDAC /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\m_g_wack~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label RDT -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/MAD
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/WDT
add wave -noupdate -expand -group flex -expand -group GPU -label rack_d /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\rack_d~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label regkey /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\gpu_i|regkey~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label reglut /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\gpu_i|reglut\\
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/regad
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/regsx
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/regsy
add wave -noupdate -expand -group flex -expand -group GPU -label dir /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\dir~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label reading /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\gpu_i|reading~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label writing /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\gpu_i|writing~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label l_enable /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\l_enable~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/l_ptr
add wave -noupdate -expand -group flex -expand -group GPU -label mad -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\gpu_i|mad_rtl_0|wysi_counter|counter_cell\\
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/mdata
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/sc_ad
add wave -noupdate -expand -group flex -expand -group GPU -label sc_we /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\sc_we~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label vs_trig /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\vs_trig~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label s_idle /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\s_idle~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label s_load /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\s_load_ptr~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label s_nextline /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\s_nextline~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label s_save_ptr /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\s_save_ptr~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label s_start /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\s_start~regout\\
add wave -noupdate -expand -group flex -expand -group GPU -label to_go_x /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\to_go_x_rtl_2|wysi_counter|counter_cell\\
add wave -noupdate -expand -group flex -expand -group GPU -label to_go_y /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/gpu_i/\\to_go_y_rtl_3|wysi_counter|counter_cell\\
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_gate_arch_g/U6_flex/\\gpu_i|scratch_i|lpm_ram_dq_component|sram|q\\
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {126808372 ps} 1} {{Cursor 2} {126913173 ps} 0}
quietly wave cursor active 2
configure wave -namecolwidth 235
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
WaveRestoreZoom {0 ps} {1958906 ps}
