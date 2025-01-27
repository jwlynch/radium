/* Copyright 2003 Kjetil S. Matheussen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

#define __STDC_FORMAT_MACROS 1

#include "../common/includepython.h"

#include <inttypes.h>


#include <boost/version.hpp>
#if (BOOST_VERSION < 100000) || ((BOOST_VERSION / 100 % 1000) < 63)
  #error "Boost too old. Need at least 1.58.\n Quick fix: cd $HOME ; wget http://downloads.sourceforge.net/project/boost/boost/1.63.0/boost_1_63_0.tar.bz2 ; tar xvjf boost_1_63_0.tar.bz2 (that's it!)"
#endif
#include <boost/lockfree/queue.hpp>



#include <signal.h>
#include <unistd.h>

#include <gc.h>

#if USE_QT5
#include <QPluginLoader>
#endif

#include <QtWebKitWidgets/QWebView>

#define TEST_CRASHREPORTER 0

#include <qapplication.h>
#include <qsplashscreen.h>
#include <qsplitter.h>
#include <qpalette.h>
#include <qtabwidget.h>
#include <qfontdatabase.h>
#include <QEvent>
#include <QKeyEvent>
#include <QMenuBar>
#include <Qt>
#include <QDir>
#include <QTextEdit>
#include <QLayout>
#include <QDesktopServices>
#include <QTextCodec>
#include <QWindow>
#include <QScreen>

#include <QStyleFactory>


#ifdef __linux__
#ifndef USE_QT5
#  include <QX11Info>
#endif
#endif

#ifdef USE_QT4

//Added by qt3to4:
#include <QEvent>
#ifndef USE_QT5
#  include <QCustomEvent>
#endif
#endif


#define INCLUDE_SNDFILE_OPEN_FUNCTIONS 1
#include "../common/nsmtracker.h"
#include "../common/threading.h"
#include "../common/blocks_proc.h"
#include "../common/disk_load_proc.h"
#include "../common/patch_proc.h"
#include "../common/undo.h"
#include "../common/nag.h"
#include "../common/OS_settings_proc.h"
#include "../common/OS_visual_input.h"
#include "../common/scancodes_proc.h"
#include "../common/player_proc.h"
#include "../common/player_pause_proc.h"
#include "../common/gfx_wtrackheaders_proc.h"
#include "../common/data_as_text_proc.h"
#include "../common/sequencer_proc.h"
#include "../common/Process.hpp"

#include "../api/api_proc.h"
#include "../api/api_gui_proc.h"
#include "../api/api_various_proc.h"
#include "../api/api_instruments_proc.h"
#include "../api/api_common_proc.h"

#include "../mixergui/QM_MixerWidget.h"

#include "EditorWidget.h"
#include "Qt_colors_proc.h"
#include "Qt_AutoBackups_proc.h"
#include "Qt_Bs_edit_proc.h"

#include "../common/eventreciever_proc.h"
#include "../common/control_proc.h"
#include "../common/settings_proc.h"
#include "../common/visual_proc.h"

#include "../common/OS_settings_proc.h"
#include "../common/OS_Player_proc.h"
#include "../common/OS_system_proc.h"

#include "../crashreporter/crashreporter_proc.h"

#include "../windows/W_Keyboard_proc.h"

#include "../audio/Juce_plugins_proc.h"
#include "../audio/Mixer_proc.h"
#include "../audio/MultiCore_proc.h"
#include "../audio/Faust_plugins_proc.h"
#include "../audio/SampleRecorder_proc.h"
#include "../audio/AudioMeterPeaks_proc.h"
#include "../audio/SampleReader_proc.h"
#include "../audio/Peaks.hpp"

#include "../common/OS_Bs_edit_proc.h"
//#include "../X11/X11_Ptask2Mtask_proc.h"
#include "../posix/posix_Player_proc.h"
#include "../common/OS_Ptask2Mtask_proc.h"

#include "Qt_instruments_proc.h"
#include "Qt_MainWindow_proc.h"
#include "Qt_Menues_proc.h"
#include "Qt_sequencer_proc.h"


/*
  Some day I wish gcc or clang would warn about this code:

namespace{
class Hepp{
  int gakk(void){
    return 5;
  }
};
class Hepp2 : public Hepp{
  int gakk(void){
    return 6;
  }
};
}
*/


#if !defined(RELEASE)

// The address sanitizer (asan) tends to zero out all allocated memory, covering up bugs. 
// This is a workaround.
#if defined(RADIUM_USES_ASAN)
void * operator new(decltype(sizeof(0)) size) noexcept(false)
{
  R_ASSERT(!PLAYER_current_thread_has_lock());
  void *mem = malloc(size);
  if (size > 1048576) // If changing 1048576, also change 1048576 in make_and_run_linux.sh
    memset(mem, rand(), size);
  return mem;
}
#endif

#endif



#include "../GTK/GTK_visual_proc.h"

#if 0
#ifdef FOR_WINDOWS
#  include <windows.h>
#  include "../windows/W_Keyboard_proc.h"
#endif
#endif

#include "../OpenGL/Render_proc.h"
#include "../OpenGL/Widget_proc.h"

#include "../embedded_scheme/s7extra_proc.h"
#include "../embedded_scheme/scheme_proc.h"


#include "Qt_Main_proc.h"


  //QApplication *qapplication;
class MyApplication;
MyApplication *qapplication = NULL;
QApplication *g_qapplication = NULL;
//QSplashScreen *g_splashscreen = NULL;

static bool g_mouse_is_pressed = false;

static int g_last_pressed_key = EVENT_NO;

void CancelMaybeNavigateMenus(void){
  if (g_last_pressed_key==EVENT_ALT_L && GFX_MenuActive()==false)
    g_last_pressed_key = EVENT_NO;
}

static QRect g_startup_rect;

extern bool doquit;

extern bool g_show_key_codes;

bool g_do_grey_editor = false;
static bool editor_has_keyboard = true; // "editor" means radium.
static int someone_else_has_keyboard_counting = 0;
bool g_radium_runs_custom_exec = false;
bool g_and_its_not_safe_to_paint = true;
bool g_qt_is_painting = false;
const char *g_qt_is_painting_where = "nowhere";

bool g_gc_is_incremental = false;

QWidget *g_mixerstripparent = NULL;
QHBoxLayout *g_mixerstriplayout = NULL;

DEFINE_ATOMIC(bool, is_starting_up) = true;
bool g_is_starting_up = true;
bool g_qt_is_running = false;
bool g_qtgui_has_started = false;
DEFINE_ATOMIC(bool, g_qtgui_has_started_step2) = false;
bool g_qtgui_exec_has_started = false;
bool g_qtgui_has_stopped = false;
bool g_program_has_ended = false;

#define RTWIDGET_SIZE 50
static QPointer<QWidget> g_rtwidgets[RTWIDGET_SIZE];
static bool g_widgets_needing_update[RTWIDGET_SIZE] = {};

void RT_RTWIDGET_mark_needing_update(int pos){
  if(pos==-1)
    return;

  R_ASSERT_NON_RELEASE(pos>=0 && pos<RTWIDGET_SIZE);
  
  safe_bool_write(&g_widgets_needing_update[pos], true);
}

void RTWIDGET_release_slot(int pos){
  R_ASSERT_NON_RELEASE(g_rtwidgets[pos].data() != NULL);
  g_rtwidgets[pos] = NULL;
}

int RTWIDGET_allocate_slot(QWidget *widget){
  for(int i=0;i<RTWIDGET_SIZE;i++)
    if (g_rtwidgets[i].data()==NULL){
      g_rtwidgets[i] = widget;
      //printf("POSITIOINING AT pos %d. Widget: %p. (%p)\n", i, g_rtwidgets[i].data(), widget);
      return i;
    }

  R_ASSERT_NON_RELEASE(false);
  return -1;
}
                           

static void RTWIDGET_call_often(void){
  for(int i=0;i<RTWIDGET_SIZE;i++){
    bool needs_update = safe_bool_read(&g_widgets_needing_update[i]);
    if(needs_update){
      //printf("    RTWIDGETG call often. Needs update at %d. Widget: %p\n", i, g_rtwidgets[i].data());
      safe_bool_write(&g_widgets_needing_update[i], false); // not atomically correct, but it's not that important hopefully.
      if(g_rtwidgets[i].data() != NULL)
        g_rtwidgets[i]->update();
    }
  }
}
  
static boost::lockfree::queue<int64_t, boost::lockfree::capacity<64> > g_mixer_strips_needing_remake;

DEFINE_ATOMIC(bool, g_all_mixer_strips_needs_remake) = false;
void RT_schedule_mixer_strips_remake(int64_t id){
  if (id==-1 || g_mixer_strips_needing_remake.bounded_push(id)==false)
    ATOMIC_SET(g_all_mixer_strips_needs_remake, true);
}

DEFINE_ATOMIC(bool, g_mixer_strips_needs_redraw) = false;
void RT_schedule_mixer_strips_redraw(void){
  ATOMIC_SET(g_mixer_strips_needs_redraw, true);
}

// "editor" means radium.
bool editor_has_keyboard_focus(void){
  return editor_has_keyboard && someone_else_has_keyboard_counting==0;
}

static void set_grey_editor(bool new_value){
  if (g_do_grey_editor != new_value){
    g_do_grey_editor = new_value;
    if (g_is_starting_up==false)
      root->song->tracker_windows->must_redraw_editor = true;
  }
}

void obtain_keyboard_focus_without_greying(void){
  if (g_radium_runs_custom_exec==false){
    editor_has_keyboard = false;
    set_grey_editor(false);
  }
  //root->song->tracker_windows->must_redraw_editor = true;
}

void obtain_keyboard_focus(void){
  //printf("OBTAIN_KEYBOARD_FOCUS called.... ");
  if (g_radium_runs_custom_exec==false){
    //printf("... Got it\n");
    editor_has_keyboard = false;
    set_grey_editor(!editor_has_keyboard_focus());
  }else{
    //printf("... Failed\n");
  }
  //root->song->tracker_windows->must_redraw_editor = true;
}

void release_keyboard_focus(void){
  //printf("  RELEASE keyboard focus called.... ");
  if (g_radium_runs_custom_exec==false){
    //printf("... Got it\n");
    editor_has_keyboard = true;
    set_grey_editor(!editor_has_keyboard_focus());
  }else{
    //printf("... Failed\n");
  }
  //root->song->tracker_windows->must_redraw_editor = true;
}

void obtain_keyboard_focus_counting(void){
  if (g_radium_runs_custom_exec==false){
    someone_else_has_keyboard_counting++;
    set_grey_editor(!editor_has_keyboard_focus());
  }
  //root->song->tracker_windows->must_redraw_editor = true;
}

void release_keyboard_focus_counting(void){
  if (g_radium_runs_custom_exec==false){
    if (someone_else_has_keyboard_counting==0)
      RError("release_keyboard_focus_counting called without first calling obtain_keyboard_focus_counting");
    else
      someone_else_has_keyboard_counting--;
    set_grey_editor(!editor_has_keyboard_focus());
  }
  //root->song->tracker_windows->must_redraw_editor = true;
}


static bool g_a_non_radium_window_has_focus = false;

// OSX needs to call this function since sub windows (created by for instance VST plugins) use our key events, and then we can not eat them.
void call_me_if_another_window_may_have_taken_focus_but_still_need_our_key_events(void){
  R_ASSERT(THREADING_is_main_thread());
    
  if (a_radium_window_has_focus())
    g_a_non_radium_window_has_focus = false;
  else
    g_a_non_radium_window_has_focus = true;

  //printf("main_window_has_focus(): %d\n", main_window_has_focus());
}



extern struct TEvent tevent;
bool tevent_autorepeat = false;

static bool g_up_downs[EVENT_DASMAX];

static bool maybe_got_key_window(QWindow *window);
  
uint32_t OS_SYSTEM_add_mouse_keyswitches(uint32_t keyswitch){
  bool mixer_strips_has_focus = false;

  uint32_t to_add = 0;
  static uint32_t s_last_to_add = 0; // cache result since this function is called very often.

  static double s_last_time = 0;
  double time = TIME_get_ms();

  if( (time - s_last_time) < 10)
    return keyswitch | s_last_to_add;

  //static int num_calls=0; printf("%d\n", num_calls++);

  QVector<QWidget*> all_windows = MIXERSTRIPS_get_all_widgets();

  for(auto *window : all_windows){
    if (window==QApplication::topLevelAt(QCursor::pos())){
      //if (maybe_got_key_window(window)){
      mixer_strips_has_focus = true;
      //printf("        MIXER STRIP HAS FOCUS\n");
      break;
    }
    //if (maybe_got_key_window(window))
    //  printf("        KEY. MIXER STRIP HAS KEY WINDOW.\n");
  }

  if (mixer_strips_has_focus){
    to_add |= EVENT_MOUSE_MIXERSTRIPS2;
    //printf("  MOUSE: Mixerstrips\n");
  
  }else if (SEQUENCER_has_mouse_pointer()){
    to_add |= EVENT_MOUSE_SEQUENCER2;
    //printf("  MOUSE: Sequencer\n");
    
  }else if (MW_has_mouse_pointer()){
    to_add |= EVENT_MOUSE_MIXER2;
    //printf("  MOUSE: Mixer\n");
    
  }else {
    to_add |= EVENT_MOUSE_EDITOR2;
    //printf("  MOUSE: Editor\n");
  }

  if (FOCUSFRAMES_has_focus(radium::KeyboardFocusFrameType::EDITOR)){
    to_add |= EVENT_FOCUS_EDITOR2;
    
  } else if (FOCUSFRAMES_has_focus(radium::KeyboardFocusFrameType::SEQUENCER)){
    to_add |= EVENT_FOCUS_SEQUENCER2;
    
  } else if (FOCUSFRAMES_has_focus(radium::KeyboardFocusFrameType::MIXERSTRIPS)){
    to_add |= EVENT_FOCUS_MIXERSTRIPS2;
    
  } else if (FOCUSFRAMES_has_focus(radium::KeyboardFocusFrameType::MIXER)){
    to_add |= EVENT_FOCUS_MIXER2;
    
  }
  
  s_last_to_add = to_add;
  s_last_time = time;

  return keyswitch | to_add;
}

static void set_mouse_keyswitches(void){
  if(g_is_starting_up==true)
    return;
  
  tevent.keyswitch = OS_SYSTEM_add_mouse_keyswitches(tevent.keyswitch);
}

#ifdef FOR_WINDOWS

// W_Keyboars.c updates tevent.keyswitch in it's own way. Has to be that way since we don't register the left windows key the normal way, since that causes the windows menu to appear.
static void set_keyswitch(void){
  set_mouse_keyswitches();
}

#else
static void set_keyswitch(void){
  static int keynumswitch[]={
    EVENT_CTRL_L, EVENT_SHIFT_L, EVENT_CAPS,
    EVENT_EXTRA_L,EVENT_ALT_L,EVENT_ALT_R,
    EVENT_EXTRA_R, EVENT_CTRL_R, EVENT_SHIFT_R
  };
  
  static int radiumswitch[]={
    EVENT_LEFTCTRL,EVENT_LEFTSHIFT,EVENT_CAPSLOCK,
    EVENT_LEFTEXTRA1,EVENT_LEFTALT,EVENT_RIGHTALT,
    EVENT_RIGHTEXTRA1,EVENT_RIGHTCTRL,EVENT_RIGHTSHIFT
  };
  
  int numswitches = sizeof(keynumswitch)/sizeof(int);
  
  tevent.keyswitch=0;
  
  for(int lokke=0;lokke<numswitches;lokke++){
    int keynum = keynumswitch[lokke];
    //printf("keynum: %d. is_down: %d\n",keynum, g_up_downs[keynum]);
    if(g_up_downs[keynum]){
      tevent.keyswitch |= radiumswitch[lokke];
    }
  }

  set_mouse_keyswitches();
  
  //printf("keyswtich: %x\n",tevent.keyswitch);
}
#endif

void OS_SYSTEM_ResetKeysUpDowns(void){
  //if (root!=NULL && ATOMIC_GET(root->editonoff)==false)
  //  printf("   Reset keys up downs. Backtrace:\n %s\n\n", JUCE_get_backtrace());
  for(int i=0;i<EVENT_DASMAX;i++)
    g_up_downs[i]=false;
  set_keyswitch();
}



