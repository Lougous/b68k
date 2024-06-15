
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all;

library std;
use std.textio.all;

entity fpdram is
  generic (
    ROOT_NAME : string := "./fpdram_mif_";
    VERBOSE   : boolean := true;
    VERBOSE2  : boolean := false;
    BSIZE     : integer range 1 to 8 := 8;
    NPAGE     : integer := 4;  -- for internal cache purpose

    -- timing
    -- 4C1024-80
    tASR : time := 0 ns;
    tRAH : time := 12 ns;
    tRCDmin : time := 22 ns;
    tRCDmax : time := 60 ns;
    tASC : time := 0 ns;
    tRCS : time := 0 ns;
    tRAS : time := 80 ns;
    tRC  : time := 150 ns;
    tRP  : time := 60 ns;
    tCAS : time := 20 ns;
    tCLZ : time := 0 ns;  -- not found
    tCAC : time := 20 ns;
    tCAH : time := 15 ns;
    tOFF : time := 20 ns;
    tAA  : time := 40 ns;
    tRAC : time := 80 ns;
    tACH : time := 15 ns;  -- tCAH
    tWCS : time := 0 ns;
    tWCH : time := 15 ns;
    tDS  : time := 0 ns;
    tDH  : time := 15 ns;
    tRSH : time := 20 ns;
    tRWL : time := 20 ns;
    tCSH : time := 80 ns;
    tCWL : time := 20 ns;
    tWP  : time := 15 ns;
    tWCR : time := 60 ns;
    tAR  : time := 60 ns;
    tCP  : time := 10 ns;
    tPC  : time := 50 ns;
    tCOH : time := 0 ns;  -- not found
    tCSR : time := 10 ns;
    tCHR : time := 20 ns
--  tWRP : time := 10 ns;
--  tWRH : time := 10 ns;
    
    );
  port (
    ADDR : in    std_logic_vector(10 downto 0);
    RWn  : in    std_logic;
    RASn : in    std_logic;
    CASn : in    std_logic;
    DQ   : inout std_logic_vector(BSIZE-1 downto 0)
    );
end fpdram;

