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

#define DEBUG_CARTESIAN_PLOT 0
#define DEBUG_CREATE 0
#define DEBUG_EDIT_POPUP 0
#define DEBUG_EVENTS 0
#define DEBUG_HIGHLIGHTS 0
#define DEBUG_KEYS 0
#define DEBUG_POPUP 0
#define DEBUG_PVINFO 0
#define DEBUG_SEND_EVENT 0

#include "medm.h"
#include <X11/IntrinsicP.h>

#include <XrtGraph.h>
#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
#include <XrtGraphProp.h>
#endif
#endif

/* Function prototypes */
static void updateDraggedElements(Position x0, Position y0,
  Position x1, Position y1);
static void updateResizedElements(Position x0, Position y0,
  Position x1, Position y1);

extern Widget resourceMW, resourceS;
extern Widget objectPaletteSelectToggleButton;
extern XButtonPressedEvent lastEvent;

void toggleSelectedElementHighlight(DlElement *element);

static DlList *tmpDlElementList = NULL;

int initEventHandlers(void)
{
    if (tmpDlElementList) return 0;
    if (tmpDlElementList = createDlList()) {
	return 0;
    } else {
	return -1;
    }
}

/*
 * event handlers
 */

#ifdef __cplusplus
void handleExecuteButtonPress(Widget, XtPointer cd, XEvent *event, Boolean *)
#else
void handleExecuteButtonPress(Widget w, XtPointer cd, XEvent *event, Boolean *ctd)
#endif
{
    DisplayInfo *displayInfo = (DisplayInfo *) cd;
    XButtonEvent *xEvent = (XButtonEvent *)event;
    Widget widget;
    DlElement *pE;
    Position x, y;

#if DEBUG_POPUP
    printf("\nhandleExecuteButtonPress: Entered\n");
    printf("  Window: %x SubWindow: %x\n",
      xEvent->window,xEvent->subwindow);
    printf("  shell Window: %x drawingArea Window: %x\n",
      XtWindow(displayInfo->shell),XtWindow(displayInfo->drawingArea));
#endif    

    if (displayInfo != currentDisplayInfo) {
	currentDisplayInfo = displayInfo;
	currentColormap = currentDisplayInfo->colormap;
	currentColormapSize = currentDisplayInfo->dlColormapSize;
    }
  /* KE: Shouldn't be called in EDIT mode in the first place */
    if (globalDisplayListTraversalMode == DL_EDIT) return;

/* Make sure the window is on top and has input focus */
    XRaiseWindow(display,XtWindow(displayInfo->shell));
    XSetInputFocus(display,XtWindow(displayInfo->shell),
      RevertToParent,CurrentTime);

    if (xEvent->button == Button3) {
      /* Button 3 */
      /* Lookup to see if Btn3 occured in an object that cares */
	x = xEvent->x;
	y = xEvent->y;
	pE = findSmallestTouchedExecuteElementFromWidget(w, displayInfo,
	  &x, &y);
	if (pE) {
#if DEBUG_PVINFO
	    printf("handleExecuteButtonPress: Element: %s\n",elementType(pE->type));
	    printf("  xEvent->x: %4d\n",xEvent->x);
	    printf("  xEvent->y: %4d\n",xEvent->y);
#endif    
	    switch(pE->type) {
	    case DL_Valuator:
		if (widget = pE->widget) {
		    popupValuatorKeyboardEntry(widget,displayInfo,event);
		    XUngrabPointer(display,CurrentTime);
		    XFlush(display);
		}
		break;
		
	    case DL_CartesianPlot:
		if (widget = pE->widget) {
		    if (xEvent->state & ControlMask) {
#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
		      /* Bring up XRT Property Editor */
			dmSetAndPopupWarningDialog(displayInfo,
			  "The XRT/graph Property Editor is known to cause memory leaks.\n"
			  "XRT has been notified of the problem.  Until it is fixed, the\n"
			  "Property Editor should be used sparingly and with care.", "OK",
			  "Cancel", "Help");
			if(displayInfo->warningDialogAnswer == 1) {
			    XrtUpdatePropertyEditor(widget);
			  /* True means allow load/save of .XRT data files */
			    XrtPopupPropertyEditor(widget,
			      "XRT Property Editor",True);
			} else if(displayInfo->warningDialogAnswer == 3) {
			    callBrowser(MEDM_HELP_PATH"/MEDM.html#XRTGraphInteractions");
			}
#endif			    
#endif			    
		    } else {
		      /* Bring up plot axis data dialog */
		      /* update globalResourceBundle with this element's info */
			executeTimeCartesianPlotWidget = widget;
			updateGlobalResourceBundleFromElement(pE);
			if (!cartesianPlotAxisS) {
			    cartesianPlotAxisS = createCartesianPlotAxisDialog(mainShell);
			} else {
			    XtSetSensitive(cartesianPlotAxisS,True);
			}
			
		      /* update cartesian plot axis data from globalResourceBundle */
		      /* KE:  Actually from XtGetValues */
			updateCartesianPlotAxisDialogFromWidget(widget);
			
			XtManageChild(cpAxisForm);
			XtPopup(cartesianPlotAxisS,XtGrabNone);
		    }
#if DEBUG_CARTESIAN_PLOT
		    dumpCartesianPlot();
#endif			
		  /* End DEBUG */
		    
		    XUngrabPointer(display,CurrentTime);
		    XFlush(display);
		}
		break;
	    default:
	      /* Popup execute-mode popup menu */
		XmMenuPosition(displayInfo->executePopupMenu,
		  (XButtonPressedEvent *)event);
		XtManageChild(displayInfo->executePopupMenu);
#if 0		    
	      /* KE: Are these necessary ? */
		XtPopup(XtParent(currentDisplayInfo->executePopupMenu),XtGrabNone);
		XRaiseWindow(display,XtWindow(currentDisplayInfo->executePopupMenu));
#endif		    
		break;
	    }
	} else {
	  /* Popup execute-mode popup menu */
	    XmMenuPosition(displayInfo->executePopupMenu,
	      (XButtonPressedEvent *)event);
	    XtManageChild(displayInfo->executePopupMenu);
	}
    } else if (xEvent->button == Button1) {
      /* Button 1 */
	x = xEvent->x;
	y = xEvent->y;
	pE = findSmallestTouchedExecuteElementFromWidget(w, displayInfo,
	  &x, &y);
	if (pE) {
	    if (pE->type == DL_RelatedDisplay &&
	      pE->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
		DlRelatedDisplay *pRD = pE->structure.relatedDisplay;
		Boolean replace = False;
		int i;
		
	      /* Check the display array to find the first non-empty one */
		for(i=0; i < MAX_RELATED_DISPLAYS; i++) {
		    printf("\nhandleExecuteButtonPress: name[%d] = \"%s\"\n",
		      i, pRD->display[i].name);
		    if(*(pRD->display[i].name)) {
		      /* See if it was a ctrl-click indicating replace */
			if(xEvent->state & ControlMask) replace = True;

		      /* Create the related display */
			relatedDisplayCreateNewDisplay(displayInfo,
			  &(pRD->display[i]),
			  replace);

			break;
		    }
		}
	    }
	}
    }
}

