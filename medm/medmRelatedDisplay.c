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

#define DEBUG_COMPOSITE 0
#define DEBUG_FONTS 0

/* KE: May need to set this to 0 for WIN32 to improve performance */
#define USE_MARQUEE 1

#if 0     /* Not using _NTSDK anymore */
#ifdef WIN32
  /* math.h for WIN32 with _NTSDK defined defines y1 as _y1, also y0
   *  as _y0 for Bessel functions.  This screws up XSegment
   *  Exceed 5 includes math.h differently than Exceed 6
   *  Include math.h here and undefine y1 since we don't use it
   *  Should work for both Exceed 5 and Exceed 6 */
#include <math.h>
#undef y1    
#endif    
#endif    

#include "medm.h"
#include "Marquee.h"
#include <Xm/MwmUtil.h>

#define RD_APPLY_BTN  0
#define RD_CLOSE_BTN  1
 
#define MARKER_ON_TIME 5000     /* msec */
#define MARKER_OFF_TIME 1000     /* msec */
#define MARKER_COUNT 1

static Widget rdMatrix = NULL, rdForm = NULL;
static Widget table[MAX_RELATED_DISPLAYS][4];
static Pixmap stipple = (Pixmap)0;

static void relatedDisplayInheritValues(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplaySetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplaySetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplayGetValues(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplayButtonPressedCb(Widget, XtPointer, XtPointer);
static void renderRelatedDisplayPixmap(Display *display, Pixmap pixmap,
  Pixel fg, Pixel bg, Dimension width, Dimension height,
  XFontStruct *font, int icon, char *label);
static int countHiddenButtons(DlElement *dlElement);
static void createMarkerWidgets(DisplayInfo *displayInfo, DlElement *dlElement);
static void createMarkerWidget(DisplayInfo *displayInfo, DlElement *dlElement);
static void markerWidgetsDestroy(DisplayInfo *displayInfo);

static DlDispatchTable relatedDisplayDlDispatchTable = {
    createDlRelatedDisplay,
    NULL,
    executeDlRelatedDisplay,
    hideDlRelatedDisplay,
    writeDlRelatedDisplay,
    NULL,
    relatedDisplayGetValues,
    relatedDisplayInheritValues,
    relatedDisplaySetBackgroundColor,
    relatedDisplaySetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

static void freePixmapCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    Pixmap pixmap = (Pixmap) cd;

    UNREFERENCED(cbs);

/*     if(pixmap != (Pixmap)0) XmDestroyPixmap(XtScreen(w),pixmap); */
    if(pixmap != (Pixmap)0) XFreePixmap(display,pixmap);
    pixmap=(Pixmap)0;
}

/*
 * Local function to render the related display icon into a pixmap
 */
static void renderRelatedDisplayPixmap(Display *display, Pixmap pixmap,
  Pixel fg, Pixel bg, Dimension width, Dimension height,
  XFontStruct *font, int icon, char *label)
{
    typedef struct { float x; float y;} XY;
/* Icon is based on the 25 pixel (w & h) bitmap relatedDisplay25 */
    static float rectangleX = (float)(4./25.), rectangleY = (float)(4./25.),
      rectangleWidth = (float)(13./25.), rectangleHeight = (float)(14./25.);
    static XY segmentData[] = {
        {(float)(16./25.),(float)(9./25.)},
        {(float)(22./25.),(float)(9./25.)},
        {(float)(22./25.),(float)(22./25.)},
        {(float)(10./25.),(float)(22./25.)},
        {(float)(10./25.),(float)(18./25.)},
    };
    XSegment segments[4];
    GC gc = XCreateGC(display,pixmap,0,NULL);

#if 0
    print("renderRelatedDisplayPixmap: width=%d height=%d label=|%s|\n",
      width,height,label?label:"NULL");
#endif    
    
  /* Draw the background */
    XSetForeground(display,gc,bg);
    XFillRectangle(display,pixmap,gc,0,0,width,height);
    XSetForeground(display,gc,fg);
	
  /* Draw the icon */
    if(icon) {
	segments[0].x1 = (short)(segmentData[0].x*height);
	segments[0].y1 = (short)(segmentData[0].y*height);
	segments[0].x2 = (short)(segmentData[1].x*height);
	segments[0].y2 = (short)(segmentData[1].y*height);
	
	segments[1].x1 = (short)(segmentData[1].x*height);
	segments[1].y1 = (short)(segmentData[1].y*height);
	segments[1].x2 = (short)(segmentData[2].x*height);
	segments[1].y2 = (short)(segmentData[2].y*height);
	
	segments[2].x1 = (short)(segmentData[2].x*height);
	segments[2].y1 = (short)(segmentData[2].y*height);
	segments[2].x2 = (short)(segmentData[3].x*height);
	segments[2].y2 = (short)(segmentData[3].y*height);
	
	segments[3].x1 = (short)(segmentData[3].x*height);
	segments[3].y1 = (short)(segmentData[3].y*height);
	segments[3].x2 = (short)(segmentData[4].x*height);
	segments[3].y2 = (short)(segmentData[4].y*height);
	
	XDrawSegments(display,pixmap,gc,segments,4);

      /* Erase any out-of-bounds edges due to roundoff error by blanking out
       *  area of top rectangle */
	XSetForeground(display,gc,bg);
	XFillRectangle(display,pixmap,gc,
	  (int)(rectangleX*height),
	  (int)(rectangleY*height),
	  (unsigned int)(rectangleWidth*height),
	  (unsigned int)(rectangleHeight*height));
	
      /* Draw the top rectangle */
	XSetForeground(display,gc,fg);
	XDrawRectangle(display,pixmap,gc,
	  (int)(rectangleX*height),
	  (int)(rectangleY*height),
	  (unsigned int)(rectangleWidth*height),
	  (unsigned int)(rectangleHeight*height));
    }

  /* Draw the label */
    if(label && *label) {
	int base;
	
	XSetFont(display,gc,font->fid);
	base=(height+font->ascent-font->descent)/2;
	XDrawString(display,pixmap,gc,
	  icon?height:0,base,label,strlen(label));
    }

  /* Free the GC */
    XFreeGC(display,gc);
}

int relatedDisplayFontListIndex(DlRelatedDisplay *dlRelatedDisplay,
  int numButtons, int maxChars)
{
    int i;
 
    UNREFERENCED(maxChars);

#define SHADOWS_SIZE 4    /* each Toggle Button has 2 shadows...*/
 
/* more complicated calculation based on orientation, etc */
    for(i = MAX_FONTS-1; i >= 0; i--) {
	switch(dlRelatedDisplay->visual) {
	case RD_COL_OF_BTN:
	    if((int)(dlRelatedDisplay->object.height/MAX(1,numButtons)
	      - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	case RD_ROW_OF_BTN:
	    if((int)(dlRelatedDisplay->object.height - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	}
    }
    return(0);
}

void executeDlRelatedDisplay(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Widget localMenuBar, tearOff;
    Arg args[30];
    int n;
    int i, displayNumber=0, index, icon;
    char *label;
    XmString xmString;
    Pixmap pixmap;
    Dimension pixmapH, pixmapW;
    int iNumberOfDisplays = 0;
    DlRelatedDisplay *dlRelatedDisplay = dlElement->structure.relatedDisplay;

/* These are widget ids, but they are recorded in the otherChild
   widget list as well, for destruction when new displays are selected
   at the top level */
    Widget relatedDisplayPulldownMenu, relatedDisplayMenuButton;
    Widget widget;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(dlElement->widget) {
	if(displayInfo->traversalMode == DL_EDIT) {
	  /* In EDIT mode destroy it */
	    XtDestroyWidget(dlElement->widget);
	    dlElement->widget = NULL;
	} else {
	  /* In EXECUTE manage it if necessary */
	    if(!XtIsManaged(dlElement->widget)) {
		XtManageChild(dlElement->widget);
	    }
	    return;
	}
    }
  /* Count number of displays with non-NULL labels */
    for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	if(dlRelatedDisplay->display[i].label[0] != '\0') {
	    iNumberOfDisplays++;
	}
    } 
  /* One display, not hidden */
    if(iNumberOfDisplays <= 1 && dlRelatedDisplay->visual != RD_HIDDEN_BTN) {
      /* Case 1 0f 4 */
      /* One item, any type but hidden */
      /* Get size for graphic part of pixmap */
	pixmapH = MIN(dlRelatedDisplay->object.width,
	  dlRelatedDisplay->object.height);
      /* Allow for shadows, etc. */
	pixmapH = (unsigned int)MAX(1,(int)pixmapH - 8);
      /* Create the pixmap */
	if(dlRelatedDisplay->label[0] == '\0') {
	    label=NULL;
	    index=0;
	    icon=1;
	    pixmapW=pixmapH;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else if(dlRelatedDisplay->label[0] == '-') {
	    int usedWidth;
	    
	    label=dlRelatedDisplay->label+1;
	    index=messageButtonFontListIndex(dlRelatedDisplay->object.height);
	    icon=0;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=MAX(usedWidth,1);
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else {
	    int usedWidth;
	    
	    label=dlRelatedDisplay->label;
	    index=messageButtonFontListIndex(dlRelatedDisplay->object.height);
	    icon=1;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=pixmapH+usedWidth;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	}
      /* Draw the pixmap */
	renderRelatedDisplayPixmap(display,pixmap,
	  displayInfo->colormap[dlRelatedDisplay->clr],
	  displayInfo->colormap[dlRelatedDisplay->bclr],
	  pixmapW, pixmapH, fontTable[index], icon, label);
      /* Create a push button */
	n = 0;
	dlElement->widget = createPushButton(
	  displayInfo->drawingArea,
	  &(dlRelatedDisplay->object),
	  displayInfo->colormap[dlRelatedDisplay->clr],
	  displayInfo->colormap[dlRelatedDisplay->bclr],
	  pixmap,
	  NULL,     /* There a pixmap, not a label on the button */
	  (XtPointer)displayInfo);
      /* Add the callbacks for bringing up the menu */
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    int i;
	    
	  /* Check the display array to find the first non-empty one */
	    for(i=0; i < MAX_RELATED_DISPLAYS; i++) {
		if(*(dlRelatedDisplay->display[i].name)) {
		    XtAddCallback(dlElement->widget,XmNarmCallback,
		      relatedDisplayButtonPressedCb,
		      (XtPointer)&(dlRelatedDisplay->display[i]));
		    break;
		}
	    }
	}
      /* Add handlers */
	addCommonHandlers(dlElement->widget, displayInfo);
	XtManageChild(dlElement->widget);
    } else if(dlRelatedDisplay->visual == RD_MENU) {
      /* Case 2 of 4 */
      /* Single column menu, more than 1 item */
	n = 0;
	XtSetArg(args[n],XmNbackground,(Pixel)
	  displayInfo->colormap[dlRelatedDisplay->bclr]); n++;
	XtSetArg(args[n],XmNforeground,
	  (Pixel)displayInfo->colormap[dlRelatedDisplay->clr]); n++;
	XtSetArg(args[n],XmNhighlightThickness,1); n++;
	XtSetArg(args[n],XmNwidth,dlRelatedDisplay->object.width); n++;
	XtSetArg(args[n],XmNheight,dlRelatedDisplay->object.height); n++;
	XtSetArg(args[n],XmNmarginHeight,0); n++;
	XtSetArg(args[n],XmNmarginWidth,0); n++;
	XtSetArg(args[n],XmNresizeHeight,(Boolean)FALSE); n++;
	XtSetArg(args[n],XmNresizeWidth,(Boolean)FALSE); n++;
	XtSetArg(args[n],XmNspacing,0); n++;
	XtSetArg(args[n],XmNx,(Position)dlRelatedDisplay->object.x); n++;
	XtSetArg(args[n],XmNy,(Position)dlRelatedDisplay->object.y); n++;
	XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
	XtSetArg(args[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
      /* KE: Beware below if changing the number of args here */
	localMenuBar =
	  XmCreateMenuBar(displayInfo->drawingArea, "relatedDisplayMenuBar",
	    args, n);
	dlElement->widget = localMenuBar;
      /* Add handlers */
	addCommonHandlers(dlElement->widget, displayInfo);
	XtManageChild(dlElement->widget);
	
	colorMenuBar(localMenuBar,
	  (Pixel)displayInfo->colormap[dlRelatedDisplay->clr],
	  (Pixel)displayInfo->colormap[dlRelatedDisplay->bclr]);
	
	relatedDisplayPulldownMenu = XmCreatePulldownMenu(
	  localMenuBar,"relatedDisplayPulldownMenu",args,2);
	tearOff = XmGetTearOffControl(relatedDisplayPulldownMenu);
	if(tearOff) {
	    XtVaSetValues(tearOff,
	      XmNforeground,(Pixel)displayInfo->colormap[dlRelatedDisplay->clr],
	      XmNbackground,(Pixel)displayInfo->colormap[dlRelatedDisplay->bclr],
	      XmNtearOffModel,XmTEAR_OFF_DISABLED,
	      NULL);
	}
	
      /* Get size for graphic part of pixmap */
	pixmapH = MIN(dlRelatedDisplay->object.width,
	  dlRelatedDisplay->object.height);
      /* Allow for shadows, etc. */
	pixmapH = (unsigned int)MAX(1,(int)pixmapH - 8);
      /* Create the pixmap */
	if(dlRelatedDisplay->label[0] == '\0') {
	    label=NULL;
	    index=0;
	    icon=1;
	    pixmapW=pixmapH;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else if(dlRelatedDisplay->label[0] == '-') {
	    int usedWidth;
	    
	    label=dlRelatedDisplay->label+1;
	    index=messageButtonFontListIndex(dlRelatedDisplay->object.height);
	    icon=0;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=MAX(usedWidth,1);
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else {
	    int usedWidth;
	    
	    label=dlRelatedDisplay->label;
	    index=messageButtonFontListIndex(dlRelatedDisplay->object.height);
	    icon=1;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=pixmapH+usedWidth;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	}
      /* Draw the pixmap */
	renderRelatedDisplayPixmap(display,pixmap,
	  displayInfo->colormap[dlRelatedDisplay->clr],
	  displayInfo->colormap[dlRelatedDisplay->bclr],
	  pixmapW, pixmapH, fontTable[index], icon, label);
      /* Create a cascade button */
	n = 7;     /* (Using the args defined above) */
	XtSetArg(args[n],XmNrecomputeSize,(Boolean)False); n++;
	XtSetArg(args[n],XmNlabelPixmap,pixmap); n++;
	XtSetArg(args[n],XmNlabelType,XmPIXMAP); n++;
	XtSetArg(args[n],XmNsubMenuId,relatedDisplayPulldownMenu); n++;
	XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
	XtSetArg(args[n],XmNalignment,XmALIGNMENT_BEGINNING); n++;
	
	XtSetArg(args[n],XmNmarginLeft,0); n++;
	XtSetArg(args[n],XmNmarginRight,0); n++;
	XtSetArg(args[n],XmNmarginTop,0); n++;
	XtSetArg(args[n],XmNmarginBottom,0); n++;
	XtSetArg(args[n],XmNmarginWidth,0); n++;
	XtSetArg(args[n],XmNmarginHeight,0); n++;
	
	widget = XtCreateManagedWidget("relatedDisplayMenuLabel",
	  xmCascadeButtonGadgetClass,
	  dlElement->widget, args, n);
	
#if 0
      /* KE: Can't do this, it points to the stack
       *   and shouldn't be necessary */
	XtAddCallback(widget, XmNdestroyCallback,freePixmapCallback,
	  (XtPointer)relatedDisplayPixmap);
#endif	
	
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    if(strlen(dlRelatedDisplay->display[i].name) > (size_t)1) {
		xmString = XmStringCreateLocalized(
		  dlRelatedDisplay->display[i].label);
		XtSetArg(args[3], XmNlabelString,xmString);
		XtSetArg(args[4], XmNuserData, displayInfo);
		relatedDisplayMenuButton = XtCreateManagedWidget("relatedButton",
		  xmPushButtonWidgetClass, relatedDisplayPulldownMenu, args, 5);
		XtAddCallback(relatedDisplayMenuButton,XmNactivateCallback,
		  relatedDisplayButtonPressedCb,&(dlRelatedDisplay->display[i]));
		XmStringFree(xmString);
	    }
	}
    } else if(dlRelatedDisplay->visual == RD_ROW_OF_BTN || 
      dlRelatedDisplay->visual == RD_COL_OF_BTN) {
      /* Case 3 of 4 */
      /* Rows or columns of buttons */
	Arg wargs[20];
	int i, n, maxChars, usedWidth, usedHeight;
	XmFontList fontList;
	Pixel fg, bg;
	Widget widget;
	
	maxChars = 0;
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    maxChars = MAX((size_t) maxChars,
	      strlen(dlRelatedDisplay->display[i].label));
	}
	fg = displayInfo->colormap[dlRelatedDisplay->clr];
	bg = displayInfo->colormap[dlRelatedDisplay->bclr];
	n = 0;
	XtSetArg(wargs[n],XmNx,(Position)dlRelatedDisplay->object.x); n++;
	XtSetArg(wargs[n],XmNy,(Position)dlRelatedDisplay->object.y); n++;
	XtSetArg(wargs[n],XmNwidth,(Dimension)dlRelatedDisplay->object.width); n++;
	XtSetArg(wargs[n],XmNheight,(Dimension)dlRelatedDisplay->object.height); n++;
	XtSetArg(wargs[n],XmNforeground,fg); n++;
	XtSetArg(wargs[n],XmNbackground,bg); n++;
	XtSetArg(wargs[n],XmNindicatorOn,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNmarginWidth,0); n++;
	XtSetArg(wargs[n],XmNmarginHeight,0); n++;
	XtSetArg(wargs[n],XmNresizeWidth,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNresizeHeight,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNspacing,0); n++;
	XtSetArg(wargs[n],XmNrecomputeSize,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNuserData,displayInfo); n++;
	switch(dlRelatedDisplay->visual) {
	case RD_COL_OF_BTN:
	    XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
	    usedWidth = dlRelatedDisplay->object.width;
	    usedHeight = (int)(dlRelatedDisplay->object.height/
	      MAX(1,iNumberOfDisplays));
	    break;
	case RD_ROW_OF_BTN:
	    XtSetArg(wargs[n],XmNorientation,XmHORIZONTAL); n++;
	    usedWidth = (int)(dlRelatedDisplay->object.width/
	      MAX(1,iNumberOfDisplays));
	    usedHeight = dlRelatedDisplay->object.height;
	    break;
	default:
	    break;
	}
	widget = XmCreateRowColumn(displayInfo->drawingArea,"radioBox",wargs,n);
	dlElement->widget = widget;
	
      /* Make push-in type radio buttons of the correct size */
	fontList = fontListTable[relatedDisplayFontListIndex(
	  dlRelatedDisplay,iNumberOfDisplays,maxChars)];
	n = 0;
	XtSetArg(wargs[n],XmNindicatorOn,False); n++;
	XtSetArg(wargs[n],XmNshadowThickness,2); n++;
	XtSetArg(wargs[n],XmNhighlightThickness,1); n++;
	XtSetArg(wargs[n],XmNrecomputeSize,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNwidth,(Dimension)usedWidth); n++;
	XtSetArg(wargs[n],XmNheight,(Dimension)usedHeight); n++;
	XtSetArg(wargs[n],XmNfontList,fontList); n++;
	XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER); n++;
	XtSetArg(wargs[n],XmNindicatorOn,False); n++;
	XtSetArg(wargs[n],XmNindicatorSize,0); n++;
	XtSetArg(wargs[n],XmNspacing,0); n++;
	XtSetArg(wargs[n],XmNvisibleWhenOff,False); n++;
	XtSetArg(wargs[n],XmNforeground,fg); n++;
	XtSetArg(wargs[n],XmNbackground,bg); n++;
	XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER); n++;
	XtSetArg(wargs[n],XmNuserData,displayInfo); n++;
	for(i = 0; i < iNumberOfDisplays; i++) {
	    Widget toggleButton;
	    
	    xmString = XmStringCreateLocalized(
	      dlRelatedDisplay->display[i].label);
	    XtSetArg(wargs[n],XmNlabelString,xmString);
	  /* Use gadgets here so that changing foreground of
	   *   radioBox changes buttons */
	    toggleButton = XmCreatePushButtonGadget(widget,"toggleButton",
	      wargs,n+1);
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
		XtAddCallback(toggleButton,XmNarmCallback,
		  relatedDisplayButtonPressedCb,
		  (XtPointer)&(dlRelatedDisplay->display[i]));
	    }  else {
		XtSetSensitive(toggleButton,False);
	    }
	  /* MDA: For some reason, need to do this after the fact for gadgets */
	    XtVaSetValues(toggleButton,XmNalignment,XmALIGNMENT_CENTER,NULL);
	    XtManageChild(toggleButton);
	    XmStringFree(xmString);
	}
      /* Add handlers */
	addCommonHandlers(dlElement->widget, displayInfo);
	XtManageChild(dlElement->widget);
    } else if(dlRelatedDisplay->visual == RD_HIDDEN_BTN) {
      /* Case 4 of 4 */
      /* Hidden button: No widget. No callbacks. Just draw a stippled
       *  rectangle.  Handle in executeModeButtonPress, not
       *  relatedDisplayButtonPressedCb */
	unsigned long gcValueMask;
	XGCValues gcValues;
	Drawable drawable=updateInProgress?
	  displayInfo->updatePixmap:displayInfo->drawingAreaPixmap;

	dlElement->updateType = STATIC_GRAPHIC;

	gcValueMask = GCForeground | GCBackground | GCFillStyle | GCStipple;
	gcValues.foreground = displayInfo->colormap[dlRelatedDisplay->clr];
	gcValues.background = displayInfo->colormap[dlRelatedDisplay->bclr];
	gcValues.fill_style = FillStippled;
	if(!stipple) {
	    static char stipple_bitmap[] = {0x03, 0x03, 0x0c, 0x0c};
	    
	    stipple = XCreateBitmapFromData(display,
	      RootWindow(display, DefaultScreen(display)),
	      stipple_bitmap, 4, 4);
	}
	gcValues.stipple = stipple;
	XChangeGC(display, displayInfo->gc, gcValueMask, &gcValues);
	XFillRectangle(display, drawable, displayInfo->gc,
	  dlRelatedDisplay->object.x, dlRelatedDisplay->object.y,
	  dlRelatedDisplay->object.width, dlRelatedDisplay->object.height);

      /* Restore GC */
	XSetFillStyle(display, displayInfo->gc, FillSolid);
    }
}

void hideDlRelatedDisplay(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    dlElement->updateType = STATIC_GRAPHIC;

    hideWidgetElement(displayInfo, dlElement);
}

static void createDlRelatedDisplayEntry(DlRelatedDisplayEntry *relatedDisplay,
  int displayNumber)
{
  /* Structure copy */
    *relatedDisplay = globalResourceBundle.rdData[displayNumber];
}

DlElement *createDlRelatedDisplay(DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay;
    DlElement *dlElement;
    int displayNumber;

    dlRelatedDisplay = (DlRelatedDisplay *)malloc(sizeof(DlRelatedDisplay));
    if(!dlRelatedDisplay) return 0;
    if(p) {
	*dlRelatedDisplay = *p->structure.relatedDisplay;
    } else {
	objectAttributeInit(&(dlRelatedDisplay->object));
	for(displayNumber = 0; displayNumber < MAX_RELATED_DISPLAYS;
	     displayNumber++)
	  createDlRelatedDisplayEntry(
	    &(dlRelatedDisplay->display[displayNumber]),
	    displayNumber );

	dlRelatedDisplay->clr = globalResourceBundle.clr;
	dlRelatedDisplay->bclr = globalResourceBundle.bclr;
	dlRelatedDisplay->label[0] = '\0';
	dlRelatedDisplay->visual = RD_MENU;
    }

    if(!(dlElement = createDlElement(DL_RelatedDisplay,
      (XtPointer)dlRelatedDisplay,
      &relatedDisplayDlDispatchTable))) {
	free(dlRelatedDisplay);
    }

    return(dlElement);
}

void parseRelatedDisplayEntry(DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *relatedDisplay)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;

    do {
	switch(tokenType=getToken(displayInfo,token)) {
	case T_WORD:
	    if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(relatedDisplay->label,token);
	    } else if(!strcmp(token,"name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(relatedDisplay->name,token);
	    } else if(!strcmp(token,"args")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(relatedDisplay->args,token);
	    } else if(!strcmp(token,"policy")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,stringValueTable[REPLACE_DISPLAY]))
		  relatedDisplay->mode = REPLACE_DISPLAY;
	    }
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF));
}

DlElement *parseRelatedDisplay(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlRelatedDisplay *dlRelatedDisplay = 0;
    DlElement *dlElement = createDlRelatedDisplay(NULL);
    int displayNumber;

    if(!dlElement) return 0;
    dlRelatedDisplay = dlElement->structure.relatedDisplay;

    do {
	switch(tokenType=getToken(displayInfo,token)) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlRelatedDisplay->object));
	    } else if(!strncmp(token,"display",7)) {
/*
 * compare the first 7 characters to see if a display entry.
 *   if more than one digit is allowed for the display index, then change
 *   the following code to pick up all the digits (can't use atoi() unless
 *   we get a null-terminated string
 */
		displayNumber = MIN(token[8] - '0', MAX_RELATED_DISPLAYS-1);
		parseRelatedDisplayEntry(displayInfo,
		  &(dlRelatedDisplay->display[displayNumber]) );
	    } else if(!strcmp(token,"clr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlRelatedDisplay->clr = atoi(token) % DL_MAX_COLORS;
	    } else if(!strcmp(token,"bclr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlRelatedDisplay->bclr = atoi(token) % DL_MAX_COLORS;
	    } else if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlRelatedDisplay->label,token);
	    } else if(!strcmp(token,"visual")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,stringValueTable[FIRST_RD_VISUAL+1])) {
		    dlRelatedDisplay->visual = RD_ROW_OF_BTN;
		} else
		  if(!strcmp(token,stringValueTable[FIRST_RD_VISUAL+2])) {
		      dlRelatedDisplay->visual = RD_COL_OF_BTN;
		  } else
		    if(!strcmp(token,stringValueTable[FIRST_RD_VISUAL+3])) {
			dlRelatedDisplay->visual = RD_HIDDEN_BTN;
		    }
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF));

    return dlElement;

}

