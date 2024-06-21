--
-- https://github.com/Lougous/b68k
--
-- b68k computer DUT for simulation
--
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity b68_cpu is

  generic (
    -- reduce bootstrap size 
    FAST_START : boolean := false;
    -- gate level for AV board flex FPGA
    FLEX_GATE  : boolean := false;
    
--    tGCLK_ps : integer := 125000    -- 8 MHz
    tGCLK_ps : integer := 50000    -- 20 MHz
--    tGCLK_ps : integer := 62500    -- 16 MHz
--    tGCLK_ps : integer := 83333    -- 12 MHz
    );

end entity b68_cpu;

architecture rtl of b68_cpu is

  signal RSTn : std_logic;
  signal IRQn : std_logic;
  signal GCLK : std_logic := '0';
  
  signal CPU_CLK    : std_logic;
  signal CPU_A      : std_logic_vector(23 downto 0);
  signal CPU_ASn    : std_logic;
  signal CPU_LDSn   : std_logic;
  signal CPU_UDSn   : std_logic;
  signal CPU_RWn    : std_logic;
  signal CPU_FC     : std_logic_vector(2 downto 0);
  signal CPU_BERRn  : std_logic;
  signal CPU_VPAn   : std_logic;
  signal CPU_D      : std_logic_vector(15 downto 0);
  signal CPU_DTACKn : std_logic;
  signal CPU_IPLn   : std_logic_vector(2 downto 0);
  
  signal M_A     : std_logic_vector(10 downto 0);
  signal M_CASLn : std_logic;
  signal M_CASUn : std_logic;
  signal M_RASn  : std_logic;
  signal M_WEn   : std_logic;

  -- Bus
  signal B_WEn  : std_logic;
  signal B_ACKn : std_logic;
  signal B_A0   : std_logic;
  signal B_AD   : std_logic_vector(7 downto 0);
  signal B_CEn  : std_logic_vector(7 downto 3);
  signal B_ALEn : std_logic;

  signal G_OEn_LO : std_logic;
  signal G_OEn_HI : std_logic;
  signal G_DIR    : std_logic;
  signal G_CSn    : std_logic;
  signal G_OEn    : std_logic;

  signal MS_CLK : std_logic;
  signal MS_DAT : std_logic;
  signal KB_CLK : std_logic;
  signal KB_DAT : std_logic;

  signal P_RX : std_logic;
  signal P_TX : std_logic;

  signal Y_wire : std_logic_vector(7 downto 0);

