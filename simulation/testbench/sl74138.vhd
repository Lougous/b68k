
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sl74138 is

  generic (
    tPD_D : time := 4 ns
    );
  port (
    A    : in  std_logic;
    B    : in  std_logic;
    C    : in  std_logic;
    G2An : in  std_logic;
    G2Bn : in  std_logic;
    G1   : in  std_logic;
    Y    : out std_logic_vector(7 downto 0)
    );

end entity sl74138;

architecture behave of sl74138 is

  signal cs  : std_logic;
  signal sel : std_logic_vector(7 downto 0);
  
begin

  cs <= (not G2An) and (not G2Bn) and G1;
  
  sel(0) <= '0' when A = '0' and B = '0' and C = '0' and cs = '1' else '1';
  sel(1) <= '0' when A = '1' and B = '0' and C = '0' and cs = '1' else '1';
  sel(2) <= '0' when A = '0' and B = '1' and C = '0' and cs = '1' else '1';
  sel(3) <= '0' when A = '1' and B = '1' and C = '0' and cs = '1' else '1';
  sel(4) <= '0' when A = '0' and B = '0' and C = '1' and cs = '1' else '1';
  sel(5) <= '0' when A = '1' and B = '0' and C = '1' and cs = '1' else '1';
  sel(6) <= '0' when A = '0' and B = '1' and C = '1' and cs = '1' else '1';
  sel(7) <= '0' when A = '1' and B = '1' and C = '1' and cs = '1' else '1';
    
  Y <= sel after tPD_D;
  
end architecture behave;