#if 0
static bool handle_qt_keyboard(QKeyEvent *event, bool is_key_down){
  int keynum = EVENT_NO;

#define S(QT_VAL, EVENT_VAL) case Qt::Key_##QT_VAL: keynum=EVENT_##EVENT_VAL;break;  
  switch(event->key()){
    S(Escape, ESC);
    S(Tab, TAB);
    S(Backspace, BACKSPACE);
    S(Return, RETURN);
    S(Enter, KP_ENTER);
    S(Insert, INSERT);
    S(Delete, DEL);

    S(Home, HOME);
    S(End, END);
    S(Left, LEFTARROW);
    S(Right, RIGHTARROW);
    S(Down, DOWNARROW);
    S(Up, UPARROW);
    S(PageUp, PAGE_UP);
    S(PageDown, PAGE_DOWN);

    S(F1, F1);
    S(F2, F2);
    S(F3, F3);
    S(F4, F4);
    S(F5, F5);
    S(F6, F6);
    S(F7, F7);
    S(F8, F8);
    S(F9, F9);
    S(F10, F10);
    S(F11, F11);
    S(F12, F12);

    S(0,0);
    S(1,1);
    S(2,2);
    S(3,3);
    S(4,4);
    S(5,5);
    S(6,6);
    S(7,7);
    S(8,8);
    S(9,9);

    S(A,A);
    S(B,B);
    S(C,C);
    S(D,D);
    S(E,E);
    S(F,F);
    S(G,G);
    S(H,H);
    S(I,I);
    S(J,J);
    S(K,K);
    S(L,L);
    S(M,M);
    S(N,N);
    S(O,O);
    S(P,P);
    S(Q,Q);
    S(R,R);
    S(S,S);
    S(T,T);
    S(V,U);
    S(U,V);
    S(W,W);
    S(X,X);
    S(Y,Y);
    S(Z,Z);

    S(Space, SPACE);
    
    S(Menu, MENU);

    S(MediaPlay, PLAY);
    S(MediaStop, STOP);
    S(VolumeDown, VOLUME_DOWN);
    S(VolumeUp, VOLUME_UP);
    S(VolumeMute, MUTE);

    S(Calculator, CALCULATOR);
    S(LaunchMail, MAIL);
    S(HomePage, HOMEPAGE);    
  }
#undef S

#if FOR_LINUX
  const int sub=8;
#else
  const int sub=0;
#endif

  return handle_keyboard(keynum, event->nativeScanCode()-sub, event->nativeVirtualKey(), is_key_down);
}
#endif

#if USE_QT5
#include <QAbstractNativeEventFilter>
#endif

static void schedule_set_editor_focus(int ms){
  QTimer::singleShot(ms, set_editor_focus);
}

namespace{
  struct MouseCycle{    
    QPointer<QWidget> widget;
    int64_t id;
    QPointF local_pos;
    Qt::MouseButton button;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
  };
}


static MouseCycle g_curr_mouse_cycle;

Qt::MouseButtons MOUSE_CYCLE_get_mouse_buttons(void){
  if (g_curr_mouse_cycle.widget.data()==NULL)
    return Qt::NoButton;
  
  return g_curr_mouse_cycle.buttons;
}

  
static void MOUSE_CYCLE_unregister_all(int64_t id);

static void set_curr_mouse_cycle(QMouseEvent *event){
  g_curr_mouse_cycle.local_pos = event->localPos();
  g_curr_mouse_cycle.button = event->button();
  g_curr_mouse_cycle.buttons = event->buttons();
  g_curr_mouse_cycle.modifiers = event->modifiers();
}

bool MOUSE_CYCLE_register(QWidget *widget, QMouseEvent *event){
  
  R_ASSERT(widget!=NULL);
  
  R_ASSERT_NON_RELEASE2(dynamic_cast<radium::MouseCycleFix*>(widget) != NULL, true);

  if (event->buttons()==Qt::NoButton){
    R_ASSERT_NON_RELEASE(false);
    return true;
  }

#if !defined(RELEASE)
  printf("CYCLE: registering %p. Type: %s\n", widget, widget->metaObject()->className());
#endif

  bool same_widget = widget == g_curr_mouse_cycle.widget.data();

  bool same_widget_and_buttons = same_widget && g_curr_mouse_cycle.buttons==event->buttons();

  if(same_widget_and_buttons){
    //R_ASSERT_NON_RELEASE(false); // Out of curiousity, want to know if this can happen. Yes, happens when double-right-clicking in a help window.
  }
  
  MOUSE_CYCLE_unregister_all(g_curr_mouse_cycle.id); // The main call to MOUSE_CYCLE_unregister_all is delayed a little bit, so we could have an alive cycle.
    
  if(!same_widget) {  // widget==g_curr_mouse_cycle.widget.data() when subclass::mousePressEvent calls parent::mousePressEvent.
    g_curr_mouse_cycle.widget = widget;
  }

  static int64_t g_mouse_cycle_id = 0;
  g_curr_mouse_cycle.id = g_mouse_cycle_id++;

  set_curr_mouse_cycle(event);

  return true;
}


bool MOUSE_CYCLE_move(QWidget *widget, QMouseEvent *event){
  if (g_curr_mouse_cycle.widget.data() != widget){

    /*
    // even this can happen. Sigh.
    R_ASSERT_NON_RELEASE(event->buttons()==Qt::NoButton);
    R_ASSERT_NON_RELEASE(event->button()==Qt::NoButton);
    */

    if(event->buttons() != Qt::NoButton){
      MOUSE_CYCLE_unregister_all(g_curr_mouse_cycle.id);
    }
    
    return g_curr_mouse_cycle.widget.data() == NULL;    
  }

  if(event->buttons() == Qt::NoButton){
    MOUSE_CYCLE_unregister_all(g_curr_mouse_cycle.id);
  }
    
  //printf("CYCLE: moving %p\n", widget);
  
  set_curr_mouse_cycle(event);

  return true;
}

bool MOUSE_CYCLE_unregister(QWidget *widget){
#if !defined(RELEASE)
  printf("CYCLE: unregistering %p. Type: %s\n", widget, widget->metaObject()->className());
#endif
  
  if(g_curr_mouse_cycle.widget.data() != widget)
    MOUSE_CYCLE_unregister_all(g_curr_mouse_cycle.id);
  
  g_curr_mouse_cycle.widget.clear();
  return true;
}

static void MOUSE_CYCLE_unregister_all(int64_t id){
  QWidget *w = g_curr_mouse_cycle.widget.data();
  if(w==NULL)
    return;

  //printf("   unregister all: %d vs. %d (%d)\n", (int)g_curr_mouse_cycle.id, (int)id, g_curr_mouse_cycle.id != id);
  
  if (g_curr_mouse_cycle.id != id){
    R_ASSERT(g_curr_mouse_cycle.id > id);
    return;
  }
  
  QMouseEvent e(QEvent::MouseButtonRelease,
                g_curr_mouse_cycle.local_pos,
                g_curr_mouse_cycle.button,
                g_curr_mouse_cycle.buttons,
                g_curr_mouse_cycle.modifiers);
                                             
  g_curr_mouse_cycle.widget.clear();
                                            
  auto *mouse_cycle_fix = dynamic_cast<radium::MouseCycleFix*>(w);  
  R_ASSERT_RETURN_IF_FALSE(mouse_cycle_fix != NULL);

#if !defined(RELEASE)
  printf("   CYCLE: Unregistering all. Calling %p\n", w);
#endif
  
  radium::MouseCycleEvent e2(&e, false);
  mouse_cycle_fix->fix_mouseReleaseEvent(e2);
}


void MOUSE_CYCLE_schedule_unregister_all(void){
  QWidget *w = g_curr_mouse_cycle.widget.data();
  if(w==NULL)
    return;
    
  static int64_t last_id = -1;  
  int64_t id = g_curr_mouse_cycle.id;
  
  if (id > last_id){

    last_id = id;

#if !defined(RELEASE)
    printf("   CYCLE: Scheduling unregistering all to run in 3ms for %p.\n", w);
#endif

    QTimer::singleShot(3, [id]{ // delay it a little bit since we might be called from a mouse event. (need to wait enough to make the callback be called in the next event, but not so long that the event is not unregistered soon enough)
        MOUSE_CYCLE_unregister_all(id);
      });
    
  }    
}


#if !defined(FOR_WINDOWS)
namespace{

  static radium::Process g_check_backtrace_process;
  static bool g_check_backtrace_process_finished = false;
  
  static void start_check_backtrace_process(void){
    g_check_backtrace_process.start(OS_get_full_program_file_path("radium_plugin_scanner") + " test_backtrace");
  }
  
  static void checkup_on_check_backtrace_process(void){
    if (g_check_backtrace_process_finished)
      return;
    
    radium::Process::Status status = g_check_backtrace_process.get_status(30000);

    //printf("Status: %d. %s\n", (int)status, g_check_backtrace_process.get_status_string().toUtf8().constData());
    
    if (status == radium::Process::Status::RUNNING)
      return;
    
    g_check_backtrace_process_finished = true;
    
    if (status == radium::Process::Status::FINISHED)
      return;
        
#if defined(FOR_LINUX) && !defined(IS_LINUX_BINARY)
    SYSTEM_show_error_message(talloc_format("Error: Unable to get backtrace: \"Bactracing %s\".\nPlease check that Radium is compiled and installed properly.", g_check_backtrace_process.get_status_string().toUtf8().constData()));
#else
    SYSTEM_show_error_message(talloc_format("Error: Unable to get backtrace: \"Bactracing %s\".\nPlease report this to k.s.matheussen@notam02.no", g_check_backtrace_process.get_status_string().toUtf8().constData()));
#endif

    g_check_backtrace_process.kill();
  }

}
#endif



class MyApplication
  : public QApplication
#if USE_QT5
  , public QAbstractNativeEventFilter