#if 0
#ifdef __cplusplus
void popdownMenu(Widget, XtPointer cd, XEvent *event, Boolean *)
#else
void popdownMenu(Widget w, XtPointer cd, XEvent *event, Boolean *ctd)
#endif
{
    DisplayInfo *displayInfo = (DisplayInfo *) cd;
    XButtonEvent *xEvent = (XButtonEvent *)event;

#if DEBUG_POPUP
    printf("\npopdownMenu: Entered\n");
#endif    

  /* Button 3 */
    if (xEvent->button == Button3) {
	if (globalDisplayListTraversalMode == DL_EDIT) {
	  /* edit menu doesn't have valid/unique displayInfo ptr, hence use current */
	    XtUnmanageChild(currentDisplayInfo->editPopupMenu);
	} else {
	  /* execute menu does have valid/unique displayInfo ptr, hence use it */
	    XtUnmanageChild(displayInfo->executePopupMenu);
	}
    }
}
#endif

#ifdef __cplusplus
void handleEditEnterWindow(Widget, XtPointer cd, XEvent *, Boolean *)
#else
void handleEditEnterWindow(Widget w, XtPointer cd, XEvent *event, Boolean *ctd)
#endif
{
    pointerInDisplayInfo = (DisplayInfo *) cd;
}


#ifdef __cplusplus
void handleEditButtonPress(Widget w, XtPointer clientData, XEvent *event,
  Boolean *)
#else
void handleEditButtonPress(Widget w, XtPointer clientData, XEvent *event,
  Boolean *continueToDispatch)
