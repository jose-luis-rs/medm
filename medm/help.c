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

#define TIME_STRING_MAX 81

#include "medm.h"
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

/* Function prototypes */

extern FILE *popen(const char *, const char *);     /* May not be defined for strict ANSI */
extern int	pclose(FILE *);     /* May not be defined for strict ANSI */

static Widget errMsgDlg = NULL;
static Widget errMsgText = NULL;
static Widget errMsgSendDlg = NULL;
static Widget errMsgSendSubjectText = NULL;
static Widget errMsgSendToText = NULL;
static Widget errMsgSendText = NULL;
static XtIntervalId errMsgDlgTimeOutId = 0;

void errMsgSendDlgCreateDlg();
void errMsgSendDlgSendButtonCb(Widget,XtPointer,XtPointer);
void errMsgSendDlgCloseButtonCb(Widget,XtPointer,XtPointer);

void errMsgDlgCreateDlg();
void errMsgDlgSendButtonCb(Widget,XtPointer,XtPointer);
void errMsgDlgClearButtonCb(Widget,XtPointer,XtPointer);
void errMsgDlgCloseButtonCb(Widget,XtPointer,XtPointer);
static void medmUpdateCAStudtylDlg(XtPointer data, XtIntervalId *id);

#ifdef __cplusplus
void globalHelpCallback(Widget, XtPointer cd, XtPointer)
#else
void globalHelpCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int helpIndex = (int) cd;
    XmString string;

    switch (helpIndex) {
    case HELP_MAIN:
	string = XmStringCreateSimple("In Main Help...");
	XtVaSetValues(helpMessageBox,XmNmessageString,string,
	  NULL);
	XtPopup(helpS,XtGrabNone);
	XmStringFree(string);
	break;

    }
}

#ifdef __cplusplus
void errMsgDlgCloseButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgDlgCloseButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    if (errMsgDlg != NULL) {
	XtUnmanageChild(errMsgDlg);
    }
    return;
}

#ifdef __cplusplus
void errMsgDlgClearButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgDlgClearButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    long now; 
    struct tm *tblock;
    char timeStampStr[TIME_STRING_MAX];
    XmTextPosition curpos = 0;
    
    if (errMsgText == NULL) return;

  /* Clear the buffer */
    XmTextSetString(errMsgText,"");
    XmTextSetInsertionPosition(errMsgText, 0);

  /* Reinitialize */
    time(&now);
    tblock = localtime(&now);
    strftime(timeStampStr,TIME_STRING_MAX,
      "Message Log cleared at %a %h %e %k:%M:%S %Z %Y\n",tblock);
    timeStampStr[TIME_STRING_MAX-1]='0';
    XmTextInsert(errMsgText, curpos, timeStampStr);
    curpos+=strlen(timeStampStr);
    XmTextSetInsertionPosition(errMsgText, curpos);

    return;
}

#ifdef __cplusplus
void errMsgDlgPrintButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgDlgPrintButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    long now; 
    struct tm *tblock;
    FILE *file;
    char timeStampStr[TIME_STRING_MAX];
    char *tmp, *psFileName, *commandBuffer;

    if (errMsgText == NULL) return;
    if (getenv("PSPRINTER") == (char *)NULL) {
	medmPrintf(
	  "\nerrMsgDlgPrintButtonCb: PSPRINTER environment variable not set,"
	  " printing disallowed\n");
	return;
    }

  /* Get selection and timestamp */
    time(&now);
    tblock = localtime(&now);
    tmp = XmTextGetSelection(errMsgText);
    strftime(timeStampStr,TIME_STRING_MAX,
      "MEDM Message Window Selection at %a %h %e %k:%M:%S %Z %Y:\n\n",tblock);
    if (tmp == NULL) {
	tmp = XmTextGetString(errMsgText);
	strftime(timeStampStr,TIME_STRING_MAX,
	  "MEDM Message Window at %a %h %e %k:%M:%S %Z %Y:\n\n",tblock);
    }
    timeStampStr[TIME_STRING_MAX-1]='0';

  /* Create filename */
    psFileName = (char *)calloc(1,256);
    sprintf(psFileName,"/tmp/medmMessageWindowPrintFile%d",getpid());

  /* Write file */
    file = fopen(psFileName,"w+");
    if (file == NULL) {
	medmPrintf("\nerrMsgDlgPrintButtonCb:  Unable to open file: %s",
	  psFileName);
	XtFree(tmp);
	free(psFileName);
	return;
    }
    fprintf(file,timeStampStr);
    fprintf(file,tmp);
    XtFree(tmp);
    fclose(file);

  /* Print file */
    commandBuffer = (char *)calloc(1,256);
    sprintf(commandBuffer,"lp -d$PSPRINTER %s",psFileName);
    system(commandBuffer);

  /* Delete file */
    sprintf(commandBuffer,"rm %s",psFileName);
    system(commandBuffer);

  /* Clean up */
    free(psFileName);
    free(commandBuffer);
    XBell(display,50);
}