#endif
{
  Q_OBJECT;
  
public:

  MyApplication(int &argc,char **argv);

protected:

  double _time_of_last_alt = -1;
  bool menu_was_active_at_least_key_press = false;
  bool last_released_key_was_lalt = false;  
  bool last_key_was_lalt = false;
  int menu_should_be_active = 0; // When value is 1, or higher, we are about to navigate menues. We need this variable since GFX_MenuActive() doesn't return true until the menu actually pops up.
  
   bool SystemEventFilter(void *event){

     if(g_is_starting_up==true){
       //printf("   ret false 0\n");
       return false;
     }
     
    OS_SYSTEM_EventPreHandler(event);

    /*
    QMainWindow *main_window = static_cast<QMainWindow*>(root->song->tracker_windows->os_visual.main_window);
    printf("   focus: %d,   active: %d.  key: %d\n",
           QApplication::focusWidget() != NULL,
           QApplication::activeWindow() != NULL,
           OS_OSX_is_key_window((void*)main_window->winId())
           );
    */

    //printf("Got key. Another window has focus? %d\n",(int)g_a_non_radium_window_has_focus);
    //return false;    
    if (g_a_non_radium_window_has_focus && JUCE_native_gui_grabs_keyboard()){

      //printf("   Ret false 1. non: %d. juce: %d\n", g_a_non_radium_window_has_focus, JUCE_native_gui_grabs_keyboard());
      
      /*
      static int downcount = 10;
      if ((--downcount) == 0){
        printf(" Got key -2. Focus widget: %s\n",QApplication::focusWidget()==NULL ? "(null)" : QApplication::focusWidget()->metaObject()->className());
        downcount = 40;
      }
      */
      //printf("ret false 1\n");
      return false;
    }

    if (MIXER_is_saving()){
      //printf(" Got key -1\n");
      //printf("ret false 2\n");
      return false;
    }

    struct Tracker_Windows *window = root->song->tracker_windows;

    bool ignore_autorepeat = !doAutoRepeat() && editor_has_keyboard_focus() == true;
    
    int type = OS_SYSTEM_get_event_type(event, ignore_autorepeat);

#if 0
    switch(type){
      case TR_KEYBOARD: printf("  Down\n");break;
      case TR_KEYBOARDUP: printf("  Up\n");break;
      case TR_AUTOREPEAT: printf("  Autorepeat\n");break;
    }
#endif
    
    tevent_autorepeat = (type==TR_AUTOREPEAT);

    if (type==TR_AUTOREPEAT)
      type = TR_KEYBOARD;
    
    if (type!=TR_KEYBOARD && type!=TR_KEYBOARDUP){
      return false;
    }
    
#if 0 //FOR_LINUX
    return true;
#endif

    bool is_key_press = type==TR_KEYBOARD;

    int modifier = OS_SYSTEM_get_modifier(event); // Note that OS_SYSTEM_get_modifier is unable to return an EVENT_EXTRA_L event on windows. Not too sure about EVENT_EXTRA_R either (VK_APPS key) (doesn't matter, EVENT_EXTRA_R is abandoned, and the key is just used to configure block). In addition, the release value order might be wrong if pressing several modifier keys, still windows only.

    //printf("modifier: %d. Right shift: %d\n",modifier, modifier==EVENT_SHIFT_R);
    if (g_show_key_codes){
      char *message = talloc_format("%d - %d: %s", modifier, OS_SYSTEM_get_scancode(event), tevent_autorepeat ? "Autorepeat" : is_key_press ? "Down" : "Up");
      printf("  Got key: %s\n",message);
      window->message=message;
      
      GL_create(window);
    }
              
    //printf(" Got key 1. modifier: %d. Left ctrl: %d, Press: %d. Is EVENT_NO: %d. Scancode: %d\n", modifier, EVENT_CTRL_L, is_key_press, modifier==EVENT_NO, OS_SYSTEM_get_scancode(event));

    if (modifier != EVENT_NO) {

      bool must_return_true = false;

      bool menu_is_active = GFX_MenuActive();
      
      //printf("   Modifier: %d. EVENT_ALT_L: %d. Menu is active: %d. Is key press: %d\n", modifier, EVENT_ALT_L, GFX_MenuActive(), is_key_press);

      /*
      if(modifier==EVENT_ALT_L)       
        printf("   Main window has focus: %d. Menu active: %d. Menu visible: %d. Last was alt: %d\n", OS_GFX_main_window_has_focus(), GFX_MenuActive(), GFX_MenuVisible(window), g_last_pressed_key==EVENT_ALT_L);
      */
      
      if (modifier==EVENT_ALT_L && OS_GFX_main_window_has_focus()){

        //printf(   " last_key_was_alt: %d. Time now: %f. last time: %f. press: %d. Menu active: %d.  Duration: %f\n", last_released_key_was_lalt, TIME_get_ms(), _time_of_last_alt, is_key_press, GFX_MenuActive(), TIME_get_ms()- _time_of_last_alt);

        if(last_released_key_was_lalt && !is_key_press && menu_was_active_at_least_key_press && g_last_pressed_key==EVENT_ALT_L && g_mouse_is_pressed==false) { //last_key_was_lalt==true && (TIME_get_ms() - _time_of_last_alt) < 200) {

            // Double-pressed left alt key.
            
            last_key_was_lalt = false;
            menu_should_be_active = 0;
            must_return_true = true;
            
            //printf("  EVENT_ALT_L. Visible: %d.\n", GFX_MenuVisible(window));

            if (true){ //GFX_MenuVisible(window)){
              //printf("   HIDING 1\n");
              GFX_HideMenu(window);
              schedule_set_editor_focus(20);
            }
            
        } else if (menu_is_active) {

          //printf("   MENU ACTIVE while pressing left alt. Hiding\n");
          last_key_was_lalt = false;
          menu_should_be_active = 0;

        } else if (is_key_press){

            //must_return_true = true;

          //last_key_was_lalt = true;
          
        }else {
          
          // release
          
          //printf("**** last_key_was_lalt: %d. menu_should_be_active: %d. Menu is active: %d\n", last_key_was_lalt, menu_should_be_active, GFX_MenuActive());

          if (!GFX_MenuVisible(window) && g_last_pressed_key==EVENT_ALT_L && g_mouse_is_pressed==false){
            
            // It doesn't work trying to start navigating the menues immediately after calling GFX_ShowMenu(). Qt doesn't allow that.
            // So the only thing we can do is to show the menu.
            
            last_key_was_lalt = false;
            menu_should_be_active = 0;
            must_return_true = true;
            
            printf("   SHOW MENU\n");

            GFX_ShowMenu(window);
            
          } else {
            
            if (g_last_pressed_key==EVENT_ALT_L && g_mouse_is_pressed==false) {
              // Single-pressed left alt key.

              //printf("    Making MENU active. Last pressed: %d\n", g_last_pressed_key);
              menu_should_be_active = 1;
              GFX_MakeMakeMainMenuActive();
              _time_of_last_alt = TIME_get_ms();
            
              
            }else {
              
              menu_should_be_active = 0;
              must_return_true = true; // Another key was pressed as well when releasing left alt. Must return true to prevent Qt from opening the menues.
              
            }
            
            last_key_was_lalt = true;

          }

        }

        last_released_key_was_lalt = true;

        if (is_key_press)
          menu_was_active_at_least_key_press = menu_is_active;
        
      } else {

        //printf("Setting lalt==false 1\n");
        last_key_was_lalt = false;
        last_released_key_was_lalt = false;
        
      }
      
      static double last_pressed_key_time = 0;
      static int last_pressed_undo_num = 0;

      double time_now = TIME_get_ms();

      if (is_key_press) {

        last_pressed_undo_num = Undo_num_undos();
        last_pressed_key_time = time_now;
        g_last_pressed_key = modifier;
               
      } else {
        
        // key release:

        if (editor_has_keyboard_focus()==true) {
          
          if( (time_now - last_pressed_key_time) < 1000/4){ // i.e. only play if holding the key less than 0.25 seconds.
            
            if(modifier==g_last_pressed_key && modifier==EVENT_ALT_R) {
              PlayBlockFromStart(window,true); // true == do_loop
            }

            /*
            if (modifier==EVENT_SHIFT_R){
              printf("modifier==g_last_pressed_key: %d. modifier==EVENT_SHIFT_R: %d. Undo_num_undos()==last_pressed_undo_num: %d. QGuiApplication::mouseButtons()==Qt::NoButton: %d. QGuiApplication::mouseButtons(): %d. cycle buttons: %d\n",
                     modifier==g_last_pressed_key, modifier==EVENT_SHIFT_R, Undo_num_undos()==last_pressed_undo_num, QGuiApplication::mouseButtons()==Qt::NoButton, (int)QGuiApplication::mouseButtons(), (int)MOUSE_CYCLE_get_mouse_buttons()
                     );
            }
            */
            
            if(modifier==g_last_pressed_key && modifier==EVENT_SHIFT_R && Undo_num_undos()==last_pressed_undo_num && MOUSE_CYCLE_get_mouse_buttons()==Qt::NoButton) {
              PlayBlockFromStart(window,true); // true == do_loop
            }
            
          }
          
        }
      }

      g_up_downs[modifier] = is_key_press;

#if !defined(FOR_WINDOWS)
      set_keyswitch(); // In windows, tevent.keyswitch was set when calling OS_SYSTEM_EventPreHandler.
#endif
      
      //printf("__________________________ Got modifier %s. Returning false\n",is_key_press ? "down" : "up");

      //printf(" Got key 2\n");
      
      if (modifier==EVENT_ALT_R || must_return_true){
        //printf("  Returning true 1. modifier: %d, must_return_true: %d\n", modifier==EVENT_ALT_R, must_return_true);
        return true; // If not, Qt starts to navigate the menues.
      }

      //printf("  Returning false 1\n");
      return false;
    }

    
#if !defined(FOR_WINDOWS)
    set_keyswitch(); // In windows, tevent.keyswitch was set when calling OS_SYSTEM_EventPreHandler.
#endif
      

#if 0
    printf("is_key_press: %d, keynum: %d, EVENT_MENU: %d\n",is_key_press,keynum,EVENT_MENU);
    if (is_key_press==false && keynum==EVENT_MENU)
      return true; // swallow the general qt menu popup menu. Sometimes it pops up when configuring block. If you need it, just press right mouse button.
#endif

    //printf(" Got key 3\n");

    RETURN_IF_DATA_IS_INACCESSIBLE(false);
      
    if (g_grab_next_eventreceiver_key==false && editor_has_keyboard_focus()==false){
      //printf("  Returning false 2.2\n");
      return false;
    }

    int keynum = OS_SYSTEM_get_keynum(event);
    
    //printf(" Got key 4. Keynum: %d. down: %d. up: %d\n", keynum, EVENT_VOLUME_DOWN, EVENT_VOLUME_UP);
    
    g_last_pressed_key = keynum;
            
    //printf("keynum1: %d. switch: %d\n",keynum,tevent.keyswitch);


    //printf("  menu_should_be_active: %d. GFX_MenuActive(): %d\n", menu_should_be_active, GFX_MenuActive());
    
    if (GFX_MenuActive() || menu_should_be_active>0){

      if (GFX_MenuActive())
        menu_should_be_active = 0; // we can't rely entirely on menu_should_be_active to be true, since it won't be false when using the mouse to cancel menu navigation.
      
      // Qt doesn't do anything when pressing page down / page up / shift+down / shift+up / home / end
      switch(keynum){
        case EVENT_DOWNARROW:
          if (is_key_press && shiftPressed())
            send_key_down(GFX_GetActiveMenu(), 3);
          break;
        case EVENT_UPARROW:
          if (is_key_press && shiftPressed())
            send_key_up(GFX_GetActiveMenu(), 3);
          break;
        case EVENT_PAGE_DOWN:
          if (is_key_press)
            send_key_down(GFX_GetActiveMenu(), 3);
          break;
        case EVENT_PAGE_UP:
          if (is_key_press)
            send_key_up(GFX_GetActiveMenu(), 3);
          break;
        case EVENT_HOME:
          if (is_key_press)
            send_key_down(GFX_GetActiveMenu(), 5);
          break;
        case EVENT_END:
          if (is_key_press)
            send_key_up(GFX_GetActiveMenu(), 5);
          break;
      }
      
      switch(keynum){

        case EVENT_ESC:
        case EVENT_RETURN:{
          //printf("Pressed esc or return\n");
          menu_should_be_active = 0; // In case we press esc or return right after pressing left alt.
          //printf("  Returning false 3.1\n");
          if (keynum==EVENT_ESC)
            schedule_set_editor_focus(20);
          return false;
          break;
        }
          
        case EVENT_NO: // We get one or more of these right after showing menues on windows.          
        case EVENT_KP_ENTER:
        case EVENT_PAGE_UP:
        case EVENT_PAGE_DOWN:
        case EVENT_HOME:
        case EVENT_END:
        case EVENT_UPARROW:
        case EVENT_DOWNARROW:
        case EVENT_LEFTARROW:
        case EVENT_RIGHTARROW:{
          //printf("     Menu active? %d. menu_should_be_active: %d\n", GFX_MenuActive(), menu_should_be_active);
          if (keynum != EVENT_NO && menu_should_be_active>0)
            menu_should_be_active++;

          if (menu_should_be_active > 3)
            menu_should_be_active = 0; // Something is wrong. Qt doesn't navigate the menues now. (happens on windows after making menues visible)
          else {
            //printf("  Returning false 3.2 %d\n", menu_should_be_active);
            return false;
          }
          break;
        }
          
        default:{
          //printf("  Something else: %d (left alt: %d)\n", keynum, EVENT_ALT_L);
          if (GFX_MenuActive()){
            //printf("  Returning false 3.3 %d\n", menu_should_be_active);
            return false; // Since the menus are active, we can assume that the user knew, or should know, that we are navigating the menues.
          }else
            menu_should_be_active = 0; // There's a good chance the user doesn't know that we are navigating the menues now, so we stop navigating it to avoid further confusion.
        }
      }

    }
    //printf("ret true 1\n");

    //printf("Setting lalt==false 2\n");
    last_key_was_lalt = false;

    //printf(" Got key 5\n");

    
    // Eventually we will not set window->must_redraw=true here.
#if 1 //defined(RELEASE)
    if (AnyModifierKeyPressed(tevent.keyswitch) || (keynum!=EVENT_UPARROW && keynum!=EVENT_DOWNARROW))
      window->must_redraw = true;
#endif
    
    if (is_key_press)
      tevent.ID=TR_KEYBOARD;
    else
      tevent.ID=TR_KEYBOARDUP;
    
    tevent.SubID=keynum;  
    
    bool ret;

    bool dat_used_key;

    {
      dat_used_key = DAT_keypress(window, tevent.SubID, is_key_press);
      /*
        // This code should perhaps not be commented out.
      if (dat_used_key==false){
        int scancode_keynum = OS_SYSTEM_get_qwerty_keynum(event); // e.g. using scancode.
        dat_used_key = DAT_keypress(window, scancode_keynum, is_key_press);
      }
      */
    }
    
    if (dat_used_key) {
      
      ret = true;

    } else {

      if (keynum==EVENT_NO)
        ret = false;
      else if (keynum==EVENT_EAT_BUT_DO_NOTHING)
        ret = true;
      else
        ret = EventReciever(&tevent,window);
      
      if (ret==false) {
        keynum = OS_SYSTEM_get_qwerty_keynum(event); // e.g. using scancode.
        
        //printf("keynum2: %d. switch: %d\n",keynum,tevent.keyswitch);
        
        if (keynum==EVENT_NO){
          //printf("Return true. Unknown key for event type %d. Key: %d\n",type, tevent.SubID);//virtual_key);
          return true;
        }
        
        tevent.SubID=keynum;
        ret = EventReciever(&tevent,window);
      }
    }

    //printf("ret2: %d\n",ret);

    //printf(" Got key 6\n");
    
    if(ret==true)
      static_cast<EditorWidget*>(window->os_visual.widget)->updateEditor();

    /*
    if (keynum==EVENT_ALT_L){
      printf("Last key was left alt. Set keyboard focus, to be sure\n");
      set_editor_focus();
    }
    */
    
    //printf(" Got key 7\n");

    //printf("  Returning true 4\n");
    return true;
   }

  bool eventFilter(QObject *obj, QEvent *event) override {

    bool activation_changed = event->type() == QEvent::WindowDeactivate || event->type() == QEvent::WindowActivate;

#if FOR_LINUX

    if (activation_changed){ //event->type()==QEvent::FocusAboutToChange){
      //if (event->type()==QEvent::FocusAboutToChange){
      /*
      QFocusEvent *fevent = static_cast<QFocusEvent*>(event);
      printf("QEvent::FocusAboutToChange: EventFilter called: %d/%d %d\n", fevent->gotFocus(), fevent->lostFocus(), fevent->reason());
      */

      // Schedule to run later. Now we could be called from anywhere, for instance a qwidget destructor.
      QTimer::singleShot(3,OS_SYSTEM_ResetKeysUpDowns);
    }

#elif 0 //FOR_MACOSX

    // Call os_osx_clear_modifiers in applicationStateChanged instead.
    if (event->type()==QEvent::ApplicationDeactivate){
      printf("       app deactivate called\n");
      OS_OSX_clear_modifiers();    
    }
    
#endif

    switch(event->type()){
    case QEvent::GraphicsSceneMousePress:
      CancelMaybeNavigateMenus();
      break;
      
    case QEvent::GraphicsSceneWheel:
      CancelMaybeNavigateMenus();
      break;
      
    case QEvent::NonClientAreaMouseButtonPress:
      CancelMaybeNavigateMenus();
      break;
      
    case QEvent::MouseButtonPress:
      CancelMaybeNavigateMenus();
      g_mouse_is_pressed = true;
      break;
      
    case QEvent::MouseButtonRelease:
      CancelMaybeNavigateMenus();
      g_mouse_is_pressed = false;
      break;

    case QEvent::Enter:
      g_mouse_is_pressed = false;
      break;
      
    case QEvent::Wheel:
      CancelMaybeNavigateMenus();
      break;

    case QEvent::Close:
      if (true || qobject_cast<QMenu*>(obj)==NULL){
        //printf("        CLOSE\n");
        CancelMaybeNavigateMenus();
      }
      break;

    default:
      break;
    }
      
    /*
    if(event->type() != 1 && event->type() != 43 && event->type() != 12 && event->type() != 52 && event->type() != 71 && event->type() != 76 && event->type() != 68 && event->type() != 77)
      if(event->type()==QEvent::Show || event->type()==QEvent::Hide)
        printf("Event: %d\n",event->type());
    */

    // TODO: Check if getting this event:
    // QEvent::ApplicationFontChange
    // could be used for something. For instance to fix OpenGL widget position.

    auto ret = QApplication::eventFilter(obj, event);

    if (activation_changed){
      //static int counter = 0;  printf("   %d: Activation changed: activate: %d deactivate: %d\n", counter++, event->type() == QEvent::WindowActivate, event->type() == QEvent::WindowDeactivate);
      MOUSE_CYCLE_schedule_unregister_all();
    }
    
    return ret;
  }
  
  
#ifdef USE_QT5

  bool nativeEventFilter(const QByteArray &eventType, void *message, long *) Q_DECL_OVERRIDE
  {
    //printf("NAtive event filter!\n");
    return SystemEventFilter(message);
  }

#else // USE_QT5
  
#ifdef __linux__
  bool x11EventFilter(XEvent *event) override {
    bool ret = SystemEventFilter(event);
    //printf("         eventfilter ret: %d\n",ret);
    return ret;
  }
#endif

#ifdef FOR_WINDOWS
  bool 	winEventFilter ( MSG * msg, long * result ) override {
    return SystemEventFilter(msg);
  }
#endif

#ifdef FOR_MACOSX
  bool macEventFilter ( EventHandlerCallRef caller, EventRef event ) override {
    return SystemEventFilter(event);
  }
#endif

#endif // !USE_QT5
    
  /*
  bool event(QEvent *event){
    return QApplication::event(event);
  }
  */

public slots:
  void applicationStateChanged(Qt::ApplicationState state){
    //printf("   *** applicationStateChanged called: %s.\n", state==Qt::ApplicationHidden ? "HIDDEN" :  state==Qt::ApplicationInactive ? "INACTIVE" : state==Qt::ApplicationActive ? "ACTIVE" : "WHAT?");
#if FOR_MACOSX
    OS_OSX_clear_modifiers();
#elif FOR_LINUX
    //OS_SYSTEM_ResetKeysUpDowns();
#endif
  }

};

#include "mQt_Main.cpp"

MyApplication::MyApplication(int &argc,char **argv)
  : QApplication(argc,argv)
{
  //setStyleSheet("QStatusBar::item { border: 0px solid black }; ");
#if USE_QT5
  installNativeEventFilter(this);
  installEventFilter(this);
#endif

  connect(this, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(applicationStateChanged(Qt::ApplicationState)));
}

#if 0
#include "Timer.hpp"

namespace{
  struct MyTime : public radium::Timer{
    QWidget *widget;
    QWindow *window;
    MyTime(QWidget *widget, QWindow *window)
      : Timer(100)
      , widget(widget)
      , window(window)
    {
    }

    void calledFromTimer(void) override {
      printf("   ADJUSTING %d %d\n",window->width(), window->height());
      widget->resize(window->size());
    }
  };
}
#endif

namespace{
  class EmbeddedNativeWindow : public QWidget{

    void *_child_handle;
    std::function<void(void*)> _delete_child_handle_func;
    
  public:

    bool _child_is_deleted = false;

    EmbeddedNativeWindow(QWidget *widget, void *child_handle, std::function<void(void*)> delete_child_handle_func)
      : _child_handle(child_handle)
      , _delete_child_handle_func(delete_child_handle_func)
    {
#if 1
      widget->setParent(this);
      widget->move(0,0);
#else
      QVBoxLayout *layout = new QVBoxLayout(this);
      layout->setSpacing(0);
      layout->setContentsMargins(0,0,0,0);
      
      setLayout(layout);
      
      layout->addWidget(widget);
#endif
    }

    void closeEvent(QCloseEvent *event) override {
      if (_child_is_deleted==false){
        _child_is_deleted = true;
        _delete_child_handle_func(_child_handle);
      }
      event->accept();
    }
  };
}


void OS_GFX_close_embedded_native_window(void *daswidget){
  R_ASSERT_RETURN_IF_FALSE(daswidget != NULL);
  
  EmbeddedNativeWindow *widget = static_cast<EmbeddedNativeWindow*>(daswidget);
  
  widget->_child_is_deleted = true;
  widget->close();
}
  