#endif
{
    DisplayInfo *displayInfo = (DisplayInfo *)clientData;
    XButtonEvent *xEvent = (XButtonEvent *)event;
    int j, k;
    Position x0, y0, x1, y1;
    Dimension daWidth, daHeight;
    Boolean validDrag, validResize;
    int minSize;
    Boolean objectDataOnly, foundVertex = False, foundPoly = False;
    int doTextByTyping;
    DisplayInfo *di, *cdi;
    DlElement *dlElement;

#if DEBUG_POPUP
    printf("\nhandleEditButtonPress: Entered\n");
#endif    
#if DEBUG_EVENTS
    fprintf(stderr,"\n>>> handleEditButtonPress: %s Button: %d Shift: %d Ctrl: %d\n",
      currentActionType == SELECT_ACTION?"SELECT":"CREATE",xEvent->button,
      xEvent->state&ShiftMask,xEvent->state&ControlMask);
#endif    

  /* If in execute mode, update currentDisplayInfo and simply return */
    if (globalDisplayListTraversalMode == DL_EXECUTE) {
	currentDisplayInfo = displayInfo;
	return;
    }

  /* Unselect everything in other displays */
    di = displayInfoListHead->next;
    while (di) {
	if (di != displayInfo) {
	    currentDisplayInfo = di;
	    unhighlightSelectedElements();
	    clearDlDisplayList(currentDisplayInfo->selectedDlElementList);
	}
	di = di->next;
    }

  /* Set current values */
    cdi = currentDisplayInfo = displayInfo;
    currentColormap = cdi->colormap;
    currentColormapSize = cdi->dlColormapSize;
    
  /* Make sure the window is on top and has input focus */
    XRaiseWindow(display,XtWindow(cdi->shell));
    XSetInputFocus(display,XtWindow(cdi->shell),
      RevertToParent,CurrentTime);
    
  /* Get button coordinates */
    x0 = event->xbutton.x;
    y0 = event->xbutton.y;
  /* Add offsets if the ButtonPress was in another window */
    if (w != cdi->drawingArea) {
	Dimension dx0, dy0;
	
	XtVaGetValues(w,XmNx,&dx0,XmNy,&dy0,NULL);
	x0 += dx0;
	y0 += dy0;
    }
    
  /* Change drawingArea's cursor to the appropriate cursor */
    XDefineCursor(display,XtWindow(cdi->drawingArea),
      (currentActionType == SELECT_ACTION ?
	rubberbandCursor : crosshairCursor));
    
  /* Branch depending on action */
    if (currentActionType == SELECT_ACTION) {
      /* SELECT_ACTION
       * ****************************************
       * Btn1        =  select (rubberband)     *
       * Ctrl-Btn1   =  cycle selections        *
       * Shift-Btn1  =  multi-select            *
       * Btn2        =  move                    *
       * Ctrl-Btn2   =  resize                  *
       * Shift-Btn2  =  nothing                 *
       * Btn3        =  nothing here            *
       * ****************************************/
	switch (xEvent->button) {
	case Button1:
	  /* SELECT_ACTION Btn1 */
	    if (xEvent->state & ControlMask) {
	      /* SELECT_ACTION Ctrl-Btn1 */
	      /* Cycle through selections */
#if DEBUG_EVENTS
		fprintf(stderr,"SELECT_ACTION Ctrl-Btn1\n");
#endif
		doRubberbanding(XtWindow(cdi->drawingArea),&x0,&y0,&x1,&y1,
		  False);
		findSelectedElements(cdi->dlElementList,
		  x0,y0,x1,y1,tmpDlElementList,AllTouched|AllEnclosed);
		if (!IsEmpty(tmpDlElementList)) {
		    DlElement *pT = FirstDlElement(tmpDlElementList);
		    int found = False;
		    
#if DEBUG_EVENTS > 1
		    fprintf(stderr,"\n[handleEditButtonPress: SELECT Ctrl-Btn1] tmpDlElementList:\n");
		    dumpDlElementList(tmpDlElementList);
		    fprintf(stderr,"\n[handleEditButtonPress: SELECT Ctrl-Btn1] cdi->selectedDlElementList:\n");
		    dumpDlElementList(cdi->selectedDlElementList);
		    fprintf(stderr,"\n");
#endif
		    unhighlightSelectedElements();
		    while (pT) {
			DlElement *pE =
			  FirstDlElement(cdi->selectedDlElementList);
		      /* Traverse the selected list */
			while (pE) {
#if DEBUG_EVENTS > 1
			    printf("Temp: Type=%s (%s) %x\n",
			      elementType(pT->type),elementType(pT->structure.element->type),
			      pT->structure.element);
			    printf("Sel:  Type=%s (%s) %x\n",
			      elementType(pE->type),elementType(pE->structure.element->type),
			      pE->structure.element);
#endif
			    if (pE->structure.element == pT->structure.element) {
				DlElement *pENew;
				
				clearDlDisplayList(cdi->selectedDlElementList);
				if (pT->next) {
				  /* Use the next one */
				    pENew = createDlElement(DL_Element,
				      (XtPointer)pT->next->structure.rectangle,NULL);
				} else {
				  /* No next one, use the first */
				    DlElement *pEFirst = FirstDlElement(tmpDlElementList);
				    
				    pENew = createDlElement(DL_Element,
				      (XtPointer)pEFirst->structure.rectangle,
				      NULL);
				}
				if (pENew) {
				    appendDlElement(cdi->selectedDlElementList,
				      pENew);
				}
				found=True;
#if DEBUG_EVENTS > 1
				printf("Found: found=%d\n",found);
#endif
				break;
			    }
			    pE = pE->next;
			}
			if (found) break;
			else pT = pT->next;
		    }
		  /* If no matches found use the first one in the new list */
		    if (!found) {
			DlElement *pENew;
			DlElement *pEFirst = FirstDlElement(tmpDlElementList);
			
#if DEBUG_EVENTS > 1
			printf("Not found: found=%d\n",found);
#endif
			clearDlDisplayList(cdi->selectedDlElementList);
			pENew = createDlElement(DL_Element,
			  (XtPointer)pEFirst->structure.rectangle,
			  NULL);
			if (pENew) {
			    appendDlElement(cdi->selectedDlElementList,
			      pENew);
			}
		    }
		    highlightSelectedElements();
		    clearDlDisplayList(tmpDlElementList);
		    clearResourcePaletteEntries();
		    if (NumberOfDlElement(cdi->selectedDlElementList)==1){
			currentElementType =
			  FirstDlElement(cdi->selectedDlElementList)
			  ->structure.element->type;
			setResourcePaletteEntries();
		    }
		}
#if DEBUG_EVENTS > 1
		fprintf(stderr,"\n[handleEditButtonPress: SELECT Ctrl-Btn1 Done] cdi->selectedDlElementList :\n");
		dumpDlElementList(cdi->selectedDlElementList);
#endif
	    } else if (xEvent->state & ShiftMask) {
	      /* SELECT_ACTION Shift-Btn1 */
	      /* Toggle and append selections */
		doRubberbanding(XtWindow(cdi->drawingArea),&x0,&y0,&x1,&y1,
		  False);
		findSelectedElements(cdi->dlElementList,
		  x0,y0,x1,y1,tmpDlElementList,SmallestTouched|AllEnclosed);
		if (!IsEmpty(tmpDlElementList)) {
		    DlElement *pT = FirstDlElement(tmpDlElementList);
		    int found = False;
		    unhighlightSelectedElements();
		    while (pT) {
			DlElement *pE =
			  FirstDlElement(cdi->selectedDlElementList);
			int found = False;
		      /* If found, remove it from the selected list */
			while (pE) {
			    if (pE->structure.element == pT->structure.element) {
				removeDlElement(cdi->selectedDlElementList,pE);
				destroyDlElement(pE);
				found = True;
				break;
			    }
			    pE = pE->next;
			}
		      /* If not found, add it to the selected list */
			if (!found) {
			    DlElement *pF = pT;
			    pT = pT->next;
			    removeDlElement(tmpDlElementList,pF);
			    appendDlElement(cdi->selectedDlElementList,pF);
			} else {
			    pT = pT->next;
			}
		    }
		    highlightSelectedElements();
		    clearDlDisplayList(tmpDlElementList);
		    clearResourcePaletteEntries();
		    if (NumberOfDlElement(cdi->selectedDlElementList)==1){
			currentElementType =
			  FirstDlElement(cdi->selectedDlElementList)
			  ->structure.element->type;
			setResourcePaletteEntries();
		    }
		}
	    } else {
	      /* SELECT_ACTION Btn1, not Shift or Crtl) */
	      /* See whether this is a vertex edit (1 already selected element
	       *   that has an editVertex function defined) and do the vertex
	       *   edit if so */
		if (NumberOfDlElement(cdi->selectedDlElementList) == 1) {
		    DlElement *dlElement =
		      FirstDlElement(cdi->selectedDlElementList)
		      ->structure.element;
		    if (dlElement->run->editVertex) {
			saveUndoInfo(cdi);
			if (cdi->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(cdi);
			}
			unhighlightSelectedElements();
		      /* Do vertex edit */
			if (dlElement->run->editVertex(dlElement,x0,y0)) {
			    foundVertex = True;
			    highlightSelectedElements();
			}
		    }
		}
	      /* If this was not a vertex edit then do rubberbanding, etc. */
		if (!foundVertex) {
		    doRubberbanding(XtWindow(cdi->drawingArea),&x0,&y0,&x1,&y1,
		      False);
		    findSelectedElements(cdi->dlElementList,
		      x0,y0,x1,y1,tmpDlElementList,SmallestTouched|AllEnclosed);
		    if (!IsEmpty(tmpDlElementList)) {
			unhighlightSelectedElements();
		      /* If this is the only element and is the same as before
		       *   then unselect it */
			if (tmpDlElementList->count == 1 &&
			  cdi->selectedDlElementList->count == 1) {
			    DlElement *pET = FirstDlElement(tmpDlElementList);
			    DlElement *pES = FirstDlElement(cdi->selectedDlElementList);

#if DEBUG_EVENTS > 1
			    printf("\nhandleEditButtonPress: Btn1\n");
			    printf("Selected: count=%d first=%x struct=%x\n",
			      cdi->selectedDlElementList->count,
			      FirstDlElement(cdi->selectedDlElementList),
			      pES->structure.element);
			    printf("Temp: count=%d first=%x struct=%x\n",
			      tmpDlElementList->count,
			      FirstDlElement(tmpDlElementList),
			      pET->structure.element);
#endif
			    if (pET->structure.element == pES->structure.element) {
				clearDlDisplayList(cdi->selectedDlElementList);
			    } else {
				clearDlDisplayList(cdi->selectedDlElementList);
				appendDlList(cdi->selectedDlElementList,tmpDlElementList);
			    }
			} else {
			    clearDlDisplayList(cdi->selectedDlElementList);
			    appendDlList(cdi->selectedDlElementList,tmpDlElementList);
			}
			highlightSelectedElements();
			clearDlDisplayList(tmpDlElementList);
			clearResourcePaletteEntries();
			if (NumberOfDlElement(cdi->selectedDlElementList) == 1) {
			    currentElementType =
			      FirstDlElement(cdi->selectedDlElementList)
			      ->structure.element->type;
			    setResourcePaletteEntries();
			}
		    }
		}
	    }
	    break;
	    
	case Button2:
	  /* SELECT_ACTION Btn2 */
	    if (xEvent->state & ShiftMask) {
	      /* SELECT_ACTION Shift-Btn2 */
#if DEBUG_SEND_EVENT
		XButtonEvent *bEvent;
		Status status;
		
		bEvent=(XButtonEvent *)calloc(1,sizeof(XButtonEvent));
		
		bEvent->type=ButtonPress;
		bEvent->button = 3;
		bEvent->window = XtWindow(cdi->drawingArea);
		
		status = XSendEvent(display, XtWindow(cdi->drawingArea), True,
		  ButtonPressMask, (XEvent *)bEvent);
		if(status != Success) {
		    printf("\nhandleEditButtonPress: XSendEvent failed\n");
		}
		free(bEvent);
#endif
		XBell(display,50);
		break;
	    }
	    findSelectedElements(cdi->dlElementList,
	      x0,y0,x0,y0,tmpDlElementList,SmallestTouched|AllEnclosed);
	    if (IsEmpty(tmpDlElementList)) break;
	    
	    if (xEvent->state & ControlMask) {
	      /* SELECT_ACTION Ctrl-Btn2 */
#if DEBUG_EVENTS
		fprintf(stderr,"SELECT_ACTION Ctrl-Btn2\n");
#endif
		if (alreadySelected(FirstDlElement(tmpDlElementList))) {
#if DEBUG_EVENTS
		    fprintf(stderr,"Already selected\n");
#endif
		  /* Element already selected - resize it and any others */
		  /* (MDA) ?? separate resize of display here? */
		  /* Unhighlight currently selected elements */
		    unhighlightSelectedElements();
		  /* This element already selected: resize all selected elements */
		    validResize = doResizing(XtWindow(cdi->drawingArea),
		      x0,y0,&x1,&y1);
#if DEBUG_EVENTS > 1
		    fprintf(stderr,"validResize=%d\n",validResize);
		    fprintf(stderr,"x0=%d, y0=%d, x1=%d, y1=%d\n",x0,y0,x1,y1);
#endif
		    if (validResize) {
			saveUndoInfo(cdi);
			if (cdi->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(cdi);
			}
			updateResizedElements(x0,y0,x1,y1);
		    }
		  /* Re-highlight currently selected elements */
		    highlightSelectedElements();
		  /* Update object prooperties of this element */
		    if (cdi->selectedDlElementList->count == 1) {
			objectDataOnly = True;
			updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
		    }
		} else {
		  /* This element not already selected, deselect others and resize it */
		  /* Unhighlight currently selected elements */
#if DEBUG_EVENTS
		    fprintf(stderr,"Not already selected\n");
#endif
		    unhighlightSelectedElements();
		    clearDlDisplayList(cdi->selectedDlElementList);
		    appendDlList(cdi->selectedDlElementList,
		      tmpDlElementList);
		    validResize = doResizing(XtWindow(cdi->drawingArea),
		      x0,y0,&x1,&y1);
#if DEBUG_EVENTS > 1
		    fprintf(stderr,"validResize=%d\n",validResize);
		    fprintf(stderr,"x0=%d, y0=%d, x1=%d, y1=%d\n",x0,y0,x1,y1);
#endif
		    if (validResize) {
			saveUndoInfo(cdi);
			if (cdi->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(cdi);
			}
			updateResizedElements(x0,y0,x1,y1);
		    }
		    highlightSelectedElements();
		    clearDlDisplayList(tmpDlElementList);
		    clearResourcePaletteEntries();
		    if (NumberOfDlElement(cdi->selectedDlElementList)==1){
			currentElementType =
			  FirstDlElement(cdi->selectedDlElementList)
			  ->structure.element->type;
			setResourcePaletteEntries();
		    }
		}
	    } else {
	      /* SELECT_ACTION Btn2, not Shift or Crtl) */
		XtVaGetValues(cdi->drawingArea,
		  XmNwidth,&daWidth,
		  XmNheight,&daHeight,
		  NULL);
#if DEBUG_EVENTS
		fprintf(stderr,"\n[handleEditButtonPress: SELECT Btn2] tmpDlElementList :\n");
		dumpDlElementList(tmpDlElementList);
#endif
		if (alreadySelected(FirstDlElement(tmpDlElementList))) {
		  /* Element already selected - move it and any others
		   * Unhighlight currently selected elements */
		    unhighlightSelectedElements();
#if DEBUG_EVENTS
		    fprintf(stderr,"Already selected\n");
#endif
		  /* This element already selected: move all selected elements */
		    validDrag = doDragging(XtWindow(cdi->drawingArea),
		      daWidth,daHeight,x0,y0,&x1,&y1);
#if DEBUG_EVENTS > 1
		    fprintf(stderr,"validDrag=%d\n",validDrag);
		    fprintf(stderr,"x0=%d, y0=%d, x1=%d, y1=%d\n",x0,y0,x1,y1);
#endif
		    if (validDrag) {
			saveUndoInfo(cdi);
			if (cdi->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(cdi);
			}
			updateDraggedElements(x0,y0,x1,y1);
		    }
		  /* Re-highlight currently selected elements */
		    highlightSelectedElements();
		  /* Update object prooperties of this element */
		    if (cdi->selectedDlElementList->count == 1) {
			objectDataOnly = True;
			updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
		    }
		} else {
		  /* This element not already selected,deselect others and move it */
		  /* Unhighlight currently selected elements */
#if DEBUG_EVENTS
		    fprintf(stderr,"Not already selected\n");
#endif
		    unhighlightSelectedElements();
		    clearDlDisplayList(cdi->selectedDlElementList);
		    appendDlList(cdi->selectedDlElementList,
		      tmpDlElementList);
		    validDrag = doDragging(XtWindow(cdi->drawingArea),
		      daWidth,daHeight,x0,y0,&x1,&y1);
#if DEBUG_EVENTS > 1
		    fprintf(stderr,"validDrag=%d\n",validDrag);
		    fprintf(stderr,"x0=%d, y0=%d, x1=%d, y1=%d\n",x0,y0,x1,y1);
#endif
		  /* Move this element */
		    if (validDrag) {
			saveUndoInfo(cdi);
			if (cdi->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(cdi);
			}
			updateDraggedElements(x0,y0,x1,y1);
		    }
		    highlightSelectedElements();
		    clearDlDisplayList(tmpDlElementList);
		    clearResourcePaletteEntries();
		    if (NumberOfDlElement(cdi->selectedDlElementList)==1){
			currentElementType =
			  FirstDlElement(cdi->selectedDlElementList)
			  ->structure.element->type;
			setResourcePaletteEntries();
		    }
		}
	    }
	    clearDlDisplayList(tmpDlElementList);
	    break;
	    
	case Button3:
	  /* SELECT_ACTION Btn3 */
#if DEBUG_EVENTS
	    fprintf(stderr,"SELECT_ACTION Btn3\n");
#endif
	  /* Edit menu doesn't have valid/unique displayInfo ptr, hence use current */
	    lastEvent = *((XButtonPressedEvent *)event);
	    XmMenuPosition(cdi->editPopupMenu,
	      (XButtonPressedEvent *)event);
	    XtManageChild(cdi->editPopupMenu);
#if DEBUG_EDIT_POPUP
	    {
		Dimension height, width;
		Position x, y;
		WidgetList children;
		Cardinal numChildren;
		Boolean resizeHeight, resizeWidth, adjustLast;
		int i;
		
		XtVaGetValues(cdi->editPopupMenu,
		  XmNheight,&height,
		  XmNwidth,&width,
		  XmNresizeHeight,&resizeHeight,
		  XmNresizeWidth,&resizeWidth,
		  XmNadjustLast,&adjustLast,
		  XmNchildren,&children,
		  XmNnumChildren,&numChildren,
		  NULL);
		printf("\ncdi->editPopupMenu: "
		  "height: %d   width: %d  children: %d\n"
		  "  resizeHeight: %d   resizeWidth: %d   adjustLast: %d\n",
		  height, width, (int)numChildren,
		  resizeHeight, resizeWidth, adjustLast);
		for(i=0; i < (int)numChildren; i++) {
		    XtVaGetValues(children[i],
		      XmNheight, &height,
		      XmNwidth, &width,
		      XmNx, &x,
		      XmNy, &y,
		      NULL);
		    printf("  %2d  %x  height: %2d   width: %3d   x: %d   y: %3d (%3d)\n",
		      i, children[i], height, width, x, y, y-9);
		}
	    }
#endif	    
#if 0	    
	  /* KE: Are these necessary ? */
	    XtPopup(XtParent(cdi->editPopupMenu),XtGrabNone);
	    XRaiseWindow(display,XtWindow(cdi->editPopupMenu));
#endif	    
	    
	    break;
	}
#if DEBUG_EVENTS
	fprintf(stderr,"\n[handleEditButtonPress: SELECT done] selectedDlElement list :\n");
	dumpDlElementList(cdi->selectedDlElementList);
#endif
    } else if (currentActionType == CREATE_ACTION) {
      /* CREATE_ACTION
       * ********************************************
       * Btn1    =  create (rubberband)             *
       * Btn2    =  nothing                         *
       * Btn3    =  nothing here                    *
       * ********************************************/
	DlElement *dlElement = 0;
	
	switch (xEvent->button) {
	case Button1:
	  /* CREATE_ACTION Btn1 */
	  /* Wait for next event and look at it */
#if DEBUG_CREATE
	    printf("\nhandleButtonPress EVENT: Type: %-13s  Button: %d  Window %x  SubWindow: %x\n"
	      "  Shift: %s  Ctrl: %s\n",
	      (xEvent->type == ButtonPress)?"ButtonPress":"ButtonRelease",
	      xEvent->button, xEvent->window, xEvent->subwindow,
	      xEvent->state&ShiftMask?"Yes":"No",
	      xEvent->state&ControlMask?"Yes":"No");
	    printf("  Send_event: %s  State: %x\n",
	      xEvent->send_event?"True":"False",xEvent->state);
	    printf("  ButtonPress=%d ButtonRelease=%d MotionNotify=%d\n",
	      ButtonPress, ButtonRelease, MotionNotify);
	    {
		XEvent newEvent;
		
		XPeekEvent(display,&newEvent);
		printf("              peekEVENT: Type: %d  Button: %d  Window %x  SubWindow: %x\n"
		  "  Shift: %s  Ctrl: %s\n",
		  newEvent.xbutton.type,
		  newEvent.xbutton.button, newEvent.xbutton.window, newEvent.xbutton.subwindow,
		  newEvent.xbutton.state&ShiftMask?"Yes":"No",
		  newEvent.xbutton.state&ControlMask?"Yes":"No");
		printf("  Send_event: %s  State: %x\n",
		  newEvent.xbutton.send_event?"True":"False",newEvent.xbutton.state);
	    }
#endif
	  /* Don't allow starting on a widget */
	    if(xEvent->window != XtWindow(cdi->drawingArea)) {
		XBell(display,50);
		return;
	    }
	  /* For DL_Text check if a ButtonRelease comes before a MotionNotify
	   *   (Indicating create text by typing) */
	    doTextByTyping = 0;
	    if(currentElementType == DL_Text) {
		XEvent newEvent;

		XWindowEvent(display,XtWindow(cdi->drawingArea),
		  ButtonReleaseMask|Button1MotionMask,&newEvent);
		if(newEvent.type == ButtonRelease) doTextByTyping = 1;
		XPutBackEvent(display,&newEvent);
#if DEBUG_CREATE
		printf("               newEVENT: Type: %d  Button: %d  Window %x  SubWindow: %x\n"
		  "  Shift: %s  Ctrl: %s\n",
		  newEvent.xbutton.type,
		  newEvent.xbutton.button, newEvent.xbutton.window, newEvent.xbutton.subwindow,
		  newEvent.xbutton.state&ShiftMask?"Yes":"No",
		  newEvent.xbutton.state&ControlMask?"Yes":"No");
		printf("  Send_event: %s  State: %x\n",
		  newEvent.xbutton.send_event?"True":"False",newEvent.xbutton.state);
#endif
	    }
	  /* Create the element according to its method */
	    if (currentElementType == DL_Text && doTextByTyping) {
		saveUndoInfo(cdi);
		if (cdi->hasBeenEditedButNotSaved == False) {
		    medmMarkDisplayBeingEdited(cdi);
		}
		dlElement = handleTextCreate(x0,y0);
	    } else if (currentElementType == DL_Polyline) {
		saveUndoInfo(cdi);
		if (cdi->hasBeenEditedButNotSaved == False) {
		    medmMarkDisplayBeingEdited(cdi);
		}
		dlElement = handlePolylineCreate(x0,y0,(Boolean)False);
	    } else if (currentElementType == DL_Line) {
		saveUndoInfo(cdi);
		if (cdi->hasBeenEditedButNotSaved == False) {
		    medmMarkDisplayBeingEdited(cdi);
		}
		dlElement = handlePolylineCreate(x0,y0,(Boolean)True);
	    } else if (currentElementType == DL_Polygon) {
		saveUndoInfo(cdi);
		if (cdi->hasBeenEditedButNotSaved == False) {
		    medmMarkDisplayBeingEdited(cdi);
		}
		dlElement = handlePolygonCreate(x0,y0);
	    } else {
	      /* Do rubber banding for initial size */
		if(!doRubberbanding(XtWindow(cdi->drawingArea),
		  &x0,&y0,&x1,&y1,True)) break;
	      /* Set minimum initial size */
		if (ELEMENT_HAS_WIDGET(currentElementType)) {
		    minSize = 12;
		} else {
		    minSize = 2;
		}
		if(cdi->grid->snapToGrid) {
		    int gridSpacing = cdi->grid->gridSpacing;
		    int minSize0 = minSize;
		    
		    minSize = ((minSize + gridSpacing/2)/gridSpacing)*
		      gridSpacing;
		    if(minSize < minSize0) minSize+=gridSpacing;
		}
	      /* Create elements */
		dlElement = handleRectangularCreates(currentElementType, x0, y0,
		  MAX(minSize,x1 - x0),MAX(minSize,y1 - y0));
	    }
	  /* Element defined, do bookkeeping */
	    if (dlElement) {
		DlElement *pSE = 0;

	      /* Mark display as not saved */
		saveUndoInfo(cdi);
		if (cdi->hasBeenEditedButNotSaved == False) {
		    medmMarkDisplayBeingEdited(cdi);
		}
	      /* Add it to cdi */
		appendDlElement(cdi->dlElementList,dlElement);
		(*dlElement->run->execute)(cdi,dlElement);
	      /* Unselect any selected elements */
		unhighlightSelectedElements();
		clearDlDisplayList(cdi->selectedDlElementList);
	      /* Create a DL_Element */
		pSE = createDlElement(DL_Element,(XtPointer)dlElement,NULL);
	      /* Add it to selectedDlElementList */
		if (pSE) {
		    appendDlElement(cdi->selectedDlElementList,pSE);
		}
	      /* Set currentElementType */
		currentElementType =
		  FirstDlElement(cdi->selectedDlElementList)
		  ->structure.element->type;
	      /* Hightlight it */
		highlightSelectedElements();
	    }
	    break;
	case Button2:
	  /* CREATE_ACTION Btn2 */
	    XBell(display,50);
	    break;
	    
	case Button3:
	  /* CREATE_ACTION Btn3 */
	    XBell(display,50);
	    break;
	}
#if DEBUG_EVENTS
	fprintf(stderr,"\n[handleEditButtonPress: CREATE done] selectedDLElement list :\n");
	dumpDlElementList(cdi->selectedDlElementList);
#endif
      /* Clean up */
      /* Toggle CREATE_ACTION to SELECT_ACTION */
	if (objectS != NULL) {
	    XmToggleButtonSetState(objectPaletteSelectToggleButton,True,True);
	}
      /* Set the object palette to Select */
	setActionToSelect();
      /* Set the resource palette according to currentElementType */
	setResourcePaletteEntries();
    }
}

