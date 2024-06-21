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
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/B_A0
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/B_ALEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/B_CEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/B_WEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/B_ACKn
add wave -noupdate -expand -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/B_AD
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/DAC_REn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/DAC_WEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/FLEX_REn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/FLEX_WEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/G_DIR
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/G_OEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/VSYNC
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/HSYNC
add wave -noupdate -expand -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/VB
add wave -noupdate -expand -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/VG
add wave -noupdate -expand -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/VR
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/IRQn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/L_A
add wave -noupdate -expand -group {AV board} -radix hexadecimal /b68_cpu/av_board_gen/av_board/L_AD
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/L_RSTn
add wave -noupdate -expand -group {AV board} -radix hexadecimal -childformat {{/b68_cpu/av_board_gen/av_board/MA(16) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(15) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(14) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(13) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(12) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(11) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(10) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(9) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(8) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(7) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(6) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(5) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(4) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(3) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(2) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(1) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MA(0) -radix hexadecimal}} -subitemconfig {/b68_cpu/av_board_gen/av_board/MA(16) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(15) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(14) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(13) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(12) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(11) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(10) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(9) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(8) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(7) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(6) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(5) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(4) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(3) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(2) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(1) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MA(0) {-height 17 -radix hexadecimal}} /b68_cpu/av_board_gen/av_board/MA
add wave -noupdate -expand -group {AV board} -radix hexadecimal -childformat {{/b68_cpu/av_board_gen/av_board/MAD(15) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(14) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(13) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(12) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(11) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(10) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(9) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(8) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(7) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(6) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(5) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(4) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(3) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(2) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(1) -radix hexadecimal} {/b68_cpu/av_board_gen/av_board/MAD(0) -radix hexadecimal}} -subitemconfig {/b68_cpu/av_board_gen/av_board/MAD(15) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(14) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(13) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(12) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(11) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(10) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(9) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(8) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(7) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(6) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(5) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(4) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(3) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(2) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(1) {-height 17 -radix hexadecimal} /b68_cpu/av_board_gen/av_board/MAD(0) {-height 17 -radix hexadecimal}} /b68_cpu/av_board_gen/av_board/MAD
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/MALE
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/MOEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/MOEn_dly
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/MSEL
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/MWEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/OPL2_CEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/OPL2_REn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/OPL2_WEn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/OPL_MO
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/OPL_RSTn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/OPL_SH
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/PBLKn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/PCLK
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/PDAT
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/RGB
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/RSTn
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/clk25
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/clk3p58
add wave -noupdate -expand -group {AV board} /b68_cpu/av_board_gen/av_board/vga_ena_wire
add wave -noupdate -expand -group flex /b68_cpu/av_board_gen/av_board/L_RSTn
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/RSTn
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/CLK25
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/TOP_64
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/OPL_RSTn
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/OPL_MO
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/OPL_SH
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/PWM_R_HI
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/PWM_R_LO
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/PWM_L_HI
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/PWM_L_LO
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/opl2_dat
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/opl2_top_50khz
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/opl2_latch
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/pwm_hi
add wave -noupdate -expand -group flex -group APU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/apu_i/pwm_lo
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RSTn
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/CLK25
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/LWE
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/LA
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/LD
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/BUSY
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RB_BUFFER
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RB_BASE
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RB_LOWRES
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RB_LUT_PAGE
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/VS
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/WE
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RE
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/AD
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/WDT
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/WM
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/ACK
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/WDACK
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RDT
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RDT_swp
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/RDACK
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/sc_ad
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/sc_wd
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/sc_we
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/sc_rd
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/regad
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/regsx
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/regsy
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/reglut
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/regkey
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/mad
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/mdata
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/s_nextline
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/s_idle
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/s_start
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/s_save_ptr
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/s_load_ptr
add wave -noupdate -expand -group flex -expand -group GPU -radix hexadecimal /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/l_ptr
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/l_enable
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/vs_trig
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/reading
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/writing
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/dir
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/rack_d
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/to_go_x
add wave -noupdate -expand -group flex -expand -group GPU /b68_cpu/av_board_gen/av_board/flex_rtl_arch_g/U6_flex/gpu_i/to_go_y
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {126808372 ps} 1} {{Cursor 2} {126867000 ps} 0}
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
WaveRestoreZoom {126270067 ps} {130682350 ps}
