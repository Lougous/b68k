--
-- https://github.com/Lougous/b68k
--
-- VGA video frame recorder
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all;

library std;
use std.textio.all;

entity vga_recorder is
  generic (
    ROOT_NAME   : string := "./vga";
    WIDTH       : integer;
    HEIGHT      : integer;
    AUTO_ENABLE : boolean
    );
  port (
    CLK : in std_logic;
    VSn : in std_logic;
    HSn : in std_logic;
    ENA : in std_logic;
    PIX : in std_logic_vector(11 downto 0)
    );
end vga_recorder;

architecture simu of vga_recorder is

  signal ENA_int : std_logic;
  
begin  -- simu

  ENA_int <= ENA;
  
  recorder : process
    variable fnum : integer := 0;
    variable npxl : integer := 0;
    variable red : integer := 0;
    variable green : integer := 0;
    variable blue : integer := 0;
    
    file vga_file    : TEXT;
    variable fstatus : FILE_OPEN_STATUS;
    variable fbuf    : LINE;
      
  begin  -- process recorder
    wait until rising_edge(VSn);

    file_open(fstatus, vga_file,
              ROOT_NAME & integer'image(fnum) & ".ppm", write_mode);

    -- ppm file header
    WRITE(fbuf, "P" & integer'image(3));
    WRITELINE(vga_file, fbuf);
    WRITE(fbuf, integer'image(WIDTH));
    WRITELINE(vga_file, fbuf);
    WRITE(fbuf, integer'image(HEIGHT));
    WRITELINE(vga_file, fbuf);
    WRITE(fbuf, integer'image(15));
    WRITELINE(vga_file, fbuf);

    npxl := 0;
    
    recording : while true loop
      wait until rising_edge(CLK);

      if VSn = '0' then
        -- not finished
        report "VGA frame recording aborted - unexpected VSYNC";
        exit recording;
      end if;

      if ENA_int = '1' then
        red := to_integer(unsigned(PIX(11 downto 8)));
        green := to_integer(unsigned(PIX(7 downto 4)));
        blue := to_integer(unsigned(PIX(3 downto 0)));
        
        WRITE(
          fbuf, integer'image(red) & " " & integer'image(green) & " " &
          integer'image(blue));
        WRITELINE(vga_file, fbuf);

        npxl := npxl + 1;

        if npxl = WIDTH*HEIGHT then
          -- end of VGA frame
          report "VGA frame n°" & integer'image(fnum) & " recorded";
          exit recording;
        end if;
      end if;
      
    end loop;

    file_close(vga_file);
    
    fnum := fnum + 1;
    
  end process recorder;

end simu;