void *OS_GFX_create_embedded_native_window(void *child_handle, int x, int y, int width, int height, std::function<void(void*)> delete_child_handle_func){
  QWindow *window = QWindow::fromWinId((WId)child_handle);
  R_ASSERT_RETURN_IF_FALSE2(window!=NULL, NULL);

  QWidget *widget = QWidget::createWindowContainer(window, NULL, Qt::ForeignWindow);
  R_ASSERT_RETURN_IF_FALSE2(widget!=NULL, NULL);

  QWidget *main_widget;
  
#if 0
  
  main_widget = widget;
  
#else
  
  main_widget = new EmbeddedNativeWindow(widget, child_handle, delete_child_handle_func);
  
#endif

  set_window_parent(main_widget, g_main_window, radium::NOT_MODAL);

  for(int height2=height + 2, i=0 ; height2 >= height ; height2--, i++){
    
    std::function<void(void)> adjust_size = [main_widget, x, y, width, height2] {
      printf("   ADJUSTING %d %d\n", width, height2);
      main_widget->setGeometry(x, y, width, height2);
    };

    R_ASSERT(i<=2);
    
    const int times[] = {100,1000,2000};
    const int time = times[R_MIN(i, 2)];
    
    QTimer::singleShot(time, adjust_size); // Workaround for non-updated bottom graphics in VST gui. (might be a Qt bug. Developed this code in Qt 5.9)
  }
  
  main_widget->setGeometry(x, y, width, height+3); // Add 3 to avoid the resize call in singleshot and this one doesn't have the same parameters.
  safeShow(main_widget);

  return main_widget;
  
  //new MyTime(widget, window);
  /*
  widget->adjustSize();

  std::function<void(void)> adjust = [widget, window, adjust] {
    printf("   ADJUSTING %d %d\n",window->width(), window->height());
    widget->resize(window->size());
    QTimer::singleShot(1000, adjust);
  };

  QTimer::singleShot(1000, adjust);
  */
  
  /*
  RememberGeometryQDialog *dialog = new RememberGeometryQDialog(g_main_window, radium::NOT_MODAL);

  QVBoxLayout *layout = new QVBoxLayout(dialog);
  layout->setSpacing(0);
  layout->setContentsMargins(0,0,0,0);
  
  dialog->setLayout(layout);

  layout->addWidget(widget);
  //layout->adjustLayout();
  dialog->adjustSize();
  
  safeShow(dialog);
  */

  //layout->adjustLayout();
  //dialog->adjustSize();
}

void *OS_GFX_get_native_main_window(void){
  R_ASSERT_RETURN_IF_FALSE2(g_is_starting_up==false, NULL);
      
  QWidget *main_window = static_cast<QWidget*>(root->song->tracker_windows->os_visual.main_window);
  return (void*)main_window->winId();
}

void *OS_GFX_get_mixer_toplevel_window_if_visible(void){
  QWidget *w = get_qwidget(g_mixer_widget);
  if (w->isWindow())
    return (void*)w->effectiveWinId();
  else
    return NULL;
}
  
void *OS_GFX_get_native_toplevel_window(int toplevelwindow_num){
  if (OS_GFX_get_num_toplevel_windows() > toplevelwindow_num)
    return (void*)QGuiApplication::topLevelWindows().at(toplevelwindow_num)->winId();
  else
    return NULL;
}

int OS_GFX_get_num_toplevel_windows(void){
  return QGuiApplication::topLevelWindows().size();
}

int OS_get_main_window_width(void){
  if (g_main_window==NULL)
    return 0;

  return g_main_window->width();
}

int OS_get_main_window_height(void){
  if (g_main_window==NULL)
    return 0;

  return g_main_window->height();
}


// Warning: Does not always work on windows.
//
// Maybe this test is better: QWidget::find(GetForegroundWindow()) != NULL
//
// Or: Use effectWinId() instead of winId().
//
static bool maybe_got_key_window(QWindow *window){
#if FOR_MACOSX
  return OS_OSX_is_key_window((void*)window->winId());
#elif FOR_WINDOWS
  return OS_WINDOWS_is_key_window((void*)window->winId());
#elif FOR_LINUX
  //return g_qapplication->focusWidget()!=NULL && window==g_qapplication->focusWidget()->window(); //activeWindow();
  QWidget *topwindow = QApplication::topLevelAt(QCursor::pos());
  if (topwindow==NULL)
    return false;
  if (topwindow->window()==NULL) // got a crash below. it was impossible to find out what went wrong from the backtrace, so added this test.
    return false;
  return window==topwindow->window()->windowHandle();
  //->isActiveWindow();
#else
  RError("Unknown platform");
  return true;
#endif
}

// Warning: Does not always work on windows.
bool OS_GFX_main_window_has_focus(void){
  QWidget *main_window = static_cast<QWidget*>(root->song->tracker_windows->os_visual.main_window);
  return maybe_got_key_window(main_window->window()->windowHandle());
}

// Warning: Does not always work on windows.
bool a_radium_window_has_focus(void){
  if(g_is_starting_up==true)
    return false;

#if FOR_LINUX
  
  return g_qapplication->activeWindow() != NULL;
  
#else

  /*
  QMainWindow *main_window = static_cast<QMainWindow*>(root->song->tracker_windows->os_visual.main_window);
  if (maybe_got_key_window(main_window))
    return true;

  if (get_qwidget(g_mixer_widget)->parentWidget()()==NULL)
    if (maybe_got_key_window(get_qwidget(g_mixer_widget)->parentWidget()))
      return true;
  */

  /*
  QVector<QWidget*> all_windows = MIXERSTRIPS_get_all_widgets();
  for(auto *window : all_windows)
    if (maybe_got_key_window(window->window()->windowHandle()))
      return true;
  */
  
  for(auto *window : QGuiApplication::topLevelWindows()){
    //printf("window: %p. Got it? %d\n",window, maybe_got_key_window(window));
    if (maybe_got_key_window(window))
      return true;
  }

  return false;
#endif
}


extern "C" void run_main_loop(void);
void run_main_loop(void){
  g_qapplication->exec();
}

#if 1 //USE_QT_VISUAL

#if 0
#if __linux__
static double get_ms(void){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ((double)ts.tv_nsec) / 1000000.0;
}
#endif
#endif

#if 0
// Should ideally be atomic, but read and write are usually atomic anyway.
static volatile int using_the_event;

class MyQCustomEvent : public QCustomEvent{
public:
  MyQCustomEvent()
    : QCustomEvent(QEvent::User+1)
  {}

  // We don't want to malloc() from the player thread, since it's running SCHED_FIFO.
  void* operator new(size_t size){
    static MyQCustomEvent event;
    // new is always called from the player thread
    using_the_event = 1;
    return &event;
  }
  void operator delete(void *p){
    // delete is always called from the GUI thread
    using_the_event = 0;
  }
};
#endif

// This is now a dummy function
void Ptask2Mtask(void){
#if 0 // Polling instead.
  if(using_the_event==1)
    return;

  // Check that we're not overflowing the Qt Event system.
#if __linux__
  {
    static double last_time = 0.0;
    double time = get_ms();
    //if(time-last_time < 150) // this looks much better! (but it needs to be configurable)
    if(time-last_time < 40)
      return;
    last_time = time;
  }
#endif

  if(1){
    QObject *qobject=(QObject *)root->song->tracker_windows->os_visual.widget;
    MyQCustomEvent *qce = new MyQCustomEvent();

    qapplication->postEvent(qobject,qce);
  }
#endif
}

#endif // USE_QT_VISUAL

#if !GTK_IS_USED

#include <QTimer>
#include "../midi/midi_i_input_proc.h"
#include "../common/gfx_proc.h"
#include "../common/gfx_op_queue_proc.h"
#include "../common/player_proc.h"
#include "../common/Ptask2Mtask_proc.h"


//#define TEST_GC

#ifdef TEST_GC
#  include "gc.h"
#endif



enum RT_MESSAGE_STATUS {
  RT_MESSAGE_READY,
  RT_MESSAGE_FILLING_UP,
  RT_MESSAGE_READY_FOR_SHOWING,
  RT_MESSAGE_SHOWING
};

static DEFINE_ATOMIC(int, rt_message_status) = RT_MESSAGE_READY;
static const int rt_message_length = 1024;
static char rt_message[rt_message_length];

static DEFINE_ATOMIC(int64_t, g_request_from_jack_transport_to_start_playing) = -1;
static DEFINE_ATOMIC(bool, g_request_to_start_playing) = false;
static DEFINE_ATOMIC(bool, g_request_to_continue_playing) = false;
static DEFINE_ATOMIC(bool, g_request_to_stop_playing) = false;

int g_main_timer_num_calls = 0;

namespace{
  struct NoKeyboardEventsQMessageBox : public QMessageBox {
    NoKeyboardEventsQMessageBox(QWidget *parent)
      : QMessageBox(parent)
    {
      register_modal_qwidget(this);
      setFocusPolicy( Qt::NoFocus );
    }
    
    void keyPressEvent(QKeyEvent * event) override {
      event->ignore();
    }
    void keyReleaseEvent(QKeyEvent * event) override {
      event->ignore();
    }
  };
}

namespace{
class CalledPeriodically : public QTimer {

  QPointer<NoKeyboardEventsQMessageBox> rt_msgBox;
  QAbstractButton *rt_msgBox_ok;
  QAbstractButton *rt_msgBox_stop_playing;
  QAbstractButton *rt_msgBox_dontshowagain;
  QSet<QString> dontshow;

  const int interval;
  
public:
  CalledPeriodically()
    : interval(MAIN_TIMER_INTERVAL) // is set to either 1, 2, 5, 10, 25, or 50.
  {
    //R_ASSERT( (50 % interval) == 0);
    
    setInterval(interval);
    start();
  }
protected:

  void createRtMsgBox(void){
    R_ASSERT(rt_msgBox==NULL);
    
    rt_msgBox = new NoKeyboardEventsQMessageBox(NULL); //g_main_window  // Sometimes freezes the program on OSX if parent is not NULL.
    set_window_flags(rt_msgBox, radium::NOT_MODAL);
    
    rt_msgBox_dontshowagain = (QAbstractButton*)rt_msgBox->addButton("Dont show this message again",QMessageBox::ApplyRole);
    rt_msgBox_stop_playing = (QAbstractButton*)rt_msgBox->addButton("Stop playing!",QMessageBox::ApplyRole);
    rt_msgBox_ok = (QAbstractButton*)rt_msgBox->addButton("Ok",QMessageBox::AcceptRole);
    rt_msgBox->open();
    rt_msgBox->hide();
  }
  
