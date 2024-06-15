--
-- https://github.com/Lougous/b68k
--
-- MC68000 processor memory model
--

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity mc68k is

  generic (
    t6      : time := 25 ns;
    t6A     : time := 25 ns;
    t7      : time := 42 ns;
    t8      : time := 0 ns;
    t9_min  : time := 3 ns;
    t9_max  : time := 25 ns;
    t12_min : time := 3 ns;
    t12_max : time := 25 ns;
    t18_min : time := 0 ns;
    t18_max : time := 25 ns;
    t20_min : time := 1 ns;
    t20_max : time := 25 ns;
    t23     : time := 25 ns;
    t27     : time := 5 ns;
    t47     : time := 5 ns;
    t53     : time := 0 ns
    );

  port (
    CLK  : in std_logic;
    IPLn : in std_logic_vector(2 downto 0);

    BGACKn : in  std_logic;
    BGn    : out std_logic;
    BRn    : in  std_logic;

    FC : out std_logic_vector(2 downto 0);

    VMAn : out std_logic;
    E    : out std_logic;
    VPAn : in  std_logic;

    BERRn  : inout std_logic;
    DTACKn : in    std_logic;
    HALTn  : inout std_logic;
    RESETn : inout std_logic;

    A : out   std_logic_vector(23 downto 1);
    D : inout std_logic_vector(15 downto 0);

    ASn  : out std_logic;
    UDSn : out std_logic;
    LDSn : out std_logic;
    RWn  : out std_logic
    );

end entity mc68k;

architecture rtl of mc68k is

