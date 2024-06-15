
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sl74373 is

  generic (
    tPD_D : time := 5 ns;
    tPD_LE : time := 9 ns;
    tPZ : time := 5 ns
    );
  port (
    OEn : in    std_logic;
    LE  : in    std_logic;
    D   : in    std_logic_vector(7 downto 0);
    O   : inout std_logic_vector(7 downto 0) := (others => 'Z')
    );

end entity sl74373;

architecture behave of sl74373 is

  signal ena : std_logic := '0';
  
  signal latched : std_logic_vector(7 downto 0);
  signal delayed : std_logic_vector(7 downto 0);
  
begin

  ena <= not OEn after tPZ;

  latched <= D when LE = '1';
  delayed <= latched after tPD_D;
  
  O <= delayed when ena = '1' else (others => 'Z');
  
end architecture behave;