begin

  -- free running clock
  GCLK <= not GCLK after 1 ps * (tGCLK_ps / 2);

  -- use 24 bits address (A0 is not exposed by the 68k) to help readability
  CPU_A(0) <= '0';
  
  U4_glue : entity work.glue
   port map (
      RSTn => RSTn,
      GCLK => GCLK,
    
      CPU_VPAn   => CPU_VPAn,
      CPU_A      => CPU_A(23 downto 1),
      CPU_ASn    => CPU_ASn,
      CPU_LDSn   => CPU_LDSn,
      CPU_UDSn   => CPU_UDSn,
      CPU_RWn    => CPU_RWn,
      CPU_FC2    => CPU_FC(2),
      CPU_BERRn  => CPU_BERRn,
      CPU_D      => CPU_D(15 downto 8),
      CPU_DTACKn => CPU_DTACKn,

      M_A     => M_A,
      M_CASLn => M_CASLn,
      M_CASUn => M_CASUn,
      M_RASn  => M_RASn,
      M_WEn   => M_WEn,
    
      B_WEn  => B_WEn,
      B_ACKn => B_ACKn,
      B_A0   => B_A0,
      
      G_OEn_LO => G_OEn_LO,
      G_OEn_HI => G_OEn_HI,
      G_CSn    => G_CSn,
      G_DIR    => G_DIR,
      G_OEn    => G_OEn
      );

  U3_74ls245 : entity work.sl74245
    generic map (
      tPD => 4.2 ns,
      tPZ => 5 ns
      )
    port map (
      OEn => G_OEn,
      TRn => G_DIR,
      A   => CPU_D(15 downto 8),
      B   => CPU_D(7 downto 0)
      );

  CPU_CLK <= GCLK;
  
  U1_mc68k : entity work.mc68k
    port map (
      CLK  => CPU_CLK,
      IPLn => CPU_IPLn,

      BGACKn => '1',
      BGn    => open,
      BRn    => '1',

      FC => CPU_FC,

      VMAn => open,
      E    => open,

      BERRn  => CPU_BERRn,
      DTACKn => CPU_DTACKn,
      VPAn   => CPU_VPAn,
      HALTn  => RSTn,
      RESETn => RSTn,

      A => CPU_A(23 downto 1),
      D => CPU_D,
    
      ASn  => CPU_ASn,
      UDSn => CPU_UDSn,
      LDSn => CPU_LDSn,
      RWn  => CPU_RWn
      );

  U5_mfp : entity work.mfp
    generic map (
      FAST_START => FAST_START
      )
    port map (
      RSTn => RSTn,
      IRQn => IRQn,
    
      B_D    => B_AD,
      B_CEn  => B_CEn(7),
      B_ACKn => B_ACKn,
      B_WEn  => B_WEn,
      B_A0   => B_A0,
      
      MS_CLK => MS_CLK,
      MS_DAT => MS_DAT,
      
      KB_CLK => KB_CLK,
      KB_DAT => KB_DAT,
      
      P_RX => P_RX,
      P_TX => P_TX,
      
      BSTP => '0'
      );

  -- pull-ups
  MS_CLK <= 'H';
  MS_DAT <= 'H';
  KB_CLK <= 'H';
  KB_DAT <= 'H';
  RSTn   <= 'H';
  IRQn   <= 'H';

  B_ACKn   <= 'H';
  B_ALEn   <= 'H';
  G_OEn_HI <= 'H';
  G_OEn_LO <= 'H';

  G_OEn    <= 'H';

  U6_74ls245 : entity work.sl74245
    generic map (
      tPD => 4 ns,
      tPZ => 5 ns
      )
    port map (
      OEn => G_OEn_LO,
      TRn => G_DIR,
      A   => CPU_D(7 downto 0),
      B   => B_AD
      );

  U7_74ls245 : entity work.sl74245
    generic map (
      tPD => 4 ns,
      tPZ => 5 ns
      )
    port map (
      OEn => G_OEn_HI,
      TRn => '1',  -- oups !
      A   => M_A(7 downto 0),
      B   => B_AD
      );

  U2_74f138 : entity work.sl74138
    generic map (
      tPD_D => 5 ns
      )
    port map (
      A    => M_A(10),
      B    => M_A(9),
      C    => M_A(8),
      G2An => G_CSn,
      G2Bn => G_CSn,
      G1   => '1',
      Y    => Y_wire
      );

  B_CEn(7) <= Y_wire(0);
  B_CEn(6) <= Y_wire(1);
  B_CEn(5) <= Y_wire(2);
  B_CEn(4) <= Y_wire(3);
  B_CEn(3) <= Y_wire(4);

  B_ALEn <= Y_wire(6);

  J3_DRAM_lo : entity work.fpdram
    generic map (
      ROOT_NAME => "./output/fpdram/mif_lo_",
      VERBOSE   => false,  --true,
      VERBOSE2  => false,

      tRP => 50 ns  -- 70 ns chip or better
      )
    port map (
      ADDR => M_A,
      RWn  => M_WEn,
      RASn => M_RASn,
      CASn => M_CASLn,
      DQ   => CPU_D(7 downto 0)
    );
  
  J2_DRAM_hi : entity work.fpdram
    generic map (
      ROOT_NAME => "./output/fpdram/mif_hi_",
      VERBOSE   => false,  --true,
      VERBOSE2  => false,

      tRP => 50 ns  -- 70 ns chip or better
      )
    port map (
      ADDR => M_A,
      RWn  => M_WEn,
      RASn => M_RASn,
      CASn => M_CASUn,
      DQ   => CPU_D(15 downto 8)
      );

  -- AV board
  av_board_gen: if true generate

    av_board : entity work.b68_av
      generic map (
        FLEX_GATE => FLEX_GATE
        )
      port map (
        -- local bus
        B_CEn  => B_CEn(6),
        B_WEn  => B_WEn,
        IRQn   => IRQn,
        RSTn   => RSTn,
        B_ALEn => B_ALEn,
        B_ACKn => B_ACKn,
        B_A0   => B_A0,
        B_AD   => B_AD
        );

 end generate av_board_gen;
 
end architecture rtl;