begin
     
  process is

    variable read_data : std_logic_vector(15 downto 0);

    type access_type_t is (C_BYTE, C_WORD);
    
    procedure CREAD (
      address : in std_logic_vector(23 downto 0);
      fcode : in std_logic_vector(2 downto 0);
      size  : in access_type_t;
      data  : out std_logic_vector(15 downto 0);
      cval  : in std_logic_vector(15 downto 0) := (others => 'X')
      ) is
      variable acked : boolean := false;
      variable lds_n : std_logic;
      variable uds_n : std_logic;

      constant C_Z16 : std_logic_vector(15 downto 0) := (others => 'Z');
      constant C_X16 : std_logic_vector(15 downto 0) := (others => 'X');

    begin

      case size is
        when C_BYTE =>
          uds_n := address(0);
          lds_n := not address(0);
        when C_WORD =>
          assert address(0) = '0' report "unaligned access" severity failure;
          uds_n := '0';
          lds_n := '0';
      end case;
      
      -- S0
      FC  <= "UUU" after t8, fcode after t6A;
      A   <= (others => 'U') after t8, (others => 'Z') after t7;
      RWn <= 'U' after t18_min, '1' after t18_max;
      wait until falling_edge(CLK);
      
      -- S1
      A  <= address(23 downto 1) after t6;
      wait until rising_edge(CLK);

      -- S2
      ASn <= 'U' after t9_min, '0' after t9_max;

      
      LDSn <= 'U' after t9_min, lds_n after t9_max;
      UDSn <= 'U' after t9_min, uds_n after t9_max;
      wait until falling_edge(CLK);

      while not acked loop
        -- S3
        wait until rising_edge(CLK);
        
        -- S4
        wait until falling_edge(CLK);

        if
          (DTACKn = '0' and DTACKn'stable(t47)) or
          (BERRn = '0' and BERRn'stable(t47)) or
          (VPAn = '0' and VPAn'stable(t47))
        then
          acked := true;
        end if;
      end loop;
 
      -- S5
      wait until rising_edge(CLK);
        
      -- S6
      wait until falling_edge(CLK);

      assert D'stable(t27) report "read data setup violation" severity error;
      assert D /= C_Z16 report "read data is Z" severity error;
      assert D /= C_X16 report "read data is X" severity error;

      -- get data here
      if lds_n = '0' then
        if cval(7 downto 0) /= "XXXXXXXX" then
          assert D(7 downto 0) = cval(7 downto 0) report "check failed" severity error;
        end if;
        
        data(7 downto 0) := D(7 downto 0);
      else
        data(7 downto 0) := (others => 'U');
      end if;
      
      if uds_n = '0' then
        if cval(15 downto 8) /= "XXXXXXXX" then
          assert D(15 downto 8) = cval(15 downto 8) report "check failed" severity error;
        end if;
        
        data(15 downto 8) := D(15 downto 8);
      else
        data(15 downto 8) := (others => 'U');
      end if;
            
      -- S7
      if VPAn = '0' then
        -- some cycles
        wait until falling_edge(CLK);
        wait until falling_edge(CLK);
        wait until falling_edge(CLK);
        wait until falling_edge(CLK);
        wait until falling_edge(CLK);
        wait until falling_edge(CLK);
      elsif BERRn = '0' then
        -- + 2 cycles
        wait until falling_edge(CLK);
        wait until falling_edge(CLK);
      end if;
      
      ASn <= 'U' after t12_min, '1' after t12_max;
      
      if lds_n = '0' then
        LDSn <= 'U' after t12_min, '1' after t12_max;
      end if;
      
      if uds_n = '0' then
        UDSn <= 'U' after t12_min, '1' after t12_max;
      end if;

      wait until rising_edge(CLK);
        
   end procedure CREAD;
    
    procedure CWRITE (
      address : in std_logic_vector(23 downto 0);
      fcode : in std_logic_vector(2 downto 0);
      size  : in access_type_t;
      value : in std_logic_vector(15 downto 0)
      ) is
      variable acked : boolean := false;
      variable lds_n : std_logic;
      variable uds_n : std_logic;
    begin
      case size is
        when C_BYTE =>
          uds_n := address(0);
          lds_n := not address(0);
        when C_WORD =>
          assert address(0) = '0' report "unaligned access" severity failure;
          uds_n := '0';
          lds_n := '0';
      end case;
      
      -- S0
      FC  <= "UUU" after t8, fcode after t6A;
      A   <= (others => 'U') after t8, (others => 'Z') after t7;
      RWn <= 'U' after t18_min, '1' after t18_max;
      wait until falling_edge(CLK);
      
      -- S1
      A  <= address(23 downto 1) after t6;
      wait until rising_edge(CLK);

      -- S2
      ASn <= 'U' after t9_min, '0' after t9_max;
      RWn <= 'U' after t20_min, '0' after t20_max;
      wait until falling_edge(CLK);

      while not acked loop
        -- S3
        D <= value after t23;
        wait until rising_edge(CLK);
        
        -- S4
        LDSn <= lds_n after t9_max;
        UDSn <= uds_n after t9_max;
        wait until falling_edge(CLK);

        if
          (DTACKn = '0' and DTACKn'stable(t47)) or
          (BERRn = '0' and BERRn'stable(t47)) or
          (VPAn = '0' and VPAn'stable(t47))
        then
          acked := true;
        end if;
      end loop;
 
      -- S5
      wait until rising_edge(CLK);
        
      -- S6
      wait until falling_edge(CLK);

      -- get data here
      
      -- S7
      ASn <= 'U' after t12_min, '1' after t12_max;

      if lds_n = '0' then
        LDSn <= 'U' after t12_min, '1' after t12_max;
      end if;
      
      if uds_n = '0' then
        UDSn <= 'U' after t12_min, '1' after t12_max;
      end if;

      wait until rising_edge(CLK);
      D <= (others => 'Z') after t53;
        
   end procedure CWRITE;

      
  begin
    ASn   <= '1';
    BGn   <= '1';
    FC    <= "111";
    VMAn  <= '1';
    E     <= '1';
    BERRn <= 'Z';
    A     <= (others => 'U');
    D     <= (others => 'Z');
    UDSn  <= '1';
    LDSn  <= '1';
    RWn   <= '1';

    HALTn  <= '0';
    RESETn <= '0';

    wait for 500 ns;
    
    HALTn  <= 'Z';
    RESETn <= 'Z';

    wait until RESETn /= '0';
    wait until rising_edge(CLK);
    report "68K: reset not asserted" severity note;

    report "68K: boot in RAM sequence" severity note;
    
    -- SSP
    CREAD(x"000000", "101", C_WORD, read_data);
    CREAD(x"000002", "101", C_WORD, read_data);
   
    -- PC
    CREAD(x"000004", "101", C_WORD, read_data);
    CREAD(x"000006", "101", C_WORD, read_data);

--    CWRITE(x"000000", "101", C_WORD, x"ABCD");
--    CWRITE(x"000002", "101", C_WORD, x"1234");
--    CREAD(x"000000", "101", C_WORD, read_data);
--    CREAD(x"000002", "101", C_WORD, read_data);

    CREAD(x"000100", "101", C_WORD, read_data);
    CREAD(x"000102", "101", C_WORD, read_data);
    CREAD(x"000104", "101", C_WORD, read_data);
    CREAD(x"000106", "101", C_WORD, read_data);
    CREAD(x"000108", "101", C_WORD, read_data);
    CREAD(x"00010A", "101", C_WORD, read_data);
    CREAD(x"00010C", "101", C_WORD, read_data);
    CREAD(x"00010E", "101", C_WORD, read_data);
    for i in 0 to 9 loop
    CREAD(x"000110", "101", C_WORD, read_data);
    CREAD(x"000112", "101", C_WORD, read_data);
    CREAD(x"000114", "101", C_WORD, read_data);
    CWRITE(std_logic_vector(to_unsigned(65536+i, 24)), "101", C_BYTE, x"0000");  -- clrb
    CREAD(x"000116", "101", C_WORD, read_data);
    end loop;  -- i
    CREAD(x"000110", "101", C_WORD, read_data);
    CREAD(x"000112", "101", C_WORD, read_data);
    CREAD(x"000118", "101", C_WORD, read_data);
    CREAD(x"00011A", "101", C_WORD, read_data);
    CREAD(x"00011C", "101", C_WORD, read_data);
    CREAD(x"00011e", "101", C_WORD, read_data);
    CREAD(x"000120", "101", C_WORD, read_data);
    CWRITE(x"010FFC", "101", C_WORD, x"0000");  -- jsr
    CWRITE(x"010FFE", "101", C_WORD, x"0122");  -- jsr
 
    CREAD(x"00012C", "101", C_WORD, read_data);
    CREAD(x"00012E", "101", C_WORD, read_data);
    CREAD(x"000130", "101", C_WORD, read_data);
    CREAD(x"000132", "101", C_WORD, read_data);
    CWRITE(x"F00001", "101", C_BYTE, x"0707");  -- move.b #7,0xfff00001
    CREAD(x"000134", "101", C_WORD, read_data);
    CREAD(x"000136", "101", C_WORD, read_data);
    CREAD(x"000138", "101", C_WORD, read_data);
    CREAD(x"00013A", "101", C_WORD, read_data);
    CWRITE(x"F00000", "101", C_BYTE, x"0101");  -- move.b #1,0xfff00000

    CREAD(x"000138", "101", C_WORD, read_data);
    CREAD(x"00013A", "101", C_WORD, read_data);
    CWRITE(x"F00001", "101", C_BYTE, x"0000");  -- move.b #0,0xfff00001
    for i in 512 to 600 loop
      CREAD(x"000138", "101", C_WORD, read_data);
      CREAD(x"00013A", "101", C_WORD, read_data);
      CREAD(x"F00000", "101", C_BYTE, read_data);  -- move.b 0xfff00000,
      CREAD(x"000138", "101", C_WORD, read_data);
      CREAD(x"00013A", "101", C_WORD, read_data);
      CWRITE(std_logic_vector(to_unsigned(i, 24)), "101", C_BYTE, x"cdcd");
      CREAD(x"000138", "101", C_WORD, read_data);
      CREAD(x"00013A", "101", C_WORD, read_data);
    end loop;  -- i
    -- IO board
--    report "68K: IO board" severity note;
--    CREAD(x"00013C", "101", C_WORD, read_data);
--    CWRITE(x"F40001", "101", C_BYTE, x"0101");
--    CWRITE(x"F40000", "101", C_BYTE, x"0101");

    -- interrupt
    report "68K: interrupt ack" severity note;
    CREAD(x"FFFFF0", "111", C_WORD, read_data);
   
    -- interrupt
    report "68K: bus error" severity note;
    CWRITE(x"FFFFF0", "101", C_WORD, read_data);
   
    -- MMU
    report "68K: MMU test" severity note;
    CWRITE(x"FE0000", "101", C_WORD, x"3000");
    CWRITE(x"FE0002", "101", C_WORD, x"0000");
    CWRITE(x"FE0004", "101", C_WORD, x"0100");
    CWRITE(x"FE0006", "101", C_WORD, x"0000");
    CREAD(x"000000", "001", C_WORD, read_data);
    CREAD(x"000002", "001", C_WORD, read_data);
    CWRITE(x"001000", "001", C_BYTE, x"0101");
    CWRITE(x"001001", "001", C_BYTE, x"0101");
    CWRITE(x"002000", "001", C_BYTE, x"0101");
    CWRITE(x"002001", "001", C_BYTE, x"0101");

    CWRITE(x"f90000", "001", C_BYTE, x"0101");
    CREAD(x"000000", "001", C_WORD, read_data);
    CWRITE(x"f90002", "001", C_BYTE, x"0101");
    CREAD(x"000100", "001", C_WORD, read_data);
   
    report "68K: memory range" severity note;
    CWRITE(x"000000", "101", C_WORD, x"0101");
    CWRITE(x"000080", "101", C_WORD, x"0202");
    CWRITE(x"000100", "101", C_WORD, x"0303");
    CWRITE(x"000200", "101", C_WORD, x"0404");
    CWRITE(x"000400", "101", C_WORD, x"0505");
    CWRITE(x"000800", "101", C_WORD, x"0606");
    CWRITE(x"001000", "101", C_WORD, x"0707");
    CWRITE(x"002000", "101", C_WORD, x"0808");
    CWRITE(x"004000", "101", C_WORD, x"0909");
    CWRITE(x"008000", "101", C_WORD, x"0A0A");
    CWRITE(x"010000", "101", C_WORD, x"0B0B");
    CWRITE(x"020000", "101", C_WORD, x"0C0C");
    CWRITE(x"040000", "101", C_WORD, x"0D0D");
    CWRITE(x"080000", "101", C_WORD, x"0E0E");
    CWRITE(x"100000", "101", C_WORD, x"0F0F");

    CREAD(x"000000", "101", C_WORD, read_data, x"0101");
    CREAD(x"000080", "101", C_WORD, read_data, x"0202");
    CREAD(x"000100", "101", C_WORD, read_data, x"0303");
    CREAD(x"000200", "101", C_WORD, read_data, x"0404");
    CREAD(x"000400", "101", C_WORD, read_data, x"0505");
    CREAD(x"000800", "101", C_WORD, read_data, x"0606");
    CREAD(x"001000", "101", C_WORD, read_data, x"0707");
    CREAD(x"002000", "101", C_WORD, read_data, x"0808");
    CREAD(x"004000", "101", C_WORD, read_data, x"0909");
    CREAD(x"008000", "101", C_WORD, read_data, x"0A0A");
    CREAD(x"010000", "101", C_WORD, read_data, x"0B0B");
    CREAD(x"020000", "101", C_WORD, read_data, x"0C0C");
    CREAD(x"040000", "101", C_WORD, read_data, x"0D0D");
    CREAD(x"080000", "101", C_WORD, read_data, x"0E0E");
    CREAD(x"100000", "101", C_WORD, read_data, x"0F0F");

    ----------------------------------------------------------------------------
    if false then
    report "68K: AV RAM issue" severity note;
    CWRITE(x"FE0000", "101", C_WORD, x"3000");
    CWRITE(x"FE0002", "101", C_WORD, x"0000");
    CWRITE(x"FE0004", "101", C_WORD, x"0100");
    CWRITE(x"FE0006", "101", C_WORD, x"0000");

    for i in 1 to 100 loop

      -- <gpu_draw_8>:
    CREAD(x"0001A2", "001", C_WORD, read_data);
    CWRITE(x"001000", "001", C_WORD, x"1234");
    CWRITE(x"001002", "001", C_WORD, x"5678");
    CREAD(x"0001A4", "001", C_WORD, read_data);
    CREAD(x"0001A6", "001", C_WORD, read_data);
    CWRITE(x"001004", "001", C_WORD, x"1234");
    CWRITE(x"001006", "001", C_WORD, x"5678");
    CREAD(x"0001A8", "001", C_WORD, read_data);
    CREAD(x"0001AA", "001", C_WORD, read_data);
    CWRITE(x"001008", "001", C_WORD, x"1234");
    CWRITE(x"00100A", "001", C_WORD, x"5678");
    CREAD(x"0001ac", "001", C_WORD, read_data);
    CREAD(x"0001ae", "001", C_WORD, read_data);
    CREAD(x"0001b0", "001", C_WORD, read_data);
    CREAD(x"0001b2", "001", C_WORD, read_data);
    CREAD(x"0001b4", "001", C_WORD, read_data);
    CREAD(x"0001b6", "001", C_WORD, read_data);
    CREAD(x"0001b8", "001", C_WORD, read_data);
    CWRITE(x"f9a100", "001", C_BYTE, x"5678");
    CREAD(x"0001ba", "001", C_WORD, read_data);
    CREAD(x"0001bc", "001", C_WORD, read_data);
    CREAD(x"0001be", "001", C_WORD, read_data);
    CWRITE(x"f9a101", "001", C_BYTE, x"5678");
    -- interrupt
    CREAD(x"000170", "101", C_WORD, read_data);
    CREAD(x"000172", "101", C_WORD, read_data);
    -- /interrupt
    CREAD(x"0001c0", "001", C_WORD, read_data);
    CREAD(x"0001c2", "001", C_WORD, read_data);
    CREAD(x"0001c4", "001", C_WORD, read_data);
    CREAD(x"0001c6", "001", C_WORD, read_data);
    CWRITE(x"f9a102", "001", C_BYTE, x"8080");
    CREAD(x"0001c8", "001", C_WORD, read_data);
    CREAD(x"0001ca", "001", C_WORD, read_data);
    CREAD(x"0001cc", "001", C_WORD, read_data);
    CREAD(x"0001ce", "001", C_WORD, read_data);
    CWRITE(x"f9a103", "001", C_BYTE, x"7373");
    CREAD(x"0001d0", "001", C_WORD, read_data);
    CREAD(x"0001d2", "001", C_WORD, read_data);
    CREAD(x"0001d4", "001", C_WORD, read_data);
    CREAD(x"0001d6", "001", C_WORD, read_data);
    CREAD(x"0001d8", "001", C_WORD, read_data);
    CREAD(x"0001da", "001", C_WORD, read_data);
    CREAD(x"0001dc", "001", C_WORD, read_data);
    CREAD(x"0001de", "001", C_WORD, read_data);
    CREAD(x"0001e0", "001", C_WORD, read_data);
    CWRITE(x"f9a104", "001", C_BYTE, x"8181");
    CREAD(x"0001e2", "001", C_WORD, read_data);
    CREAD(x"0001e4", "001", C_WORD, read_data);
    CREAD(x"0001e6", "001", C_WORD, read_data);
    CREAD(x"0001e8", "001", C_WORD, read_data);
    CREAD(x"0001ea", "001", C_WORD, read_data);
    CWRITE(x"f9a105", "001", C_BYTE, x"8181");
    CREAD(x"0001ec", "001", C_WORD, read_data);
    CREAD(x"0001ee", "001", C_WORD, read_data);
    CREAD(x"0001f0", "001", C_WORD, read_data);
    CREAD(x"0001f2", "001", C_WORD, read_data);
    CWRITE(x"f9a106", "001", C_BYTE, x"8181");
    CREAD(x"0001f4", "001", C_WORD, read_data);
    CREAD(x"0001f6", "001", C_WORD, read_data);
    CREAD(x"0001f8", "001", C_WORD, read_data);
    CWRITE(x"f9a107", "001", C_BYTE, x"0000");
    CREAD(x"0001fa", "001", C_WORD, read_data);
    CWRITE(x"001000", "001", C_WORD, x"0000");
    CWRITE(x"001002", "001", C_WORD, x"4321");
    CREAD(x"0001fc", "001", C_WORD, read_data);
   
    end loop;
    end if;
      
    ----------------------------------------------------------------------------
    -- IO bus access
    report "68K: access to MFP" severity note;

    CWRITE(x"F00001", "101", C_BYTE, x"0000");  -- move.b #0,0xfff00001

    CREAD(x"F00000", "101", C_BYTE, read_data);  -- MFP data
    CWRITE(x"F00001", "101", C_BYTE, x"1234");  -- byte, even - address
    CWRITE(x"F00000", "101", C_BYTE, x"5678");  -- byte, odd
    CWRITE(x"F00002", "101", C_WORD, x"5678");  -- word - address + data
    CWRITE(x"F00000", "101", C_BYTE, x"0000");  -- byte, odd - data
    CREAD(x"000000", "101", C_WORD, read_data); -- word => even - address
    CWRITE(x"F00000", "101", C_BYTE, x"1234");  -- byte, even
    CWRITE(x"F00001", "101", C_BYTE, x"5678");  -- byte, odd
    
    CREAD(x"F00000", "101", C_BYTE, read_data);  -- MFP
    CREAD(x"F00001", "101", C_BYTE, read_data);  -- MFP
    CREAD(x"F00000", "101", C_WORD, read_data);  -- MFP
   
    report "68K: access to AV/avmgr" severity note;
    CWRITE(x"f80000", "101", C_BYTE, x"0000");  -- assert nCONFIG (+RSTn)
    
    for i in 1 to 10 loop
      CREAD(x"000002", "101", C_WORD, read_data); -- word => even - address
      CREAD(x"f80000", "101", C_BYTE, read_data);
      CREAD(x"000004", "101", C_WORD, read_data); -- word => even - address
    end loop;  -- i

    CWRITE(x"f80000", "101", C_BYTE, x"4040");  -- de-assert nCONFIG (+RSTn)
    
    for i in 1 to 5 loop
      CREAD(x"000002", "101", C_WORD, read_data); -- word => even - address
      CREAD(x"f80000", "101", C_BYTE, read_data);
      CREAD(x"000004", "101", C_WORD, read_data); -- word => even - address
    end loop;  -- i

    wait for 1 us;
    
    for i in 1 to 5 loop
      CWRITE(x"f80002", "101", C_BYTE, x"ffff");
      CREAD(x"000002", "101", C_WORD, read_data); -- word => even - address
      CWRITE(x"f80002", "101", C_BYTE, x"0000");
      CREAD(x"000004", "101", C_WORD, read_data); -- word => even - address
    end loop;  -- i
    
    CWRITE(x"f80000", "101", C_BYTE, x"c0c0");  -- deassert reset

    report "68K: access to AV/OPL2" severity note;
    CWRITE(x"f80180", "101", C_BYTE, x"0101");
    CWRITE(x"f80180", "101", C_BYTE, x"0101");
    CWRITE(x"f80180", "101", C_BYTE, x"0101");
    CWRITE(x"f80180", "101", C_BYTE, x"0101");
    CWRITE(x"f80180", "101", C_BYTE, x"0101");
    CREAD(x"f80180", "101", C_WORD, read_data);
    CWRITE(x"f80180", "101", C_BYTE, x"0101");
    CREAD(x"f80180", "101", C_WORD, read_data);

    report "68K: access to AV/RAMDAC" severity note;
    for i in 0 to 7 loop  -- 255
      CWRITE(x"f80102", "101", C_BYTE,
             std_logic_vector(to_unsigned(i mod 8, 3)) & "00000" &
             std_logic_vector(to_unsigned(i mod 8, 3)) & "00000");  -- LUT data
      CWRITE(x"f80102", "101", C_BYTE,
             std_logic_vector(to_unsigned((i / 8) mod 8, 3)) & "00000" &
             std_logic_vector(to_unsigned((i / 8) mod 8, 3)) & "00000");  -- LUT data
      CWRITE(x"f80102", "101", C_BYTE,
             std_logic_vector(to_unsigned(i / 64, 3)) & "00000" &
             std_logic_vector(to_unsigned(i / 64, 3)) & "00000");  -- LUT data
    end loop;  -- i
    
    CREAD(x"f80102", "101", C_BYTE, read_data);
    CREAD(x"f80102", "101", C_BYTE, read_data);
    CREAD(x"f80102", "101", C_BYTE, read_data);
    CREAD(x"f80102", "101", C_BYTE, read_data);
    CREAD(x"f80102", "101", C_BYTE, read_data);
    CREAD(x"f80102", "101", C_BYTE, read_data);

    report "68K: access to AV/flex" severity note;
    CWRITE(x"f80080", "101", C_BYTE, x"0101");  -- reg0 - enable interrupt
    CWRITE(x"f80081", "101", C_BYTE, x"1212");  -- reg1 - low res mode + bank +8000h
    CWRITE(x"f80082", "101", C_WORD, x"0200");  -- reg2-3 - read back address
    CWRITE(x"f88000", "101", C_BYTE, x"0101");  -- VRAM bank0
    CWRITE(x"f90000", "101", C_BYTE, x"0101");  -- VRAM bank1
    CWRITE(x"f98000", "101", C_BYTE, x"0101");  -- VRAM bank2
    CWRITE(x"f90000", "101", C_WORD, x"0101");  -- VRAM bank1
    CWRITE(x"f98000", "101", C_WORD, x"0101");  -- VRAM bank2

    -------------------
    -- GPU
    -------------------
    report "68K: GPU test" severity note;
    -- conf: hi res mode, LUT #0, frame buffer @ 20000h, VRAM bank #0 (00000h-0FFFFh)
    CWRITE(x"f80080", "101", C_WORD, x"0000");
    CWRITE(x"f80082", "101", C_WORD, x"0000");
    
    -- load smiley @7400 in VRAM
    -- // character 01h
    --  0x36, 0x93, 
    --  0x15, 0xA2, 
    --  0xB5, 0xA7, 
    --  0xC9, 0x6C, 
    CWRITE(x"f97400", "101", C_WORD, x"9336");
    CWRITE(x"f97402", "101", C_WORD, x"A215");
    CWRITE(x"f97404", "101", C_WORD, x"A7B5");
    CWRITE(x"f97406", "101", C_WORD, x"6CC9");

    -- a cross
    CWRITE(x"f97408", "101", C_WORD, x"6009");
    CWRITE(x"f9740a", "101", C_WORD, x"0690");
    CWRITE(x"f9740c", "101", C_WORD, x"0690");
    CWRITE(x"f9740e", "101", C_WORD, x"9006");

    -- build DL @0x0
    -- read smiley
    CWRITE(x"f90000", "101", C_WORD, x"7400"); -- ADDR 7400h
    CWRITE(x"f90002", "101", C_WORD, x"8031"); -- 4 lines of 2 words
    CWRITE(x"f90004", "101", C_WORD, x"8200"); -- read BK #0

    -- write to VRAM
    CWRITE(x"f90006", "101", C_WORD, x"0000"); -- ADDR 20000h
    CWRITE(x"f90008", "101", C_WORD, x"8033"); -- 4 lines of 4 words
    CWRITE(x"f9000a", "101", C_WORD, x"9308"); -- write BK #0, LUT #8

    CWRITE(x"f9000c", "101", C_WORD, x"8100"); -- stop
                                 
    -- start DL @0x0             
    CWRITE(x"f80084", "101", C_WORD, x"0000");

    read_data := x"8080";

    while read_data(7) = '1' loop
      CREAD(x"f80081", "101", C_BYTE, read_data);
    end loop;
        
    for y in 1 to 16 loop
      -- build DL @0x0
      -- read smiley
      CWRITE(x"f90000", "101", C_WORD, x"7400"); -- ADDR 7400h
      CWRITE(x"f90002", "101", C_WORD, x"8031"); -- 4 lines of 2 words
      CWRITE(x"f90004", "101", C_WORD, x"8200"); -- read BK #0
      
      -- read cross
      CWRITE(x"f90006", "101", C_WORD, x"7408"); -- ADDR 7408h
      CWRITE(x"f90008", "101", C_WORD, x"8031"); -- 4 lines of 2 words
      CWRITE(x"f9000a", "101", C_WORD, x"8210"); -- read BK #1

      -- write to VRAM
      -- ADDR 20000h + 200h * y
      CWRITE(x"f9000c", "101", C_WORD, std_logic_vector(to_unsigned(4*512*(y mod 16), 16))); 
      CWRITE(x"f9000e", "101", C_WORD, x"8033"); -- 4 lines of 4 words

      CWRITE(x"f90010", "101", C_WORD, x"9308"); -- write BK #0, LUT #8
      CWRITE(x"f90012", "101", C_WORD, x"9310"); -- write BK #1, LUT #0
      CWRITE(x"f90014", "101", C_WORD, x"8100"); -- stop
        
      -- start DL @0x0000            
      CWRITE(x"f80084", "101", C_WORD, x"0000");

      read_data := x"8080";

      while read_data(7) = '1' loop
        CREAD(x"f80081", "101", C_BYTE, read_data);
      end loop;
        
      for x in 1 to 37 loop
        -- start DL @0x000e
        CWRITE(x"f80084", "101", C_WORD, x"0007");

        read_data := x"8080";

        while read_data(7) = '1' loop
          CREAD(x"f80081", "101", C_BYTE, read_data);
        end loop;
      end loop;  -- x

      report "**************************************** line " & integer'image(y) & "****************************************" severity note;
                                 
    end loop;


    
    wait;
    
--    CWRITE(x"F80018", "101", "10", x"0040");
--    CWRITE(x"F8001A", "101", "10", x"0001"); -- @sb
--    CWRITE(x"F80018", "101", "10", x"0002"); 
--    CWRITE(x"F8001A", "101", "10", x"0002"); -- @msb
--    CWRITE(x"F80018", "101", "10", x"0001");
--    CWRITE(x"F8001A", "101", "10", x"0003"); -- @cmd
--    CWRITE(x"F80018", "101", "10", x"0000"); -- read

--    wait for 100 us;
    
--    CWRITE(x"F8001A", "101", "10", x"0000"); -- @lsb
--    CWRITE(x"F80018", "101", "10", x"0070");
--    CWRITE(x"F8001A", "101", "10", x"0001"); -- @sb
--    CWRITE(x"F80018", "101", "10", x"0003"); 
--    CWRITE(x"F8001A", "101", "10", x"0002"); -- @msb
--    CWRITE(x"F80018", "101", "10", x"0002");
--    CWRITE(x"F8001A", "101", "10", x"0003"); -- @cmd
--    CWRITE(x"F80018", "101", "10", x"0001"); -- write

    -- audio
    while true loop

      read_data := x"0040";

      while read_data(5) = '0' loop
        CREAD(x"F80012", "101", C_BYTE, read_data);
      end loop;

      wait for 50 us;

      for i in 0 to 15 loop
        CWRITE(x"F8001C", "101", C_BYTE,
               (x"80" or std_logic_vector(to_unsigned((i*16+i)/2, 8))) &
               (x"80" or std_logic_vector(to_unsigned((i*16+i)/2, 8)))
               );
        CWRITE(x"F8001E", "101", C_BYTE,
               std_logic_vector(to_unsigned((i*16+i)/2, 8)) &
               std_logic_vector(to_unsigned((i*16+i)/2, 8))
               );
      end loop;  -- i

      for i in 15 downto 0 loop
        CWRITE(x"F8001C", "101", C_BYTE,
               (x"80" or std_logic_vector(to_unsigned((i*16+i)/2, 8))) &
               (x"80" or std_logic_vector(to_unsigned((i*16+i)/2, 8)))
               );
        CWRITE(x"F8001E", "101", C_BYTE,
               std_logic_vector(to_unsigned((i*16+i)/2, 8)) &
               std_logic_vector(to_unsigned((i*16+i)/2, 8))
               );
      end loop;  -- i

--      for i in 0 to 15 loop
--        CWRITE(x"F8001C", "101", "10", std_logic_vector(to_unsigned((i*16+i)/2, 16)));
--        CWRITE(x"F8001E", "101", "10", x"0080" or std_logic_vector(to_unsigned((i*16+i)/2, 16)));
--      end loop;  -- i

--      for i in 15 downto 0 loop
--        CWRITE(x"F8001C", "101", "10", std_logic_vector(to_unsigned((i*16+i)/2, 16)));
--        CWRITE(x"F8001E", "101", "10", x"0080" or std_logic_vector(to_unsigned((i*16+i)/2, 16)));
--      end loop;  -- i

    end loop;
   
    -- MMU
    CREAD(x"000004", "101", C_BYTE, read_data);
    CWRITE(x"fe0000", "101", C_BYTE, x"3000");  -- ad_lo
    CWRITE(x"fe0002", "101", C_BYTE, x"0000");  -- ad_hi
    CWRITE(x"fe0004", "101", C_BYTE, x"0100");  -- ad_lo
    CWRITE(x"fe0006", "101", C_BYTE, x"0000");  -- ad_hi

    CREAD(x"000004", "101", C_WORD, read_data);
    CREAD(x"000004", "001", C_WORD, read_data);

    while true loop

      -- SSP
      CREAD(x"000000", "101", C_WORD, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);

      -- PC
      CREAD(x"000004", "101", C_WORD, read_data);
      CREAD(x"000006", "101", C_WORD, read_data);
      
       -- IO
      CREAD(x"F00000", "101", C_BYTE, read_data);
      --CREAD(x"F00002", "101");
      
      -- 
      CREAD(x"000008", "101", C_WORD, read_data);
      CWRITE(x"123456", "101", C_WORD, x"DEAD");
      CREAD(x"001008", "101", C_WORD, read_data);
      CREAD(x"123456", "101", C_WORD, read_data);

      -- set UART status address
      CWRITE(x"F00002", "101", C_WORD, x"0005");

      CREAD(x"000002", "101", C_WORD, read_data);

      CREAD(x"F80000", "101", C_BYTE, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);
      CREAD(x"F80001", "101", C_BYTE, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);
      CREAD(x"F80002", "101", C_BYTE, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);
      CREAD(x"F80003", "101", C_BYTE, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);
      CREAD(x"F80010", "101", C_BYTE, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);
      CREAD(x"F80020", "101", C_BYTE, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);
      CREAD(x"F80030", "101", C_BYTE, read_data);

      CWRITE(x"F80000", "101", C_BYTE, x"0000");
      CREAD(x"000002", "101", C_WORD, read_data);
      CREAD(x"F80000", "101", C_BYTE, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);
      CWRITE(x"F80002", "101", C_BYTE, x"0000");
      CREAD(x"000002", "101", C_WORD, read_data);
      CREAD(x"F80002", "101", C_BYTE, read_data);
      CREAD(x"000002", "101", C_WORD, read_data);

      -- PMU
      CWRITE(x"fe0000", "100", C_WORD, x"1234");
      CWRITE(x"fe0002", "100", C_WORD, x"5678");
      CWRITE(x"fe0004", "100", C_WORD, x"DEAD");
      CWRITE(x"fe0006", "100", C_WORD, x"BEEF");

      -- CPU space
      CREAD(x"fffff0", "100", C_WORD, read_data);

    -- GPU
--      CWRITE(x"F8001E", "101", x"00de", C_WORD);
--      CWRITE(x"F8001E", "101", x"00ad", C_WORD);
   end loop;

  end process;

end architecture rtl;