#ifdef __cplusplus
void errMsgDlgSendButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgDlgSendButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    char *tmp;

    if (errMsgText == NULL) return;
    if (errMsgSendDlg == NULL) {
	errMsgSendDlgCreateDlg();
    }
    XmTextSetString(errMsgSendToText,"");
    XmTextSetString(errMsgSendSubjectText,"MEDM Message Window Contents");
    tmp = XmTextGetSelection(errMsgText);
    if (tmp == NULL) {
	tmp = XmTextGetString(errMsgText);
    }
    XmTextSetString(errMsgSendText,tmp);
    XtFree(tmp);
    XtManageChild(errMsgSendDlg);
}

#ifdef __cplusplus
void errMsgDlgHelpButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgDlgHelpButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    callBrowser(MEDM_HELP_PATH"/MEDM.html#MessageWindow");
}

void errMsgDlgCreateDlg() {
    Widget pane;
    Widget actionArea;
    Widget closeButton, sendButton, clearButton, printButton, helpButton;
    Arg args[10];
    int n;

    if (errMsgDlg != NULL) {
	XtManageChild(errMsgDlg);
	return;
    }

    if (mainShell == NULL) return;

    errMsgDlg = XtVaCreatePopupShell("ErrorMessage",
      xmDialogShellWidgetClass, mainShell,
      XmNtitle, "MEDM Message Window",
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);

    pane = XtVaCreateWidget("panel",
      xmPanedWindowWidgetClass, errMsgDlg,
      XmNsashWidth, 1,
      XmNsashHeight, 1,
      NULL);
    n = 0;
    XtSetArg(args[n], XmNrows,  10); n++;
    XtSetArg(args[n], XmNcolumns, 80); n++;
    XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg(args[n], XmNeditable, False); n++;
    errMsgText = XmCreateScrolledText(pane,"text",args,n);
    XtManageChild(errMsgText);
    actionArea = XtVaCreateWidget("ActionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 11,
      XmNskipAdjust, True,
      NULL);

    closeButton = XtVaCreateManagedWidget("Close",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     1,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    2,
/*
  XmNshowAsDefault,    True,
  XmNdefaultButtonShadowThickness, 1,
  */
      NULL);
    XtAddCallback(closeButton,XmNactivateCallback,errMsgDlgCloseButtonCb, NULL);

    clearButton = XtVaCreateManagedWidget("Clear",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
      XmNtopWidget,        closeButton,
      XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
      XmNbottomWidget,     closeButton,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     3,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    4,
      NULL);
    XtAddCallback(clearButton,XmNactivateCallback,errMsgDlgClearButtonCb, NULL);

    printButton = XtVaCreateManagedWidget("Print",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     5,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    6,
      NULL);
    XtAddCallback(printButton,XmNactivateCallback,errMsgDlgPrintButtonCb, NULL);

    sendButton = XtVaCreateManagedWidget("Mail",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     7,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    8,
      NULL);
    XtAddCallback(sendButton,XmNactivateCallback,errMsgDlgSendButtonCb, NULL);

    helpButton = XtVaCreateManagedWidget("Help",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     9,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    10,
      NULL);
    XtAddCallback(helpButton,XmNactivateCallback,errMsgDlgHelpButtonCb, NULL);
    XtManageChild(actionArea);
    XtManageChild(pane);
    XtManageChild(errMsgDlg);

  /* Initialize */
    if(errMsgDlg) {
	long now; 
	struct tm *tblock;
	char timeStampStr[TIME_STRING_MAX];
	XmTextPosition curpos = 0;

	time(&now);
	tblock = localtime(&now);
	strftime(timeStampStr,TIME_STRING_MAX,
	  "Message Log started at %a %h %e %k:%M:%S %Z %Y\n",tblock);
	timeStampStr[TIME_STRING_MAX-1]='0';
	XmTextInsert(errMsgText, curpos, timeStampStr);
	curpos+=strlen(timeStampStr);
	XmTextSetInsertionPosition(errMsgText, curpos);
    }
}

