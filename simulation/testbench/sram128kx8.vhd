
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sram128kx8 is

  generic (
    tAA  : time := 12 ns;
    tOH  : time := 3 ns;
    tOE  : time := 6 ns;
    tOLZ : time := 0 ns;
    tOHZmin : time := 0 ns;
    tOHZmax : time := 6 ns;
    tCHZmin : time := 0 ns;
    tCHZmax : time := 6 ns;
    tWP : time := 8 ns;
    tAW : time := 9 ns;
    tDW : time := 6 ns;
    tCW : time := 9 ns;
    tAS : time := 0 ns
    );
  port (
    A   : in    std_logic_vector(16 downto 0);
    CSn : in    std_logic;
    WEn : in    std_logic;
    OEn : in    std_logic;
    IO  : inout std_logic_vector(7 downto 0)
    );

end entity sram128kx8;

architecture behave of sram128kx8 is

  type ram_array_type is array (0 to 2**17-1) of integer range 0 to 255;

  signal ram : ram_array_type := (others => 0);

  signal data_read : std_logic_vector(7 downto 0);
  signal data_bus  : std_logic_vector(7 downto 0);
  signal out_buf_open   : std_logic;
  signal out_buf_driven : std_logic;

  signal ad_int : integer range 0 to 2**17-1;

  signal WEn_fall_time : time := 0 ns;
  
begin

  ad_int <= to_integer(unsigned(A));

  data_read <= std_logic_vector(to_unsigned(ram(ad_int), 8));
  
  data_bus <= (others => 'U') after tOH, data_read after tAA when CSn = '0'
              else
              (others => 'U') after tCHZmin, (others => 'Z') after tCHZmax;

  out_buf_driven <= '1' after tOE when OEn = '0' else '0' after tOHZmin;
  out_buf_open <= '0' after tOLZ  when OEn = '0' else '1' after tOHZmax;

  IO <= data_bus when out_buf_driven = '1' else (others => 'U') when out_buf_open = '0' else (others => 'Z');

  
  WEn_fall_time_proc: process
  is
  begin
    wait until falling_edge(WEn);
    WEn_fall_time <= now;
  end process WEn_fall_time_proc;

  
  write_proc: process
  is
  begin
    if now = 0 ps then
      -- init
      for i in 0 to 2**17-1 loop
        ram(i) <= i mod 256;
      end loop;
    end if;
    
    wait until rising_edge(WEn) or rising_edge(CSn);

    if CSn = '0' then
      assert (now - WEn_fall_time) >= tWP report "tWP violation" severity warning;

      if not A'stable(tAW) then
        report "tAW violation" severity warning;
      end if;

      --if (A'stable - WEn'stable) < tAS then
      --  report "tAS violation" severity warning;
      --end if;

      if not CSn'stable(tAW) then
        report "tAW (CSn) violation" severity warning;
      end if;

      if not IO'stable(tDW) then
        report "tDW violation" severity warning;
      end if;

      ram(to_integer(unsigned(A))) <= to_integer(unsigned(IO));
    end if;
      
    if WEn = '0' then
      if CSn = '0' and not CSn'stable(tCW) then
        report "tCW violation" severity warning;
      end if;

      if not A'stable(tAW) then
        report "tAW violation" severity warning;
      end if;

      --if (A'stable - CSn'stable) < tAS then
      --  report "tAS violation" severity warning;
      --end if;

      if not WEn'stable(tAW) then
        report "tAW (WEn) violation" severity warning;
      end if;

      if not IO'stable(tDW) then
        report "tDW violation" severity warning;
      end if;

      ram(to_integer(unsigned(A))) <= to_integer(unsigned(IO));
    end if;

  end process write_proc;
  
end architecture behave;