void handleEditKeyPress(Widget w, XtPointer clientData, XEvent *event, Boolean *ctd)
{
    DisplayInfo *displayInfo = (DisplayInfo *)clientData;
    XKeyEvent *key = (XKeyEvent *)event;
    Modifiers modifiers;
    KeySym keysym;

#if DEBUG_EVENTS || DEBUG_KEYS
    fprintf(stderr,"\n>>> handleEditKeyPress: %s Type: %d "
      "Shift: %d Ctrl: %d\n"
      "  [KeyPress=%d, KeyRelease=%d ButtonPress=%d, ButtonRelease=%d]\n",
      currentActionType == SELECT_ACTION?"SELECT":"CREATE",key->type,
      key->state&ShiftMask,key->state&ControlMask,
      KeyPress,KeyRelease,ButtonPress,ButtonRelease);     /* In X.h */
    fprintf(stderr,"\n[handleEditKeyPress] displayInfo->selectedDlElementList:\n");
    dumpDlElementList(displayInfo->selectedDlElementList);
/*     fprintf(stderr,"\n[handleEditKeyPress] " */
/*       "currentDisplayInfo->selectedDlElementList:\n"); */
/*     dumpDlElementList(currentDisplayInfo->selectedDlElementList); */
    fprintf(stderr,"\n");

#endif
  /* Explicitly set continue to dispatch to avoid warnings */
    *ctd=True;
  /* Left/Right/Up/Down for movement of selected elements */
    if (currentActionType == SELECT_ACTION && displayInfo &&
      !IsEmpty(displayInfo->selectedDlElementList)) {
      /* Handle key press */	    
	if (key->type == KeyPress) {
	    int interested=1;
	    int ctrl;
	    
	  /* Determine if Ctrl was pressed */
	    ctrl=key->state&ControlMask;
	  /* Branch depending on keysym */
	    XtTranslateKeycode(display,key->keycode,(Modifiers)NULL,
	      &modifiers,&keysym);
#if DEBUG_EVENTS || DEBUG_KEYS
	    fprintf(stderr,"handleEditKeyPress: keycode=%d keysym=%d ctrl=%d\n",
	      key->keycode,keysym,ctrl);
#endif
	    switch (keysym) {
	    case osfXK_Left:
		if(ctrl) updateResizedElements(1,0,0,0);
		else updateDraggedElements(1,0,0,0);
		break;
	    case osfXK_Right:
		if(ctrl) updateResizedElements(0,0,1,0);
		else updateDraggedElements(0,0,1,0);
		break;
	    case osfXK_Up:
		if(ctrl) updateResizedElements(0,1,0,0);
		else updateDraggedElements(0,1,0,0);
		break;
	    case osfXK_Down:
		if(ctrl) updateResizedElements(0,0,0,1);
		else updateDraggedElements(0,0,0,1);
		break;
	    default:
		interested=0;
		break;
	    }
	    if(interested) {
		if (displayInfo->selectedDlElementList->count == 1) {
		    setResourcePaletteEntries();
		}
		if (displayInfo->hasBeenEditedButNotSaved == False) {
		    medmMarkDisplayBeingEdited(displayInfo);
		}
	    }
	}
    }
}

