--
-- https://github.com/Lougous/b68k
--
-- OPL2 model
--

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity opl2 is

  port (
    -- local bus
    CSn  : in    std_logic;
    WRn  : in    std_logic;
    RDn  : in    std_logic;
    A0   : in    std_logic;
    D    : inout std_logic_vector(7 downto 0);
    
    --
    phyM : in  std_logic;
    ICn  : in  std_logic;
    MO   : out std_logic;
    SH   : out std_logic
    );

end entity opl2;

architecture rtl of opl2 is

  constant tAS  : time := 10 ns;
  constant tAH  : time := 20 ns;
  constant tCSW : time := 100 ns;
  constant tWW  : time := 100 ns;
  constant tWDS : time := 20 ns;
  constant tWDH : time := 30 ns;
  constant tCSR : time := 200 ns;
  constant tRW  : time := 200 ns;
  constant tACC : time := 200 ns;
  constant tRDH : time := 10 ns;
  

  signal clkdiv  : unsigned(1 downto 0);
  signal clkint  : std_logic;
  
begin

  timing_check: process is
    variable t_fall_csn : time;
    variable t_fall_wrn : time;
    variable t_fall_rdn : time;
    
  begin
    wait until CSn'event or WRn'event or RDn'event;

    if falling_edge(CSn) then
      assert A0'stable(tAS) report "OPL2: tAS violation" severity error;
      t_fall_csn := now;
    end if;

    if falling_edge(WRn) then
      t_fall_wrn := now;
    end if;
   
    if falling_edge(RDn) then
      t_fall_rdn := now;
    end if;
   
    if
      (rising_edge(CSn) and rising_edge(WRn)) or
      (rising_edge(CSn) and WRn = '0') or
      (rising_edge(WRn) and CSn = '0')
    then
      -- write access
      assert (now - t_fall_csn) >= tCSW report "OPL2: tCSW violation" severity error;
      assert RDn = '1' and RDn'stable(tCSW) report "OPL2: read/write conflict" severity error;
      assert (now - t_fall_wrn) >= tWW report "OPL2: tWW violation" severity error;
      assert D'stable(tWDS) report "OPL2: tWDS violation" severity error;
      assert A0'stable(tCSW + tAS) report "OPL2: A0 changes during access" severity error;

      -- assume tWDH > TAH
      wait for tAH;
      assert A0'stable(tAH + tCSW) report "OPL2: tAH violation" severity error;
      wait for tWDH - tAH;
      assert D'stable(tWDH) report "OPL2: tWDH violation" severity error;
      
    elsif
      (rising_edge(CSn) and rising_edge(RDn)) or
      (rising_edge(CSn) and RDn = '0') or
      (rising_edge(RDn) and CSn = '0')
    then
      -- read
      assert (now - t_fall_csn) >= tCSR report "OPL2: tCSR violation" severity error;
      assert (now - t_fall_rdn) >= tRW report "OPL2: tRW violation" severity error;
      assert A0'stable(tCSR + tAS) report "OPL2: A0 changes during access" severity error;
      
      wait for tAH;
      assert A0'stable(tAH + tCSR) report "OPL2: tAH violation" severity error;
      
    end if;
    
  end process timing_check;
  
  data_read: process is
  begin
    D <= (others => 'Z');

    while true loop
      wait until falling_edge(CSn) or falling_edge(RDn);
      if CSn = '0' and RDn = '0' then
        D <= (others => 'Z'), x"CD" after tACC;

        wait until rising_edge(CSn) or rising_edge(RDn);
        D <= (others => 'Z') after tRDH;        
      end if;      
    end loop;

  end process data_read;

  opl2_play_p : process is
    procedure PLAY_FP (
      D : in std_logic_vector(0 to 9);
      S : in std_logic_vector(0 to 2)
      ) is
    begin
      for i in 0 to 9 loop
        MO <= D(i);
        if i = 5 then
          SH <= '1';
        end if;

        wait until rising_edge(clkint);
      end loop;

      for i in 0 to 2 loop
        MO <= S(i);

        wait until rising_edge(clkint);
      end loop;

      MO <= '0';
      SH <= '0';

      -- stub
      wait until rising_edge(clkint);
      wait until rising_edge(clkint);

      -- don't care x3
      wait until rising_edge(clkint);
      wait until rising_edge(clkint);
      wait until rising_edge(clkint);
    end procedure;

  begin
    SH <= '0';
    MO <= '0';
    
    wait until rising_edge(ICn);

    wait for 50 us;
    wait until rising_edge(clkint);
    
    while true loop
      PLAY_FP("1000000011", "111");
      PLAY_FP("1000000011", "011");
      PLAY_FP("1000000011", "101");
      PLAY_FP("1000000011", "001");
      PLAY_FP("1000000011", "110");
      PLAY_FP("1000000011", "010");
      PLAY_FP("1000000011", "100");
      PLAY_FP("1000000101", "100");
      PLAY_FP("1000000100", "100");
      PLAY_FP("1000000010", "100");
      PLAY_FP("1000000100", "010");
      PLAY_FP("1000000100", "110");
      PLAY_FP("1000000100", "001");
      PLAY_FP("1000000100", "101");
      PLAY_FP("1000000100", "011");
      PLAY_FP("1000000100", "111");

      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
      PLAY_FP("0000000001", "100");
    end loop;

  end process;
    
        
  clkint <= clkdiv(1);

  process (phyM, ICn) is
  begin  -- process
    if ICn = '0' then                   -- asynchronous reset (active low)
      clkdiv <= (others => '0');
      
    elsif phyM'event and phyM = '1' then  -- rising clock edge

      clkdiv <= clkdiv + 1;
    end if;
  end process;
        
  
end architecture rtl;