  void 	timerEvent ( QTimerEvent * e ) override {
#ifdef TEST_GC
    printf("triggering full collect\n");
    GC_gcollect();
#endif
    
    if (g_is_loading==true)
      return;

    g_main_timer_num_calls++; // Must be placed here since 'is_called_every_ms' depends on it.


#if 0 // Not sure whether 0 or 1 is best. In theory 1 should be the best, but I can't see any difference compared to non-cpu friendly operation right now. However, I have seen that 0 has worked before, so I'm fairly confident 0 could work. Whether 1 may work remains to be seen.
    {
      if (is_called_every_ms(20)){
        API_gui_call_regularly(); // light operation
        
        //int interval = useCPUFriendlyAudiometerUpdates() ? 40 : 20;

        static int counter = 0;
        counter++;

        int what_to_update = -1;
        if (useCPUFriendlyAudiometerUpdates())
          what_to_update = (counter % 2) == 0;

        AUDIOMETERPEAKS_call_very_often(20, what_to_update);
      }
    }
#else
    {
      int interval = useCPUFriendlyAudiometerUpdates() ? 50 : 15;

      if (is_called_every_ms(interval)){
        API_gui_call_regularly();
        
        AUDIOMETERPEAKS_call_very_often(interval, -1);

        RTWIDGET_call_often();
      }
    }
#endif

    RETURN_IF_DATA_IS_INACCESSIBLE();

    //static int hepp=0; printf("hepp %d\n",hepp++);
    
    if (ATOMIC_GET(rt_message_status) == RT_MESSAGE_READY_FOR_SHOWING) {

      QString message(rt_message);

      if (rt_msgBox==NULL)
        createRtMsgBox();

      if (dontshow.contains(message)==false){

        QWidget *parent = get_current_parent(rt_msgBox, false);
        R_ASSERT_NON_RELEASE(parent!=NULL);
        if (parent!=NULL){
          R_ASSERT_NON_RELEASE(parent==parent->window());
          parent = parent->window();
        }
        
        if (parent != rt_msgBox->window() && parent != rt_msgBox->parent()){
          //printf(" 3. Setting window parent\n");
          set_window_parent(rt_msgBox, parent, radium::NOT_MODAL);
        }

        rt_msgBox->setText(message);

        safeShow(rt_msgBox);
        //getchar();
                                        
#if 0 //def FOR_WINDOWS
        HWND wnd=(HWND)rt_msgBox->winId();
        SetFocus(rt_msgBox);
        SetWindowPos(rt_msgBox, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
#endif
      }

      ATOMIC_SET(rt_message_status, RT_MESSAGE_SHOWING);
      
    } else if (ATOMIC_GET(rt_message_status) == RT_MESSAGE_SHOWING && (rt_msgBox==NULL || rt_msgBox->isHidden())) {

      if (rt_msgBox != NULL) { // If rt_msgBox was opened on top of another window, and that other window was closed, rt_msgBox will be NULL. (QPointers are quite nice).
        if (rt_msgBox->clickedButton() == rt_msgBox_dontshowagain){
          //printf("Dontshowagain\n");
          dontshow.insert(rt_message);
        } else if (rt_msgBox->clickedButton() == rt_msgBox_stop_playing){
          PlayStop();
        }
      }
      
      ATOMIC_SET(rt_message_status, RT_MESSAGE_READY);
    }

    static int num_calls_at_this_point = 0;
    num_calls_at_this_point++;

    struct Tracker_Windows *window=root->song->tracker_windows;

    // No, we still need to do this. At least in qt 5.5.1. Seems like it's not necessary in 5.7 or 5.8 though, but that could be coincidental.
    if(num_calls_at_this_point<150/interval){ // Update the screen constantly during the first second. It's a hack to make sure graphics is properly drawn after startup. (dont know what goes wrong)
      updateWidgetRecursively(g_main_window);
    }

    if(num_calls_at_this_point==160/interval){
      show_nag_window("");
    }


    // Force full keyboard focus to the main window after startup. This seems to be the only reliable way. (if you think this is unnecessary, see if left alt works to start navigating menues after startup while using the fvwm window manager)
    {
      static QPointer<MyQMessageBox> gakkbox = NULL; // gakkbox could, perhaps, be deleted by itself if radium finds a strange parent. (got a crash once where gakkbox was deleted before explicitly calling delete below.)

      if(num_calls_at_this_point==50/interval){
        gakkbox = MyQMessageBox::create(false, NULL);
        gakkbox->setText("Forcing focus");
        safeShow(gakkbox);
        if (gakkbox != NULL)
          gakkbox->lower(); // doesn't work, at least on linux. Normally I struggle to keep window on top, now it's the opposite. Should probably change Radium to use QMdiArea. It should solve all of the window manager problems.
      }
      if(num_calls_at_this_point==60/interval){
        if (gakkbox != NULL){
          gakkbox->hide();
          g_main_window->raise();
          //g_main_window->activateWindow();
        }
      }

      if(num_calls_at_this_point==70/interval){
        delete gakkbox;
        GFX_SetMenuFontsAgain();
        GFX_CloseProgress();
        ATOMIC_SET(g_qtgui_has_started_step2, true);
      }
    }
    
#if 0
    // Does not work.
    {
      static bool has_raised = false;
      if (has_raised==false && gnum_calls_at_this_point > 300/interval){
        g_main_window->raise();
        g_main_window->activateWindow();
        //BringWindowToTop((HWND)g_main_window->winId());
        //SetWindowPos((HWND)g_main_window->winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

        has_raised = true;
      }
    }
#endif
      
#if 0 //defined(FOR_WINDOWS)
    
    // Does not work.
    
    static bool has_focused = false;
    if(has_focused==false && num_calls_at_this_point>400/interval){ // Set focus constantly between 0.4 and 1.0 seconds after startup.
      
      // We started to lose keyboard focus at startup between 4.8.8 and 4.9.0 (but only if no message windows showed up, and only in RELEASE mode). Clicking the window did not help. I don't know wny.
      OS_WINDOWS_set_key_window((void*)g_main_window->winId());

      if (num_calls_at_this_point>5000/interval){
        has_focused=true;
      }
    }
#endif
    
    {
      DO_GFX({
          MIDI_HandleInputMessage();
#if !USE_OPENGL
          TRACKREALLINES_call_very_often(window);
#endif
        });
    }

    //if (qapplication->activeWindow() != NULL)
    //  printf("   active window\n");
    
    // Check if player has shut down
    if (PLAYER_is_running()==false)
      PlayStop();

    MIXER_call_very_often();
    
    PLUGIN_call_me_very_often_from_main_thread();
        
    {
      int64_t absabstime = ATOMIC_GET(g_request_from_jack_transport_to_start_playing);
      if(absabstime >= 0){
        PlaySong_from_jack_transport(absabstime);
        ATOMIC_SET(g_request_from_jack_transport_to_start_playing, -1);
      }
    }
    
    if(ATOMIC_GET(g_request_to_start_playing) == true) {
      PlayBlockFromStart(window, true);
      ATOMIC_SET(g_request_to_start_playing, false);
    }
    
    if(ATOMIC_GET(g_request_to_continue_playing) == true) {
      PlayBlockCurrPos(window);
      ATOMIC_SET(g_request_to_continue_playing, false);
    }
    
    if(ATOMIC_GET(g_request_to_stop_playing) == true) {
      PlayStop();
      ATOMIC_SET(g_request_to_stop_playing, false);
    }
    

    P2MUpdateSongPosCallBack();

    {
      struct Tracks *track = window->wblock->wtrack->track;
      ATOMIC_SET(g_curr_midi_channel, ATOMIC_GET(track->midi_channel));
    }
    
    PlayCallVeryOften();

    SampleRecorder_called_regularly();
    
    if (is_called_every_ms(50)){ // 50ms = 3*1000ms/60 (each third frame)
      static_cast<EditorWidget*>(window->os_visual.widget)->updateEditor(); // Calls EditorWidget::updateEditor(), which is a light function      
    }
    
    if (is_called_every_ms(15)){
      API_instruments_call_regularly();
    }
    
    if(doquit==true) {
      QApplication::quit();
    }

    PATCH_call_very_often();
    BACKUP_call_very_often();

    if (window->message_duration_left > 0){
      //printf("message dur: %d\n",window->message_duration_left);
      window->message_duration_left -= interval;
      if (window->message_duration_left <= 0){
        window->message_duration_left = 0;
        window->message = NULL;
        GL_create(window);
      }
    }

    const char *gc_warning_message = "Warning: Garbage collector is turned off";

    if (window->message==NULL && GC_is_disabled()) {
      
      window->message = gc_warning_message;
      GL_create(window);
      
    } else if (window->message==gc_warning_message && GC_is_disabled()==false){
      
      window->message=NULL;
      GL_create(window);
      
    }

    /*
    if (is_called_every_ms(1000)){
      QMainWindow *main_window = static_cast<QMainWindow*>(root->song->tracker_windows->os_visual.main_window);
      //bool is_visible = OS_SYSTEM_window_is_actually_visible((void*)main_window->winId());
      auto rect = main_window->visibleRegion().boundingRect();
      printf("    Window visible: %d,%d -> %d,%d\n", rect.x(),rect.y(),rect.width(),rect.height());//is_visible);
    }
    */
        
    if (is_called_every_ms(5000)){  // Ask for gl.make_current each 5 seconds.
      GL_lock();{
        GL_EnsureMakeCurrentIsCalled();
      }GL_unlock();
    }

    // Ensure editor is redrawn after resizing. For some reason, it doesn't always work to set must_redraw=true in the resize event virtual method.
    {
      static int last_height = -1;
      EditorWidget *editor = static_cast<EditorWidget*>(window->os_visual.widget);
      int new_height = editor->gl_widget->height();
      if (new_height != last_height){
        window->must_redraw = true;
        last_height = new_height;
      }
    }

    {
      int gl_status = GL_maybe_notify_that_main_window_is_exposed(interval);
      if (gl_status>=1)
        GL_maybe_estimate_vblank(static_cast<EditorWidget*>(window->os_visual.widget)->gl_widget);
    }

    #if 0
    static bool main_window_is_exposed = false;
    if (main_window_is_exposed==false){
      QMainWindow *main_window = (QMainWindow *)window->os_visual.main_window;
      if (main_window != NULL){
        if (main_window->isExposed()) {

          main_window_is_exposed = true;
        }
      }
    }
    #endif

    SEQUENCER_WIDGET_call_very_often();

    BS_call_very_often();
    
    //MIXER_called_regularly_by_main_thread();

    if(g_pausing_level != 0){
      RError("Qt_Main. g_pausing_level: %d", g_pausing_level);
      g_pausing_level = 0;
    }

    if (is_called_every_ms(15)){

      // Remake
      if(ATOMIC_COMPARE_AND_SET_BOOL(g_all_mixer_strips_needs_remake, true, false)){ // 

        {  // clear queue. (boost should add a clear/reset function. Maybe initialize() does that? 'initialize' is not documented though.)
          int64_t id;
          while(g_mixer_strips_needing_remake.pop(id)==true);
        }

        //printf("          (remake called from qt main)\n");
        S7CALL2(void_void,"remake-mixer-strips");

      } else if (g_mixer_strips_needing_remake.empty()==false) {
        
        QSet<int64_t> remake_ids;

        while(true){
          int64_t id;
          if (g_mixer_strips_needing_remake.pop(id)==false)
            break;
          remake_ids.insert(id);
        }

        QString code = "(remake-mixer-strips";
        for(const auto id : remake_ids)
          code += " " + QString::number(id);
        
        code += ")";

        evalScheme(code.toUtf8().constData());
      }

      // Redraw
      if(ATOMIC_COMPARE_AND_SET_BOOL(g_mixer_strips_needs_redraw, true, false)){ // 
        //printf("          (redraw called from qt main)\n");
        S7CALL2(void_void, "FROM_C-redraw-mixer-strips");
      }
    }

    BLOCKS_called_very_often();

    if (num_calls_at_this_point < 100) {
      if (is_called_every_ms(20))
        MIXERSTRIP_call_regularly();
    } else {
      if (is_called_every_ms(100))
        MIXERSTRIP_call_regularly();
    }
    
    API_call_very_often();

    THREADING_call_very_often();
      
    SEQTRACK_call_me_very_often();
    if (is_called_every_ms(1000))
      DISKPEAKS_call_very_often();

    if (MIDI_insert_recorded_midi_gfx_events()){
      window->must_redraw_editor = true;
      SEQUENCER_update(SEQUPDATE_TIME);
    }

    if (is_called_every_ms(50))
      SAMPLEREADER_call_very_often();
    
#if 0
    // Update graphics when playing
    {
      struct Tracker_Windows *window=root->song->tracker_windows;
      static_cast<EditorWidget*>(window->os_visual.widget)->callCustomEvent();
    }
#endif

#if !defined(FOR_WINDOWS)    
    if (is_called_every_ms(5000))
      checkup_on_check_backtrace_process();
#endif
  }
};
}

bool RT_message_will_be_sent(void){
  return ATOMIC_GET(rt_message_status)==RT_MESSAGE_READY;
}


// TODO: Show in log window instead. At least when we can't show the message.
void RT_message_internal(const char *fmt,...){
  va_list argp;

  if (!atomic_compare_and_set_int(&ATOMIC_NAME(rt_message_status), RT_MESSAGE_READY, RT_MESSAGE_FILLING_UP)){
#if 0 //!defined(RELEASE)
    static DEFINE_ATOMIC(int, curr_num_messages) = 0;
    int num_messages = ATOMIC_ADD_RETURN_NEW(curr_num_messages, 1);
    if (num_messages < 10){
      char *message = (char*)calloc(1, rt_message_length + 1);
      va_start(argp,fmt);
      /*	vfprintf(stderr,fmt,argp); */
      vsnprintf(message,rt_message_length-1,fmt,argp);
      va_end(argp);
      SYSTEM_show_message_menu(NULL, message);
      ATOMIC_ADD_RETURN_OLD(curr_num_messages, -1);
    }
#endif
    return;
  }
  
  va_start(argp,fmt);
  /*	vfprintf(stderr,fmt,argp); */
  vsnprintf(rt_message,rt_message_length-1,fmt,argp);
  va_end(argp);

  ATOMIC_SET(rt_message_status, RT_MESSAGE_READY_FOR_SHOWING);
}


bool RT_jack_transport_play_request_is_finished(void){
  return ATOMIC_GET(g_request_from_jack_transport_to_start_playing) == -1;
}


void RT_request_from_jack_transport_to_play(int64_t absabstime){
  /*
  if(absabstime > 0)
    printf("****** RT_request_from_jack_transport_to_play. absabstime: %d\n",(int)absabstime);
  */
  ATOMIC_SET(g_request_from_jack_transport_to_start_playing, absabstime);
}


void RT_request_to_start_playing_block(void){
  //abort();
  ATOMIC_SET(g_request_to_start_playing, true);
}

void RT_request_to_continue_playing_block(void){
  //abort();
  ATOMIC_SET(g_request_to_continue_playing, true);
}

void RT_request_to_stop_playing(void){
  //abort();
  ATOMIC_SET(g_request_to_stop_playing, true);
}

#endif


int GFX_ResizeWindow(struct Tracker_Windows *tvisual,int x,int y){return 0;}

bool ControlPressed(void){
#if 0 //FOR_MACOSX
  return QApplication::keyboardModifiers() & Qt::MetaModifier;
#else
  return QApplication::keyboardModifiers() & Qt::ControlModifier;
#endif
}

static bool ShiftPressed(Qt::KeyboardModifiers modifiers){
  return modifiers & Qt::ShiftModifier;
}

bool ShiftPressed(void){
  if (editor_has_keyboard_focus()==true && MOUSE_CYCLE_get_mouse_buttons()==Qt::NoButton)
    return AnyShift(tevent.keyswitch);
  else
    return ShiftPressed(QApplication::keyboardModifiers());
}

bool Control2Pressed(Qt::KeyboardModifiers modifiers){
#if FOR_MACOSX
  return modifiers & Qt::AltModifier;
#else
  return modifiers & Qt::MetaModifier;
#endif
}

bool Control2Pressed(void){
  return Control2Pressed(QApplication::keyboardModifiers());
}

bool HorizontalModifierPressed(Qt::KeyboardModifiers modifiers){
  return Control2Pressed(modifiers);
}

bool HorizontalModifierPressed(void){
  return HorizontalModifierPressed(QApplication::keyboardModifiers());
}
                               
bool VerticalModifierPressed(Qt::KeyboardModifiers modifiers){
  return ShiftPressed(modifiers);
}

bool VerticalModifierPressed(void){
  return ShiftPressed();
}
                               
bool AltPressed(void){
  return QApplication::keyboardModifiers() & Qt::AltModifier;
}

bool MetaPressed(void){
  return QApplication::keyboardModifiers() & Qt::MetaModifier;
}

static void setCursor(int64_t guinum, const QCursor &cursor){
  R_ASSERT_RETURN_IF_FALSE(guinum>=0);
  
  QWidget *widget = API_gui_get_widget(guinum);
  if (widget==NULL)
    return;

  //printf("============Setting cursor to %d\n", cursor.shape());

#if 0 // !defined(RELEASE)
  // We have one such call now, when setting open hand cursor in a file requester.
  if (g_user_interaction_enabled==true)
    R_ASSERT(!widget->isWindow()); // Can have this assertion since we currently have no such calls. If it happens, it will cause the mouse cursor to be stuck in this mode for the whole window.
#endif
  
  widget->setCursor(cursor);
}

void SetNormalPointer(int64_t guinum){
  setCursor(guinum, Qt::ArrowCursor);
}
void SetPointingPointer(int64_t guinum){
  setCursor(guinum, Qt::PointingHandCursor);
}
void SetOpenHandPointer(int64_t guinum){
  setCursor(guinum, Qt::OpenHandCursor);
}
void SetClosedHandPointer(int64_t guinum){
  setCursor(guinum, Qt::ClosedHandCursor);
}
void SetBlankPointer(int64_t guinum){
  setCursor(guinum, Qt::BlankCursor);
}
void SetDiagResizePointer(int64_t guinum){
  setCursor(guinum, Qt::SizeFDiagCursor);
}
void SetHorizResizePointer(int64_t guinum){
  setCursor(guinum, Qt::SizeHorCursor);
}
void SetHorizSplitPointer(int64_t guinum){
  setCursor(guinum, Qt::SplitHCursor);
}
void SetVerticalResizePointer(int64_t guinum){
  setCursor(guinum, Qt::SizeVerCursor);
}
void SetVerticalSplitPointer(int64_t guinum){
  setCursor(guinum, Qt::SplitVCursor);
}


double g_last_time_mouse_pointer_was_moved_by_the_program = 0;

void MovePointer(struct Tracker_Windows *tvisual, float x, float y){
  g_last_time_mouse_pointer_was_moved_by_the_program = TIME_get_ms(); // Important to set it as soon as possible before doing the actual movement. (at least not after)
  
  EditorWidget *editor=(EditorWidget *)tvisual->os_visual.widget;
  QPoint pos;
  if (!g_editor->isVisible())
    pos = QPoint(x - 10000, y - 10000);
  else
    pos = editor->mapToGlobal(QPoint(x,y));

#if FOR_MACOSX
  OS_OSX_set_cursorpos(pos.x(), pos.y()); // https://bugreports.qt.io/browse/QTBUG-33959
#else
  QCursor::setPos(pos);
#endif
}

void MoveAbsPointer(struct Tracker_Windows *tvisual, float x, float y){
  g_last_time_mouse_pointer_was_moved_by_the_program = TIME_get_ms(); // Important to set it as soon as possible before doing the actual movement. (at least not after)

#if FOR_MACOSX
  OS_OSX_set_cursorpos(x,y); // https://bugreports.qt.io/browse/QTBUG-33959
#else
  if (tvisual==NULL)
    QCursor::setPos(x,y);
  else {
    QWidget *editor=(EditorWidget *)tvisual->os_visual.widget;
    QScreen *screen = editor->window()->windowHandle()->screen();
    QCursor::setPos(screen, x, y);
  }
#endif
}

uint64_t GetMouseButtons(void){
  uint64_t ret = 0;
  uint64_t hepp = (uint64_t)MOUSE_CYCLE_get_mouse_buttons(); //QApplication::mouseButtons();

  R_ASSERT(TR_LEFTMOUSEDOWN < 64);
  R_ASSERT(TR_RIGHTMOUSEDOWN < 64);
  R_ASSERT(TR_MIDDLEMOUSEDOWN < 64);
  
  if (hepp & Qt::LeftButton)
    ret |= (2<<TR_LEFTMOUSEDOWN);
  
  if (hepp & Qt::RightButton)
    ret |= (2<<TR_RIGHTMOUSEDOWN);
  
  if (hepp & Qt::MiddleButton)
    ret |= (2<<TR_MIDDLEMOUSEDOWN);

  return ret;
}

double GetDoubleClickInterval(void){
  return QApplication::doubleClickInterval();
}

WPoint GetPointerPos(struct Tracker_Windows *tvisual){
  WPoint ret;
  QPoint pos;
  
  if (tvisual==NULL)
    pos = QCursor::pos();
  else{
    EditorWidget *editor=(EditorWidget *)tvisual->os_visual.widget;
    if (!g_editor->isVisible()){
      pos = QPoint(QCursor::pos().x() + 10000, QCursor::pos().y() + 10000);
      //R_ASSERT_NON_RELEASE(false);
    }else
      pos = editor->mapFromGlobal(QCursor::pos());
  }
  ret.x = pos.x();
  ret.y = pos.y();
  return ret;
}

WPoint GetAbsPointerPos(struct Tracker_Windows *tvisual){
  WPoint ret;
  QPoint pos;
  
  if (tvisual==NULL)
    pos = QCursor::pos();
  else{
    QWidget *editor=(EditorWidget *)tvisual->os_visual.widget;
    QScreen *screen = editor->window()->windowHandle()->screen();
    pos = QCursor::pos(screen);
  }
  ret.x = pos.x();
  ret.y = pos.y();
  return ret;
}

Area GetScreenSize(struct Tracker_Windows *tvisual){
  Area ret;
  //QPoint pos;

  QScreen *screen;
  
  if (tvisual==NULL)
    screen = QApplication::screens()[0];
  else{
    QWidget *editor=(EditorWidget *)tvisual->os_visual.widget;
    screen = editor->window()->windowHandle()->screen();
  }
  
  ret.x = screen->size().width();
  ret.x2 = screen->size().height();
  
  return ret;
}

void GFX_toggleFullScreen(struct Tracker_Windows *tvisual){
#if defined(FOR_MACOSX) && !defined(USE_QT5)
  GFX_Message2(NULL, false, "Full screen not supported on OSX");
#else
  QWidget *main_window = (QWidget *)tvisual->os_visual.main_window;

  if(main_window->isFullScreen()){
    main_window->showNormal();
  }else{
    main_window->showFullScreen();
  }
#if defined(FOR_WINDOWS)
  OS_WINDOWS_set_key_window((void*)g_main_window->winId()); // Need to do this when setting other windows to full screen. Set it for the main window too, just in case.
#endif
    
#endif
}

void GFX_toggleCurrWindowFullScreen(void){
  QWidget *toplevel = QApplication::topLevelAt(QCursor::pos()); // Note, QApplication::topLevelAt does not return a toplevel window, but instead a top level widget, which may, or may not, be a top level window.
  //printf("       toplevel: %p\n",toplevel);
  if(toplevel==NULL)
    return;

  QVector<QWidget*> all_windows = MIXERSTRIPS_get_all_widgets();
  for(auto *window : all_windows){
    if (window==toplevel){
      S7CALL2(void_void,"toggle-current-mixer-strips-fullscreen");
      return;
    }
  }
  
  for(auto *window : QGuiApplication::topLevelWindows()){
    //printf("window: %p. toplevel: %p. Equal? %d\n",window, toplevel->windowHandle(), window==toplevel->windowHandle());
    if (window==toplevel->windowHandle()){

      if (toplevel != g_main_window){

        fprintf(stderr, "\n\n\n     TOGGLE non-main window\n\n\n");
        
        QWidget *w = get_qwidget(g_mixer_widget);
        //printf("top: %p. w: %p. g_mixer_widget: %p. parentWidget1: %p\n", toplevel, w, g_mixer_widget, w->parentWidget());
        
        if (toplevel==w->parentWidget())
          toplevel = w; // Think this was necessary on windows. Don't remember why. Not entirely sure what the releationship is between Mixer_widget and MixerWidget.
        
        // Setting full screen is a little bit complicated for other windows than g_main_window. gui_setFullScreen takes care of all that.
        int64_t guinum = API_get_gui_from_widget(toplevel);
        gui_setFullScreen(guinum, !gui_isFullScreen(guinum));
        
      } else {

        // main window

        fprintf(stderr, "\n\n     TOGGLE main window\n\n\n");

        
        if(toplevel->isFullScreen()){
          printf("Trying to set normal\n");
          toplevel->showNormal();
        }else{

          /*
          if (toplevel->parent() != NULL)
            toplevel->setParent(NULL, Qt::Window  | DEFAULT_WINDOW_FLAGS);
          */
          
          printf("Trying to set full screen\n");
          toplevel->showFullScreen();
        }

#if defined(FOR_WINDOWS)
        OS_WINDOWS_set_key_window((void*)toplevel->winId()); // Need to do this when setting other windows to full screen. Set it for the main window too, just in case.
#endif

      }
      
      return;
    }
  }
}


void GFX_EditorWindowToFront(struct Tracker_Windows *tvisual){
  QWidget *main_window=static_cast<QWidget*>(tvisual->os_visual.main_window);

  //GFX_PlayListWindowToBack();
  main_window->raise();

#ifdef __linux__
  #ifndef USE_QT5
    XSetInputFocus(QX11Info::display(),(Window)QX11Info::appRootWindow(),RevertToNone,CurrentTime);
  #endif
#endif

  OS_SYSTEM_ResetKeysUpDowns();
}

#if 0
void assertRadiumInHomeDirectory(void){
  
  QString program_path = QCoreApplication::applicationDirPath();

#if 0
  QString home_path = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);