void highlightSelectedElements()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *dlElement;

#if DEBUG_HIGHLIGHTS
    printf("In highlightSelectedElements\n");
#endif
    
    if (!cdi) return;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    if (cdi->selectedElementsAreHighlighted) return;
    cdi->selectedElementsAreHighlighted = True;
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	toggleSelectedElementHighlight(dlElement->structure.element);
	dlElement = dlElement->next;
    }
}

/*
 *  Function to unhighlight the currently highlighted (and therefore
 *    selected) elements
 */
void unhighlightSelectedElements()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *dlElement;

#if DEBUG_HIGHLIGHTS
    printf("In unhighlightSelectedElements\n");
#endif
    
    if (!cdi) return;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    if (!cdi->selectedElementsAreHighlighted) return;
    cdi->selectedElementsAreHighlighted = False;
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	toggleSelectedElementHighlight(dlElement->structure.element);
	dlElement = dlElement->next;
    }
}

/*
 * Update (move) all currently selected elements and rerender
 */
static void updateDraggedElements(Position x0, Position y0,
  Position x1, Position y1)
{
    int i, j, xOffset, yOffset;
    DisplayInfo *cdi = currentDisplayInfo;    
    Widget widget;
    DlElement *pElement;

  /* If no current display or selected elements array, simply return */
    if (!cdi) return;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    xOffset = x1 - x0;
    yOffset = y1 - y0;

  /* See destroy-recreate comments in updateResizedElements */
  /* As usual, type in union unimportant as long as object is 1st thing...*/
    unhighlightSelectedElements();
    pElement = FirstDlElement(cdi->selectedDlElementList);
    while (pElement) {
	DlElement *pE = pElement->structure.element;
#if DEBUG_EVENTS > 1
	fprintf(stderr,"\nupdateDraggedElements: x0=%d y0=%d x1=%d y1=%d"
	  " xOffset=%d yOffset=%d\n",
	  x0,y0,x1,y1,xOffset,yOffset);
	fprintf(stderr, "  %s (%s) pE->run->move=%x\n",
	  elementType(pElement->type),
	  elementType(pE->type),
	  pE->run->move);
#endif
	if (pE->run->move) {
	    pE->run->move(pE, xOffset, yOffset);
	}
	if (pE->widget) {
	  /* Destroy the widgets */
	    destroyElementWidgets(pE);
	  /* Recreate them */
	    pE->run->execute(cdi,pE);
	}
	pElement = pElement->next;
    }
    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
    highlightSelectedElements();
}

