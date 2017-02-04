<?php

# HTML Mixer styring af videomixer-shm demon
# Common code.

# Forbind til mixeren.
$S = socket_create (AF_INET, SOCK_STREAM, SOL_TCP);
if (socket_connect ($S, $host, $port) === FALSE) {
  printf ("Socket connect Error: " . socket_strerror (socket_last_error ()));
  exit (1);
}

# Hent velkommen beskeden.
$l = socket_read ($S, 999, PHP_NORMAL_READ);

#  printf ("%s\n%s\n", var_dump($_POST), $_POST["feed"]);
$cmd = "";
if ($_POST ["feed"] != "" ) {
  if ($_POST ["feed"] == "1+423" ) {
    $cmd = "feed1423";
  }
  else if ($_POST ["feed"] == "4+123" ) {
    $cmd = "feed4123";
  }
  else if ($_POST ["feed"] == "2+143" ) {
    $cmd = "feed2143";
  }
  else if ($_POST ["feed"] == "3+124" ) {
    $cmd = "feed3124";
  }
}
if ($_POST ["swop"] != "" ) {
  if ($_POST ["swop"] == "4+1" ) {
    $cmd = "swop41";
  }
  else if ($_POST ["swop"] == "4+2" ) {
    $cmd = "swop42";
  }
  else if ($_POST ["swop"] == "4+3" ) {
    $cmd = "swop43";
  }
  else if ($_POST ["swop"] == "1+4" ) {
    $cmd = "swop14";
  }
  else if ($_POST ["swop"] == "2+4" ) {
    $cmd = "swop24";
  }
  else if ($_POST ["swop"] == "3+4" ) {
    $cmd = "swop34";
  }
}
if ($_POST ["right"] != "" ) {
  if ($_POST ["right"] == "123" ) {
    $cmd = "right123";
  }
  else if ($_POST ["right"] == "423" ) {
    $cmd = "right423";
  }
  else if ($_POST ["right"] == "143" ) {
    $cmd = "right143";
  }
  else if ($_POST ["right"] == "124" ) {
    $cmd = "right124";
  }
}
if ($_POST ["left"] != "" ) {
  if ($_POST ["left"] == "123" ) {
    $cmd = "left123";
  }
  else if ($_POST ["left"] == "423" ) {
    $cmd = "left423";
  }
  else if ($_POST ["left"] == "143" ) {
    $cmd = "left143";
  }
  else if ($_POST ["left"] == "124" ) {
    $cmd = "left124";
  }
}

if ($cmd != "") socket_write ($S, $cmd . "\n");



