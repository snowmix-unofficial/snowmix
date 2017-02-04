/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Scott            pscott@mail.dk
 *		 Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */



#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
//#include <stdarg.h>
#ifdef WIN32
#include <winsock.h>
#include <direct.h>
#include "windows_util.h"
#else
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <glib.h>
#ifdef WITH_GUI
#include <gtk/gtk.h>
#endif


#include "snowmix.h"
#include "video_mixer.h"
#include "video_output.h"
#include "controller.h"

/* System wide globals. */
int		exit_program		= 0;

/********************************************************************************
*										*
*	Stuff for handling gstreamer shmsink/shmsrc data.			*
*										*
********************************************************************************/


// static void dump_cb (u_int32_t block_size, const char *src, const char *dst, struct shm_commandbuf_type *cb, size_t len)
void dump_cb (u_int32_t block_size, const char *src, const char *dst, struct shm_commandbuf_type *cb, size_t len)
{
	printf ("%s => %s %d bytes: ", src, dst, (int)len);
	switch (cb->type) {
		case 1:	{	/* COMMAND_NEW_SHM_AREA. */
			unsigned int	x;
			char	*c = ((char *)cb) + sizeof (*cb);

			printf ("New SHM Area. ID: %d size: %d  name_len: %d  name: ",
				cb->area_id,
				(int)cb->payload.new_shm_area.size,
				cb->payload.new_shm_area.path_size);
			for (x = 0; x < cb->payload.new_shm_area.path_size; x++, c++) {
				printf ("%c", *c);
			}
			printf ("\n");
			break;
		}

		case 2:		/* COMMAND_CLOSE_SHM_AREA. */
			printf ("Close SHM Area. ID: %d\n", cb->area_id);
			break;

		case 3:		/* COMMAND_NEW_BUFFER. */
			printf ("New Buffer. ID: %d  block: %4lu  offset: %9lu  size: %9lu\n",
				cb->area_id,
				cb->payload.buffer.offset/block_size,
				cb->payload.buffer.offset,
				cb->payload.buffer.bsize);
			break;

		case 4:		/* COMMAND_ACK_BUFFER. */
			printf ("ACK Buffer. ID: %d  block: %4lu  offset: %9lu\n",
				cb->area_id,
				cb->payload.ack_buffer.offset/block_size,
				cb->payload.ack_buffer.offset);
			break;

		default:
			printf ("Unknown packet. Code: %d\n", cb->type);
			break;
	}
}



// PMM. To be developed
// Must block for signals and clean up after it and exit nicely unlinking shared memory
int exit_function() {
	fprintf(stderr,"ON Exit nicely\n");
	return 0;
}

#ifdef WITH_GUI
static GtkWidget* CreateMainWindow()
{
	GtkWidget*	MainWindow = NULL;
	GtkWidget*	vbox1;
	GtkWidget*	menubar1;
	GtkWidget*	menuitem1;
	GtkWidget*	menuitem1_menu;
	GtkWidget*	new1;
	GtkWidget*	save1;
	GtkWidget*	open1;
	GtkWidget*	separatormenuitem1;
	GtkWidget*	quit1;
	GtkWidget*	menuitem2;
	GtkWidget*	menuitem2_menu;

	GtkAccelGroup*	accel_group;

	//tooltips = gtk_tooltips_new();

	accel_group = gtk_accel_group_new();

	MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(MainWindow), vbox1);

	menubar1 = gtk_menu_bar_new();
	gtk_widget_show(menubar1);
	gtk_box_pack_start(GTK_BOX(vbox1), menubar1, FALSE, FALSE, 0);

	menuitem1 = gtk_menu_item_new_with_mnemonic(("_File"));
	gtk_widget_show(menuitem1);
	gtk_container_add(GTK_CONTAINER(menubar1), menuitem1);

	menuitem1_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem1), menuitem1_menu);

	new1 = gtk_image_menu_item_new_from_stock("gtk-new", accel_group);
	gtk_widget_show(new1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), new1);

	open1 = gtk_image_menu_item_new_from_stock("gtk-open", accel_group);
	gtk_widget_show(open1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), open1);

	save1 = gtk_image_menu_item_new_from_stock("gtk-save", accel_group);
	gtk_widget_show(save1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), save1);

	separatormenuitem1 = gtk_menu_item_new();
	gtk_widget_show(separatormenuitem1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), separatormenuitem1);
	gtk_widget_set_sensitive(separatormenuitem1, FALSE);

	quit1 = gtk_image_menu_item_new_from_stock("gtk-quit", accel_group);
	gtk_widget_show(quit1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), quit1);

	menuitem2 = gtk_menu_item_new_with_mnemonic(("_Edit"));
	gtk_widget_show(menuitem2);
	gtk_container_add(GTK_CONTAINER(menubar1), menuitem2);

	menuitem2_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem2), menuitem2_menu);

	return MainWindow;
}
static int LauchGtkMain(void* p)
{
	gtk_main();
	return 0;
}
#endif // WITH_GUI

