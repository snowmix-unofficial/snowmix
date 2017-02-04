#
# Copyright Peter Maersk-Moller (C) 2012-2016
# All Rights Reserved.
#

proc GetSystemStatusFormat { handle tag host port line args } {
  global output

  set output(systemstatus,format,$host,$port) [lrange $line 1 end]
  return 0
}

proc GetSystemStatus { handle tag host port line args } {
  global output

  set i 1
  foreach valname $output(systemstatus,format,$host,$port) {
    set output(systemstatus,$valname,$host,$port) [lindex $line $i]
    incr i
  }
  set mixer_duration [expr 1000 * $output(systemstatus,mixer_duration,$host,$port)]
  AddCoors $output(graph,$host,$port) 6 $output(makeline_n,$host,$port) $mixer_duration
  set output(mixer_duration,$host,$port) "[format "%.1f" $mixer_duration] ms"
  return 0
}


proc GetOutputStatus { handle tag host port line args } {
  global output

  if {[string match "STAT: " $line]} {
    incr output(makeline_n,$host,$port)
    after 550 [list AddServerCommand snowoutput $host $port "tcl eval snowmix message \[snowmix info system status]" GetSystemStatus 1000]
    after 600 [list AddServerCommand snowoutput $host $port "system output status" GetOutputStatus 1000]
    return 0
  } elseif {[regexp {STAT:\ +Inuse[^=]+=\ +([0-9]+)\ +of\ +([0-9]+)} $line match inuse total]} {
    set output(total,$host,$port) $total
    set output(inuse,$host,$port) $inuse
    AddCoors $output(graph,$host,$port) 1 $output(makeline_n,$host,$port) $inuse
    set height [$output(canvas_fr2,$host,$port) cget -height]

    catch { $output(canvas_fr2,$host,$port) delete inuse }
    set x1 5
    set y1 $height
    set x2 15
    set y2 [expr ($total - $inuse) * $height / $total]
    if { $output(no_free,$host,$port) == "yes" } { set fill #FF0000 } else { set fill yellow }
    $output(canvas_fr2,$host,$port) create rect $x1 $y1 $x2 $y2 -outline yellow -fill $fill  -tag inuse
  } elseif {[regexp {STAT:\ +Frame\ +info.+=\ +(.+)} $line match inuse_list]} {
    set height [$output(canvas_fr2,$host,$port) cget -height]
    catch { $output(canvas_fr2,$host,$port) delete inuse_list }
    set x1 20
    set x2 30
    set y1 $height
    foreach buf $inuse_list {
      set y2 [expr $y1 - 3]
      if { $buf == "00" } { set fill grey30
      } elseif { $buf == "01" } { set fill #00FF00
      } elseif { $buf == "02" } { set fill yellow
      } elseif { $buf == "03" } { set fill #FF0000
      } else { set fill red }
      $output(canvas_fr2,$host,$port) create rect $x1 $y1 $x2 $y2 -outline yellow -fill $fill -tag inuse_list -width 0
      set y1 [expr $y1 - 5]
    }
  } elseif {[regexp {STAT:\ +Frame\ +p\.\ +min[^=]+=\ +([\.0-9]+)\ +([\.0-9]+)\ +([\.0-9]+)\ +} $line match min ave max]} {
    set output(min,$host,$port) $min
    set output(ave,$host,$port) $ave
    set output(max,$host,$port) $max
    AddCoors $output(graph,$host,$port) 2 $output(makeline_n,$host,$port) $min
    AddCoors $output(graph,$host,$port) 3 $output(makeline_n,$host,$port) $ave
    AddCoors $output(graph,$host,$port) 4 $output(makeline_n,$host,$port) $max
  } elseif {[regexp {STAT:\ +Last\ +[^=]+=\ +(.+)} $line match last]} {
    set output(last,$host,$port) $last
    AddCoors $output(graph,$host,$port) 5 $output(makeline_n,$host,$port) [lindex $last 2]
  } elseif {[regexp {STAT:\ +Connection\ +[^=]+=\ +([0-9]+)} $line match con_no]} {
    set output(con,$host,$port) $con_no
  } elseif {[regexp {STAT:\ +Timed\ +frame[^=]+=\ +([0-9]+)} $line match tframe]} {
    set output(tframe,$host,$port) $tframe
  } elseif {[regexp {STAT:\ +Timed\ +loop[^=]+=\ +(.+)} $line match tloop]} {
    set output(tloop,$host,$port) $tloop
  } elseif {[regexp {STAT:\ +Copied\ +[^=]+=\ +([0-9]+)} $line match copied]} {
    set output(copied,$host,$port) $copied
  } elseif {[regexp {STAT:\ +Ready\ +[^=]+=\ +([0-9]+)} $line match ready]} {
    set output(ready,$host,$port) $ready
  } elseif {[regexp {STAT:\ +Repeated\ +[^=]+=\ +([0-9]+)} $line match repeat]} {
    set output(repeat,$host,$port) $repeat
  } elseif {[regexp {STAT:\ +Got\ +[^=]+=\ +([^\ ]+)} $line match no_free]} {
    set output(no_free,$host,$port) $no_free
    if { $no_free == "no"  } {
      $output(canvas_fr2,$host,$port) itemconfigure buffers -text "Free bufs : [expr $output(total,$host,$port) - $output(inuse,$host,$port)]" -fill green
    } else {
      $output(canvas_fr2,$host,$port) itemconfigure buffers -text "No Free Bufs" -fill #FF0000
    }
  }
  return 1
}

proc output_pane { pane host port } {
  global output system graph

  set bg [$pane cget -bg]
  #. configure -bg $bg
  ScrollableFrame $pane.scrollframe -constrainedwidth false -constrainedheight false \
    -xscrollcommand "$pane.scrollbar set" -width 1200 -height 408 -bg $bg
  scrollbar $pane.scrollbar -command "$pane.scrollframe xview" -orient horizontal
  pack $pane.scrollframe -side top -fill both -expand 1
  pack $pane.scrollbar -side top -fill x
  set panefr [$pane.scrollframe getframe]

  set topfr [frame $panefr.top -bd 0 -bg $bg]
  set midfr [frame $panefr.mid -bd 0 -bg $bg]
  set botfr [frame $panefr.bot -bd 0 -bg $bg]
  pack $topfr $midfr $botfr -side top -fill x -expand 1

  set time1fr [frame $topfr.time1 -bd 0 -bg $bg]
  set time2fr [frame $topfr.time2 -bd 0 -bg $bg]
  pack $time1fr $time2fr -side top -fill x -expand 1 -anchor nw
  set w 6
  Label $time1fr.con1 -text "Con #" -bg $bg -fg grey70 -width $w
  Label $time1fr.min1 -text Min -bg $bg -fg grey70 -width $w
  Label $time1fr.ave1 -text Ave -bg $bg -fg grey70 -width $w
  Label $time1fr.max1 -text Max -bg $bg -fg grey70 -width $w
  Label $time1fr.last1 -text "Last 3" -bg $bg -fg grey70 -width $w
  Label $time2fr.tframe1 -text "T.Frame" -bg $bg -fg grey70 -width $w
  Label $time2fr.tloop1 -text "T.loop" -bg $bg -fg grey70 -width $w
  Label $time2fr.copied1 -text "Copied" -bg $bg -fg grey70 -width $w
  Label $time2fr.ready1 -text "Ready" -bg $bg -fg grey70 -width $w
  Label $time2fr.repeat1 -text "Repeat" -bg $bg -fg grey70 -width $w
  Label $time2fr.duration1 -text "Mix.time" -bg $bg -fg grey70 -width $w
  Label $time1fr.con2 -textvariable output(con,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time1fr.min2 -textvariable output(min,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time1fr.ave2 -textvariable output(ave,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time1fr.max2 -textvariable output(max,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time1fr.last2 -textvariable output(last,$host,$port) -bg $bg -fg grey70 -width 15 -relief sunken
  Label $time2fr.tframe2 -textvariable output(tframe,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time2fr.tloop2 -textvariable output(tloop,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time2fr.copied2 -textvariable output(copied,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time2fr.ready2 -textvariable output(ready,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time2fr.repeat2 -textvariable output(repeat,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  Label $time2fr.duration2 -textvariable output(mixer_duration,$host,$port) -bg $bg -fg grey70 -width $w -relief sunken
  foreach name { con1 con2 min1 min2 ave1 ave2 max1 max2 last1 last2 } {
    pack $time1fr.$name -side left
  }
  foreach name { tframe1 tframe2 tloop1 tloop2 copied1 copied2 ready1 ready2 repeat1 repeat2 duration1 duration2} {
    pack $time2fr.$name -side left
  }
  set output(no_free,$host,$port) "no"
  set gfr [frame $midfr.graph1]
  pack $gfr -side left

  set height 250
  set output(graph,$host,$port) "$output(hostmap,$host):$port"
  set output(canvas_fr,$host,$port) [CreateGraph $gfr $output(graph,$host,$port) 650 $height 10 5 2]
  set output(canvas_fr2,$host,$port) [canvas $midfr.canvas2 -width 60 -height $height -bg black -bd 2 -relief sunken -highlightthickness 1 -highlightbackground darkgreen]
  $output(canvas_fr2,$host,$port) create text 5 5 -tag buffers -text buffers -fill grey70 -anchor nw -font mySmallFont
  
  pack $output(canvas_fr,$host,$port) $output(canvas_fr2,$host,$port) -side left

#  set bfr [frame $botfr.buttonframe -relief raised -bg black ]
#  pack $bfr -side top -anchor w
#  Button $bfr.slower -text Slower -armcommand "GraphSpeed $output(graph,$host,$port) -1" -pady 0
#  Button $bfr.faster -text Faster -armcommand "GraphSpeed $output(graph,$host,$port) 1" -pady 0
#  Button $bfr.start -text Start -armcommand "GraphStartStop $output(graph,$host,$port) 1" -pady 0
#  Button $bfr.stop -text Stop -armcommand "GraphStartStop $output(graph,$host,$port) 0" -pady 0
#  Label $bfr.speed -textvariable graph(speed,1) -padx 6 -pady 2 -relief sunken
#  Label $bfr.speedtext -text "Update" -padx 2 -pady 2 -width 7 -relief raised
#  pack $bfr.speedtext $bfr.slower $bfr.faster $bfr.start $bfr.stop $bfr.speed -side left -anchor w


  CreateLine $output(graph,$host,$port) 1 "In Use" white 50 10
  CreateLine $output(graph,$host,$port) 2 "Min" red 50 10
  CreateLine $output(graph,$host,$port) 3 "Avg" green 50 10
  CreateLine $output(graph,$host,$port) 4 "Max" yellow 50 10
  CreateLine $output(graph,$host,$port) 5 "Last" blue 50 10
  CreateLine $output(graph,$host,$port) 6 "Duration" cyan 50 10
  SetScaleLine $output(graph,$host,$port) 1 1 5
  SetScaleLine $output(graph,$host,$port) 2 1 5
  SetScaleLine $output(graph,$host,$port) 3 1 5
  SetScaleLine $output(graph,$host,$port) 4 1 5
  SetScaleLine $output(graph,$host,$port) 5 1 5
  SetScaleLine $output(graph,$host,$port) 6 1 5
  #SetOffsetLine $output(graph,$host,$port) 2 0 -700
  #SetOffsetLine $output(graph,$host,$port) 3 0 -700
  #SetOffsetLine $output(graph,$host,$port) 4 0 -700
  #SetOffsetLine $output(graph,$host,$port) 5 0 -700
  #SetOffsetLine $output(graph,$host,$port) 6 0 -700
  LineLegendY $output(graph,$host,$port) 1
  LineLegendY $output(graph,$host,$port) 2
  LineLegendY $output(graph,$host,$port) 3
  LineLegendY $output(graph,$host,$port) 4
  LineLegendY $output(graph,$host,$port) 5
  LineLegendY $output(graph,$host,$port) 6

#  foreach but {2 3 4 1} {
#    set bfrb [frame $botfr.buttonframe$but -relief raised]
#    pack $bfrb -side top -anchor w
#    Label $bfrb.label -text $graph(line_name,$output(graph,$host,$port),$but) -relief raised -pady 1 -width 7
#    Button $bfrb.up -text Offset+ -armcommand "OffsetLine $output(graph,$host,$port) $but 1" -pady 0
#    Button $bfrb.down -text Offset- -armcommand "OffsetLine $output(graph,$host,$port) $but 0" -pady 0
#    Button $bfrb.enlarge -text Enlarge -armcommand "ScaleLine $output(graph,$host,$port) $but 1" -pady 0
#    Button $bfrb.shrink -text Shrink -armcommand "ScaleLine $output(graph,$host,$port) $but 0" -pady 0
#    pack $bfrb.label $bfrb.up $bfrb.down $bfrb.enlarge $bfrb.shrink -side left -anchor w
#  }

  set output(makeline_n,$host,$port) 1
  set graph(running,1) 1
  set graph(stopped,1) 0
#  MakeLine 1
}

proc ServerSetup { host port } {
  global output led_image system

  if {[info exist output(graph,$host,$port)]} {
    puts stderr "Server $host $port already exist"
    return
  }

  # There can't be any '.' in a tab name
  set hostmap [string map {. _} $host]
  set output(hostmap,$host) $hostmap
  set output(host,$hostmap) $host

  if { [set handle [AddServer snowoutput $host $port]] == "" } {
    puts stderr "Failed to add server $host:$port."
  } else {
    puts "Gogt handle $handle"
  }

  $output(main_nb) insert end "$hostmap:$port" -text " Snowmix $host:$port" \
	-image $led_image(darkgrey,$system(led_size))

  $output(main_nb) raise "$hostmap:$port"

  set fr [$output(main_nb) getframe "$hostmap:$port"]
  output_pane $fr $host $port
  after 400 [list AddServerCommand snowoutput $host $port "tcl eval snowmix message \[snowmix info system status format]" GetSystemStatusFormat 1000]
  after 450 [list AddServerCommand snowoutput $host $port "tcl eval snowmix message \[snowmix info system status]" GetSystemStatus 1000]
  after 500 [list AddServerCommand snowoutput $host $port "system output status" GetOutputStatus 1000]
  return
}

proc SetupOutPutPane { pane notebook page } {
  global output

  set output(bgcolor)                    grey10
  set output(fgcolor)                    grey80
  set output(butbgcolor)                 grey30
  set output(butfgcolor)                 grey70

  set bg [$pane cget -bg]
  set output(bg) $bg
  set output(main_frame) $pane
  $pane configure -padx 0 -pady 0

  set output(main_nb) [NoteBook $pane.nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2 \
	-bg $output(butbgcolor) -fg $output(butfgcolor)  -activebackground grey40  -activeforeground black]
  pack $output(main_nb) -side top -fill both -anchor w -expand 1

#  ServerSetup vostok02 9999
#  ServerSetup stream01 9999
#  ServerSetup 127.0.0.1 9999

  bind . <Enter> "focus ."
  return
}

proc AddServerSetup { host port } { ServerSetup $host $port }