if (count ($_POST)) {
  # Parse input fra formen.
  if ($_POST ["reload"] != "") {
    # Ingen opdatering - bare vis billedet igen.
  } else if ($_POST ["set"] != "") {
    # Post opdateringer til mixer demonen.
    $newlayer = array ();
    $oldlayer = array ();
    for ($x = 1; isset ($_POST ["F" . $x . "ID"]); $x++) {
      $id = 1 * $_POST ["F" . $x . "ID"];

      if ($_POST ["F" . $x . "CUTSTARTX"] != $_POST ["O" . $x . "CUTSTARTX"] ||
          $_POST ["F" . $x . "CUTSTARTY"] != $_POST ["O" . $x . "CUTSTARTY"] ||
          $_POST ["F" . $x . "CUTSIZEX"]  != $_POST ["O" . $x . "CUTSIZEX"]  ||
          $_POST ["F" . $x . "CUTSIZEY"]  != $_POST ["O" . $x . "CUTSIZEY"]) {
        # Opdater denne feed.
        $cmd = "feed cutout " . $id .
               " " . $_POST ["F" . $x . "CUTSTARTX"] .
               " " . $_POST ["F" . $x . "CUTSTARTY"] .
               " " . $_POST ["F" . $x . "CUTSIZEX"] .
               " " . $_POST ["F" . $x . "CUTSIZEY"];

        socket_write ($S, $cmd . "\n");
      }

      if ($_POST ["F" . $x . "SCALE1"] != $_POST ["O" . $x . "SCALE1"] ||
          $_POST ["F" . $x . "SCALE2"] != $_POST ["O" . $x . "SCALE2"]) {
        # Opdater scale for denne feed.
        $cmd = "feed scale " . $id .
               " " . $_POST ["F" . $x . "SCALE1"] .
               " " . $_POST ["F" . $x . "SCALE2"];

        socket_write ($S, $cmd . "\n");
      }

      if ($_POST ["F" . $x . "SHIFTX"] != $_POST ["O" . $x . "SHIFTX"] ||
          $_POST ["F" . $x . "SHIFTY"] != $_POST ["O" . $x . "SHIFTY"]) {
        # Opdater denne feed.
        $cmd = "feed shift " . $id .
               " " . $_POST ["F" . $x . "SHIFTX"] .
               " " . $_POST ["F" . $x . "SHIFTY"];

        socket_write ($S, $cmd . "\n");
      }

      # Store layer fields for later.
      if (is_numeric ($_POST ["F" . $x . "LAYER"])) {
        $newlayer [$_POST ["F" . $x . "LAYER"]] = $id;
      }
      if (is_numeric ($_POST ["O" . $x . "LAYER"])) {
        $oldlayer [$_POST ["O" . $x . "LAYER"]] = $id;
      }
    }

    ksort ($newlayer);
    ksort ($oldlayer);
    $newstack = implode (" ", $newlayer);
    $oldstack = implode (" ", $oldlayer);
    if ($newstack != $oldstack) {
      socket_write ($S, "stack " . $newstack . "\n");
    }
  }

  # Check if the layout is to be saved as a profile.
  for ($no = 1; isset ($_POST ["PRESET" . $no]); $no++) {
    if ($_POST ["PRESET" . $no] != "" && $_POST ["PRESET" . $no . "SAVE"] != "") {
      # Save it.
      $PRE = fopen ("mixer-presets/mixer-" . $mixer . "-preset-" . $no . ".cfg", "w+");
      fwrite ($PRE, $_POST ["PRESET" . $no] . "\n");

      $newlayer = array ();
      for ($x = 1; isset ($_POST ["F" . $x . "ID"]); $x++) {
        $id = 1 * $_POST ["F" . $x . "ID"];

        fwrite ($PRE, "feed cutout " . $id .
                      " " . $_POST ["F" . $x . "CUTSTARTX"] .
                      " " . $_POST ["F" . $x . "CUTSTARTY"] .
                      " " . $_POST ["F" . $x . "CUTSIZEX"] .
                      " " . $_POST ["F" . $x . "CUTSIZEY"] .
                      "\n");

        fwrite ($PRE, "feed shift " . $id .
                      " " . $_POST ["F" . $x . "SHIFTX"] .
                      " " . $_POST ["F" . $x . "SHIFTY"] .
                      "\n");

        fwrite ($PRE, "feed scale " . $id .
                      " " . $_POST ["F" . $x . "SCALE1"] .
                      " " . $_POST ["F" . $x . "SCALE2"] .
                      "\n");

        # Store layer fields for later.
        if (is_numeric ($_POST ["F" . $x . "LAYER"])) {
          $newlayer [$_POST ["F" . $x . "LAYER"]] = $id;
        }
      }
    }

    ksort ($newlayer);
    $newstack = implode (" ", $newlayer);
    fwrite ($PRE, "stack " . $newstack . "\n");
    fclose ($PRE);
  }
}

# Load preset data.
$presets = array ();
for ($no = 1; ; $no++) {
  $PRE = fopen ("mixer-presets/mixer-" . $mixer . "-preset-" . $no . ".cfg", "r");
  if ($PRE == FALSE) break;

  $n = trim (fgets ($PRE));	# First the name.

  # The rest of the file contains command statements.
  $c = "";
  while (($l = fgets ($PRE)) != FALSE) {
    $c .= $l;
  }

  $presets [$no] = array ("NAME" => $n, "COMMAND" => $c);
  fclose ($PRE);
}
# Add a blank entry last.
$presets [$no] = array ("NAME" => "");


$current_preset = -1;
if (count ($_POST)) {
  for ($no = 1; $no <= count ($presets); $no++) {
    if ($_POST ["PRESET" . $no . "ACTIVATE"] != "") {
      # Activate this preset.
      socket_write ($S, $presets [$no]["COMMAND"]);
      $current_preset = $no;
      break;
    }
  }
}


# Bed om status.
socket_write ($S, "stat\n");

# Hent resultatet.
$feedlist = array ();
$feeds = 0;
while (1) {
  $l = socket_read ($S, 999, PHP_NORMAL_READ);
  if ($l === FALSE) break;			# EOF.
  if (substr ($l, 0, 5) != "STAT:") continue;	# Ikke en status besked.
  if (substr ($l, 0, 8) == "STAT: --") break;	# Ikke mere data.
  if (substr ($l, 0, 8) == "STAT: ID") continue;# Skip header linjen.

  # Opsplit og formater resultatet.
  $n = array ();
  $n ['ID']        = trim (substr ($l,  6,  2));
  $n ['NAME']      = trim (substr ($l,  9, 20));
  $n ['STATE']     = trim (substr ($l, 32, 12));
  $n ['MODE']      = trim (substr ($l, 46, 10));
  $n ['CUTSTARTX'] = trim (substr ($l, 58,  4));
  $n ['CUTSTARTY'] = trim (substr ($l, 63,  4));
  $n ['CUTSIZEX']  = trim (substr ($l, 69,  4));
  $n ['CUTSIZEY']  = trim (substr ($l, 74,  4));
  $n ['SHIFTX']    = trim (substr ($l, 80,  4));
  $n ['SHIFTY']    = trim (substr ($l, 85,  4));
  $n ['SCALE1']    = trim (substr ($l, 92,  1));
  $n ['SCALE2']    = trim (substr ($l, 94,  1));
  $n ['LAYER']     = trim (substr ($l, 97,  2));

  $feeds++;
  $feedlist [$feeds] = $n;
}
socket_close ($S);


