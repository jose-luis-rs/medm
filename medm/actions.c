/*
*****************************************************************
                          COPYRIGHT NOTIFICATION
*****************************************************************

THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
AND IN ALL SOURCE LISTINGS OF THE CODE.

(C)  COPYRIGHT 1993 UNIVERSITY OF CHICAGO

Argonne National Laboratory (ANL), with facilities in the States of
Illinois and Idaho, is owned by the United States Government, and
operated by the University of Chicago under provision of a contract
with the Department of Energy.

Portions of this material resulted from work developed under a U.S.
Government contract and are subject to the following license:  For
a period of five years from March 30, 1993, the Government is
granted for itself and others acting on its behalf a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, and perform
publicly and display publicly.  With the approval of DOE, this
period may be renewed for two additional five year periods.
Following the expiration of this period or periods, the Government
is granted for itself and others acting on its behalf, a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, distribute copies
to the public, perform publicly and display publicly, and to permit
others to do so.

*****************************************************************
                                DISCLAIMER
*****************************************************************

NEITHER THE UNITED STATES GOVERNMENT NOR ANY AGENCY THEREOF, NOR
THE UNIVERSITY OF CHICAGO, NOR ANY OF THEIR EMPLOYEES OR OFFICERS,
MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL
LIABILITY OR RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR
USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
OWNED RIGHTS.

*****************************************************************
LICENSING INQUIRIES MAY BE DIRECTED TO THE INDUSTRIAL TECHNOLOGY
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#define DEBUG_DRAG 0

#include "medm.h"
#include <Xm/MwmUtil.h>

#include "cvtFast.h"

extern char *stripChartWidgetName;

/*
 * action routines (XtActionProc) and some associated callbacks
 */

/* since passing client_data didn't seem to work... */
static char *channelName;

#ifdef __cplusplus
static Boolean DragConvertProc(
  Widget,
  Atom *,
  Atom *target,
  Atom *typeRtn,
  XtPointer *valueRtn,
  unsigned long *lengthRtn,
  int *formatRtn,
  unsigned long *,
  XtPointer,
  XtRequestId *)
#else
static Boolean DragConvertProc(
  Widget w,
  Atom *selection,
  Atom *target,
  Atom *typeRtn,
  XtPointer *valueRtn,
  unsigned long *lengthRtn,
  int *formatRtn,
  unsigned long *max_lengthRtn,
  XtPointer client_data,
  XtRequestId *request_id)
#endif
{
    XmString cString;
    char *cText, *passText;

    if (channelName != NULL) {
	if (*target != COMPOUND_TEXT) return(False);
	cString = XmStringCreateSimple(channelName);
	cText = XmCvtXmStringToCT(cString);
	passText = XtMalloc(strlen(cText)+1);
	memcpy(passText,cText,strlen(cText)+1);
      /* probably need this too */
	XmStringFree(cString);

      /* format the value for return */
	*typeRtn = COMPOUND_TEXT;
	*valueRtn = (XtPointer)passText;
	*lengthRtn = strlen(passText);
	*formatRtn = 8;	/* from example - related to #bits for data elements */
	return(True);
    } else {
      /* monitorData not found */
	return(False);
    }

}


/*
 * cleanup after drag/drop
 */
#ifdef __cplusplus
static void dragDropFinish(
  Widget w,
  XtPointer,
  XtPointer)
#else
static void dragDropFinish(
  Widget w,
  XtPointer client,
  XtPointer call)
#endif
{
    Widget sourceIcon;
    Pixmap pixmap;
    Arg args[2];

/* perform cleanup at conclusion of DND */
    XtSetArg(args[0],XmNsourcePixmapIcon,&sourceIcon);
    XtGetValues(w,args,1);

    XtSetArg(args[0],XmNpixmap,&pixmap);
    XtGetValues(sourceIcon,args,1);

    XFreePixmap(display,pixmap);
    XtDestroyWidget(sourceIcon);
}

static XtCallbackRec dragDropFinishCB[] = {
    {dragDropFinish,NULL},
    {NULL,NULL}
};