/*
 * Update (resize) all currently selected elements and rerender
 */
void updateResizedElements(Position x0, Position y0, Position x1, Position y1)
{
    int i, j, xOffset, yOffset;
    DisplayInfo *cdi = currentDisplayInfo;
    Widget widget;
    DlElement *pElement;

/* If no current display or selected elements array, simply return */
    if (!cdi) return;
    if (IsEmpty(cdi->selectedDlElementList)) return;

    xOffset = x1 - x0;
    yOffset = y1 - y0;

  /*
   * Use expensive but reliable destroy-update-recreate sequence to get
   *   resizing right.  (XtResizeWidget mostly worked here, except for
   *   aggregate types like menu which had children which really defined
   *   their parent's size.)
   * One additional advantage - this method guarantees WYSIWYG w.r.t. fonts, etc
   * KE:  Also, it will not let you resize them with XtGetValues if they are off
   *   the edge of the display, but you can recreate them off the edge
   */

  /* As usual, type in union unimportant as long as object is 1st thing... */
    unhighlightSelectedElements();
    pElement = FirstDlElement(cdi->selectedDlElementList);
    while (pElement) {
	DlElement *pE = pElement->structure.element;
	if (pE->run->scale) {
	    pE->run->scale(pE,xOffset,yOffset);
	}
	if (pE->widget) {
	  /* Destroy the widget */
	    destroyElementWidgets(pE);
	  /* Recreate it */
	    pE->run->execute(cdi,pE);
#if DEBUG_EVENTS > 1
	    {
		DlObject *po = &pE->structure.composite->object;
		Arg args[20];
		int n=0;
		Position x,y;
		Dimension width,height;
		
		XFlush(display);
		XtSetArg(args[n],XmNx,&x); n++;
		XtSetArg(args[n],XmNy,&y); n++;
		XtSetArg(args[n],XmNwidth,&width); n++;
		XtSetArg(args[n],XmNheight,&height); n++;
		XtGetValues(pE->widget,args,n);
		
		fprintf(stderr,"\nupdateResizedElements\n");
		fprintf(stderr,"Widget:  %s: x=%d y=%d width=%u height=%u\n",
		  elementType(pE->type),
		  x,y,width,height);
		fprintf(stderr,"Element: %s: x=%d y=%d width=%u height=%u\n",
		  elementType(pE->type),
		  po->x,po->y,(int)po->width,(int)po->height);
	    }
#endif
	}
	pElement = pElement->next;
    }
    dmTraverseNonWidgetsInDisplayList(cdi);
    highlightSelectedElements();
}