  if (!program_path.startsWith(home_path)){
    GFX_Message(NULL,
                QString("Warning!\n\n") +
                "Radium is not installed in your home directory. Unless you have write access to the directory \"" + program_path + "\", undefined behaviors are likely to happen"
                );
  }
#else
  QFile file(program_path + "/checking_write_permission.txt");

  bool success = file.open(QIODevice::WriteOnly);

  if (success)
    success=file.isOpen();

  if (success)
    success = file.write("hello",2)>0;

  if (file.isOpen())
    file.close();
  
  //  QFileInfo info(program_path + QDir::separator() + "eventreceiverparser_generated.py");
  //if (!info.isWritable())

  if (!success)
    GFX_Message(NULL,
                (
                 QString("Warning!\n\n") +
                 "Radium is installed in a directory without write access. (" + program_path + ")\n"
                 "Undefined behaviors may happen"
                 ).toUtf8().constData()
                );
#endif
}
#endif



//#include <qwindowsstyle.h>
//#include <qmacstyle_mac.h>
#if 0
#include <qplatinumstyle.h>
#include <qcdestyle.h>
#include <qmotifplusstyle.h>
#include <qsgistyle.h>
#include <gtkstyle.h>
#endif

#if USE_QT4
//#include <QCleanlooksStyle>
//#include <QOxygenStyle>
#ifndef USE_QT5
#  include <QPlastiqueStyle>
#endif
#endif

#ifdef USE_QT3
#include "qwidget.h"
#include "qlistbox.h"
#endif

//#include "google/profiler.h"

//extern LANGSPEC int dasmain(int argc,char **argv);
//extern LANGSPEC int radium_main(char *arg);
//extern LANGSPEC int GC_dont_gc;
//int radium_main(int argc,char **argv){

// Called from gtk main loop
void Qt_EventHandler(void){
#if 1 // At least on windows, qapplication->hasPendingEvents() never returns false.
  qapplication->processEvents();
#else
  while(qapplication->hasPendingEvents() && doquit==false)
    qapplication->processEvents();
#endif
#if 0
  QWidget *main_window = static_cast<QWidget*>(root->song->tracker_windows->os_visual.main_window);
  if(main_window->isVisible()==false)
    doquit=true;
#endif
}


void GFX_set_bottom_widget_height(int new_height){
  struct Tracker_Windows *window = root->song->tracker_windows;
  window->bottomslider_height = new_height;
  EditorWidget *editor = static_cast<EditorWidget*>(window->os_visual.widget);
  if(editor != NULL && editor->bottom_widget != NULL){
    editor->bottom_widget->setMinimumHeight(new_height);
    editor->bottom_widget->setMaximumHeight(new_height);
  }
}


//extern void updateAllFonts(QWidget *widget);

static bool g_load_new_song=true;
static char *g_songfile_from_commandline = NULL;
static QString g_startup_path;

int radium_main(const char *arg){

  TIME_init();

#if 0
  QApplication::setStyle( new QPlatinumStyle() );
  QApplication::setStyle( new QCDEStyle() );
  QApplication::setStyle( new QMotifPlusStyle() );
  QApplication::setStyle( new QSGIStyle() );
#endif
  {
    bool override_default_qt_style = SETTINGS_read_bool((char*)"override_default_qt_style",true);
    SETTINGS_write_bool((char*)"override_default_qt_style",override_default_qt_style);

#if 1
    if(override_default_qt_style){
      //QApplication::setStyle( new QOxygenStyle());

#ifdef USE_QT5

#if 0
        QString styles_path = QCoreApplication::applicationDirPath() + QDir::separator()
          + "packages" + QDir::separator()
          + "qtstyleplugins-src-5.0.0" + QDir::separator()
          + "plugins" + QDir::separator()
          + "styles";

        // Did not work. (???) Created a symbolic link to the plugin file instead.
        QCoreApplication::addLibraryPath(styles_path);

        // Did not work either. (???) Created a symbolic link to the plugin file instead.   
        QPluginLoader *gakk = new QPluginLoader(styles_path + QDir::separator()
#ifdef FOR_WINDOWS
                                                + "qplastiquestyle.dll"
#else
                                                + "libqplastiquestyle"
#endif
                                                );

        // This call succeeds, but the call to QStyleFactory::create("plastique") fails no matter what I do, except putting libqplastiquestyle.so directly into bin/styles/
        if (gakk->load()==false)
          GFX_Message(NULL, "Unable to load style library. Ensure Radium is installed properly.");
#endif // 0

        //QStyle *style = QStyleFactory::create("plastique");
        QStyle *style = QStyleFactory::create("fusion");
        if (style==NULL)
          GFX_Message(NULL, "Unable to load fusion style");
        else
          QApplication::setStyle(style);
        
#else // USE_QT5

        QApplication::setStyle( new QPlastiqueStyle());
        
#endif
      //QApplication::setStyle( new QMacStyle());
    
    //QApplication::setStyle( new QCleanlooksStyle() );
    //QApplication::setStyle( new QWindowsStyle() );
    }
#endif
  }

  //QApplication::setEffectEnabled(Qt::UI_AnimateMenu,true);
  //QApplication::setEffectEnabled(Qt::UI_AnimateCombo,true);

  //QApplication::setGraphicsSystem("native");
  //QApplication::setGraphicsSystem("raster");


  setApplicationColors(qapplication);

  g_qtgui_has_started = true;


  g_audio_block_size = SETTINGS_read_int("audio_block_size",64);
  g_rt_do_rerendering = SETTINGS_read_bool("enable_editor_rerendering_during_playback",true);
  
  set_default_slider_height();
  
#if 0
    vector_t v = {0};
  VECTOR_push_back(&v,"hepp1");
  VECTOR_push_back(&v,"hepp2");
  VECTOR_push_back(&v,"hepp3");
  GFX_Message(&v, "hepp hepp");
#endif


  OS_SYSTEM_init_keyboard();

  SetupMainWindow();


  //X11_StartBlockSelector();
  //X11_StartMidiProperties();

  //StartGuiThread();

  // ProfilerStart("hepps");

  //SCHEME_init1();

  printf("starting\n");
  if(InitProgram()==false)
    return 0;
  printf("ending\n");

  GFX_ShowProgressMessage("Initializing various parts of the program", true);
  
  SCHEME_init2();

  //ProfilerStop();

  posix_InitPlayer();

  GFX_ShowProgressMessage("Setting up main window", true);
  
  QWidget *block_selector = BS_get();

  struct Tracker_Windows *window = root->song->tracker_windows;


  BS_UpdateBlockList();
  BS_UpdatePlayList();
  //BS_SelectBlock(root->song->blocks);
  SelectWBlock(window, window->wblocks);
  BS_SelectPlaylistPos(0, false);

  QWidget *main_window = static_cast<QWidget*>(window->os_visual.main_window);

  {
    EditorWidget *editor = static_cast<EditorWidget*>(window->os_visual.widget);

    {

      struct XSplitter : public radium::Splitter {
      public:
        
        bool _strip_on_left_side;

        XSplitter(bool strip_on_left_side)
          : radium::Splitter(Qt::Horizontal)
          , _strip_on_left_side(strip_on_left_side)
        {
        }

        struct MixerStripParentLayout : public QHBoxLayout {

          // Sets maximum size of the layout of the parent widget to be the size of the mixer strip.
          // Something was wrong. It didn't override anything. Works without this one.
          //
          /*
          virtual QSize sizeHint() override {
            QWidget *strip = itemAt(0)->widget();
            if (strip==NULL)
              return QSize(-1,-1);

            //printf("Size: %d, %d\n", strip->size().width(), strip->size().height());
            return strip->size();
          }
          */
        };

        void add_mixer_strip(void){
          g_mixerstripparent = new QWidget;
          
          g_mixerstriplayout = new MixerStripParentLayout;
          g_mixerstriplayout->setSizeConstraint(QLayout::SetMaximumSize); // I.e. as small as possible, and never bigger than sizeHint(). (the layout type names in qt are not very intuitive)
          g_mixerstriplayout->setSpacing(0);          
          g_mixerstriplayout->setContentsMargins(0,0,0,0);
          
          g_mixerstripparent->setLayout(g_mixerstriplayout);

          addWidget(g_mixerstripparent);
        }
      };
      
      XSplitter *xsplitter = new XSplitter(showMixerStripOnLeftSide());
      xsplitter->setChildrenCollapsible(false);
      xsplitter->setHandleWidth(2);
                                
      editor->xsplitter = xsplitter;

      xsplitter->setOpaqueResize(true);
      //xsplitter->setOpaqueResize(false);

      if(xsplitter->_strip_on_left_side)
        xsplitter->add_mixer_strip();

      editor->editor_layout_widget = new EditorLayoutWidget();
      xsplitter->addWidget(editor->editor_layout_widget);

      xsplitter->addWidget(block_selector);
      block_selector->move(main_window->width()-100,0);

      block_selector->resize(100,block_selector->height());

      {
        SEQUENCER_WIDGET_initialize(main_window);
        createInstrumentsWidget();
      }

      radium::Splitter *ysplitter = dynamic_cast<radium::Splitter*>(API_get_main_ysplitter()); //new radium::Splitter(Qt::Vertical, main_window);
      ysplitter->setHandleWidth(1);
      //ysplitter->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding); // NOTE: Causes main window height to grow and grow when adding/removing widget. Strange.
      
      //main_window->setCentralWidget(ysplitter);
      QBoxLayout *main_layout = dynamic_cast<QBoxLayout*>(main_window->layout());
      if(main_layout==NULL){
        R_ASSERT(false);
        exit(-100);
      }
      main_layout->insertWidget(1, ysplitter, 1); // position 0, stretch 1.
      
      ysplitter->handle(1)->setEnabled(true);
      
      xsplitter->addWidget(get_qwidget(create_mixer_widget(main_window)));

      if(!xsplitter->_strip_on_left_side)
        xsplitter->add_mixer_strip();

      // To fix y-splitter not growing horizontally, causing the lower tabs to be minimized when neither the editor nor mixer were visible.
      {
        struct Gakk : public QWidget{
          QSize 	sizeHint() const override{
            return QSize(0,0);
          }
        };
        auto *grow_widget = new Gakk;
        grow_widget->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::MinimumExpanding);
        xsplitter->addWidget(grow_widget);
      }
      
      
      //QWidget *gakk = new MixerWidget(mixerwidgetandmixerstrip);
      //g_mixerstriplayout->addWidget(gakk);
      
      //gakk->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      //mixer_widget->resize(300,mixer_widget->height());

    }

    GFX_PlayListWindowToBack();
    GFX_InstrumentWindowToBack();

    //qapplication->setMainWidget(main_window);
    if (showPlaylistDuringStartup())
      GFX_PlayListWindowToFront();

  }

  qApp->setStyleSheet("QSplitter::handle{background-color: " + get_qcolor(HIGH_BACKGROUND_COLOR_NUM).dark(110).name(QColor::HexArgb) + ";}" +
                      "QTabWidget::pane { border: 0; background: " + get_qcolor(LOW_BACKGROUND_COLOR_NUM).name(QColor::HexArgb) + "}" +
                      DISK_file_to_qstring(OS_get_full_program_file_path("stylesheet.css"))
                      );

  GFX_ShowProgressMessage("Creating main menus", true);
  S7CALL2(void_void,"generate-main-menus");
    
  //getchar();


  GFX_ShowProgressMessage("Creating new song", true);
    
  //QFontDatabase::addApplicationFont("/gammelhd/usr/share/fonts/liberation/LiberationMono-Regular.ttf");

  ResetUndo();
  if(g_load_new_song==true){
    if (g_songfile_from_commandline != NULL) {
      
      QFileInfo info(g_songfile_from_commandline);
      QString filename = g_songfile_from_commandline;
      if (!info.isAbsolute())
        filename = g_startup_path + OS_get_directory_separator() + filename;
      
      if (LoadSong_CurrPos(window, STRING_create(filename))==false)
        NewSong_CurrPos(window);
      
    }else
      NewSong_CurrPos(window);
  }

  //updateAllFonts(QApplication::mainWidget());

  main_window->repaint();
  //DrawUpTrackerWindow(window);
  

  EditorWidget *editor = static_cast<EditorWidget*>(window->os_visual.widget);

#if USE_OPENGL

  GFX_ShowProgressMessage("Creating editor window", true);
  
  editor->gl_widget = GL_create_widget(editor);
  if(editor->gl_widget==NULL)
    return -100;

  editor->editor_layout_widget->layout()->addWidget(editor);
  editor->editor_layout_widget->layout()->addWidget(editor->gl_widget);

  editor->bottom_widget = API_gui_get_widget(S7CALL2(int_void, "FROM_C-create-editor-lower-part-gui"));
  GFX_set_bottom_widget_height(window->bottomslider_height);
  editor->editor_layout_widget->layout()->addWidget(editor->bottom_widget);
