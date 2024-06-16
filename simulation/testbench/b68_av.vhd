--
-- https://github.com/Lougous/b68k
--
-- b68k-av board model
--

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity b68_av is

  port (
    -- local bus
    B_CEn  : in    std_logic;
    B_WEn  : in    std_logic;
    IRQn   : inout std_logic;
    RSTn   : in    std_logic;
    B_ALEn : in    std_logic;
    B_ACKn : inout std_logic;
    B_A0   : in    std_logic;
    B_AD   : inout std_logic_vector(7 downto 0)
    );

end entity b68_av;

architecture rtl of b68_av is

  signal clk25   : std_logic := '0';
  signal clk3p58 : std_logic;
  
  signal L_AD : std_logic_vector(7 downto 0);
  signal L_A  : std_logic_vector(2 downto 1);
  signal L_RSTn : std_logic;
  signal FLEX_WEn : std_logic;
  signal FLEX_REn : std_logic;

  signal G_OEn : std_logic;  -- AD bus driver output enable
  signal G_DIR : std_logic;  -- AD bus driver direction 
    
  signal MALE : std_logic;
  signal MAD  : std_logic_vector(15 downto 0);
  signal MA   : std_logic_vector(16 downto 0);
  signal MSEL : std_logic_vector(2 downto 0);
  signal MWEn : std_logic;
  signal MOEn : std_logic;

  signal PDAT  : std_logic_vector(7 downto 0);
  signal PCLK  : std_logic;
  signal PBLKn : std_logic;
  signal VSYNC : std_logic;
  signal HSYNC : std_logic;
  
  signal VR    : std_logic_vector(7 downto 0);
  signal VG    : std_logic_vector(7 downto 0);
  signal VB    : std_logic_vector(7 downto 0);
  signal RGB   : std_logic_vector(11 downto 0);

  signal OPL2_CEn : std_logic;
  signal OPL2_WEn : std_logic;
  signal OPL2_REn : std_logic;
  signal OPL_RSTn : std_logic;
  signal OPL_MO   : std_logic;
  signal OPL_SH   : std_logic;

  signal DAC_WEn : std_logic;
  signal DAC_REn : std_logic;

  signal vga_ena_wire : std_logic;
  
begin

    clk25 <= not clk25 after 20 ns;
    
    U7_74ls245 : entity work.sl74245
      generic map (
        tPD => 8 ns,
        tPZ => 10 ns
        )
      port map (
        OEn  => G_OEn,
        TRn  => G_DIR,
        A    => L_AD,
        B    => B_AD
        );

    U2_avmgr : entity work.avmgr
      port map (
        -- local clocks
        CLK25   => clk25,
        CLK3P58 => clk3p58,

        -- extension bus
        B_RSTn => RSTn,
        B_CEn  => B_CEn,
        B_WEn  => B_WEn,
        B_ACKn => B_ACKn,
        B_ALEn => B_ALEn,

        -- spare, to GPIO connector
        GP => open,

        -- flex conf
        FLEX_DCLK => open,
        FLEX_DAT0 => open,
        FLEX_nCFG => open,
        FLEX_CFD  => '1',
        FLEX_nSTS => '1',

        -- local bus
        L_A    => L_A(2 downto 1),
        L_AD7  => L_AD(7),
        L_AD6  => L_AD(6),
        L_AD1  => L_AD(1),
        L_AD0  => L_AD(0),
        L_RSTn => L_RSTn,

        G_OEn => G_OEn,  -- AD bus driver output enable
        G_DIR => G_DIR,  -- AD bus driver direction 

        -- OPL2
        OPL2_CEn => OPL2_CEn,
        OPL2_WEn => OPL2_WEn,
        OPL2_REn => OPL2_REn,

        -- RAMDAC
        DAC_WEn => DAC_WEn,
        DAC_REn => DAC_REn,

        -- flex
        FLEX_WEn => FLEX_WEn,
        FLEX_REn => FLEX_REn

        );

    U6_flex : entity work.flex
      port map (
        RSTn  => L_RSTn,
        CLK25 => clk25,

        -- local bus
        LWEn  => FLEX_WEn,
        LREn  => FLEX_REn,
        LA0   => B_A0,
        LALEn => B_ALEn,
        LAD   => L_AD,
        IRQn  => IRQn,

        -- memory
        MAD   => MAD,
        MA0   => MA(16),
        MALE  => MALE,
        MSEL  => MSEL,
        MWEn  => MWEn,
        MOEn  => MOEn,
    
        -- video out
        PDAT  => PDAT,
        PCLK  => PCLK,
        BLANK => PBLKn,
        VSYNC => VSYNC,
        HSYNC => HSYNC,

        -- OPL2
        OPL_RSTn => OPL_RSTn,
        OPL_MO   => OPL_MO,
        OPL_SH   => OPL_SH,

        -- audio out
        PWM_R_HI => open,
        PWM_R_LO => open,
        PWM_L_HI => open,
        PWM_L_LO => open
        );

    U7_74F373 : entity work.sl74373
      generic map (
        tPD_D  => 5 ns,
        tPD_LE => 9 ns,
        tPZ    => 5 ns
        )
      port map (
        OEn => '0',
        LE  => MALE,
        D   => MAD(7 downto 0),
        O   => MA(7 downto 0)
        );
    
    U8_74F373 : entity work.sl74373
      generic map (
        tPD_D  => 5 ns,
        tPD_LE => 9 ns,
        tPZ    => 5 ns
        )
      port map (
        OEn => '0',
        LE  => MALE,
        D   => MAD(15 downto 8),
        O   => MA(15 downto 8)
        );

    board_av_mem_lo : entity work.b68_av_mem
      generic map (
        CONF_HI_LOn => '0'
        )
      port map (
        A   => MA,
        D   => MAD,
        SEL => MSEL,
        OEn => MOEn,
        WEn => MWEn
        );
    
    board_av_mem_hi : entity work.b68_av_mem
      generic map (
        CONF_HI_LOn => '1'
        )
      port map (
        A   => MA,
        D   => MAD,
        SEL => MSEL,
        OEn => MOEn,
        WEn => MWEn
        );
    
    U12_RAMDAC : entity work.ramdac
       port map (
        -- local bus
        WRn    => DAC_WEn,
        RDn    => DAC_REn,
        RS     => L_A,
        D      => L_AD,

        -- video in
        BLANKn => PBLKn,
        P      => PDAT,
        PCLK   => PCLK,

        -- video out
        R      => VR,
        G      => VG,
        B      => VB
        );
     
    RGB <= VR(7 downto 4) & VG(7 downto 4) & VB(7 downto 4);

    vga_ena_wire <= VSYNC and HSYNC;
    
    vga_recorder_i : entity work.vga_recorder
      generic map (
        ROOT_NAME   => "./output/vga/frame",
        WIDTH       => 800-96,
        HEIGHT      => 525-2,
        AUTO_ENABLE => true
        )
      port map (
        CLK => PCLK,
        VSn => VSYNC,
        HSn => HSYNC,
        ENA => vga_ena_wire,
        PIX => RGB
        );

    U11_OPL2 : entity work.opl2
      port map (
        -- local bus
        CSn  => OPL2_CEn,
        WRn  => OPL2_WEn,
        RDn  => OPL2_REn,
        A0   => L_A(1),
        D    => L_AD,
    
        --
        phyM => CLK3p58,
        ICn  => OPL_RSTn,
        MO   => OPL_MO,
        SH   => OPL_SH
        );

     
end architecture rtl;