void writeDlRelatedDisplayEntry(FILE *stream, DlRelatedDisplayEntry *entry,
  int index, int level)
{
    char indent[16];

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%sdisplay[%d] {",indent,index);
	if(entry->label[0] != '\0')
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
	if(entry->name[0] != '\0')
	  fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->name);
	if(entry->args[0] != '\0')
	  fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
	if(entry->mode != ADD_NEW_DISPLAY)
	  fprintf(stream,"\n%s\tpolicy=\"%s\"",
	    indent,stringValueTable[entry->mode]);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%sdisplay[%d] {",indent,index);
	fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
	fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->name);
	fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

void writeDlRelatedDisplay(FILE *stream, DlElement *dlElement, int level)
{
    int i;
    char indent[16];
    DlRelatedDisplay *dlRelatedDisplay = dlElement->structure.relatedDisplay;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"related display\" {",indent);
	writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    if((dlRelatedDisplay->display[i].label[0] != '\0') ||
	      (dlRelatedDisplay->display[i].name[0] != '\0') ||
	      (dlRelatedDisplay->display[i].args[0] != '\0'))
	      writeDlRelatedDisplayEntry(stream,&(dlRelatedDisplay->display[i]),i,level+1);
	}
	fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
	if(dlRelatedDisplay->label[0] != '\0') 
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,dlRelatedDisplay->label);
	if(dlRelatedDisplay->visual != RD_MENU)
	  fprintf(stream,"\n%s\tvisual=\"%s\"",
	    indent,stringValueTable[dlRelatedDisplay->visual]);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\"related display\" {",indent);
	writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++)
	  writeDlRelatedDisplayEntry(stream,&(dlRelatedDisplay->display[i]),i,level+1);
	fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void relatedDisplayButtonPressedCb(Widget w, XtPointer clientData,
  XtPointer callbackData)
{
    DlRelatedDisplayEntry *pEntry = (DlRelatedDisplayEntry *)clientData;
    DisplayInfo *displayInfo = 0;
    XEvent *event = ((XmPushButtonCallbackStruct *)callbackData)->event;
    Boolean replace = False;
    
  /* See if it was a ctrl-click indicating replace */
    if(event->type = ButtonPress  &&
      ((XButtonEvent *)event)->state & ControlMask) {
	replace = True;
    }
    
#if DEBUG_FONTS
    print("\nrelatedDisplayButtonPressedCb: displayInfo=%x replace=%s\n",
      displayInfo,replace?"True":"False");
#endif    

  /* Create the new display */
    XtVaGetValues(w, XmNuserData, &displayInfo, NULL);
    relatedDisplayCreateNewDisplay(displayInfo, pEntry, replace);
}

