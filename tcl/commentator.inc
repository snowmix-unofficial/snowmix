
proc SetupCommentator { mainframe } {

  set but [Label $mainframe.button -text Commentators -font myVeryLargeFont -bg green -fg white \
	-relief raised -bd 8 -padx 10 -pady 20]
  bind $but <ButtonPress> "$but configure -relief sunken"
  bind $but <ButtonRelease> "$but configure -relief raised"
  
  pack $mainframe.button -side top
  
}