/* 
 * Handle creates (based on filled in globalResourceBundle values and
 *   currentElementType);
 */
DlElement *handleRectangularCreates(DlElementType type,
  int x, int y, unsigned int width,  unsigned int height)
{
    DlElement *pE = 0;

    pE = (DlElement *)NULL;

  /* Create the actual element */
    switch(type) {
    case DL_Image:
	pE = handleImageCreate();
	break;
      /* Others are more straight-forward */
    case DL_Valuator:
	pE = createDlValuator(NULL);
	break;
    case DL_ChoiceButton:
	pE = createDlChoiceButton(NULL);
	break;
    case DL_MessageButton:
	pE = createDlMessageButton(NULL);
	break;
    case DL_TextEntry:
	pE = createDlTextEntry(NULL);
	break;
    case DL_Menu:
	pE = createDlMenu(NULL);
	break;
    case DL_Meter:
	pE = createDlMeter(NULL);
	break;
    case DL_TextUpdate:
	pE = createDlTextUpdate(NULL);
	break;
    case DL_Bar:
	pE = createDlBar(NULL);
	break;
    case DL_Indicator:
	pE = createDlIndicator(NULL);
	break;
    case DL_Byte:
	pE = createDlByte(NULL);
	break;
    case DL_StripChart:
	pE = createDlStripChart(NULL);
	break;
    case DL_CartesianPlot:
	pE = createDlCartesianPlot(NULL);
	break;
    case DL_Rectangle:
	pE = createDlRectangle(NULL);
	break;
    case DL_Oval:
	pE = createDlOval(NULL);
	break;
    case DL_Arc:
	pE = createDlArc(NULL);
	break;
    case DL_RelatedDisplay:
	pE = createDlRelatedDisplay(NULL);
	break;
    case DL_ShellCommand:
	pE = createDlShellCommand(NULL);
	break;
    case DL_Text:
	pE = createDlText(NULL);
	break;
    default:
	fprintf(stderr,"handleRectangularCreates: CREATE - invalid type %d\n",
	  type);
	break;
    }

    if (pE) {
	if (pE->run->inheritValues) {
	    pE->run->inheritValues(&globalResourceBundle,pE);
	}
	objectAttributeSet(&(pE->structure.rectangle->object),x,y,width,height);
    }
    return pE;
}