void relatedDisplayCreateNewDisplay(DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *pEntry, Boolean replaceDisplay)
{
    DisplayInfo *existingDisplayInfo;
    FILE *filePtr;
    char *argsString, token[MAX_TOKEN_LENGTH];
    char filename[MAX_TOKEN_LENGTH];
    char processedArgs[2*MAX_TOKEN_LENGTH];

    strncpy(filename, pEntry->name, MAX_TOKEN_LENGTH);
    filename[MAX_TOKEN_LENGTH-1] = '\0';
    argsString = pEntry->args;
    
#if DEBUG_FONTS
    print("relatedDisplayCreateNewDisplay: displayInfo=%x replaceDisplay=%s\n"
      "  filename=%s\n",
      displayInfo,replaceDisplay?"True":"False",filename);
#endif    

  /*
   * if we want to be able to have RD's inherit their parent's
   *   macro-substitutions, then we must perform any macro substitution on
   *   this argument string in this displayInfo's context before passing
   *   it to the created child display
   */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	performMacroSubstitutions(displayInfo, argsString, processedArgs,
          2*MAX_TOKEN_LENGTH);
      /* Check for an existing display */
	existingDisplayInfo = NULL;
	if(popupExistingDisplay) {
	    existingDisplayInfo = findDisplay(filename, processedArgs,
	      displayInfo->dlFile->name);
	    if(existingDisplayInfo) {
		DisplayInfo *cdi;
		
	      /* Remove the old one if appropriate */
		if(replaceDisplay || pEntry->mode == REPLACE_DISPLAY &&
		  displayInfo != existingDisplayInfo) {
		    closeDisplay(displayInfo->shell);
		}

	      /* Make the existing one be the current one */
		cdi = currentDisplayInfo = existingDisplayInfo;
#if 0
	      /* KE: Doesn't work on WIN32 */
		XtPopdown(currentDisplayInfo->shell);
		XtPopup(currentDisplayInfo->shell,XtGrabNone);
#else
		if(cdi && cdi->shell && XtIsRealized(cdi->shell)) {
		    XMapRaised(display, XtWindow(cdi->shell));
		}
#endif
	      /* Refresh the display list dialog box */
		refreshDisplayListDlg();
		return;
	    }
	}
	
      /* There is no existing display to use.  Try to find a file,
         passing the parent's path. */
	filePtr = dmOpenUsableFile(filename, displayInfo->dlFile->name);
	if(filePtr == NULL) {
	    sprintf(token,
	      "Cannot open related display:\n  %s\nCheck %s\n",
	      filename, "EPICS_DISPLAY_PATH");
	    dmSetAndPopupWarningDialog(displayInfo,token,"OK",NULL,NULL);
	    medmPostMsg(1,token);
	} else {
	    if(replaceDisplay || pEntry->mode == REPLACE_DISPLAY) {
	      /* Don't look for an existing one.  Just reparse this one. */
		dmDisplayListParse(displayInfo,filePtr,processedArgs,
		  filename,NULL,(Boolean)True);
	    } else {
		dmDisplayListParse(NULL,filePtr,processedArgs,
		  filename,NULL,(Boolean)True);
	    }
	    fclose(filePtr);

	  /* Refresh the display list dialog box */
	    refreshDisplayListDlg();
	}
    }
}

