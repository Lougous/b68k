--
-- https://github.com/Lougous/b68k
--
-- b68k-cpu MFP model
--

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity mfp is

  generic (
    FAST_START : boolean := false
    );
  port (

    RSTn : inout std_logic;
    IRQn : inout std_logic;
    
    B_D    : inout std_logic_vector(7 downto 0);
    B_CEn  : in    std_logic;
    B_ACKn : inout std_logic;
    B_WEn  : in    std_logic;
    B_A0   : in    std_logic;
--    B_RDYn : in  std_logic;

    MS_CLK : inout std_logic;
    MS_DAT : inout std_logic;

    KB_CLK : inout std_logic;
    KB_DAT : inout std_logic;

    P_RX : in    std_logic;
    P_TX : out   std_logic;

    BSTP : in std_logic
    );

end entity mfp;

architecture rtl of mfp is

  --constant tC  : time := 62500 ps;
  constant tC  : time := 63000 ps;

  signal data_out : std_logic_vector(7 downto 0);
  signal data_oe  : std_logic := '0';
  
begin  -- architecture rtl

  RSTn <= '0', 'Z' after 1 us;

  B_D <= data_out when data_oe = '1' else (others => 'Z');

  -- stub unused interfaces
  MS_CLK <= 'Z';
  MS_DAT <= 'Z';

  KB_CLK <= 'Z';
  KB_DAT <= 'Z';

  P_TX <= '1';

  
  process is

    variable cnt : unsigned(7 downto 0) := (others => '0');
    
    variable address : std_logic_vector(7 downto 0) := (others => '0');

    variable POST : std_logic_vector(7 downto 0) := (others => '0');

    variable bootstrap_size : integer := 512;

    procedure READ_ACCESS (
      rdata : in std_logic_vector(7 downto 0);
      extra : boolean := false) is
    begin
      wait for tC;
      data_out <= rdata;

      wait for tC;
      data_oe <= '1';

      wait for tC;
      B_ACKn <= '0';

      wait for tC;
      wait for tC;
      
      while B_CEn = '0' loop
        wait for tC;
        wait for tC;
      end loop;

--      if extra then
--        -- special bootstrap extra data bus hold
--        wait for 1 us;
--      end if;
      
      wait for tC;
      data_oe <= '0';

      wait for tC;
      B_ACKn <= 'Z';

    end procedure READ_ACCESS;
    
    procedure WRITE_ACCESS is
    begin
      wait for tC;
      B_ACKn <= '0';

      wait for tC;
      wait for tC;
      
      while B_CEn = '0' loop
        wait for tC;
        wait for tC;
      end loop;
      
      wait for tC;
      data_oe <= '0';

      wait for tC;
      B_ACKn <= 'Z';

    end procedure WRITE_ACCESS;
    
  begin
    
    B_D <= (others => 'Z');
    B_ACKn <= 'Z';
    IRQn <= 'Z';
    RSTn <= '0';

    wait for 1 us;  -- actual time 500 ms + 200 ms

    RSTn <= 'Z';

    wait for 1 us;  -- actual time 200 ms

    -- bootstrap
    if FAST_START then
      bootstrap_size := 16;
    end if;

    if true then
--    for byte in 0 to 1023 loop
      for byte in 0 to bootstrap_size-1 loop
        wait for tC;
        if B_CEn = '0' then
          READ_ACCESS(std_logic_vector(to_unsigned(byte mod 256, 8)), true);
          wait for 1 us;
        else
          report "B_CEn expected to be asserted here" severity failure;
        end if;
      end loop;

--    report "1kiB boot sector loaded" severity note;
      report integer'image(bootstrap_size) & "B boot sector loaded" severity note;
    end if;
    
    while true loop
      
      while B_CEn /= '0' loop
        wait for tC;
        wait for tC;
      end loop;
      
      wait for tC;
      wait for tC;

      if B_A0 = '0' then
        -- data
        wait for tC;
        wait for tC;

        case address is
          when x"00" =>
            -- flash data
            wait for tC;
            wait for tC;

            if B_WEn = '1' then
              READ_ACCESS(x"CD");
            else
              WRITE_ACCESS;
            end if;

          when x"04" =>
            -- UART status
            wait for tC;
            wait for tC;

            if B_WEn = '1' then
              READ_ACCESS(x"01");  -- TX ready TODO
            else
              WRITE_ACCESS;
            end if;

          when x"05" =>
            -- UART data
            wait for tC;
            wait for tC;

            if B_WEn = '1' then
              READ_ACCESS(x"CD");  -- RX data TODO
            else
              wait for tC;
              -- send B_D TODO
              WRITE_ACCESS;
            end if;

          when x"07" =>
            -- POST
            wait for tC;
            wait for tC;

            if B_WEn = '1' then
              READ_ACCESS(POST);
            else
              wait for tC;
              report "MPF POST=" & integer'image(to_integer(unsigned(B_D))) severity note;
              POST := B_D;
              WRITE_ACCESS;
            end if;

          when others =>
            report "MPF access to unmapped address " & integer'image(to_integer(unsigned(address))) severity warning;
            wait for tC;
            wait for tC;

            if B_WEn = '1' then
              READ_ACCESS(x"CD");  -- RX data TODO
            else
              wait for tC;
              -- send B_D TODO
              WRITE_ACCESS;
            end if;

        end case;
      else
        -- address
        wait for tC;
        wait for tC;

        if B_WEn = '1' then
          READ_ACCESS(address);
        else
          wait for tC;
          address := B_D;
          report "MFP: address set to " & integer'image(to_integer(unsigned(address))) severity note;
          WRITE_ACCESS;
        end if;
      end if;
      
    end loop;
  end process;
  
end architecture rtl;