# Opbyg web siden.

?>
<html>
  <head>
    <title>Snowmix Video Mixer Control</title>
  </head>

  <body>
    <table border=2>
      <form name="form" action="mixer-<?php echo $mixer; ?>.php" method="post">
      <tr>
        <td>
        </td>
      </tr>
      <tr>
        <td>Mix</td>
        <td><input type="submit" name="feed" value="4+123"></td>
        <td><input type="submit" name="feed" value="1+423"></td>
        <td><input type="submit" name="feed" value="2+143"></td>
        <td><input type="submit" name="feed" value="3+124"></td>
      </tr>
      <tr>
        <td>Move Right</td>
        <td><input type="submit" name="right" value="123"></td>
        <td><input type="submit" name="right" value="423"></td>
        <td><input type="submit" name="right" value="143"></td>
        <td><input type="submit" name="right" value="124"></td>
      </tr>
      <tr>
        <td>Move Left</td>
        <td><input type="submit" name="left" value="123"></td>
        <td><input type="submit" name="left" value="423"></td>
        <td><input type="submit" name="left" value="143"></td>
        <td><input type="submit" name="left" value="124"></td>
      </tr>
      <tr>
        <td>Swop 4 to n </td>
        <td></td>
        <td><input type="submit" name="swop" value="4+1"></td>
        <td><input type="submit" name="swop" value="4+2"></td>
        <td><input type="submit" name="swop" value="4+3"></td>
      </tr>
      <tr>
        <td>Swop to 4 </td>
        <td><input type="submit" name="swop" value="1+4"></td>
        <td></td>
        <td><input type="submit" name="swop" value="2+4"></td>
        <td><input type="submit" name="swop" value="3+4"></td>
      </tr>
      </form>
    </table>
    <form name="form" action="mixer-<?php echo $mixer; ?>.php" method="post">
      <table border="4" width=500px>
        <tr>
          <td><?php echo $mixername; ?></td>
        </tr>
        <tr>
          <td>
            <table border="1" width=900>
              <tr>
                <td colspan="4" align="center">Feed</td>
                <td>&nbsp;</td>
                <td colspan="2">Cut Start</td>
                <td>&nbsp;</td>
                <td colspan="2">Cut Size</td>
                <td>&nbsp;</td>
                <td colspan="2">Offset</td>
                <td>&nbsp;</td>
                <td>Scale</td>
                <td>&nbsp;</td>
                <td>Layer</td>
              </tr>
              <tr>
                <td style="width:20px;">ID</td>
                <td style="width:120px;">Name</td>
                <td style="width:80px;">State</td>
                <td style="width:80px;">Mode</td>
                <td>&nbsp;</td>
                <td>Col</td>
                <td>Row</td>
                <td>&nbsp;</td>
                <td>Col</td>
                <td>Row</td>
                <td>&nbsp;</td>
                <td>Col</td>
                <td>Row</td>
                <td>&nbsp;</td>
                <td>&nbsp;</td>
                <td>&nbsp;</td>
                <td>&nbsp;</td>
              </tr>
