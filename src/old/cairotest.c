// gcc -Wall -L/usr/lib/x86_64-linux-gnu cairotest.c -lcairo -o cairotest

#include <stdlib.h>
#include <stdio.h>
#include <cairo/cairo.h>
#include <sys/time.h>

int overlay_frame(cairo_t* pCr, u_int8_t* src, int width, int height, double scale_x, double scale_y) {
	double clip_w = width;
	double clip_h = height;
	if (!pCr || !src || !width || !height) return 1;
	cairo_surface_t* pSurface = cairo_image_surface_create_for_data(src,
		CAIRO_FORMAT_ARGB32, width, height, width*4);
        if (!pSurface) {
		fprintf(stderr, "Failed to allocate memory for frame\n");
		return -1;
	}

	cairo_save(pCr);
	  if (scale_x != 1.0 || scale_y != 1.0) cairo_scale(pCr, scale_x, scale_y);
	    cairo_rectangle(pCr, 0.0, 0.0, clip_w, clip_h);
	    cairo_clip(pCr);
	    cairo_new_path(pCr);
	    cairo_set_source_surface(pCr, pSurface, 0.0, 0.0);
	    //cairo_pattern_set_filter (cairo_get_source (pCr), CAIRO_FILTER_FAST); //0.7-0.9 sec
	    //cairo_pattern_set_filter (cairo_get_source (pCr), CAIRO_FILTER_GOOD); // 38 sec
	    //cairo_pattern_set_filter (cairo_get_source (pCr), CAIRO_FILTER_BEST); // 38 sec
	    //cairo_pattern_set_filter (cairo_get_source (pCr), CAIRO_FILTER_NEAREST); // 0.65-0.8
	    cairo_pattern_set_filter (cairo_get_source (pCr), CAIRO_FILTER_BILINEAR); //
	    cairo_paint(pCr);
	  if (scale_x != 1.0 || scale_y != 1.0) cairo_scale(pCr, 1/scale_x, 1/scale_y);

	  cairo_reset_clip (pCr);
	cairo_restore(pCr);
	if (pSurface) cairo_surface_destroy(pSurface);
	return 0;
}

int main(int argc, char *argv[])
{
	int i = 0;
	int width =1280;
	int height = 720;
	struct timeval start, stop, time;
	u_int8_t* mixerframe = (u_int8_t*)malloc(width*height*4);
	u_int8_t* srcframe = (u_int8_t*)malloc(width*height*4);
	if (!mixerframe || !srcframe) {
		fprintf(stderr, "Failed to allocate memory for frame\n");
		exit (1);
	}
	cairo_surface_t* pSurface = cairo_image_surface_create_for_data(
		mixerframe, CAIRO_FORMAT_ARGB32, width, height, width*4);
	if (!pSurface) {
		fprintf(stderr, "Failed to allocate cairo surface\n");
		exit (1);
	}
	cairo_t* pCr = cairo_create(pSurface);
	if (!pCr) {
		fprintf(stderr, "Failed to allocate cairo context\n");
		exit (1);
	}

	gettimeofday(&start,NULL);
	for (i=0 ; i < 1000; i++) overlay_frame(pCr, srcframe, width, height, 0.5, 0.5);

	gettimeofday(&stop,NULL);
	if (pCr) cairo_destroy(pCr);
	if (pSurface) cairo_surface_destroy(pSurface);
	timersub(&stop,&start,&time);
	fprintf(stdout, "Time spent : %ld.%06ld\n", time.tv_sec, time.tv_usec);
	return 0;
}