#ifdef __cplusplus  
void errMsgSendDlgSendButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgSendDlgSendButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    char *text, *subject, *to, cmd[1024], *p;
    FILE *pp;
    int status;

    if (errMsgSendDlg == NULL) return;
    subject = XmTextFieldGetString(errMsgSendSubjectText);
    to = XmTextFieldGetString(errMsgSendToText);
    text = XmTextGetString(errMsgSendText);
    if (!(p = getenv("MEDM_MAIL_CMD")))
      p = "mail";
    p = strcpy(cmd,p);
    p += strlen(cmd);
    *p++ = ' ';
#if 0    
    if (subject && *subject) {
      /* KE: Doesn't work with Solaris */
	sprintf(p, "-s \"%s\" ", subject);
	p += strlen(p);
    }
#endif    
    if (to && *to) {
	sprintf(p, "%s", to);
    } else {
	medmPostMsg("errMsgSendDlgSendButtonCb: No recipient specified for mail\n");
	if (to) XtFree(to);
	if (subject) XtFree(subject);
	if (text) XtFree(text);
	XBell(display,50); XBell(display,50); XBell(display,50);
	return;
    }
    
    pp = popen(cmd, "w");
    if (!pp) {
	medmPostMsg("errMsgSendDlgSendButtonCb: Cannot execute mail command\n");
	if (to) XtFree(to);
	if (subject) XtFree(subject);
	if (text) XtFree(text);
	XBell(display,50); XBell(display,50); XBell(display,50);
	return;
    }
#if 1    
    if (subject && *subject) {
	fputs("Subject: ", pp);
	fputs(subject, pp);
	fputs("\n\n", pp);
    }
#endif    
    fputs(text, pp);
    fputc('\n', pp);      /* make sure there's a terminating newline */
    status = pclose(pp);  /* close mail program */
    if (to) XtFree(to);
    if (subject) XtFree(subject);
    if (text) XtFree(text);
    XBell(display,50);
    XtUnmanageChild(errMsgSendDlg);
    return;
}

#ifdef __cplusplus
void errMsgSendDlgCloseButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgSendDlgCloseButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    if (errMsgSendDlg != NULL)
      XtUnmanageChild(errMsgSendDlg);
}