#endif


  //QApplication::quit();

  window->must_redraw = true;

  //printf("col: -%s-, font: -%s-\n",SETTINGS_read_string("last_color_version","0.0"),SETTINGS_read_string("last_system_font_version","0.0"));

  
  if(strcmp(SETTINGS_read_string("last_color_version","0.0"),"3.7.2")){
    GFX_Message(NULL,
                "Note!\n\n"
                "The default colors have changed. In case you have run Radium before, you might want to go to the Edit menu and select \"Set Default Colors\".\n"
                "\n"
                "You will only see this message once.");
    SETTINGS_write_string("last_color_version","3.7.2");
  }

  if(strcmp(SETTINGS_read_string("last_system_font_version","0.0"),"1.9.21")){
    GFX_Message(NULL,
                "Note!\n\n"
                "The default system font has changed. In case you have run Radium before, you might want to go to the Edit menu and select \"Set Default System Font\".\n\n"
                "You will only see this message once.");
    SETTINGS_write_string("last_system_font_version","1.9.21");
  }

  if(strcmp(SETTINGS_read_string("last_editor_font_version","0.0"),"3.0.b2.3")){
    GFX_Message(NULL,
                "Note!\n\n"
                "The default editor font has changed. In case you have run Radium before, you might want to go to the Edit menu and select \"Set Default Editor Font\".\n\n"
                "You will only see this message once.");
    SETTINGS_write_string("last_editor_font_version","3.0.b2.3");
  }


  INIT_Pianoroll_headers();

  ATOMIC_SET(is_starting_up, false);
  g_is_starting_up = false;

  window->must_redraw = true;
  editor->update();
  editor->resize(editor->width(),editor->height());
  main_window->updateGeometry();
  
#if USE_OPENGL
  GL_create(window);
#endif

#if 0
  // Hack to make Qt text input widgets not crash the program when using intel gfx driver and running opengl in separate thread (crash caused by opening two opengl contexts simultaneously from two threads). (strange stuff)
  GL_lock();
  //GL_draw_lock(); // <-- This prevents some crashes in buggy gfx drivers, but it could also cause a deadlock (not sure).
  {
    QTextEdit e;
    e.show();
    e.setFocus();
    qApp->processEvents();
  }
  //GL_draw_unlock();
  GL_unlock();
#endif
  
#if 0
  while(1){
    qApp->processEvents();
    msleep(500);
  }
#endif

#if 0
  vector_t v = {0};
  VECTOR_push_back(&v,"hepp1");
  VECTOR_push_back(&v,"hepp2");
  VECTOR_push_back(&v,"hepp3");
  GFX_Message(&v, "hepp hepp");
#endif

  //abort();

  //getchar();
  
  //RWarning("warning!");
  //g_splashscreen->finish(main_window);
  //GFX_CloseProgress();
  //delete g_splashscreen;
  //g_splashscreen = NULL;

  GFX_ShowProgressMessage("Final setup", true);

  
#if 0
  assertRadiumInHomeDirectory();
#endif

  UPDATECHECKER_doit();
  
  //QApplication::processEvents(); // Windows spends some time to initialize proxy, or something like that (there are numerous QTBUG entries on this). We trigger that work here while the splash screen is still open. If not it seems like the program have hanged right after startup. (No, it didnt make a difference. Qt has screwed up network initialization on windows since it blocks the main thread for a few seconds when doing the first request. Qt should have done this in a separate thread. Seems like the simplest solution is to use libcurl.)
    
  
  g_qtgui_exec_has_started = true;

  updateWidgetRecursively(g_main_window);

  main_window->adjustSize();
  main_window->updateGeometry();
  main_window->resize(main_window->width()+100, main_window->height()+100);
  
  moveWindowToCentre(main_window, g_startup_rect);
  main_window->show();
  main_window->raise();
  main_window->activateWindow();

  updateWidgetRecursively(g_main_window);

#if defined(FOR_WINDWS)
  // Probably makes no difference.
  OS_WINDOWS_set_key_window((void*)g_main_window->winId());
#endif
  
#if defined(FOR_MACOSX) // Only needed on 10.12 though.
  GFX_SetSystemFont(QApplication::font());
#endif


#if defined(FOR_MACOSX)
  OS_OSX_clear_modifiers(); // Don't know why we need to call this again. (it's also called from OS_SYSTEM_init_keyboard)
#endif
  
#if defined(FOR_WINDOWS)
  W_RegisterRawInputHandler((void*)g_main_window->effectiveWinId());
#endif
  
  CalledPeriodically periodic_timer;

  FOCUSFRAMES_init();

#if !defined(FOR_WINDOWS)
  start_check_backtrace_process();
#endif
  
#if USE_QT_VISUAL
  qapplication->exec();
#else
  GTK_MainLoop();
#endif

#define D(A)
//#define D(A) A

  D(GFX_OpenProgress("Closing\n"));
  D(GFX_ShowProgressMessage("1", true));

  // Drain the g_readers_ready_for_deletion queue.
  while(SAMPLEREADER_call_very_often()){
  }
  
  D(GFX_ShowProgressMessage("2", true));
  
  g_qtgui_has_stopped = true;

  PlayStop();

  D(GFX_ShowProgressMessage("3", true));
  
  // We don't want the crashreporter to pop up if there is something wrong when program exits. Not so important, and it looks unprofessional.
  CRASHREPORTER_dont_report();

  D(GFX_ShowProgressMessage("4", true));
  
  DISK_cleanup();

  D(GFX_ShowProgressMessage("5", true));
  
  fprintf(stderr,"          ENDING 1\n");
  
  //g_qt_is_running = false;

  if (editor->gl_widget != NULL)
    GL_stop_widget(editor->gl_widget);

  D(GFX_ShowProgressMessage("6", true));
  fprintf(stderr,"          ENDING 2\n");
  
#if 0
  while(doquit==false){
    while(GTK_HasPendingEvents() || qapplication->hasPendingEvents()){
      GTK_HandleEvents();
      qapplication->processEvents();
    }
    msleep(1);
  }
#endif

  ATOMIC_SET(is_starting_up, true); // Tell the mixer that program is not running
  g_is_starting_up = true;
  
  Undo_start_ignoring_undo_operations();{
    D(GFX_ShowProgressMessage("7", true));
    MW_cleanup(false); // Stop all sound properly. Don't want clicks when exiting.
    D(GFX_ShowProgressMessage("8", true));
  }Undo_stop_ignoring_undo_operations();

  D(GFX_ShowProgressMessage("9", true));
  fprintf(stderr,"          ENDING 3\n");
    
  msleep(30); // wait a little bit so the player gets back to the main loop

  D(GFX_ShowProgressMessage("10", true));
  EndProgram(); // shut down most of the program, except audio and midi

  fprintf(stderr,"          ENDING 4\n");

  D(GFX_ShowProgressMessage("11", true));
  posix_EndPlayer();
  //EndGuiThread();

  fprintf(stderr,"          ENDING 5\n");
  
  MIXER_stop();

  D(GFX_ShowProgressMessage("12", true));
  fprintf(stderr,"          ENDING 6\n");
    
  MULTICORE_shut_down();

  D(GFX_ShowProgressMessage("12.5", true));
  fprintf(stderr,"          ENDING 6.5\n");

  CloseAllInstruments();
  
  D(GFX_ShowProgressMessage("13", true));
  fprintf(stderr,"          ENDING 7\n");
  
#ifdef WITH_FAUST_DEV
  FFF_shut_down();
  D(GFX_ShowProgressMessage("14", true));
#endif

  fprintf(stderr,"          ENDING 8\n");
  
  SAMPLEREADER_shut_down();
  
  D(GFX_ShowProgressMessage("15", true));
  fprintf(stderr,"          ENDING 9\n");
    
  DISKPEAKS_stop();

  D(GFX_ShowProgressMessage("16", true));
  fprintf(stderr,"          ENDING 10\n");

  SAMPLEREADER_delete_all_deletable_audio_files(); // This function is a little bit dirty, and because of some dirtyness that might interfere with the sample reder thread and the disk peaks, we call it after calling the two functions above (at least it feels cleaner this way, it probably works to call the function before the other ones as well.).

  D(GFX_ShowProgressMessage("17", true));
  fprintf(stderr,"          ENDING 11\n");

  //V_shutdown();
  
  //CRASHREPORTER_close();

  // Give various stuff some time to exit (not supposed to be necessary though, and hopefully it is not necessary either)
#if defined(RELEASE)
  OS_WaitForAShortTime(100);
#endif
  
  fprintf(stderr,"          ENDING 12\n");
  
  return 0;

}

extern "C" {
  static void finish(int sig){
    fprintf(stderr,"FINISH!\n");
    QApplication::quit();
  }
#if 0
  static void crash(int sig){
    fprintf(stderr,"CRASH!!! %d\n",sig);
    abort();
  }
#endif
}


// based on qglobal::qunsetenv from the qt 5 source
#ifndef USE_QT5

static void qunsetenv(const char *varName)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    _putenv_s(varName, "") == 0;
#else

#if (defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L) || defined(Q_OS_BSD4) || defined(Q_OS_HAIKU)
    // POSIX.1-2001 and BSD have unsetenv
    unsetenv(varName);
#endif
    
#if FOR_WINDOWS
    {
      // On mingw, putenv("var=") removes "var" from the environment
      QByteArray buffer(varName);
      buffer += '=';
      putenv(buffer.constData());
    }
#endif
    
    {
      // Fallback to putenv("var=") which will insert an empty var into the
      // environment and leak it
      QByteArray buffer(varName);
      buffer += '=';
      char *envVar = qstrdup(buffer.constData());
      putenv(envVar);
    }
#endif
}

#endif

#ifdef USE_QT5
static void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
#else
static void myMessageOutput(QtMsgType type, const char *localMsg)
#endif