void toggleSelectedElementHighlight(DlElement *dlElement)
{
    DisplayInfo *cdi = currentDisplayInfo;
    int x, y, width, height; 
    DlObject *po = &(dlElement->structure.display->object);
    
    if (dlElement->type == DL_Display) {
	x = HIGHLIGHT_LINE_THICKNESS;
	y = HIGHLIGHT_LINE_THICKNESS;
	width = (int)po->width - 2*HIGHLIGHT_LINE_THICKNESS;
	height = (int)po->height - 2*HIGHLIGHT_LINE_THICKNESS;
    } else {
	x = po->x-HIGHLIGHT_LINE_THICKNESS;
	y = po->y-HIGHLIGHT_LINE_THICKNESS;
	width = (int)po->width + 2*HIGHLIGHT_LINE_THICKNESS;
	height = (int)po->height + 2*HIGHLIGHT_LINE_THICKNESS;
    }
#if 0
    XSetForeground(display,highlightGC,cdi->drawingAreaBackgroundColor);
    XSetBackground(display,highlightGC,cdi->drawingAreaForegroundColor);
#endif
    XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
      x, y, width, height);
    XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
      x, y, width, height);
}

void addCommonHandlers(Widget w, DisplayInfo *displayInfo)
{
  /* Switch depending on mode */
    switch (displayInfo->traversalMode) {
    case DL_EDIT :
	XtUninstallTranslations(w);     /* KE: This is necessary */
	XtAddEventHandler(w, KeyPressMask, False, handleEditKeyPress,
	  (XtPointer)displayInfo);
	XtAddEventHandler(w, ButtonPressMask, False, handleEditButtonPress,
	  (XtPointer)displayInfo);
	break;
    case DL_EXECUTE :
	XtOverrideTranslations(w, parsedTranslations);
	XtAddEventHandler(w, ButtonPressMask, False, handleExecuteButtonPress,
	  (XtPointer)displayInfo);
	break;
    }
}