void errMsgSendDlgCreateDlg() {
    Widget pane;
    Widget rowCol;
    Widget toForm;
    Widget toLabel;
    Widget subjectForm;
    Widget subjectLabel;
    Widget actionArea;
    Widget closeButton;
    Widget sendButton;
    Arg    args[10];
    int n;

    if (errMsgDlg == NULL) return;
    if (errMsgSendDlg == NULL) {
	errMsgSendDlg = XtVaCreatePopupShell("ErrorMessage",
	  xmDialogShellWidgetClass, mainShell,
	  XmNtitle, "MEDM Mail Message Window",
	  XmNdeleteResponse, XmDO_NOTHING,
	  NULL);
	pane = XtVaCreateWidget("panel",
	  xmPanedWindowWidgetClass, errMsgSendDlg,
	  XmNsashWidth, 1,
	  XmNsashHeight, 1,
	  NULL);
	rowCol = XtVaCreateWidget("rowCol",
	  xmRowColumnWidgetClass, pane,
	  NULL);
	toForm = XtVaCreateWidget("form",
	  xmFormWidgetClass, rowCol,
	  XmNshadowThickness, 0,
	  NULL);
	toLabel = XtVaCreateManagedWidget("To:",
	  xmLabelGadgetClass, toForm,
	  XmNleftAttachment,  XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  XmNbottomAttachment,XmATTACH_FORM,
	  NULL);
	errMsgSendToText = XtVaCreateManagedWidget("text",
	  xmTextFieldWidgetClass, toForm,
	  XmNleftAttachment,  XmATTACH_WIDGET,
	  XmNleftWidget,      toLabel,
	  XmNrightAttachment, XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  XmNbottomAttachment,XmATTACH_FORM,
	  NULL);
	XtManageChild(toForm);
	subjectForm = XtVaCreateManagedWidget("form",
	  xmFormWidgetClass, rowCol,
	  XmNshadowThickness, 0,
	  NULL);
	subjectLabel = XtVaCreateManagedWidget("Subject:",
	  xmLabelGadgetClass, subjectForm,
	  XmNleftAttachment,  XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  XmNbottomAttachment,XmATTACH_FORM,
	  NULL);
	errMsgSendSubjectText = XtVaCreateManagedWidget("text",
	  xmTextFieldWidgetClass, subjectForm,
	  XmNleftAttachment,  XmATTACH_WIDGET,
	  XmNleftWidget,      subjectLabel,
	  XmNrightAttachment, XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  XmNbottomAttachment,XmATTACH_FORM,
	  NULL);
	XtManageChild(subjectForm);
	n = 0;
	XtSetArg(args[n], XmNrows,  10); n++;
	XtSetArg(args[n], XmNcolumns, 80); n++;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNeditable, True); n++;
	errMsgSendText = XmCreateScrolledText(rowCol,"text",args,n);
	XtManageChild(errMsgSendText);
	actionArea = XtVaCreateWidget("ActionArea",
	  xmFormWidgetClass, pane,
	  XmNfractionBase, 5,
	  XmNskipAdjust, True,
	  XmNshadowThickness, 0,
	  NULL);
	closeButton = XtVaCreateManagedWidget("Close",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     1,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    2,
/*
  XmNshowAsDefault,    True,
  XmNdefaultButtonShadowThickness, 1,
  */
	  NULL);
	XtAddCallback(closeButton,XmNactivateCallback,errMsgSendDlgCloseButtonCb, NULL);
	sendButton = XtVaCreateManagedWidget("Send",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     3,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    4,
	  NULL);
	XtAddCallback(sendButton,XmNactivateCallback,errMsgSendDlgSendButtonCb, NULL);
    }
    XtManageChild(actionArea);
    XtManageChild(rowCol);
    XtManageChild(pane);
}

static char medmPrintfStr[2048]; /* DANGER: Fixed buffer size */