void StartDrag(Widget w, XEvent *event)
{
    Arg args[8];
    Cardinal n;
    Atom exportList[1];
    Widget sourceIcon;
    UpdateTask *pt;
    int textWidth, maxWidth, maxHeight, fontHeight, ascent;
    unsigned long fg, bg;
    Widget searchWidget;
    XButtonEvent *xbutton;
    XGCValues gcValues;
    unsigned long gcValueMask;
    DisplayInfo *displayInfo;

    static char *channelNames[MAX(MAX_PENS,MAX_TRACES)][2];
    Pixmap sourcePixmap = (Pixmap)NULL;
    static GC gc = NULL;

/* a nice sized font */
#define FONT_TABLE_INDEX 6
/* move the text over... */
#define X_SHIFT 8
#define MARGIN  2

  
/* (MDA) since widget doing drag could be toggleButton or optionMenu button
 *   (which has more than just flat, single parent),
 *   find the widget that has a parent that is the drawing area and
 *   search based on that *   (since that is what is rooted in the display)
 * - NB if drawing areas as children of the main drawing area are allowed
 *   as parents of controllers/monitors, this logic must change...
 */
    searchWidget = w;
    if (XtClass(searchWidget) == xmDrawingAreaWidgetClass
      && strcmp(XtName(searchWidget),stripChartWidgetName)) {
      /* starting search from a DrawingArea which is not a StripChart 
       *  (i.e., DL_Display) therefore lookup "graphic" (non-widget) elements 
       *  ---get data from position
       */
	displayInfo = dmGetDisplayInfoFromWidget(searchWidget);
	xbutton = (XButtonEvent *)event;
	pt = getUpdateTaskFromPosition(displayInfo,
	  xbutton->x,xbutton->y);
    } else {
      /* ---get data from widget */
#if 0
	while (XtClass(XtParent(searchWidget)) != xmDrawingAreaWidgetClass)
	  searchWidget = XtParent(searchWidget);
#endif
	pt = getUpdateTaskFromWidget(searchWidget);
    }

#if DEBUG_DRAG
    printf("start drag : 0x%08x\n",pt);
#endif
    
    if (pt) {
#if ((2*MAX_TRACES)+2) > MAX_PENS
#define MAX_COUNT 2*MAX_TRACES+2
#define MAX_COL 2
#else
#define MAX_COUNT MAX_PENS
#define MAX_COL 1
#endif
	Record *record[MAX_COUNT];
	int count;
	int column;
	int row;

      /* Call the getRecord procedure, if there is one */
	if (pt->getRecord == NULL) return;
	pt->getRecord(pt->clientData, record, &count);

	column = count / 100;
	if (column == 0) column = 1;
	if (column > MAX_COL) {
	    column = MAX_COL;
	    medmPostMsg("StartDrag: Maximum number of columns exceeded\n"
	      "  Programming Error: Please notify person in charge of MEDM\n");
	    return;
	}
	row = 0;
	count = count % 100;
	if (count > MAX_COUNT) {
	    medmPostMsg("StartDrag: Maximum count exceeded\n"
	      "  Programming Error: Please notify person in charge of MEDM\n");
	    return;
	}
	
	bg = BlackPixel(display,screenNum);
	fg = WhitePixel(display,screenNum);
	ascent = fontTable[FONT_TABLE_INDEX]->ascent;
	fontHeight = ascent + fontTable[FONT_TABLE_INDEX]->descent;
	
	if (count == 0) {
	    channelName = NULL;
	    return;
	} else {
	    int i, j;
	    int x, y;

	    i = 0; j = 0;
	    textWidth = 0;
	    while (i < count) {
		if (record[i] && record[i]->name) {
		    textWidth = MAX(textWidth,XTextWidth(
		      fontTable[FONT_TABLE_INDEX],record[i]->name,
		      strlen(record[i]->name)));
		}
		j++;
		if (j >= column) {
		    j = 0;
		    row++;
		}
		i++;
	    }
	    maxWidth = X_SHIFT + (textWidth + MARGIN) * column;
	    maxHeight = row*fontHeight + 2*MARGIN;
	    sourcePixmap = XCreatePixmap(display,
	      RootWindow(display, screenNum),maxWidth,maxHeight,
	      DefaultDepth(display,screenNum));
	    if (gc == NULL) gc = XCreateGC(display,sourcePixmap,0,NULL);
	    gcValueMask = GCForeground|GCBackground|GCFunction|GCFont;
	    gcValues.foreground = bg;
	    gcValues.background = bg;
	    gcValues.function = GXcopy;
	    gcValues.font = fontTable[FONT_TABLE_INDEX]->fid;
	    XChangeGC(display,gc,gcValueMask,&gcValues);
	    XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,maxHeight);
	    i = 0; j = 0;
	    x = X_SHIFT;
	    y = ascent + MARGIN;
	    while (i < count) {
		if (record[i] && record[i]->name) {
		    XSetForeground(display,gc,
		      alarmColorPixel[record[i]->severity]);
		    XDrawString(display,sourcePixmap,gc,x,y,record[i]->name,
		      strlen(record[i]->name));
		    channelName = record[i]->name;
		}
		j++;
		if (j < column) {
		    x += textWidth + MARGIN;
		} else {
		    j = 0;
		    x = X_SHIFT;
		    y += fontHeight;
		}
		i++;
	    }
	} 
    }


    if (sourcePixmap != (Pixmap)NULL) {
      /* Use source widget as parent - can inherit visual attributes that way */
	n = 0;
	XtSetArg(args[n],XmNpixmap,sourcePixmap); n++;
	XtSetArg(args[n],XmNwidth,maxWidth); n++;
	XtSetArg(args[n],XmNheight,maxHeight); n++;
	XtSetArg(args[n],XmNdepth,DefaultDepth(display,screenNum)); n++;
	sourceIcon = XmCreateDragIcon(XtParent(searchWidget),"sourceIcon",args,n);

      /* Establish list of valid target types */
	exportList[0] = COMPOUND_TEXT;

	n = 0;
	XtSetArg(args[n],XmNexportTargets,exportList); n++;
	XtSetArg(args[n],XmNnumExportTargets,1); n++;
	XtSetArg(args[n],XmNdragOperations,XmDROP_COPY); n++;
	XtSetArg(args[n],XmNconvertProc,DragConvertProc); n++;
	XtSetArg(args[n],XmNsourcePixmapIcon,sourceIcon); n++;
	XtSetArg(args[n],XmNcursorForeground,fg); n++;
	XtSetArg(args[n],XmNcursorBackground,bg); n++;
	XtSetArg(args[n],XmNdragDropFinishCallback,dragDropFinishCB); n++;
	XmDragStart(searchWidget,event,args,n);
    }
#undef MAX_COUNT
#undef MAX_COL
}