int CheckAndMakeDirectory(const char* pDirectory, bool create) {
	struct stat info;
	int res;

	// Check if it exist
	if (!stat(pDirectory, &info)) {

		// exist. Now check if is a directory
		if (info.st_mode & S_IFDIR) return 0;

		// pDirectory exist, but it is not a directory
		fprintf(stderr, "Error. %s exist but it is not a directory.\n", pDirectory);
		return -1;
	}

	// pDirectory does not exist.
	if (!create) return -1;

	// Now create it
	res = 
#ifdef WIN32
		CreateDirectory(pDirectory, NULL);
#else
		mkdir(pDirectory, S_IRWXU | S_IRGRP | S_IXGRP);
#endif
	fprintf(stderr, "%s directory %s\n",
		res ? "Error: Failed to create" : "Created",
		pDirectory);
	return res;
}

char *CurrentWorkingDirectory = NULL;
char *SnowmixHomeDirectory = NULL;
char *SnowmixDirectory = NULL;

void CheckDirectories() {
	const char *pHome, *pSnowmix, *pSnowmixPrefix;
	char *pDirectory;
	int lenSnowmix, lenSnowmixHome, maxlen;

	const char *directories[] = { "frames", "images", "ini", "slib", "scripts", "tcl", "test", NULL };
#ifdef WIN32
	const char *pHomeSnowmixPrefix = "Application Data/Snowmix";
#else
#ifdef HAVE_MACOSX
	const char *pHomeSnowmixPrefix = "Snowmix";
#else
	const char *pHomeSnowmixPrefix = ".snowmix";
#endif
#endif

	// First we set the current working directory
	if (!(CurrentWorkingDirectory = getcwd(NULL,0))) {
		fprintf(stderr, "Error: Failed to set the current working directory\n");
		exit(-1);
	}

	// Check for HOME environment variable
	if (!(pHome = getenv("HOME"))) {
		fprintf(stderr, "Error. Environment variable HOME was not set.\n");
		exit(1);
	}
	if (CheckAndMakeDirectory(pHome, false)) {
		fprintf(stderr, "Error. Home directory %s does not exist or "
			"is not accessible\n", pHome);
		exit(1);
	}

	// Check for SNOWMIX environment variable
	if (!(pSnowmix = getenv("SNOWMIX"))) {
		fprintf(stderr, "Error. Environment variable SNOWMIX was not set.\n"
			"The environment variable must be set to the installation directory\n"
			" of Snowmix or the base directory of the Snowmix package.\n");
		exit(1);
	}
	if (!(SnowmixDirectory = strdup(pSnowmix))) {
		fprintf(stderr, "Error: Failed to copy SNOWMIX environment variable.\n");
		exit(-1);
	}
	if (CheckAndMakeDirectory(SnowmixDirectory, false)) {
		fprintf(stderr, "Error. Snowmix directory %s does not exist or "
			"is not accessible\n", SnowmixDirectory);
		exit(1);
	}
	lenSnowmix = strlen(SnowmixDirectory);

	// Check for SNOWMIX_PREFIX environment variable
	if (!(pSnowmixPrefix = getenv("SNOWMIX_PREFIX"))) {
		fprintf(stderr, "Warning. Environment variable SNOWMIX_PREFIX was not "
			"set. Using SNOWMIX_PREFIX=%s\n",pHomeSnowmixPrefix);
		pSnowmixPrefix = pHomeSnowmixPrefix;
	}
	lenSnowmixHome = strlen(pHome)+1+strlen(pSnowmixPrefix)+1;
	if (!(SnowmixHomeDirectory = (char*) malloc(lenSnowmixHome))) {
		fprintf(stderr, "Error: Failed to allocate memory for Snowmix prefix.\n");
		exit(-1);
	}
	sprintf(SnowmixHomeDirectory, "%s/%s", pHome, pSnowmixPrefix);

	// Check that Snowmix installation directories exists
	maxlen = 0;
	for (int i=0 ; directories[i] ; i++) {
		int len = strlen(directories[i]);
		if (len>maxlen) maxlen=len;
		pDirectory = (char*)malloc(lenSnowmix+1+len+1);
		if (!pDirectory) {
			fprintf(stderr, "Failed to allocate memory for checking %s/%s\n",
				pSnowmix,directories[i]);
			exit(-1);
		}
		sprintf(pDirectory, "%s/%s", pSnowmix, directories[i]);
		//fprintf(stderr, "Checking %s\n",pDirectory);
		if (CheckAndMakeDirectory(pDirectory, false))
			fprintf(stderr, "Warning: Directory %s does not exist.\n", pDirectory);
		if (pDirectory) free(pDirectory);
	}

	// Check and create personal SnowmixHomeDirectory
	if (CheckAndMakeDirectory(SnowmixHomeDirectory, true)) {
		fprintf(stderr, "Warning: Failed to create directory %s\n", SnowmixHomeDirectory);
	}
	if (!(pDirectory = (char*) malloc(strlen(SnowmixHomeDirectory)+1+maxlen+1))) {
		fprintf(stderr, "Error: Failed to allocate memory for creating "
			"Snowmix home directories.\n");
		exit(-1);
	}
	for (int i=0 ; directories[i] ; i++) {
		sprintf(pDirectory, "%s/%s", SnowmixHomeDirectory, directories[i]);
		if (CheckAndMakeDirectory(pDirectory, true)) {
			fprintf(stderr, "Warning: Failed to create directory %s\n", pDirectory);
		}
	}
	if (pDirectory) free(pDirectory);

//fprintf(stderr, "Directories checked\n");
}