static void relatedDisplayActivate(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonType = (int)cd;
    int i;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

    switch(buttonType) {
    case RD_APPLY_BTN:
      /* Commit changes in matrix to global matrix array data */
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    char *tmp = NULL;
	    if(tmp = XmTextFieldGetString(table[i][0])) {
		strcpy(globalResourceBundle.rdData[i].label, tmp);
		XtFree(tmp);
	    }
	    if(tmp = XmTextFieldGetString(table[i][1])) {
		strcpy(globalResourceBundle.rdData[i].name, tmp);
		XtFree(tmp);
	    }
	    if(tmp = XmTextFieldGetString(table[i][2])) {
		strcpy(globalResourceBundle.rdData[i].args, tmp);
		XtFree(tmp);
	    }
	    if(XmToggleButtonGetState(table[i][3])) {
		globalResourceBundle.rdData[i].mode = REPLACE_DISPLAY;
	    } else {
		globalResourceBundle.rdData[i].mode = ADD_NEW_DISPLAY;
	    }
	}
	if(currentDisplayInfo) {
	    DlElement *dlElement =
	      FirstDlElement(currentDisplayInfo->selectedDlElementList);
	    while(dlElement) {
		if(dlElement->structure.element->type == DL_RelatedDisplay) {
		    updateElementFromGlobalResourceBundle(
		      dlElement->structure.element);
		}
		dlElement = dlElement->next;
	    }
	}
	medmMarkDisplayBeingEdited(currentDisplayInfo);
	XtPopdown(relatedDisplayS);
	break;
    case RD_CLOSE_BTN:
	if(XtClass(w) == xmPushButtonWidgetClass) {
	    XtPopdown(relatedDisplayS);
	}
	break;
    }
}
 
