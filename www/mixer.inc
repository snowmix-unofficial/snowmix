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

#printf ("%s\n%s\n", var_dump($_POST), $_POST["feed"]);
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
} else if ($_POST ["swop"] != "" ) {
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
} else if ($_POST ["right"] != "" ) {
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
} else if ($_POST ["left"] != "" ) {
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
} else if ($_POST ["hide"] != "" ) {
  if ($_POST ["hide"] == "logo" ) {
    $cmd = "image place alpha 0 0";
  }
  else if ($_POST ["hide"] == "rollertext" ) {
    $cmd = "text place backgr move clip 5 0 0 0.04 0 0 0 25 0";
  }
  else if ($_POST ["hide"] == "date" ) {
    $cmd = "text place backgr move clip 20 0 0 0.04 0 0 0 25 0";
  }
  else if ($_POST ["hide"] == "timing" ) {
    $cmd = "text place backgr move clip 6 0 0 0.04 0 0 0 25 0\n" .
    "text place backgr move clip 7 0 0 0.04 0 0 0 25 0\n" .
    "text place backgr move clip 8 0 0 0.04 0 0 0 25 0\n" .
    "text place backgr move clip 9 0 0 0.04 0 0 0 25 0";
  }
  else if ($_POST ["hide"] == "Head1" ) {
    $cmd = "text place backgr move clip 1 0 0 0.04 0 0 0 25 0";
  }
  else if ($_POST ["hide"] == "Head2" ) {
    $cmd = "text place backgr move clip 2 0 0 0.04 0 0 0 25 0";
  }
  else if ($_POST ["hide"] == "Head3" ) {
    $cmd = "text place backgr move clip 3 0 0 0.04 0 0 0 25 0";
  }
  else if ($_POST ["hide"] == "Head4" ) {
    $cmd = "text place backgr move clip 4 0 0 0.04 0 0 0 25 0";
  }
} else if ($_POST ["show"] != "" ) {
  if ($_POST ["show"] == "logo" ) {
    $cmd = "image place alpha 0 1";
  }
  else if ($_POST ["show"] == "rollertext" ) {
    $cmd = "text place backgr move clip 5 0 0 -0.04 0 0 0 25 0";
  }
  else if ($_POST ["show"] == "date" ) {
    $cmd = "text place backgr move clip 20 0 0 -0.04 0 0 0 25 0";
  }
  else if ($_POST ["show"] == "timing" ) {
    $cmd = "text place backgr move clip 6 0 0 -0.04 0 0 0 25 0\n" .
    "text place backgr move clip 7 0 0 -0.04 0 0 0 25 0\n" .
    "text place backgr move clip 8 0 0 -0.04 0 0 0 25 0\n" .
    "text place backgr move clip 9 0 0 -0.04 0 0 0 25 0";
  }
  else if ($_POST ["show"] == "Head1" ) {
    $cmd = "text place backgr move clip 1 0 0 -0.04 0 0 0 25 0";
  }
  else if ($_POST ["show"] == "Head2" ) {
    $cmd = "text place backgr move clip 2 0 0 -0.04 0 0 0 25 0";
  }
  else if ($_POST ["show"] == "Head3" ) {
    $cmd = "text place backgr move clip 3 0 0 -0.04 0 0 0 25 0";
  }
  else if ($_POST ["show"] == "Head4" ) {
    $cmd = "text place backgr move clip 4 0 0 -0.04 0 0 0 25 0";
  }
} else if ($_POST ["cover"] != "" ) {
  if ($_POST ["cover"] == "small1" ) {
    $cmd = "image place move alpha 1 0.02 50";
  }
  if ($_POST ["cover"] == "small2" ) {
    $cmd = "image place move alpha 2 0.02 50";
  }
  if ($_POST ["cover"] == "small3" ) {
    $cmd = "image place move alpha 3 0.02 50";
  }
} else if ($_POST ["uncover"] != "" ) {
  if ($_POST ["uncover"] == "small1" ) {
    $cmd = "image place move alpha 1 -0.02 50";
  }
  if ($_POST ["uncover"] == "small2" ) {
    $cmd = "image place move alpha 2 -0.02 50";
  }
  if ($_POST ["uncover"] == "small3" ) {
    $cmd = "image place move alpha 3 -0.02 50";
  }
} else if ($_POST ["counter"] != "" ) {
  if ($_POST ["counter"] == "Start" ) {
    $cmd = "command pop DigiCounterSeconds_Switch 10\n" .
	"command push DigiCounterSeconds_Switch DigiCounterSeconds\n" .
	"command push DigiCounterSeconds_Switch loop";
  }
  else if ($_POST ["counter"] == "Stop" ) {
    $cmd = "command pop DigiCounterSeconds_Switch 10";
  }
  else if ($_POST ["counter"] == "Set" ) {
    $hours = $_POST ["hours"];
    $minutes = $_POST ["minutes"];
    $seconds = $_POST ["seconds"];
    $cmd = "command pop DigiCounterTcl 10\n" .
	"command push DigiCounterTcl set a [SetDigiCounter " . $hours . " " . $minutes . " " . $seconds . "]\n" .
	"command push DigiCounterTcl return [format \"\\n%s\" \$a]\n" .
	"tcl exec DigiCounterTcl";
  } else if ($_POST ["counter"] == "Hide" ) {
    $cmd = "image place move alpha 21 -0.02 50\n" .
      "image place move alpha 22 -0.02 50\n" .
      "image place move alpha 23 -0.02 50\n" .
      "image place move alpha 24 -0.02 50\n" .
      "image place move alpha 25 -0.02 50\n" .
      "image place move alpha 26 -0.02 50\n" .
      "image place move alpha 27 -0.02 50\n" .
      "image place move alpha 28 -0.02 50";

  } else if ($_POST ["counter"] == "Show" ) {
    $cmd = "image place move alpha 21 0.02 50\n" .
      "image place move alpha 22 0.02 50\n" .
      "image place move alpha 23 0.02 50\n" .
      "image place move alpha 24 0.02 50\n" .
      "image place move alpha 25 0.02 50\n" .
      "image place move alpha 26 0.02 50\n" .
      "image place move alpha 27 0.02 50\n" .
      "image place move alpha 28 0.02 50";
  } else if ($_POST ["counter"] == "Up" ) {
    $cmd = "image place move coor 21 0 -4 0 48\n" .
      "image place move coor 22 0 -4 0 48\n" .
      "image place move coor 23 0 -4 0 48\n" .
      "image place move coor 24 0 -4 0 48\n" .
      "image place move coor 25 0 -4 0 48\n" .
      "image place move coor 26 0 -4 0 48\n" .
      "image place move coor 27 0 -4 0 48\n" .
      "image place move coor 28 0 -4 0 48";
  } else if ($_POST ["counter"] == "Down" ) {
    $cmd = "image place move coor 21 0 4 0 48\n" .
      "image place move coor 22 0 4 0 48\n" .
      "image place move coor 23 0 4 0 48\n" .
      "image place move coor 24 0 4 0 48\n" .
      "image place move coor 25 0 4 0 48\n" .
      "image place move coor 26 0 4 0 48\n" .
      "image place move coor 27 0 4 0 48\n" .
      "image place move coor 28 0 4 0 48";
  } else if ($_POST ["counter"] == "Position" ) {
    $cmd = "image place coor 21 950 480\n" .
      "image place coor 22 925 480\n" .
      "image place coor 23 907 480\n" .
      "image place coor 24 890 480\n" .
      "image place coor 25 865 480\n" .
      "image place coor 26 847 480\n" .
      "image place coor 27 830 480\n" .
      "image place coor 28 805 480";
  }
} else if ($_POST ["textset"] != "" ) {
  $cmd = "text string " . $_POST["number"] . " " . $_POST["string"];
}
if ($cmd != "") socket_write ($S, $cmd . "\n");