// Environment Variables
//  HOME : home directory
//  SNOWMIX : Snowmix installation directory or main directory for Snowmix package
//  SM : As SNOWMIX
//
//  Snowmix Directory Layout
//  SNOWMIX
//    +--frames : Directory for raw BGRA frames
//    +--images : Directory for images (mostly PNG images with alpha channel)
//    +--ini    : Directory for Snowmix ini files
//    +--slib   : Snowmix library directory
//    +--tcl    : Snowmix Tcl/Tk utilities

/* Opening file preference : CWD, $HOME/$SNOWMIX_PREFIX, $SNOWMIX
 * 
 */


int main(int argc, char *argv[])
{
#ifdef WITH_GUI
	GtkWidget *MainWindow = NULL;
	SDL_Thread* GtkThread = NULL;
fprintf(stderr, "With GUI\n");
#endif // WITH_GUI
	CheckDirectories();
/*
	fprintf(stderr, "Snowmix file search path : %s, %s, %s\n", 
		CurrentWorkingDirectory ? CurrentWorkingDirectory : "none",
		SnowmixHomeDirectory ? SnowmixHomeDirectory : "none",
		SnowmixDirectory ? SnowmixDirectory : "none");
*/
	int loop = 0;
	if (argc > 1) {
		if (!strncasecmp(argv[1], "-l1", 3)) loop=1;
		else if (!strncasecmp(argv[1], "-l2", 3)) loop=2;
	}
	CVideoMixer* pVideoMixer = new CVideoMixer();
	if (!pVideoMixer) {
		fprintf(stderr, "Failed to allocate VideoMixer\n");
		return 1;
	}
	pVideoMixer->AddSearchPath(CurrentWorkingDirectory);
	pVideoMixer->AddSearchPath(SnowmixHomeDirectory);
	pVideoMixer->AddSearchPath(SnowmixDirectory);

	// Create Class CVideoFeeds
	pVideoMixer->m_pVideoFeed = new CVideoFeeds(pVideoMixer, MAX_VIDEO_FEEDS);
	if (!pVideoMixer->m_pVideoFeed) {
		fprintf(stderr, "Failed to create feeds for video mixer\n");
		return 1;
	}

	// Create Controller/parser CController for mixer
	pVideoMixer->m_pController = 
		loop == 0 ? ((argc > 1) ? new CController(pVideoMixer, argv [1]) :
			new CController(pVideoMixer, (char*) "ini/videomixer.ini")) :
		  ((argc > 2) ? new CController(pVideoMixer, argv [2]) :
			new CController(pVideoMixer, (char*) "videomixer.ini"));
	if (!pVideoMixer->m_pController) {
		fprintf(stderr, "Failed to create controller for video mixer.\n");
		return 1;
	}

	// Create Video Output Class for mixer
	pVideoMixer->m_pVideoOutput = new CVideoOutput(pVideoMixer);
	if (!pVideoMixer->m_pVideoOutput) {
		fprintf(stderr, "Failed to create output class for video mixer\n");
		return 1;
	}

#ifdef WITH_GUI
	gtk_init(&argc, &argv);
	MainWindow = CreateMainWindow();
	gtk_window_set_title(GTK_WINDOW(MainWindow), "Snowmix");
	gtk_widget_show(MainWindow);
	GtkThread = SDL_CreateThread(LauchGtkMain, NULL);
#endif // WITH_GUI

	//int ret = main_poller (pVideoMixer);
	int ret = loop < 2 ? pVideoMixer->MainMixerLoop() : pVideoMixer->MainMixerLoop2();
	delete pVideoMixer;
#ifdef WITH_GUI
	SDL_KillThread(GtkThread);
#endif // WITH_GUI
	//if (CurrentWorkingDirectory) free(CurrentWorkingDirectory);
	//if (SnowmixDirectory) free(SnowmixDirectory);
	//if (SnowmixHomeDirectory) free(SnowmixHomeDirectory);
	return ret;
}