/* Create related display data dialog */
Widget createRelatedDisplayDataDialog(Widget parent)
{
    Widget shell, applyButton, closeButton;
    Dimension cWidth, cHeight, aWidth, aHeight;
    Arg args[12];
    int i, n;
    
/*
 * now create the interface
 *
 *         label | name | args | mode
 *         --------------------------
 *      1 |  A      B      C      D
 *      2 |
 *      3 |
 *         ...
 *     OK     CANCEL
 */
 
  /* Create form dialog (rdForm) */
    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
    XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    rdForm = XmCreateFormDialog(parent,"relatedDisplayDataF",args,n);
    
  /* Set values for the shell parent of the form */
    shell = XtParent(rdForm);
    XtVaSetValues(shell,
      XmNtitle,"Related Display Data",
#if OMIT_RESIZE_HANDLES
      XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
    /* KE: The following is necessary for Exceed, which turns off the
       resize function with the handles.  It should not be necessary */
      XmNmwmFunctions, MWM_FUNC_ALL,
#endif
      NULL);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      relatedDisplayActivate,(XtPointer)RD_CLOSE_BTN);
#if DEBUG_COMPOSITE
    {
	Widget w=rdForm;
	Window win=XtWindow(w);

	int i=0;
	
	while(1) {
	    print("%4d w=%x win=%x",i++,w,win);
	    if(w == mainShell) {
		print(" (mainShell)\n");
		break;
	    } else if(w == shell) {
		print(" (shell)\n");
	    } else if(w == rdForm) {
		print(" (rdForm)\n");
	    } else if(w == parent) {
		print(" (parent)\n");
	    } else {
		print("\n");
	    }
	    w=XtParent(w);
	    win=XtWindow(w);
	}
    }
