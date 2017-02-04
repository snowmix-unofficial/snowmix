set snowmix_lock 0

proc SnowmixLock { id } {
  global snowmix_lock
  if {$snowmix_lock != 0} {
    vwait snowmix_lock
  }
  set snowmix_lock $id
}

proc SnowmixUnlock { id } {
  global snowmix_lock
  if {$snowmix_lock == $id} {
    set snowmix_lock 0
  } else {
    puts "WARNING : Tried to unlock snowmix for $id, but lock was $snowmix_lock"
  }
}

proc SetComboBox { cbox value } {
  set entry_id 0
#puts "SetComboBox $cbox <$value>"
  if {[string match "-*" $value]} { set value "-" }
  foreach entry [$cbox cget -values] {
    if {[string match $value $entry]} {
      $cbox setvalue @$entry_id
      break
    }
    incr entry_id
  }
}


proc LoadSnowmixInfo { snowmix types } {
  global text_strings text_fonts image_loaded

#puts "================================================"
  foreach type $types {
#puts "TYPE=$type"
    if {[string match text $type]} {
      set command "text string"
      set var "text_strings"
    } elseif {[string match font $type]} {
      set command "text font"
      set var text_fonts
    } elseif {[string match image $type]} {
      set command "image load"
      set var image_loaded
    } elseif {[string match textplaced $type]} {
      set command "text info"
      set var text_placed
    }
  
    upvar $var arrayvar
    # Get text strings
    SnowmixLock snowmixinfo1
    puts $snowmix $command
    if {[string match text $type] || [string match font $type] || [string match image $type]} {
      while {[gets $snowmix line] >= 0} {
#puts "GOT<$line>"
        if {[string compare "STAT: " $line] == 0} {
	    break
        }
        if {[string compare "MSG:" $line] == 0} {
	    break
        }
        set id -1
        regexp {[^0-9]+([0-9]+)\ <(.*)>$} $line match id text
        if { $id > -1 } {
          set arrayvar($id) $text
        }
      }
    } elseif {[string match textplaced $type]} {
      while {[gets $snowmix line] >= 0} {
        if {[string compare "STAT: " $line] == 0} {
	    break
        }
        if {[string compare "MSG:" $line] == 0} {
	    break
        }
        set id -1
      }
      # STAT:  text place id : pos offset wxh align anchor scale rot col clip pad colbg clipbg
      #regexp {[^0-9]+([0-9]+)\ +:\ +([0-9]+)\ ([0-9]+)\ ([\+\-0-9]+)\,([\+\-0-9]+)\ +([0-9]+)\ ([0-9]+)\ } $line match id x  y 
  
    }
  }
  SnowmixUnlock snowmixinfo1
}