architecture simu of fpdram is
 
                            
  type t_page_data is array (0 to 2**(ADDR'left + 1) - 1) of integer range 0 to 2**BSIZE - 1;
  type t_page is record
    data     : t_page_data;
    valid    : boolean;
    modified : boolean;
    addr     : integer range 0 to 2**(ADDR'left + 1) - 1;
  end record;
  
  type t_memory is array (0 to NPAGE - 1) of t_page;

  signal s_memory : t_memory;

  type t_state is (
    START_CYCLE,
    ACCESS_CYCLE
    );
  
begin

    process
      procedure LoadPage (
        bank_ad : in integer range 0 to NPAGE - 1;
        ras_ad  : in integer range 0 to 2**(ADDR'left + 1) - 1)
      is
        variable filename : line := null;
        variable fstatus  : file_open_status;
        file     infile   : text;
        variable inline   : line;
        variable rd       : std_logic_vector(7 downto 0);
        variable n        : integer range 0 to 2**(ADDR'left + 1);
      begin
        -- build file name
        write(filename, ROOT_NAME);
        write(filename, '_');
        hwrite(filename, std_logic_vector(to_unsigned(ras_ad, 12)));
        write(filename, string'(".txt"));

        file_open(fstatus, infile, filename.all, read_mode);

        s_memory(bank_ad).data <= (others => 0);
        
        if fstatus = open_ok then
          n := 0;
          
          while (not endfile(infile)) or n < 2**(ADDR'left + 1) loop
            readline(infile, inline);
            hread(inline, rd);
            s_memory(bank_ad).data(n) <= to_integer(unsigned(rd));
            n := n + 1;
          end loop;

          assert not VERBOSE report
            "[FPDRAM] raw " & integer'image(ras_ad) & ", bank " &
            integer'image(bank_ad) & ": loaded file " & filename.all & ", " & integer'image(n) & " bytes"
            severity note;
        else
          assert not VERBOSE report
            "[FPDRAM] raw " & integer'image(ras_ad) & ", bank " &
            integer'image(bank_ad) & ": no file found (" & filename.all & "), using default content"
            severity note;
        end if;

        file_close(infile);

        s_memory(bank_ad).valid    <= true;
        s_memory(bank_ad).modified <= false;
        s_memory(bank_ad).addr     <= ras_ad;
      end procedure LoadPage;
      
      procedure SavePage (
        bank_ad : in integer range 0 to 3)
      is
        variable filename : line := null;
        variable fstatus  : file_open_status;
        file     infile   : text;
        variable inline   : line;
        variable rd       : std_logic_vector(7 downto 0);
      begin
        -- build file name
        write(filename, ROOT_NAME);
        write(filename, '_');
        hwrite(filename, std_logic_vector(to_unsigned(s_memory(bank_ad).addr, 12)));
        write(filename, string'(".txt"));

        file_open(fstatus, infile, filename.all, write_mode);

        assert fstatus = OPEN_OK report "[FPDRAM] failed to open file " & filename.all
          severity failure;

        for i in 0 to 2**(ADDR'left + 1) - 1 loop
          hwrite(inline, std_logic_vector(to_unsigned(s_memory(bank_ad).data(i), 8)));
          writeline(infile, inline);
        end loop;

        assert not VERBOSE report
          "[FPDRAM] raw " & integer'image(to_integer(to_unsigned(s_memory(bank_ad).addr, 12))) &
          ", bank " & integer'image(bank_ad) & ": saved file " & filename.all
          severity note;

        file_close(infile);
      end procedure SavePage;
      
      variable v_ba : integer range 0 to NPAGE - 1;
      variable v_ra : integer range 0 to 2**(ADDR'left + 1) - 1;
      variable v_ca : integer range 0 to 2**(ADDR'left + 1) - 1;
      variable v_time_ras : time;
      variable v_time_cas : time;
      variable v_time_cas_addr : time;
      variable v_time_cas_rwn : time;
      variable v_in_burst : boolean;
      variable v_state : t_state := START_CYCLE;
    begin
      DQ <= (others => 'Z');
      v_state := START_CYCLE;

      -- memory init
      for j in 0 to NPAGE-1 loop
--        for k in 0 to 2**(ADDR'left + 1) - 1 loop
--          s_memory(byte)(j).data(k) <= (4*k+i) mod 2**BSIZE;
--        end loop;  -- j
        s_memory(j).valid    <= false;
        s_memory(j).modified <= false;
        s_memory(j).addr     <= 0;
      end loop;  -- j
      
      while true loop
        case v_state is
          when START_CYCLE =>
            wait until falling_edge(RASn) or falling_edge(CASn);
            
            if RASn = '0' then
              -- ram access, read or write
              v_ba := to_integer(unsigned(ADDR)) mod NPAGE;
              v_ra := to_integer(unsigned(ADDR));

              if s_memory(v_ba).valid then
                if s_memory(v_ba).addr = v_ra then
                  -- hit, do not modify anything
                  null;
                else
                  -- miss
                  if s_memory(v_ba).modified then
                    -- save old page content
                    SavePage(v_ba);

                    -- load new page content
                    LoadPage(v_ba, v_ra);
                  else
                    -- not modified
                    -- load new page content
                    LoadPage(v_ba, v_ra);
                  end if;
                end if;
              else
                -- not valid
                -- load new page content
                LoadPage(v_ba, v_ra);
              end if;
              
              v_time_ras := now;
              v_in_burst := false;
              v_state := ACCESS_CYCLE;
            else
              -- CBR
              wait until rising_edge(RASn);

              assert not VERBOSE report "CAS before RAS refresh" severity note;
            end if;

          when ACCESS_CYCLE =>
            -- wait for CASn falling edge or RASn rising edge
            wait until falling_edge(CASn) or rising_edge(RASn);
            DQ <= (others => 'Z') after tCOH;

            if RASn = '1' then
              -- could be a refresh, or end of cycle
              DQ <= (others => 'Z') after tOFF;
              v_state := START_CYCLE;
            else
              -- CASn sequence
              v_ca := to_integer(unsigned(ADDR));
              v_time_cas := now;
              v_time_cas_addr := ADDR'last_event;
              v_time_cas_rwn := RWn'last_event;

              if not v_in_burst then
                assert (v_time_cas - v_time_ras) >= tRCDmin report "tRCDmin violation" severity error;
                --assert (v_time_cas - v_time_ras) <= tRCDmax report "tRCDmax violation" severity error;
              end if;

              v_in_burst := true;
              
              if RWn = '1' then
                -- read operation
                assert not VERBOSE2 report "read" severity note;
                DQ <=
                  (others => 'U') after tCLZ,
                  std_logic_vector(to_unsigned(s_memory(v_ba).data(v_ca), BSIZE)) after tCAC;
                
                --assert (now + tCAC) >= (v_time_ras + tRAC) report "tRAC violation (?)" severity error;
                assert (now + tCAC) >= (v_time_cas_addr + tAA) report "tAA violation (?)" severity error;
                -- wait for CASn rising edge
                wait until rising_edge(CASn) or rising_edge(RASn);

                if RASn = '1' then
                  -- end of cycle
                  if CASn = '0' then
                    wait until rising_edge(CASn);
                  end if;
                  
                  DQ <= (others => 'Z') after tOFF;
                  v_state := START_CYCLE;
                else
                  -- maybe continue in a burst
                  null;
                end if;

                assert (v_time_ras + tCSH) <= now report "tCSH violation" severity error;
                assert (v_time_cas_addr + tACH) <= now report "tACH violation" severity error;
                assert (v_time_cas_rwn + tCWL) <= now report "tCWL violation" severity error;
              else
                -- write operation
                s_memory(v_ba).modified <= true;
              
                assert not VERBOSE2 report "write" severity note;
                s_memory(v_ba).data(v_ca) <= to_integer(unsigned(DQ));
                
                wait until rising_edge(CASn) or rising_edge(RASn);

                if RASn = '1' then
                  -- end of cycle
                  assert (v_time_cas + tRSH) <= now report "tRSH violation" severity error;
                  assert (v_time_cas_rwn + tRWL) <= now report "tRWL violation" severity error;
                  
                  if CASn = '0' then
                    wait until rising_edge(CASn);
                  end if;

                  v_state := START_CYCLE;
                else
                  -- maybe continue in a burst
                  null;
                end if;

                assert (v_time_ras + tCSH) <= now report "tCSH violation" severity error;
                assert (v_time_cas_addr + tACH) <= now report "tACH violation" severity error;
                assert (v_time_cas_rwn + tCWL) <= now report "tCWL violation" severity error;
              end if;
            end if;
          when others => null;
        end case;
      end loop;
    end process;
    
    -----------------------------------------------------------------------------
    -- CASn monitoring
    -----------------------------------------------------------------------------
    process
      variable v_time_cas_rise : time := 0 ns;
      variable v_time_cas_fall : time := 0 ns;
    begin
      wait until falling_edge(CASn);
      assert (now - v_time_cas_rise) >= tCP report "tCP violation" severity error;
      v_time_cas_fall := now;
      wait until rising_edge(CASn);
      assert (now - v_time_cas_fall) >= tCAS report "tCAS violation" severity error;
      assert (now - v_time_cas_rise) >= tPC report "tPC violation" severity error;
      v_time_cas_rise := now;
    end process;

    -----------------------------------------------------------------------------
    -- ADDR setup/hold // CASn
    -----------------------------------------------------------------------------
    process
    begin
      wait until falling_edge(CASn);
      -- not applicable for refresh
      if RASn = '0' then
        assert ADDR'stable(tASC) report "tASC violation" severity error;
        wait for tCAH;
        assert ADDR'stable(tCAH) report "tCAH violation" severity error;
      end if;
    end process;
  
    -----------------------------------------------------------------------------
    -- RWn setup/hold // CASn
    -----------------------------------------------------------------------------
    process
    begin
      wait until falling_edge(CASn);
      if RWn = '1' then
        -- read
        assert RWn'stable(tRCS) report "tRCS violation" severity error;
      else
        -- write
        assert RWn'stable(tWCS) report "tWCS violation" severity error;
        wait for tWCH;
        assert RWn'stable(tWCH) report "tWCH violation" severity error;
      end if;
    end process;
  
    -----------------------------------------------------------------------------
    -- ADDR setup/hold // data written
    -----------------------------------------------------------------------------
    process
    begin
      wait until falling_edge(CASn);
      if RWn = '0' then
        assert DQ'stable(tDS) report "tDS violation" severity error;
        wait for tDH;
        assert DQ'stable(tDH) report "tDH violation" severity error;
      end if;
    end process;
    
    -----------------------------------------------------------------------------
    -- tAR check
    -----------------------------------------------------------------------------
    --process
    --  variable v_time : time;
      
    --begin
    --  wait until falling_edge(RASn);
    --  v_time := now;
    --  wait until falling_edge(CASn(byte));
    --  wait until ADDR'event or rising_edge(CASn(byte));
    --  assert (now - v_time) >= tAR report "tAR violation" severity error;
    --end process;
    

  -----------------------------------------------------------------------------
  -- RASn monitoring
  -----------------------------------------------------------------------------
  process
    variable v_time_ras_rise : time := 0 ns;
    variable v_time_ras_fall : time := 0 ns;
  begin
    wait until falling_edge(RASn);
    assert (now - v_time_ras_rise) >= tRP report "tRP violation" severity error;
    assert (now - v_time_ras_fall) >= tRC report "tRC violation" severity error;
    v_time_ras_fall := now;
    wait until rising_edge(RASn);
    assert (now - v_time_ras_fall) >= tRAS report "tRAS violation" severity error;
--    assert (now - v_time_ras_fall) >= tRCP report "tRCP violation" severity error;
    v_time_ras_rise := now;
  end process;

  -----------------------------------------------------------------------------
  -- ADDR setup/hold // RASn
  -----------------------------------------------------------------------------
  process
  begin
    wait until falling_edge(RASn);
    assert ADDR'stable(tASR) report "tASR violation" severity error;
    wait for tRAH;
    assert ADDR'stable(tRAH) report "tRAH violation" severity error;
  end process;
  
  -----------------------------------------------------------------------------
  -- RWn pulse duration
  -----------------------------------------------------------------------------
  process
    variable v_time : time;
    
  begin
    wait until falling_edge(RWn);
    v_time := now;
    wait until rising_edge(RWn);
    assert (now - v_time) >= tWP report "tWP violation" severity error;
  end process;
  
  -----------------------------------------------------------------------------
  -- tWCR check
  -----------------------------------------------------------------------------
  process
    variable v_time : time;
    
  begin
    wait until falling_edge(RASn);
    v_time := now;
    wait until rising_edge(RWn);
    assert (now - v_time) >= tWCR report "tWCR violation" severity error;
  end process;
  
  -----------------------------------------------------------------------------
  -- refresh // CASn
  -----------------------------------------------------------------------------
  process
    variable v_time : time;
    
  begin
    wait until falling_edge(RASn);
    if CASn = '0' then
      assert CASn'stable(tCSR) report "tCSR violation" severity error;
      wait for tCHR;
      assert CASn'stable(tCHR) report "tCHR violation" severity error;
    end if;
  end process;
  
  -----------------------------------------------------------------------------
  -- refresh // RWn
  -----------------------------------------------------------------------------
  -- process
  --   variable v_time : time;
    
  -- begin
  --   wait until falling_edge(RASn);
  --   if CASn /= "1111" then
  --     assert RWn'stable(tWRP) report "tWRP violation" severity error;
  --     wait for tWRH;
  --     assert RWn'stable(tWRH) report "tWRH violation" severity error;
  --   end if;
  -- end process;

end simu;