#endif

  /* rdMatrix */
    n = 0;
    rdMatrix = XtVaCreateManagedWidget("rdMatrix",
      xmRowColumnWidgetClass,rdForm,
      XmNpacking, XmPACK_COLUMN,
      XmNorientation,XmHORIZONTAL,
      XmNnumColumns, MAX_RELATED_DISPLAYS + 1,
      NULL);
  /* create column label */
    XtVaCreateManagedWidget("Display Label",
      xmLabelWidgetClass, rdMatrix,
      XmNalignment, XmALIGNMENT_CENTER,
      NULL);
    XtVaCreateManagedWidget("Display File",
      xmLabelWidgetClass, rdMatrix,
      XmNalignment, XmALIGNMENT_CENTER,
      NULL);
    XtVaCreateManagedWidget("Arguments",
      xmLabelWidgetClass, rdMatrix,
      XmNalignment, XmALIGNMENT_CENTER,
      NULL);
    XtVaCreateManagedWidget("Policy",
      xmLabelWidgetClass, rdMatrix,
      XmNalignment, XmALIGNMENT_CENTER,
      NULL);
    for(i=0; i<MAX_RELATED_DISPLAYS; i++) {
	table[i][0] = XtVaCreateManagedWidget("label",
	  xmTextFieldWidgetClass, rdMatrix, NULL);
	table[i][1] = XtVaCreateManagedWidget("display",
	  xmTextFieldWidgetClass, rdMatrix, NULL);
	table[i][2] = XtVaCreateManagedWidget("arguments",
	  xmTextFieldWidgetClass, rdMatrix, NULL);
	table[i][3] = XtVaCreateManagedWidget("Remove Parent Display",
	  xmToggleButtonWidgetClass, rdMatrix,
	  XmNshadowThickness, 0,
	  NULL);
    }

  /* Close Button */
    closeButton = XtVaCreateWidget("Cancel",
      xmPushButtonWidgetClass, rdForm,
      NULL);
    XtAddCallback(closeButton,XmNactivateCallback,
      relatedDisplayActivate,(XtPointer)RD_CLOSE_BTN);
    XtManageChild(closeButton);

  /* Apply Button */
    applyButton = XtVaCreateWidget("Apply",
      xmPushButtonWidgetClass, rdForm,
      NULL); 
    XtAddCallback(applyButton,XmNactivateCallback,
      relatedDisplayActivate,(XtPointer)RD_APPLY_BTN);
    XtManageChild(applyButton);
 
  /* Make APPLY and CLOSE buttons same size */
    XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
    XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
    XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);
 
  /* Make the APPLY button the default for the form */
    XtVaSetValues(rdForm,XmNdefaultButton,applyButton,NULL);

  /* Do form layout */
 
  /* rdMatrix */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(rdMatrix,args,n);

  /* Apply */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,rdMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNleftPosition,30); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(applyButton,args,n);

  /* Close */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,rdMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNrightPosition,70); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(closeButton,args,n);
 
    XtManageChild(rdForm);
 
    return shell;
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *  related display data dialog with the values currently in
 *  globalResourceBundle
 */