void medmPostMsg(char *format, ...) {
    va_list args;
    long now; 
    struct tm *tblock;
    char timeStampStr[TIME_STRING_MAX];
    XmTextPosition curpos;

  /* Create (or manage) the error dialog */
    errMsgDlgCreateDlg();

  /* Do timestamp */
    time(&now);
    tblock = localtime(&now);
    strftime(timeStampStr,TIME_STRING_MAX,"\n%a %h %e %k:%M:%S %Z %Y\n",tblock);
    timeStampStr[TIME_STRING_MAX-1]='0';
    if(errMsgText) {
	curpos = XmTextGetLastPosition(errMsgText);
	XmTextInsert(errMsgText, curpos, timeStampStr);
	curpos+=strlen(timeStampStr);
    }
  /* Also print to stderr */
    fprintf(stderr, timeStampStr);

  /* Start variable arguments */
    va_start(args,format);
    vsprintf(medmPrintfStr, format, args);
    if(errMsgText) {
	XmTextInsert(errMsgText, curpos, medmPrintfStr);
	curpos+=strlen(medmPrintfStr);
	XmTextSetInsertionPosition(errMsgText, curpos);
	XmTextShowPosition(errMsgText, curpos);
	XFlush(display);
    }

  /* Also print to stderr */
    fprintf(stderr, medmPrintfStr);
    va_end(args);

  /* Raise window */
    if(errMsgDlg && XtIsRealized(errMsgDlg))
      XRaiseWindow(display,XtWindow(errMsgDlg));
}

void medmPrintf(char *format, ...)
{
    va_list args;
    XmTextPosition curpos;

  /* Create (or manage) the error dialog */
    errMsgDlgCreateDlg();

    va_start(args,format);
    vsprintf(medmPrintfStr, format, args);
    if(errMsgText) {
	curpos = XmTextGetLastPosition(errMsgText);
	XmTextInsert(errMsgText, curpos, medmPrintfStr);
	curpos+=strlen(medmPrintfStr);
	XmTextSetInsertionPosition(errMsgText, curpos);
	XmTextShowPosition(errMsgText, curpos);
	XFlush(display);
    }
  /* Also print to stderr */
    fprintf(stderr, medmPrintfStr);
    va_end(args);

  /* Raise window */
    if(errMsgDlg && XtIsRealized(errMsgDlg))
      XRaiseWindow(display,XtWindow(errMsgDlg));
}

/* KE: No longer used */
void medmPostTime() {
    long now; 
    struct tm *tblock;
    char timeStampStr[TIME_STRING_MAX];
    XmTextPosition curpos;

  /* Create (or manage) the error dialog */
    errMsgDlgCreateDlg();

    time(&now);
    tblock = localtime(&now);
    strftime(timeStampStr,TIME_STRING_MAX,"\n%a %h %e %k:%M:%S %Z %Y\n",tblock);
    timeStampStr[TIME_STRING_MAX-1]='0';
    if(errMsgText) {
	curpos = XmTextGetLastPosition(errMsgText);
	XmTextInsert(errMsgText, curpos, timeStampStr);
	curpos+=strlen(timeStampStr);
	XmTextSetInsertionPosition(errMsgText, curpos);
	XmTextShowPosition(errMsgText, curpos);
	XFlush(display);
    }
  /* Also print to stderr */
    fprintf(stderr, timeStampStr);

  /* Raise window */
    if(errMsgDlg && XtIsRealized(errMsgDlg))
      XRaiseWindow(display,XtWindow(errMsgDlg));
}

static char caStudyMsg[512];
static Widget caStudyDlg = NULL;
static Boolean caUpdateStudyDlg = False;
static char *caStatusDummyString =
"Time Interval (sec)       =         \n"
"CA connection(s)          =         \n"
"CA connected              =         \n"
"CA incoming event(s)      =         \n"
"Active Objects            =         \n"
"Object(s) Updated         =         \n"
"Update Requests           =         \n"
"Update Requests Discarded =         \n"
"Update Requests Queued    =         \n";


static double totalTimeElapsed = 0.0;
static double aveCAEventCount = 0.0;
static double aveUpdateExecuted = 0;
static double aveUpdateRequested = 0;
static double aveUpdateRequestDiscarded = 0;
static Boolean caStudyAverageMode = False;

#ifdef __cplusplus
void caStudyDlgCloseButtonCb(Widget, XtPointer, XtPointer)
#else
void caStudyDlgCloseButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    if (caStudyDlg != NULL) {
	XtUnmanageChild(caStudyDlg);
	caUpdateStudyDlg = False;
    }
    return;
}

