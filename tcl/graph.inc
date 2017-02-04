proc CreateGraph { frame graph_id width height xlines ylines border } {
  global graph

  # puts "CREATING GRAPH : $graph_id $width $height"
  set thisgraph [canvas $frame.canvas -width $width -height $height -bg black -bd $border -relief sunken -highlightthickness 1 -highlightbackground darkgreen]

  set i 1
  while { $i < $xlines } {
    set x [expr $width * $i / $xlines]
    $thisgraph create line $x 0 $x $height -fill darkgreen -width 1 -tag raster
    incr i
  }
  set i 1
  while { $i < $ylines } {
    set y [expr $height * $i / $ylines]
    $thisgraph create line 0 $y $width $y -fill darkgreen -width 1 -tag raster
    incr i
  }
  set graph(canvas,$graph_id) $thisgraph
  set graph(width,$graph_id) $width
  set graph(height,$graph_id) $height
  set graph(rastertag,$graph_id) raster
  set graph(lines,$graph_id) ""
  return $thisgraph
}

#$canvas create text $x $y -text "Frame $frame_id" -fill black -tag frame_$frame_id
# [expr -$y1 * $graph(scale_y,$graph_id,$line_id) - $graph(offset_y,$graph_id,$line_id)]
proc LineLegendY {graph_id line_id} {
  global graph
  catch {
    $graph(canvas,$graph_id) delete ttext$graph(line_tag,$graph_id,$line_id)
    $graph(canvas,$graph_id) delete btext$graph(line_tag,$graph_id,$line_id)
  }
#puts "scale $graph(scale_y,$graph_id,$line_id) offset $graph(offset_y,$graph_id,$line_id)"
  set bval [expr -$graph(offset_y,$graph_id,$line_id)/$graph(scale_y,$graph_id,$line_id)]
  set tval [expr $bval + $graph(height,$graph_id)/$graph(scale_y,$graph_id,$line_id)]
#puts "Legend $bval  $tval"
  set t_ext ""
  set b_ext ""
  if {$tval >= 1000} {
    if {$tval >= 1000000} { set t_ext M ; set div 1000000
    } else { set t_ext k ; set div 1000 }
    set tval [expr $tval / $div]
  }
  if {$bval >= 1000} {
    if {$bval >= 1000000} { set b_ext M ; set div 1000000
    } else { set b_ext k ; set div 1000 }
    set bval [expr $bval / $div]
  }

  set ttext [format "%.1f%s" $tval $t_ext]
  set btext [format "%.1f%s" $bval $b_ext]
  set x 10
  set y [expr $graph(height,$graph_id) - 5 - ($line_id - 1) * 15]
  $graph(canvas,$graph_id) create text $x $y -text $btext -fill $graph(line_color,$graph_id,$line_id) -tag btext$graph(line_tag,$graph_id,$line_id) -anchor w
  set y [expr 15 + ($line_id - 1) * 15]
  $graph(canvas,$graph_id) create text $x $y -text $ttext -fill $graph(line_color,$graph_id,$line_id) -tag btext$graph(line_tag,$graph_id,$line_id) -anchor w
}

proc CreateLine { graph_id line_id line_name line_color max_coors delta_x} {
  global graph
#puts stderr "CreateLine $graph_id $line_id"
  lappend graph(lines,$graph_id) $line_id
  set graph(line_name,$graph_id,$line_id) $line_name
  set graph(line_color,$graph_id,$line_id) $line_color
  set graph(max_coors,$graph_id,$line_id) $max_coors
  set graph(num_coors,$graph_id,$line_id) 0
#puts stderr " - DEFINING graph(num_coors,$graph_id,$line_id)"
  set graph(width,$graph_id,$line_id) 1
  set graph(delta_x,$graph_id,$line_id) $delta_x
  set graph(delta_y,$graph_id,$line_id) 0
  set graph(scale_x,$graph_id,$line_id) 1
  set graph(scale_y,$graph_id,$line_id) 1
  set graph(offset_x,$graph_id,$line_id) 0
  set graph(offset_y,$graph_id,$line_id) 0
  set graph(origin_x,$graph_id,$line_id) [expr $graph(width,$graph_id) - 10]
  set graph(origin_y,$graph_id,$line_id) $graph(height,$graph_id)
  set graph(line_tag,$graph_id,$line_id) line$line_id
  set graph(width,$graph_id,$line_id) 1
  set graph(speed,$graph_id) 500
  #lappend graph(coords,$graph_id,$line_id) ""
}

proc SetScaleLine { graph_id line_id scale_x scale_y} {
  global graph
  set graph(scale_x,$graph_id,$line_id) $scale_x
  set graph(scale_y,$graph_id,$line_id) $scale_y
}

proc GetScaleLine { graph_id line_id scale_x scale_y} {
  global graph
  return "$graph(scale_x,$graph_id,$line_id) $graph(scale_y,$graph_id,$line_id)"
}

proc SetOffsetLine { graph_id line_id offset_x offset_y} {
  global graph
  set graph(offset_x,$graph_id,$line_id) $offset_x
  set graph(offset_y,$graph_id,$line_id) $offset_y
}


proc GraphLineWidth { graph_id line_id width } {
  global graph
  if {[info exist graph(line_tag,$graph_id,$line_id)]} {
    set graph(width,$graph_id,$line_id) $width
    $graph(canvas,$graph_id) itemconfigure $graph(line_tag,$graph_id,$line_id) -width $width
  }
}