void updateRelatedDisplayDataDialog()
{
    int i;

    if(rdMatrix) { 
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    XmTextFieldSetString(table[i][0],globalResourceBundle.rdData[i].label);
	    XmTextFieldSetString(table[i][1],globalResourceBundle.rdData[i].name);
	    XmTextFieldSetString(table[i][2],globalResourceBundle.rdData[i].args);
	    if(globalResourceBundle.rdData[i].mode == REPLACE_DISPLAY) {
		XmToggleButtonSetState(table[i][3],True,False);
	    } else {
		XmToggleButtonSetState(table[i][3],False,False);
	    }
	}
    }
}

void relatedDisplayDataDialogPopup(Widget w)
{
    if(relatedDisplayS == NULL) {
	relatedDisplayS = createRelatedDisplayDataDialog(w);
    }
  /* update related display data from globalResourceBundle */
    updateRelatedDisplayDataDialog();
    XtManageChild(rdForm);
    XtPopup(relatedDisplayS,XtGrabNone);
}

static void relatedDisplayInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
    medmGetValues(pRCB,
      CLR_RC,        &(dlRelatedDisplay->clr),
      BCLR_RC,       &(dlRelatedDisplay->bclr),
      -1);
}

static void relatedDisplayGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
    medmGetValues(pRCB,
      X_RC,          &(dlRelatedDisplay->object.x),
      Y_RC,          &(dlRelatedDisplay->object.y),
      WIDTH_RC,      &(dlRelatedDisplay->object.width),
      HEIGHT_RC,     &(dlRelatedDisplay->object.height),
      CLR_RC,        &(dlRelatedDisplay->clr),
      BCLR_RC,       &(dlRelatedDisplay->bclr),
      RD_LABEL_RC,   &(dlRelatedDisplay->label),
      RD_VISUAL_RC,  &(dlRelatedDisplay->visual),
      RDDATA_RC,     &(dlRelatedDisplay->display),
      -1);
}