<?php
# Udfyld resten af tabellen.
for ($key = 1; $key <= $feeds; $key++) {
  $val = $feedlist [$key];
  printf ("              <tr>\n");
  printf ("                <td>\n");
  printf ("                  %d\n", $val ['ID']);
  printf ("                  <input type=\"hidden\" name=\"F%dID\" value=\"%d\" />\n", $key, $val ['ID']);
  printf ("                </td>\n");
  printf ("                <td>%s</td>\n", $val ['NAME']);
  printf ("                <td>%s</td>\n", $val ['STATE']);
  printf ("                <td>%s</td>\n", $val ['MODE']);
  printf ("                <td>&nbsp;</td>\n");
  printf ("                <td>\n");
  printf ("                  <input type=\"text\"   name=\"F%dCUTSTARTX\" size=\"5\" value=\"%d\" />\n", $key, $val ['CUTSTARTX']);
  printf ("                  <input type=\"hidden\" name=\"O%dCUTSTARTX\" size=\"5\" value=\"%d\" />\n", $key, $val ['CUTSTARTX']);
  printf ("                </td>\n");
  printf ("                <td>\n");
  printf ("                  <input type=\"text\"   name=\"F%dCUTSTARTY\" size=\"5\" value=\"%d\" />\n", $key, $val ['CUTSTARTY']);
  printf ("                  <input type=\"hidden\" name=\"O%dCUTSTARTY\" size=\"5\" value=\"%d\" />\n", $key, $val ['CUTSTARTY']);
  printf ("                </td>\n");
  printf ("                <td>&nbsp;</td>\n");
  printf ("                <td>\n");
  printf ("                  <input type=\"text\"   name=\"F%dCUTSIZEX\"  size=\"5\" value=\"%d\" />\n", $key, $val ['CUTSIZEX']);
  printf ("                  <input type=\"hidden\" name=\"O%dCUTSIZEX\"  size=\"5\" value=\"%d\" />\n", $key, $val ['CUTSIZEX']);
  printf ("                </td>\n");
  printf ("                <td>\n");
  printf ("                  <input type=\"text\"   name=\"F%dCUTSIZEY\"  size=\"5\" value=\"%d\" />\n", $key, $val ['CUTSIZEY']);
  printf ("                  <input type=\"hidden\" name=\"O%dCUTSIZEY\"  size=\"5\" value=\"%d\" />\n", $key, $val ['CUTSIZEY']);
  printf ("                </td>\n");
  printf ("                <td>&nbsp;</td>\n");
  printf ("                <td>\n");
  printf ("                  <input type=\"text\"   name=\"F%dSHIFTX\"    size=\"5\" value=\"%d\" />\n", $key, $val ['SHIFTX']);
  printf ("                  <input type=\"hidden\" name=\"O%dSHIFTX\"    size=\"5\" value=\"%d\" />\n", $key, $val ['SHIFTX']);
  printf ("                </td>\n");
  printf ("                <td>\n");
  printf ("                  <input type=\"text\"   name=\"F%dSHIFTY\"    size=\"5\" value=\"%d\" />\n", $key, $val ['SHIFTY']);
  printf ("                  <input type=\"hidden\" name=\"O%dSHIFTY\"    size=\"5\" value=\"%d\" />\n", $key, $val ['SHIFTY']);
  printf ("                </td>\n");
  printf ("                <td>&nbsp;</td>\n");
  printf ("                <td>\n");
  printf ("                  <input type=\"text\"   name=\"F%dSCALE1\"    size=\"1\" value=\"%s\" />\n", $key, $val ['SCALE1']);
  printf ("                  <input type=\"hidden\" name=\"O%dSCALE1\"    size=\"1\" value=\"%s\" />\n", $key, $val ['SCALE1']);
  printf ("                  <input type=\"text\"   name=\"F%dSCALE2\"    size=\"1\" value=\"%s\" />\n", $key, $val ['SCALE2']);
  printf ("                  <input type=\"hidden\" name=\"O%dSCALE2\"    size=\"1\" value=\"%s\" />\n", $key, $val ['SCALE2']);
  printf ("                </td>\n");
  printf ("                <td>&nbsp;</td>\n");
  printf ("                <td>\n");
  printf ("                  <input type=\"text\"   name=\"F%dLAYER\"    size=\"2\" value=\"%s\" />\n", $key, $val ['LAYER']);
  printf ("                  <input type=\"hidden\" name=\"O%dLAYER\"    size=\"2\" value=\"%s\" />\n", $key, $val ['LAYER']);
  printf ("                </td>\n");
  printf ("              </tr>\n");
}

?>
              <tr>
                <td colspan="15">&nbsp;</td>
              </tr>
              <tr>
                <td colspan="15">
                  <input type="submit" name="set"    value="Activate above values"/>
                  <input type="submit" name="reload" value="Load from mixer"/>
                </td>
              </tr>
            </table>
          </td>
        </tr>
        <tr>
          <td>
            <table border="0">
              <tr>
                <td>&nbsp;</td>
                <td>Preset Navn</td>
                <td>&nbsp;</td>
              </tr>
<?php for ($no = 1; $no <= count ($presets); $no++) {
  if ($current_preset == $no) {
    $style = "style=\"background-color: #00FF00;\"";
  } else {
    $style = "";
  }
  printf ("              <tr>\n");
  printf ("                <td><input type=\"submit\" name=\"PRESET%dSAVE\"     size=\"10\" %s value=\"Save\"/></td>\n", $no, $style);
  printf ("                <td><input type=\"text\"   name=\"PRESET%d\"         size=\"70\" %s value=\"%s\"/></td>\n", $no, $style, $presets [$no]['NAME']);
  if ($presets [$no]['NAME'] != "") {
    printf ("                <td><input type=\"submit\" name=\"PRESET%dACTIVATE\" size=\"10\" %s value=\"Load from file\"/></td>\n", $no, $style);
  } else {
    printf ("                <td>&nbsp;</td>\n");
  }
  printf ("              </tr>\n");
} ?>
            </table>
          </td>
        </tr>
      </table>
    </form>
  </body>
</html>
