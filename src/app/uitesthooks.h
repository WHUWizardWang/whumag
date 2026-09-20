#ifndef UITESTHOOKS_H
#define UITESTHOOKS_H

#ifdef WHUMAG_UI_TEST

#include <QString>

class QWidget;

// Development-only helpers, compiled only when qmake is run with "DEFINES+=WHUMAG_UI_TEST".
// Driven by environment variables so a build can be smoke-tested without clicking through the UI:
//   WHUMAG_TEST_OPEN   comma separated slot names invoked on the main window (e.g. on_action_query_triggered)
//   WHUMAG_TEST_GRAB   directory that receives a PNG of every visible top-level window
//   WHUMAG_TEST_DELAY  milliseconds to wait before grabbing (default 2500)
void uiTestScheduleForMainWindow(QWidget *mainWindow);
// Grabs |w| to <WHUMAG_TEST_GRAB>/<name>.png and returns true if the variable is set.
bool uiTestGrab(QWidget *w, const QString &name);

#endif // WHUMAG_UI_TEST

#endif // UITESTHOOKS_H
