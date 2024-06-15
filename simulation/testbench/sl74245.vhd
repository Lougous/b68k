
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sl74245 is

  generic (
    tPD : time := 4.2 ns;
    tPZ : time := 5 ns
    );
  port (
    OEn : in    std_logic;
    TRn : in    std_logic;  -- 1: A -> B
    A   : inout std_logic_vector(7 downto 0) := (others => 'Z');
    B   : inout std_logic_vector(7 downto 0) := (others => 'Z')
    );

end entity sl74245;

architecture behave of sl74245 is

  signal ena_a : std_logic;
  signal ena_b : std_logic;
  
  signal a_delayed : std_logic_vector(7 downto 0);
  signal b_delayed : std_logic_vector(7 downto 0);
  
begin

  ena_a <= TRn and not OEn after tPZ;
  ena_b <= (not TRn) and not OEn after tPZ;

  a_delayed <= A after tPD;
  b_delayed <= B after tPD;
  
  B <= a_delayed when ena_a = '1' else (others => 'Z');
  A <= b_delayed when ena_b = '1' else (others => 'Z');
  
end architecture behave;
