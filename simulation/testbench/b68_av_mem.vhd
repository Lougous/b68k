--
-- https://github.com/Lougous/b68k
--
-- b68k-av memory module model
--

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity b68_av_mem is

  generic (
    CONF_HI_LOn : std_logic
    );
  port (
    -- local bus
    OEn  : in    std_logic;
    WEn  : in    std_logic;
    SEL  : in    std_logic_vector(2 downto 0);
    A    : in    std_logic_vector(16 downto 0);
    D    : inout std_logic_vector(15 downto 0)
    );

end entity b68_av_mem;

architecture rtl of b68_av_mem is

  signal w_LCEn : std_logic;
  signal w_UCEn : std_logic;
  
begin

  w_LCEn <= '0' when CONF_HI_LOn = '0' and SEL(0) = '0' and SEL(2) = '1' else  -- LO
            '0' when CONF_HI_LOn = '1' and SEL(2) = '0' and SEL(0) = '1' else  -- HI
            '1';
  w_UCEn <= '0' when CONF_HI_LOn = '0' and SEL(0) = '0' and SEL(1) = '1' else  -- LO
            '0' when CONF_HI_LOn = '1' and SEL(1) = '0' and SEL(0) = '1' else  -- HI
            '1';
  
  U1_SRAM : entity work.sram128kx8
    generic map (
      tOHZmin => 5 ns,
      tOHZmax => 10 ns
      )
    port map (
      A   => A,
      CSn => w_LCEn,
      WEn => WEn,
      OEn => OEn,
      IO  => D(7 downto 0)
      );
  
  U2_SRAM : entity work.sram128kx8
    generic map (
      tOHZmin => 5 ns,
      tOHZmax => 10 ns
      )
    port map (
      A   => A,
      CSn => w_UCEn,
      WEn => WEn,
      OEn => OEn,
      IO  => D(15 downto 8)
      );

end architecture rtl;