proc AddCoors { graph_id line_id x1 y1 } {
  global graph
  if {$graph(num_coors,$graph_id,$line_id) >= $graph(max_coors,$graph_id,$line_id)} {
    set graph(coords,$graph_id,$line_id) [lreplace $graph(coords,$graph_id,$line_id) 0 1]
    set graph(line_coords,$graph_id,$line_id) [lreplace $graph(line_coords,$graph_id,$line_id) 0 1]
  } else {
    incr graph(num_coors,$graph_id,$line_id)
  }
  lappend graph(coords,$graph_id,$line_id) $x1 $y1

  set x [expr $x1 * $graph(delta_x,$graph_id,$line_id) - $graph(offset_x,$graph_id,$line_id)]
  set y [expr -$y1 * $graph(scale_y,$graph_id,$line_id) - $graph(offset_y,$graph_id,$line_id)]

  lappend graph(line_coords,$graph_id,$line_id) $x $y

  if {$graph(num_coors,$graph_id,$line_id) > 2} {
    $graph(canvas,$graph_id) delete $graph(line_tag,$graph_id,$line_id)
  }
  if {$graph(num_coors,$graph_id,$line_id) > 1} {
    $graph(canvas,$graph_id) create line $graph(line_coords,$graph_id,$line_id) \
      -fill $graph(line_color,$graph_id,$line_id) -width $graph(width,$graph_id,$line_id) \
	-tag $graph(line_tag,$graph_id,$line_id)
    $graph(canvas,$graph_id) move $graph(line_tag,$graph_id,$line_id) \
      $graph(origin_x,$graph_id,$line_id) $graph(origin_y,$graph_id,$line_id)
  }
  #puts "offset $graph(origin_x,$graph_id,$line_id),$graph(origin_y,$graph_id,$line_id) line [llength $graph(line_coords,$graph_id,$line_id)] $graph(line_coords,$graph_id,$line_id)"
  #puts "Line $graph(num_coors,$graph_id,$line_id) of $graph(max_coors,$graph_id,$line_id) [llength $graph(coords,$graph_id,$line_id)]: $graph(coords,$graph_id,$line_id)"
  set graph(origin_x,$graph_id,$line_id) [expr $graph(origin_x,$graph_id,$line_id) - $graph(delta_x,$graph_id,$line_id)]
  set graph(origin_y,$graph_id,$line_id) [expr $graph(origin_y,$graph_id,$line_id) - $graph(delta_y,$graph_id,$line_id)]
  catch { $graph(canvas,$graph_id) lower $graph(line_tag,$graph_id,$line_id) label }
}


proc ScaleLine {graph_id line_id scale} {
  global graph
  if {$scale} {
    set graph(scale_y,$graph_id,$line_id) [expr 1.25 * $graph(scale_y,$graph_id,$line_id)]
  } else {
    set graph(scale_y,$graph_id,$line_id) [expr 0.80 * $graph(scale_y,$graph_id,$line_id)]
  }
  LineLegendY $graph_id $line_id
  puts "Offset $graph(offset_y,$graph_id,$line_id) scale $graph(scale_y,$graph_id,$line_id)"
}

proc OffsetLine {graph_id line_id offset} {
  global graph
  if {$offset} {
    set graph(offset_y,$graph_id,$line_id) [expr $graph(offset_y,$graph_id,$line_id) + $graph(height,$graph_id)/2]
  } else {
    set graph(offset_y,$graph_id,$line_id) [expr $graph(offset_y,$graph_id,$line_id) - $graph(height,$graph_id)/2]
  }
  LineLegendY $graph_id $line_id
  puts "Offset $graph(offset_y,$graph_id,$line_id) scale $graph(scale_y,$graph_id,$line_id)"
}


proc GraphSpeed {graph_id speed} {
  global graph
puts "GRAPH SPEED $graph_id $speed"
  if {$speed < 0 } { set graph(speed,$graph_id) [expr 2*$graph(speed,$graph_id)]
  } elseif {$speed > 0 } { set graph(speed,$graph_id) [expr $graph(speed,$graph_id)/2]
  }
}

proc GraphStartStop { graph_id status } {
  global graph
  if { $status } {
    if {$graph(stopped,$graph_id)} {
      set graph(running,$graph_id) 1
      set graph(stopped,$graph_id) 0
      MakeLine $graph_id
    } else {
      set graph(running,$graph_id) 1
    }
  } else {
    set graph(running,$graph_id) 0
  }
}

proc MakeLine {graph_id} {
  global snowmix makeline_n graph
  if {!$graph(running,$graph_id)} {
    set graph(stopped,$graph_id) 1
    return
  }
  set min 0
  set max 0
  set avg 0
  set inuse 0
  catch {[puts $snowmix "system output status"]} { return }
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +Inuse[^=]+=\ +([0-9]+)} $line match inuse]} {
      AddCoors $graph_id 1 $makeline_n $inuse
    } elseif {[regexp {STAT:\ +Frame\ +p\.\ +min[^=]+=\ +([\.0-9]+)\ +([\.0-9]+)\ +([\.0-9]+)\ +} $line match min avg max]} {
      AddCoors $graph_id 2 $makeline_n $min
      AddCoors $graph_id 3 $makeline_n $avg
      AddCoors $graph_id 4 $makeline_n $max
    }
  }
  incr makeline_n
  after $graph(speed,$graph_id) MakeLine $graph_id
}

set makeline_n 0