{
#ifdef USE_QT5
  QByteArray localMsg = msg.toLocal8Bit();
#endif

  bool old_g_qtgui_has_started = g_qtgui_has_started;
  
  g_qtgui_has_started=false;

  char temp[1024];
    
  switch (type) {
    case QtDebugMsg:
      fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
      break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
      fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
      break;
#endif
    case QtWarningMsg:
      snprintf(temp, 1000, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
      fprintf(stderr, "%s", temp);
#ifndef RELEASE
#if !RADIUM_USES_TSAN
      SYSTEM_show_error_message(temp);
#endif
#endif
      break;
    case QtCriticalMsg:
      snprintf(temp, 1000, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
      fprintf(stderr, "%s", temp);
#ifndef RELEASE
      SYSTEM_show_error_message(temp);
#endif
      break;
    case QtFatalMsg:
      snprintf(temp, 1000, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
      fprintf(stderr, "%s", temp);
#ifndef RELEASE
      SYSTEM_show_error_message(temp);
#endif        
      break;
      //abort();
    default:
      fprintf(stderr, "Unkwon qt: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
  }

  g_qtgui_has_started=old_g_qtgui_has_started;
}
 
void MONOTONIC_TIMER_init(void);


#if FOR_MACOSX
#include <mach-o/dyld.h> 
static char *get_macosx_executable_path(void){
  uint32_t size = 1024;
  
  char *ret = (char*)malloc(size);
  
  if (_NSGetExecutablePath(ret, &size) == -1) {
    size++;
    free(ret);
    ret = (char*)malloc(size);
    R_ASSERT(_NSGetExecutablePath(ret, &size) != -1);
  }
  
  return ret;
}
#endif


static char g_static_char;
static char g_static_char2 = 5;

char g_char;
char g_char2 = 6;

static bool char_inside(const char *a, const char *b, const char *c) {
  if (b >= a && b < c)
    return true;
  else
    return false;
}


#if defined(FOR_WINDOWS)

#include <windows.h>
#include <dbghelp.h>

/*
void OS_WINDOWS_move_main_window_to_front(void){
  HWND wnd=(HWND)g_main_window->effectiveWinId();
  SetWindowPos(wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
}
*/                                        

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms680341(v=vs.85).aspx
// http://www.csn.ul.ie/~caolan/pub/winresdump/winresdump/doc/pefile2.html
static void add_windows_gc_roots(void){

  static char l_static_char;
  static char l_static_char2 = 7;
  
  HMODULE module_handle = GetModuleHandle(NULL);
  
  char *image_base = (char*)module_handle;
  
  IMAGE_NT_HEADERS *image_nt_header = ImageNtHeader(module_handle);
  
  IMAGE_SECTION_HEADER *image_section_headers = (IMAGE_SECTION_HEADER *) (image_nt_header + 1);
  
  int total_bytes = 0;
  
  for ( int i = 0 ; i < image_nt_header->FileHeader.NumberOfSections ; i++) {

    char name[16] = {};
    strncpy(name, (char*)image_section_headers[i].Name, 8);
    
    char *start = image_base + image_section_headers[i].VirtualAddress;
    int size = image_section_headers[i].Misc.VirtualSize;
    char *end = start + size;

    bool in1 = char_inside(start,&g_static_char,end);
    bool in2 = char_inside(start,&g_static_char2,end);
    bool in3 = char_inside(start,&g_char,end);
    bool in4 = char_inside(start,&g_char2,end);
    bool in5 = char_inside(start,&l_static_char,end);
    bool in6 = char_inside(start,&l_static_char2,end);
    bool is_inside = in1 || in2 || in3 || in4 || in5 || in6;

    bool writable = image_section_headers[i].Characteristics & IMAGE_SCN_MEM_WRITE;
    
    if (writable || is_inside) {
      GC_add_roots(start, end);
      total_bytes += size;
    }
#if !defined(RELEASE)
    if (is_inside)
      R_ASSERT(writable);
#endif
    
    printf("\"%s\". Writable: %d. Size: %d, start: %p, inside: %d %d %d %d %d %d\n", name, writable, size, start, in1,in2,in3,in4,in5,in6);
  }
    
  printf("finished. Total roots added: %fMb\n", (double)total_bytes / (1024*1024.0));
  //getchar();
}

#endif // defined(FOR_WINDOWS)


#if defined(FOR_MACOSX) || defined(FOR_LINUX) // Doesn't seem like the GC_has_static_roots callback is used in windows.
static int gc_has_static_roots_func(
                                     const char * dlpi_name,
                                     void * p,
                                     size_t size
                                     )
{
  static char l_static_char;
  static char l_static_char2 = 7;
  
  char *start = (char*)p;
  char *end = start + size;

#if FOR_MACOSX
  static char *executable_path = get_macosx_executable_path();
#endif

  bool in1 = char_inside(start,&g_static_char,end);
  bool in2 = char_inside(start,&g_static_char2,end);
  bool in3 = char_inside(start,&g_char,end);
  bool in4 = char_inside(start,&g_char2,end);
  bool in5 = char_inside(start,&l_static_char,end);
  bool in6 = char_inside(start,&l_static_char2,end);
  bool is_inside = in1 || in2 || in3 || in4 || in5 || in6;
  
  bool is_main_root;

  if (is_inside) // Normally, this should(?) be good enough. But we have extra checks below as well.
    is_main_root = true;
#if defined(FOR_LINUX)
  else if (!strcmp("PT_GNU_RELRO", dlpi_name))
    is_main_root = false;
  else if (!strcmp("", dlpi_name)) // This is a bit flaky. Haven't found any way any other way to identify the main name on linux. Note that there are two assertion in !RELEASE mode for this check below.
    is_main_root = true;
  else if (dlpi_name[0] != '/') // Add this check as well, might be a useless check.
    is_main_root = true;
#elif defined(FOR_MACOSX)
  else if (!strcmp(executable_path, dlpi_name)) // This should not be flaky
    is_main_root = true;
#endif
  else
    is_main_root = false;
  
#if !defined(RELEASE)
  static int64_t total = 0;

  #if FOR_LINUX
    if (is_inside && strcmp("", dlpi_name)){
      fprintf(stderr, "1. start: %p, static: %p, end: %p, size: %d\n",start,&g_static_char,end,(int)size);
      abort();
    }
    if (!is_inside && !strcmp("", dlpi_name)){
      fprintf(stderr, "2. start: %p, static: %p, end: %p, size: %d\n",start,&g_static_char,end,(int)size);
      abort();
    }
  #endif
  
  if (is_main_root)
    total = size;
  else if (!strcmp("PT_GNU_RELRO", dlpi_name))
    total -= size;
  else
    total += size;

  
  
#if 0

  #if !defined(FOR_MACOSX)
    const char *executable_path = "";
  #endif

  printf("   ===== has_static_roots: -%s-, %fMB (%f). is_main: %d.  (%p). argv0: -%s-\n", dlpi_name, (double)total / (1024*1024.0), (double)size / (1024*1024.0), is_main_root, p, executable_path);
  if(is_main_root)
    getchar();
  //abort();
#endif
  
#endif // !defined(RELEASE)

#if defined(FOR_LINUX)
  {
    QString name(dlpi_name);
    if (name.contains("libxcb.so")){
      if (!name.contains("bin/packages/libxcb-1.13/src/.libs/")){
        fprintf(stderr,"\n\n%c[31mError. A version of libxcb not included with Radium has been dynamically linked into the program. Something is wrong the installation of Radium.\nOlder versions of libxcb (probably before 1.11.1) are unstable with Radium.%c[0m\n\n", 0x1b, 0x1b);
        abort();
      }
    }
  }
#endif
 
 
  if (is_main_root)
    return 1;
  else
    return 0;
}
#endif // defined(FOR_MACOSX) || defined(FOR_LINUX)

void processEventsALittleBit(void){
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1);
}

//extern LANGSPEC void testme(void);

extern LANGSPEC void init_weak_jack(void);


#if TEST_CRASHREPORTER
static void
crashreporter_foo()
{
  int *f=NULL;
  *f = 0;
}

static void
bar()
{
  crashreporter_foo();
}
#endif

int main(int argc, char **argv){  
  //  testme();

#if TEST_CRASHREPORTER
  QApplication dasqapp(argc,argv);
  CRASHREPORTER_init();

  bar();
  printf("gakkgakk\n");
  return 0;
#endif
  
#if defined(FOR_WINDOWS)
  GC_set_no_dls(1);
#endif

#if defined(FOR_LINUX)
  if (getenv("QT_QPA_PLATFORMTHEME")!=NULL){
    printf("Environment variable QT_QPA_PLATFORMTHEME is set. Will not start program.\n");
    abort();
  }
  if (getenv("QT_QPA_PLATFORM")!=NULL){
    printf("Environment variable QT_QPA_PLATFORM is set. Will not start program.\n");
    abort();
  }
  if (getenv("QT_PLUGIN_PATH")!=NULL){
    printf("Environment variable QT_PLUGIN_PATH is set. Will not start program.\n");
    abort();
  }
#endif
           

#if defined(FOR_MACOSX) || defined(FOR_LINUX)
  GC_register_has_static_roots_callback(gc_has_static_roots_func);
#endif
  
  GC_INIT();

#if defined(FOR_WINDOWS)
  add_windows_gc_roots();
#endif


#ifdef FOR_MACOSX
  OS_OSX_show_icon_in_dock();
#endif

  THREADING_init_main_thread_type();
  THREADING_init_player_locks();
  
#ifdef USE_QT5
  qInstallMessageHandler(myMessageOutput);
#else
  qInstallMsgHandler(myMessageOutput);
#endif
  
  QCoreApplication::setLibraryPaths(QStringList());  

#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR == 9
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);

#ifndef USE_QT5
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif
  
  MONOTONIC_TIMER_init();
  
  PLUGINHOST_init();
  
  //signal(SIGSEGV,crash);
  //signal(SIGFPE,crash);

  // http://stackoverflow.com/questions/27982443/qnetworkaccessmanager-crash-related-to-ssl
  qunsetenv("OPENSSL_CONF");

#if !defined(FOR_WINDOWS)
  setenv("LC_NUMERIC", "C", 1); // Qt insists on doing strange things with locale settings, causing commans to appear instead of punctation. In an ideal world, LC_NUMERIC/LANG should never be set to anything else than "C", but unfortunately, many computers runs with uncommon language settings such as french or swedish. By default, programs seems to respect the sane behaviour (in the programming world), namely to never use commas when converting between strings and floats, but Qt does something strange with the world inside the QApplication contructor, and causes commas to be used everywhere if there is an uncommon LC_NUMERIC settings (or uncommon LANG setting). This setenv call is the only way I was able to make Pd work, without modifying Pd itself. (I modified Pd too though, but kept this line to prevent similar errors to appear in other libraries.) This behaviour should be changed in Qt.)
#endif

  // for mingw
  putenv(V_strdup("LC_NUMERIC=C"));
  
  //QLocale::setDefault(QLocale::C);
  QLocale::setDefault(QLocale::c());

#if 0
  printf("argv0: \"%s\"\n",argv[0]);
  return 0;
#endif

  QApplication::setDesktopSettingsAware(false);

  QLocale::setDefault(QLocale::C);

    
  argv = getQApplicationConstructorArgs(argc, argv); // Add Qt things to the command line arguments. (freetype).
  
  // Create application here in order to get default style. (not recommended, but can't find another way)
  qapplication=new MyApplication(argc,argv);
  qapplication->setAttribute(Qt::AA_MacDontSwapCtrlAndMeta, true);
  qapplication->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings); // Fix splitter handlers on OSX. Seems like a good flag to set in general. Seems like a hack qt has added to workaround bugs in qt. https://bugreports.qt.io/browse/QTBUG-33479

  QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, false);

  init_weak_jack();
  
  g_startup_rect = QApplication::desktop()->screenGeometry(); // Probably no point. Hoped that it would force radium to open on the same desktop that was current when program started.

  printf("********* Has set startup rect %d, %d**********\n", g_startup_rect.x(), g_startup_rect.y());
  //getchar();
  
#if 0
 #if defined(IS_LINUX_BINARY) || defined(FOR_WINDOWS) || defined(FOR_MACOSX)
    QApplication::addLibraryPath(QCoreApplication::applicationDirPath() + QDir::separator() + "qt5_plugins");
  #endif
#endif
  
  g_qapplication = qapplication;
  g_startup_path = QDir::currentPath();
  
  OS_set_argv0(argv[0]);

  R_ASSERT(THREADING_is_main_thread());
  
  g_qt_is_running = true;

  CRASHREPORTER_init();
  
  SETTINGS_init();

  bool try_incremental_gc = SETTINGS_read_bool("try_incremental_gc",false);
  if (try_incremental_gc || SETTINGS_read_bool("incremental_gc",false)) {
    if (try_incremental_gc)
      SETTINGS_write_bool("try_incremental_gc",false); // Set back before calling 'GC_enable_incremental' in case 'GC_enable_incremental' crashes.
    
#if defined(RELEASE) // incremental crashes under gdb
    GC_enable_incremental();
#endif
    
    g_gc_is_incremental = true;
  }


#if defined(FOR_MACOSX)
  if (QSysInfo::productVersion()=="10.14"){
    GFX_Message(NULL,
                "Radium has not been tested on this very much on this version of macOS. Please report problems you have with it.\n"
                "\n"
                "If Radium crashes right after startup, it's probably the OpenGL library that crashes. "
                "Fortunately, the bug is usually only hit during startup, and not every time."
                );
  } else if (QSysInfo::productVersion()=="10.15" || QSysInfo::productVersion()=="10.16" || QSysInfo::productVersion()=="10.17"){
    GFX_Message(NULL, "Radium has not been tested on this version of macOS. Latest supported version of macOS is 10.14. Radium might now freeze, crash, or misbehave in subtle ways. Please report your experience running Radium on this operating system to the forum, the mailing list, or to k.s.matheussen@notam02.no.");
  }
#endif



  //GC_disable();
  //QPixmap pixmap(OS_get_full_program_file_path("radium_256x256x32.png"));
  //QPixmap pixmap(QPixmap(OS_get_full_program_file_path("/home/kjetil/radium/pictures/logo_big.png")).scaled(QSize(256,256), Qt::KeepAspectRatioByExpanding));
  GFX_OpenProgress("Please wait, starting program");
  
  SCHEME_init1();

  GFX_ShowProgressMessage("Starting OpenGL", true);
  if (GL_check_compatibility()==false){
    GFX_CloseProgress();
    return -1;
  }

  GFX_ShowProgressMessage("Initializing GUI", true);
  
  //g_splashscreen = new QSplashScreen(pixmap);
#if 0 //def RELEASE
  g_splashscreen->adjustSize();
  g_splashscreen->showMessage("Starting up");
  g_splashscreen->show();
  g_splashscreen->raise();
  QApplication::processEvents();
#endif

#if 0 //def FOR_WINDOWS
  HWND wnd=(HWND)g_splashscreen->winId();
  //SetFocus(rt_msgBox);
  SetWindowPos(wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE); // The splash screen window doesn't always show at top.
#endif
  
  printf("1: argv[0]: \"%s\"\n",argv[0]);

#if 0
  {
    int i=0;
    int k=5;
    printf("%d\n",k/i);
    sscanf("12345", "%i", (int *) (k=i));
  }
#endif

  if(argc>1){
    if (!strcmp(argv[1],"--dont-load-new-song"))
      g_load_new_song=false;
    else
      g_songfile_from_commandline = strdup(argv[1]);
  }

#if defined(IS_LINUX_BINARY)
#if 0  
  setenv("PYTHONHOME","temp/dist",1);
  setenv("PYTHONPATH","temp/dist",1);
#else
  QString pythonlibpath = OS_get_full_program_file_path(QString("python2.7/lib"));
  setenv("PYTHONHOME",V_strdup(pythonlibpath.toLocal8Bit().constData()),1);
  setenv("PYTHONPATH",V_strdup(pythonlibpath.toLocal8Bit().constData()),1);

#endif
#endif

#if defined(FOR_MACOSX)
  QString pythonlibpath = OS_get_full_program_file_path(QString("python2.7/lib"));
  setenv("PYTHONHOME",V_strdup(pythonlibpath.toLocal8Bit().constData()),1);
  setenv("PYTHONPATH",V_strdup(pythonlibpath.toLocal8Bit().constData()),1);
#endif
  
#if defined(FOR_WINDOWS)
#if 0 //__WIN64

  //QString pythonlibpath = QCoreApplication::applicationDirPath() + QDir::separator() + "python2.7" + QDir::separator() + "lib"; // + QDir::separator() + "lib" + QDir::separator() + "python2.7";
#if 1 //__WIN64
  QString pythonlibpath = OS_get_full_program_file_path("python2.7"); // + QDir::separator() + "lib" + QDir::separator() + "python2.7";
#else
  QString pythonlibpath = OS_get_full_program_file_path("colors"); // + QDir::separator() + "lib" + QDir::separator() + "python2.7";
#endif
  //putenv(strdup(QString("PYTHONHOME="+pythonlibpath).toLocal8Bit().constData()));
  //putenv(strdup(QString("PYTHONPATH="+pythonlibpath).toLocal8Bit().constData()));
  printf("pythonlibpath: -%s-\n",pythonlibpath.toLocal8Bit().constData());
  //Py_SetPythonHome(V_strdup(pythonlibpath.toLocal8Bit().constData()));

  if (STRING_is_local8Bit_compatible(pythonlibpath)==false){
    printf("   String is not compatible %d %d %d\n", STRING_is_local8Bit_compatible(pythonlibpath), STRING_is_local8Bit_compatible("hello"), STRING_is_local8Bit_compatible("helloø"));
    vector_t v={};
    VECTOR_push_back(&v,"Try to run anywyay"); // (but please don't send a bug report if Radium crashes)");
    int quit = VECTOR_push_back(&v,"Quit");
    
    int res = GFX_Message(&v,
                          "Error. The path \"%s\" is not compatible with the local 8 bit charset.\n"
                          "\n"
                          "In order to run Radium, you most move the program to a different directory.\n"
                          "\n"
                          "(This problem is caused by a 3rd party library which can't be replaced or fixed easily. The problem only exist on Windows. Sorry for the inconvenience.)\n",
                          pythonlibpath.toUtf8().constData()
                          );
    
    if (res==quit){
      exit(-1);
      abort();
    }
  }

#if __WIN64
  Py_SetPythonHome(V_strdup("python2.7")); //V_strdup(pythonlibpath.toLocal8Bit().constData()));
#endif
#endif
#endif
  //Py_SetProgramName(QString(python
  //Py_SetPythonHome(V_strdup(OS_get_full_program_file_path("").toLocal8Bit().constData()));

#if __WIN64
  // Py_SetPath(".;Lib") // Py_SetPath doesn't seem available. Perhaps it's Python 3.0 only. As a workaround we call sys.path.append in python2.7/Lib/site.py
#endif
    
  Py_Initialize();

  {
    char temp[500];

    // Set loading path to argv[0]
    PyRun_SimpleString("import sys");

    PyRun_SimpleString("import os");

#if 0
    //#if defined(FOR_WINDOWS)
    sprintf(temp,"sys.g_program_path = \"\"");
#else
    // This doesn't work on mingw. Could be a wine problem only.
    sprintf(temp,"sys.g_program_path = os.path.abspath(os.path.dirname(\"%S\"))", OS_get_full_program_file_path(L"start.py"));
#endif
    PyRun_SimpleString(temp);

    PyRun_SimpleString("print \"hepp:\",sys.g_program_path,23");
    
    PyRun_SimpleString("sys.path = [sys.g_program_path] + sys.path");
    //PyRun_SimpleString("sys.path = [sys.g_program_path]");

    PyRun_SimpleString("print \"hepp sys.path:\",sys.path,24");
    
    // Set sys.argv[0]
    sprintf(temp,"sys.argv=[\"%s\",\"%s\", \"%s\"]",
            argv[0],
#if defined(FOR_WINDOWS)
            "keybindings.conf",
#else
            OS_get_keybindings_conf_filename().replace("\\","\\\\").toLocal8Bit().constData(),
#endif
            OS_get_custom_keybindings_conf_filename().replace("\\","\\\\").toLocal8Bit().constData()
            );
    PyRun_SimpleString(temp);
    
    printf("argv[0]: %s\n",argv[0]);
    PyRun_SimpleString("print \"path:\",sys.g_program_path,239");
    
    //exit(0);
  }

  qapplication->setWindowIcon(QIcon(OS_get_full_program_file_path("radium_256x256x32.png")));

  {
    // Add fonts in the "fonts" directory
    {
      QDir dir(OS_get_full_program_file_path("fonts"));
      QFileInfoList list = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot);
      for (int i=0;i<list.count();i++){
        QFileInfo file_info = list[i];
        
        QString file_path = file_info.filePath();
        if(file_info.suffix()=="ttf"){
          //printf("file_path: %s\n",file_path.toLocal8Bit().constData());
          QFontDatabase::addApplicationFont(file_path);
        }
      }
    }

    // set system font

    bool custom_config_set = false;
    QString fontstring = SETTINGS_read_qstring("system_font","");

    if(fontstring=="") {
      SETTINGS_set_custom_configfile(OS_get_full_program_file_path("config"));
      fontstring = SETTINGS_read_qstring("system_font","");
      R_ASSERT(fontstring != "");
      custom_config_set = true;
    }

    {
      QFont font;
      font.fromString(fontstring);
      //font.fromString("Cousine,11,-1,5,75,0,0,0,0,0");
 
#if 0 //FOR_MACOSX
      if(custom_config_set)
        font.setPointSizeF(font.pointSizeF()*96.0/72.0); // macs have dpi of 72, while linux and windows have 96.
#endif
      
      if(SETTINGS_read_qstring("system_font_style","")!="")
        font.setStyleName(SETTINGS_read_qstring("system_font_style",""));
      qapplication->setFont(font);
      QApplication::setFont(font);
    }

    if (custom_config_set==true){
      SETTINGS_unset_custom_configfile();
    }


  }


  //signal(SIGSEGV,finish);
  //signal(SIGFPE,finish);
  //signal(SIGTERM,finish);

  signal(SIGINT,finish);

#if GTK_IS_USED
  GTK_Init(argc,argv);
#endif

  printf("2\n");

  initradium();

  OS_get_full_program_file_path("start.py"); // just ensure file is there

#if 0 //defined(FOR_WINDOWS)
  PyRun_SimpleString("execfile(\"start.py\")");
#else
  PyRun_SimpleString("execfile(os.path.join(sys.g_program_path,\"start.py\"))");
  //PyRun_SimpleString(talloc_format("execfile(\"%S\")", STRING_create(OS_get_full_program_file_path("start.py")))); //os.path.join(sys.g_program_path,\"start.py\"))");
#endif
  
  fprintf(stderr,"          ENDING B 1\n");
  D(GFX_ShowProgressMessage("19", true));
  
  Py_Finalize();

  D(GFX_ShowProgressMessage("20", true));
  fprintf(stderr,"          ENDING B 2\n");

  DISKPEAKS_stop();
  D(GFX_ShowProgressMessage("21", true));
  
  PLUGINHOST_shut_down();
  //msleep(5000);

  //abort();
  D(GFX_ShowProgressMessage("22", true));
  fprintf(stderr, "My pid: %d\n",(int)getpid());

  // Make it quit faster. Qt 5.7.0 crashes during shut down on linux.
#if defined(FOR_LINUX) && defined(RELEASE) && QT_VERSION==0x050700
  kill(getpid(), SIGKILL);
#endif

  // Linux/debug usually hangs when exiting. (qxcb thread)
#if 0 //defined(FOR_LINUX) && !defined(RELEASE)
  kill(getpid(), SIGKILL);
#endif

  g_program_has_ended = true;
  
  //RError("hepp");
  return 0;
}