# Get text strings
socket_write ($S, "text string\n");
$textlist = array();
$textnumber = array();
$textcount = 0;
while (1) {
  $l = socket_read ($S, 999, PHP_NORMAL_READ);
  if ($l === FALSE) break;			# EOF.
  else if (substr ($l, 0, 16) == "MSG: text string") {
    $string_start = strpos ($l, "<");
    $string_stop = strlen($l);
    #printf( "%d %d <%s> <%s>\n", $string_start, $string_stop, substr($l, 17, $string_start - 18), substr($l, $string_start+1, $string_stop-$string_start-3));
    $textlist [$textcount] = substr($l, $string_start+1, $string_stop-$string_start-3);
    $textnumber [$textcount] = substr($l, 17, $string_start - 18);
    $textcount++; 

  } else break;
}

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
      <tr>
        <td>Hide</td>
        <td><input type="submit" name="hide" value="logo"></td>
        <td><input type="submit" name="hide" value="rollertext"></td>
        <td><input type="submit" name="hide" value="date"></td>
        <td><input type="submit" name="hide" value="timing"></td>
        <td>Show</td>
        <td><input type="submit" name="show" value="logo"></td>
        <td><input type="submit" name="show" value="rollertext"></td>
        <td><input type="submit" name="show" value="date"></td>
        <td><input type="submit" name="show" value="timing"></td>
      </tr>
      <tr>
        <td>Hide</td>
        <td><input type="submit" name="hide" value="Head1"></td>
        <td><input type="submit" name="hide" value="Head2"></td>
        <td><input type="submit" name="hide" value="Head3"></td>
        <td><input type="submit" name="hide" value="Head4"></td>
        <td>Show</td>
        <td><input type="submit" name="show" value="Head1"></td>
        <td><input type="submit" name="show" value="Head2"></td>
        <td><input type="submit" name="show" value="Head3"></td>
        <td><input type="submit" name="show" value="Head4"></td>
      </tr>
      <tr>
        <td>Cover</td>
        <td><input type="submit" name="cover" value="small1"></td>
        <td><input type="submit" name="cover" value="small2"></td>
        <td><input type="submit" name="cover" value="small3"></td>
        <td></td>
        <td>UnCover</td>
        <td><input type="submit" name="uncover" value="small1"></td>
        <td><input type="submit" name="uncover" value="small2"></td>
        <td><input type="submit" name="uncover" value="small3"></td>
        <td></td>
      </tr>
      <tr>
        <td>Counter</td>
        <td><input type="text" size=2 name="hours" value="99"></td>
        <td><input type="text" size=2 name="minutes" value="59"></td>
        <td><input type="text" size=2 name="seconds" value="59"></td>
        <td><input type="submit" name="counter" value="Set"></td>
        <td><input type="submit" name="counter" value="Start"></td>
        <td><input type="submit" name="counter" value="Stop"></td>
      </tr>
      <tr>
        <td>Counter</td>
        <td><input type="submit" name="counter" value="Hide"></td>
        <td><input type="submit" name="counter" value="Show"></td>
        <td><input type="submit" name="counter" value="Up"></td>
        <td><input type="submit" name="counter" value="Down"></td>
        <td><input type="submit" name="counter" value="Position"></td>
      </tr>
      </form>
    </table>
    <table border=2>
      <tr> <td colspan="3">Text Strings</td> </tr>
      <tr><td>Update</td><td>Number</td><td>String</td></tr>
<?php
  printf("      <form name=\"form\" action=\"mixer-%s.php\" method=\"post\">\n", $mixer);
  printf("      <tr><td><input type=\"submit\" name=\"textset\" value=\"Add\"></td>\n");
  printf("          <td><input type=\"text\" size=2 name=\"number\" value=\"\"></td>\n");
  printf("          <td><input type=\"text\" size=50 name=\"string\" value=\"\"></td>\n");
  printf("      </tr>\n      </form>\n");
$n = 0;
while ($n < $textcount) {
  printf("      <form name=\"form\" action=\"mixer-%s.php\" method=\"post\">\n", $mixer);
  printf("      <tr><td><input type=\"submit\" name=\"textset\" value=\"Update\"></td>\n");
  printf("          <td>%s</td>\n", $textnumber[$n]);
  printf("          <td><input type=\"text\" size=50 name=\"string\" value=\"%s\"></td>\n", $textlist[$n]);
  printf("          <td><input type=\"hidden\" size=50 name=\"number\" value=\"%s\"></td>\n", $textnumber[$n]);
  printf("      </tr>\n      </form>\n");
  $n++;
}
?>
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
