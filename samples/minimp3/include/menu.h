// menu.h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>  // sleep()
#include <debugnet.h>
#include <orbisPad.h>
#include <orbis2d.h>
#include <orbisFile.h>
#include <orbisFileBrowser.h>
//#include <orbisXbmFont.h>
#include <ps4link.h>

enum MENU_PAGE {
	CLOSED,
	MAIN,
//more views
	MAX_MENU_PAGES
};

#define MAX_DRAWN_LINES  (27)

/* a vector of 4 ints */
typedef int v4i __attribute__((ext_vector_type(4)));

int  browserUpdateController(v4i *menu_pos);
void browserDraw(v4i *menu_pos);
void browserDrawSelection(v4i *menu_pos);