#ifdef __cplusplus
void caStudyDlgResetButtonCb(Widget, XtPointer, XtPointer)
#else
void caStudyDlgResetButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    totalTimeElapsed = 0.0;
    aveCAEventCount = 0.0;
    aveUpdateExecuted = 0.0;
    aveUpdateRequested = 0.0;
    aveUpdateRequestDiscarded = 0.0;
    return;
}

#ifdef __cplusplus
void caStudyDlgModeButtonCb(Widget, XtPointer, XtPointer)
#else
void caStudyDlgModeButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    caStudyAverageMode = !(caStudyAverageMode);
    return;
}

void medmCreateCAStudyDlg() {
    Widget pane;
    Widget actionArea;
    Widget closeButton;
    Widget resetButton;
    Widget modeButton;
    XmString str;

    if (!caStudyDlg) {

	if (mainShell == NULL) return;

	caStudyDlg = XtVaCreatePopupShell("status",
	  xmDialogShellWidgetClass, mainShell,
	  XmNtitle, "MEDM Message Window",
	  XmNdeleteResponse, XmDO_NOTHING,
	  NULL);

	pane = XtVaCreateWidget("panel",
	  xmPanedWindowWidgetClass, caStudyDlg,
	  XmNsashWidth, 1,
	  XmNsashHeight, 1,
	  NULL);

	str = XmStringLtoRCreate(caStatusDummyString,XmSTRING_DEFAULT_CHARSET);

	caStudyLabel = XtVaCreateManagedWidget("status",
	  xmLabelWidgetClass, pane,
	  XmNalignment, XmALIGNMENT_BEGINNING,
	  XmNlabelString,str,
	  NULL);
	XmStringFree(str);
  
    
	actionArea = XtVaCreateWidget("ActionArea",
	  xmFormWidgetClass, pane,
	  XmNshadowThickness, 0,
	  XmNfractionBase, 7,
	  XmNskipAdjust, True,
	  NULL);
	closeButton = XtVaCreateManagedWidget("Close",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     1,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    2,
	  NULL);
	resetButton = XtVaCreateManagedWidget("Reset",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     3,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    4,
	  NULL);
	modeButton = XtVaCreateManagedWidget("Mode",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     5,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    6,
	  NULL);
	XtAddCallback(closeButton,XmNactivateCallback,caStudyDlgCloseButtonCb, NULL);
	XtAddCallback(resetButton,XmNactivateCallback,caStudyDlgResetButtonCb, NULL);
	XtAddCallback(modeButton,XmNactivateCallback,caStudyDlgModeButtonCb, NULL);
	XtManageChild(actionArea);
	XtManageChild(pane);
    }

    XtManageChild(caStudyDlg);
    caUpdateStudyDlg = True;
    if (globalDisplayListTraversalMode == DL_EXECUTE) {
	if (errMsgDlgTimeOutId == 0)
	  errMsgDlgTimeOutId = XtAppAddTimeOut(appContext,1000,medmUpdateCAStudtylDlg,NULL);
    } else {
	errMsgDlgTimeOutId = 0;
    }
}

