<?php

header ("Content-type: video/MP4");
ob_end_flush ();	# Flush and end buffering.
set_time_limit (0);	# The video feed may run for quite some time. Don't abort it.

passthru ('(gst-launch-0.10 -v tcpclientsrc port=5010 ! mpegtsdemux ! qtmux ! filesink location=/dev/fd/3 buffer-mode=2 > /tmp/video.php.err 2>&1) 3>&1');

?>