static void relatedDisplaySetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlRelatedDisplay->bclr),
      -1);
}

static void relatedDisplaySetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
    medmGetValues(pRCB,
      CLR_RC,        &(dlRelatedDisplay->clr),
      -1);
}

/*** Hidden button marking routines ***/

void markHiddenButtons(DisplayInfo *displayInfo)
{
    DlElement *pE;
    
  /* Don't do anything if the displayInfo is invalid */
    if(!displayInfo) return;
    
  /* This is a toggle */
    if(displayInfo->nMarkerWidgets) {
      /* They are on.  Turn them off */
	markerWidgetsDestroy(displayInfo);
    } else {
      /* They are off.  Turn them on */
      /* Count the number of hidden buttons */
	pE = FirstDlElement(displayInfo->dlElementList);
	while(pE) {
	    displayInfo->nMarkerWidgets += countHiddenButtons(pE);
	    
	    pE = pE->next;
	}
	
      /* Popup a dialog and return if there are no hidden buttons */
	if(!displayInfo->nMarkerWidgets) {
	    char token[MAX_TOKEN_LENGTH];
	    
	    sprintf(token,
	      "There are no hidden buttons in this display.");
	    dmSetAndPopupWarningDialog(displayInfo, token, "OK", NULL, NULL);
	    return;
	}
	
      /* Allocate space for the list of widgets */
	displayInfo->markerWidgetList =
	  (Widget *)malloc(displayInfo->nMarkerWidgets*sizeof(Widget));
	if(!displayInfo->markerWidgetList) {
	    displayInfo->nMarkerWidgets = 0;
	    medmPrintf(1,"\nmarkHiddenButtons: Cannot create widget list\n");
	    return;
	}
	
      /* Create the marker widgets */
#ifdef WIN32
      /* Seems to take a long time on WIN32 */
	XDefineCursor(display,XtWindow(displayInfo->drawingArea),watchCursor);
	XFlush(display);
#endif    
	displayInfo->nMarkerWidgets = 0;
	pE = FirstDlElement(displayInfo->dlElementList);
	while(pE) {
	    createMarkerWidgets(displayInfo, pE);
	    
	    pE = pE->next;
	}
#ifdef WIN32
	XUndefineCursor(display,XtWindow(displayInfo->drawingArea));
#endif    
    }
}

static int countHiddenButtons(DlElement *dlElement)
{
    DlElement *pE;
    int n = 0;
    
    if(dlElement->type == DL_Composite) {
	pE = FirstDlElement(dlElement->structure.composite->dlElementList);
	while(pE) {
	    if(pE->type == DL_Composite) {
		n+=countHiddenButtons(pE);
	    } else if(pE->type == DL_RelatedDisplay &&
	      pE->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
		n++;
	    }
	    pE = pE->next;
	}
    } else if(dlElement->type == DL_RelatedDisplay &&
      dlElement->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
	n++;
    }

    return(n);
}

static void createMarkerWidgets(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlElement *pE;
    
    if(dlElement->type == DL_Composite) {
	pE = FirstDlElement(dlElement->structure.composite->dlElementList);
	while(pE) {
	    if(pE->type == DL_Composite) {
		createMarkerWidgets(displayInfo, pE);
	    } else if(pE->type == DL_RelatedDisplay &&
	      pE->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
		createMarkerWidget(displayInfo, pE);
	    }
	    pE = pE->next;
	}
    } else if(dlElement->type == DL_RelatedDisplay &&
      dlElement->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
	createMarkerWidget(displayInfo, dlElement);
    }
}

static void createMarkerWidget(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlObject *pO = &(dlElement->structure.composite->object);
    int nargs;
    Arg args[10];
    Widget w;
    int x, y, width, height;

  /* Make the marquee surround the hidden button */
    x = pO->x - 1;
    y = pO->y - 1;
    width = pO->width + 2;
    height = pO->height + 2;

    nargs = 0;
    XtSetArg(args[nargs], XmNx, x); nargs++;
    XtSetArg(args[nargs], XmNy, y); nargs++;
    XtSetArg(args[nargs], XmNwidth, width); nargs++;
    XtSetArg(args[nargs], XmNheight, height); nargs++;
    XtSetArg(args[nargs], XmNforeground, WhitePixel(display,screenNum)); nargs++;
    XtSetArg(args[nargs], XmNbackground, BlackPixel(display,screenNum)); nargs++;
    XtSetArg(args[nargs], XtNborderWidth, 0); nargs++;
#if !USE_MARQUEE    
    XtSetArg(args[nargs], XtNblinkTime, 0); nargs++;
    XtSetArg(args[nargs], XtNtransparent, False); nargs++;
#endif	
    w=XtCreateManagedWidget("marquee", marqueeWidgetClass,
      displayInfo->drawingArea, args, nargs);
    displayInfo->markerWidgetList[displayInfo->nMarkerWidgets++] = w;
}

static void markerWidgetsDestroy(DisplayInfo *displayInfo)
{
    int i;
    int nWidgets = displayInfo->nMarkerWidgets;
    Widget *widgets = displayInfo->markerWidgetList;

#ifdef WIN32
      /* Seems to take a long time on WIN32 */
	XDefineCursor(display,XtWindow(displayInfo->drawingArea),watchCursor);
	XFlush(display);
#endif    
  /* Unmap them to be sure */
    for(i=0; i < nWidgets; i++) {
	XtUnmapWidget(widgets[i]);
    }
    XFlush(display);

  /* Destroy them */
    for(i=0; i < nWidgets; i++) {
	XtDestroyWidget(widgets[i]);
    }
    if(widgets) {
	free((char *)widgets);
	displayInfo->markerWidgetList = NULL;
    }
    displayInfo->nMarkerWidgets = 0;
#ifdef WIN32
	XUndefineCursor(display,XtWindow(displayInfo->drawingArea));
#endif    
}