#ifdef __cplusplus
static void medmUpdateCAStudtylDlg(XtPointer, XtIntervalId *id)
#else
static void medmUpdateCAStudtylDlg(XtPointer cd, XtIntervalId *id)
#endif
{
    if (caUpdateStudyDlg) {
	XmString str;
	int taskCount;
	int periodicTaskCount;
	int updateRequestCount;
	int updateDiscardCount;
	int periodicUpdateRequestCount;
	int periodicUpdateDiscardCount;
	int updateRequestQueued;
	int updateExecuted;
	int totalTaskCount;
	int totalUpdateRequested;
	int totalUpdateDiscarded;
	double timeInterval; 
	int channelCount;
	int channelConnected;
	int caEventCount;

	updateTaskStatusGetInfo(&taskCount,
	  &periodicTaskCount,
	  &updateRequestCount,
	  &updateDiscardCount,
	  &periodicUpdateRequestCount,
	  &periodicUpdateDiscardCount,
	  &updateRequestQueued,
	  &updateExecuted,
	  &timeInterval); 
	CATaskGetInfo(&channelCount,&channelConnected,&caEventCount);
	totalUpdateDiscarded = updateDiscardCount+periodicUpdateDiscardCount;
	totalUpdateRequested = updateRequestCount+periodicUpdateRequestCount + totalUpdateDiscarded;
	totalTaskCount = taskCount + periodicTaskCount;
	if (caStudyAverageMode) {
	    double elapseTime = totalTimeElapsed;
	    totalTimeElapsed += timeInterval;
	    aveCAEventCount = (aveCAEventCount * elapseTime + caEventCount) / totalTimeElapsed;
	    aveUpdateExecuted = (aveUpdateExecuted * elapseTime + updateExecuted) / totalTimeElapsed;
	    aveUpdateRequested = (aveUpdateRequested * elapseTime + totalUpdateRequested) / totalTimeElapsed;
	    aveUpdateRequestDiscarded =
	      (aveUpdateRequestDiscarded * elapseTime + totalUpdateDiscarded) / totalTimeElapsed;
	    sprintf(caStudyMsg,
	      "AVERAGE :\n"
	      "Total Time Elapsed        = %8.1f\n"
	      "CA Incoming Event(s)      = %8.1f\n"
	      "Object(s) Updated         = %8.1f\n"
	      "Update Requests           = %8.1f\n"
	      "Update Requests Discarded = %8.1f\n",
	      totalTimeElapsed,
	      aveCAEventCount,
	      aveUpdateExecuted,
	      aveUpdateRequested,
	      aveUpdateRequestDiscarded);
	} else { 
	    sprintf(caStudyMsg,  
	      "Time Interval (sec)       = %8.2f\n"
	      "CA connection(s)          = %8d\n"
	      "CA connected              = %8d\n"
	      "CA incoming event(s)      = %8d\n"
	      "Active Objects            = %8d\n"
	      "Object(s) Updated         = %8d\n"
	      "Update Requests           = %8d\n"
	      "Update Requests Discarded = %8d\n"
	      "Update Requests Queued    = %8d\n",
	      timeInterval,
	      channelCount,
	      channelConnected,
	      caEventCount,
	      totalTaskCount,
	      updateExecuted,
	      totalUpdateRequested,
	      totalUpdateDiscarded,
	      updateRequestQueued);
	}                   
	str = XmStringLtoRCreate(caStudyMsg,XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(caStudyLabel,XmNlabelString,str,NULL);
	XmStringFree(str);
	XFlush(XtDisplay(caStudyDlg));
	XmUpdateDisplay(caStudyDlg);
	if (globalDisplayListTraversalMode == DL_EXECUTE) {
	    if (errMsgDlgTimeOutId == *id)
	      errMsgDlgTimeOutId = XtAppAddTimeOut(appContext,1000,medmUpdateCAStudtylDlg,NULL);
	} else {
	    errMsgDlgTimeOutId = 0;
	}
    } else {
	errMsgDlgTimeOutId = 0;
    }
}

void medmStartUpdateCAStudyDlg() {
    if (globalDisplayListTraversalMode == DL_EXECUTE) {
	if (errMsgDlgTimeOutId == 0) 
	  errMsgDlgTimeOutId = XtAppAddTimeOut(appContext,3000,medmUpdateCAStudtylDlg,NULL);
    } else {
	errMsgDlgTimeOutId = 0;
    }
}

int xErrorHandler(Display *dpy, XErrorEvent *event)
{
    char buf[4096];     /* Warning: Fixed Size */
    
    XGetErrorText(dpy,event->error_code,buf,BUFSIZ);
    fprintf(stderr,"\n%s\n",buf);
    return 0;
}

void xtErrorHandler(char *message)
{
    fprintf(stderr,"\n%s\n",message);
}
