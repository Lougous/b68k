--
-- https://github.com/Lougous/b68k
--
-- RAMDAC model
--

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity ramdac is

  port (
    -- local bus
    WRn    : in    std_logic;
    RDn    : in    std_logic;
    RS     : in    std_logic_vector(1 downto 0);
    D      : inout std_logic_vector(7 downto 0);
    
    -- video in
    BLANKn : in    std_logic;
    P      : in    std_logic_vector(7 downto 0);
    PCLK   : in    std_logic;
    
    -- video out
    R      : out   std_logic_vector(7 downto 0);
    G      : out   std_logic_vector(7 downto 0);
    B      : out   std_logic_vector(7 downto 0)
    );

end entity ramdac;

architecture rtl of ramdac is

  constant tAS    : time := 15 ns;
  constant tAH    : time := 20 ns;  -- check TODO
  constant tWP_lo : time := 50 ns;
  constant tWP_hi : time := 4*28 ns;
  constant tWDS   : time := 15 ns;
  constant tWDH   : time := 15 ns;
  constant tRP_lo : time := 50 ns;
  constant tRP_hi : time := 4*28 ns;
  constant tROE   : time := 5 ns;
  constant tRACC  : time := 40 ns;
  constant tRDH   : time := 20 ns;

  type LUT_t is array (0 to 255) of std_logic_vector(23 downto 0);
  signal LUT : LUT_t := (others => (others => '0'));

  signal reg_ad_w    : unsigned(7 downto 0) := (others => '0');
  signal reg_ad_wsel : integer range 0 to 2 := 0;
  signal reg_ad_r    : unsigned(7 downto 0) := (others => '0');
  signal reg_ad_rsel : integer range 0 to 2 := 0;

begin

  -- local bus
  timing_check: process is
    variable t_fall_wrn : time := 0 ns;
    variable t_fall_rdn : time := 0 ns;
    variable t_rise_wrn : time := 0 ns;
    variable t_rise_rdn : time := 0 ns;

    variable rs_latch : std_logic_vector(1 downto 0);
    
  begin

    -- prevent unexpected timing violation at t=0
    wait for 1 ns;

    while true loop
      
      wait until WRn'event or RDn'event;

      if falling_edge(WRn) then
        assert (now - t_rise_wrn) >= tWP_hi report "RAMDAC: tWP_hi violation" severity error;
        assert RS'stable(tAS) report "RAMDAC: tAS violation" severity error;
        t_fall_wrn := now;
        rs_latch := RS;
      end if;
      
      if falling_edge(RDn) then
        assert (now - t_rise_rdn) >= tRP_hi report "RAMDAC: tWP_hi violation" severity error;
        assert RS'stable(tAS) report "RAMDAC: tAS violation" severity error;
        t_fall_rdn := now;
        rs_latch := RS;
      end if;
      
      if rising_edge(WRn) then
        -- write access
        t_rise_wrn := now;
        assert (now - t_fall_wrn) >= tWP_lo report "RAMDAC: tWP_lo violation" severity error;
        assert RDn = '1' and RDn'stable(tWP_lo) report "RAMDAC: read/write conflict" severity error;
        assert D'stable(tWDS) report "RAMDAC: tWDS violation" severity error;

        case rs_latch is
          when "00" =>
            -- Address Register (RAM Write Mode)
            reg_ad_w    <= unsigned(D);
            reg_ad_wsel <= 0;
          when "11" =>
            -- Address Register (RAM Read Mode)
            reg_ad_r    <= unsigned(D);
            reg_ad_rsel <= 0;
          when "01" =>
            -- Color Palette RAM
            if reg_ad_wsel = 0 then
              LUT(to_integer(reg_ad_w))(23 downto 16) <= D;
              reg_ad_wsel <= 1;
            elsif reg_ad_wsel = 1 then
              LUT(to_integer(reg_ad_w))(15 downto 8) <= D;
              reg_ad_wsel <= 2;
            else
              LUT(to_integer(reg_ad_w))(7 downto 0) <= D;
              reg_ad_wsel <= 0;
              reg_ad_w    <= reg_ad_w + 1;
            end if;
            
          when others =>
            -- "10" Pixel Read Mask Register
            null;
        end case;

        wait for tWDH;
        assert D'stable(tWDH+tWDS) report "RAMDAC: tWDH violation" severity error;
        
      elsif rising_edge(RDn) then
        -- read
        t_rise_rdn := now;
        assert (now - t_fall_rdn) >= tRP_lo report "RAMDAC: tRP_lo violation" severity error;

        if reg_ad_rsel = 0 then
          reg_ad_rsel <= 1;
        elsif reg_ad_rsel = 1 then
          reg_ad_rsel <= 2;
        else
          reg_ad_rsel <= 0;
          reg_ad_r    <= reg_ad_r + 1;
        end if;
        
      end if;
    end loop;
    
  end process timing_check;
  
  data_read: process is
    variable rd : std_logic_vector(7 downto 0);
    
  begin
    D <= (others => 'Z');

    while true loop
      wait until falling_edge(RDn);

      if reg_ad_rsel = 0 then
        rd := LUT(to_integer(reg_ad_r))(23 downto 16);
      elsif reg_ad_rsel = 1 then
        rd := LUT(to_integer(reg_ad_r))(15 downto 8);
      else
        rd := LUT(to_integer(reg_ad_r))(7 downto 0);
      end if;
      
      D <= (others => 'U') after tROE, rd after tRACC;

      wait until rising_edge(RDn);
      D <= (others => 'Z') after tRDH;        
    end loop;

  end process data_read;


  LUT_out : process is

    variable index : integer range 0 to 255 := 0;
    variable blkn : std_logic := '0';
    
  begin

    wait until rising_edge(PCLK);

    if blkn = '0' then
      R <= (others => '0') after 8 ns;
      G <= (others => '0') after 8 ns;
      B <= (others => '0') after 8 ns;
    else
      R <= LUT(index)(23 downto 16) after 8 ns;
      G <= LUT(index)(15 downto 8)  after 8 ns;
      B <= LUT(index)(7 downto 0)   after 8 ns;        
    end if;

  end process LUT_out;

end architecture rtl;
