/*
Simple X11 1-key Keyboard layout locker.

Only LockGroup switch type supported (each key select the own layout).
No indicator supported.

To switch layout You must press and realease the corresponding key;
layout will be switched when the key is released,
but only yf no other key(s) has been pressed.

 Developed by Alexey Korop, 2014

 Compile:
    gcc -lX11 -pthread s1kls.c -o xkbs1kls

 This program is free software, under the GNU General Public License
 version 3.
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>

//#define TRACE

static int locker[10];
static struct {int key, layout;} switcher[10];
static int n_switcher, n_locker; // counts of the each type switchers
static bool view_codes = 0;

/* some keycodes on the pc105 keyboard:
 62   rshift
105   rctrl
108   ralt
134   rwin
 50   lshift
 37   lctrl
 64   lalt
 66   caps
133   lwin
*/

/* set realtime priority if possible */
void set_rt_prio(void){
  struct sched_param param;

  param.__sched_priority = 1;
  pthread_setschedparam(pthread_self(), SCHED_FIFO, &param); // do not check success
  }

static int get_group(Display *dpy) {
  XkbStateRec state[1];
  memset(state, 0, sizeof(state));
  XkbGetState(dpy, XkbUseCoreKbd, state);
  return state->group;
}

static void init(int argc, char *argv[]){
  int n;
  char c;

  n_switcher = 0;
  n_locker = 0;
  for (n=1; n<argc-1; n++){
      if (argv[n][0] == '-'){
        if (strlen(argv[n]) != 3){
          printf("'%s' wrong lengh\n\n", argv[n]);
          goto help;
          }
        if (argv[n][1] != 's'){
          printf("'%s' wrong key\n\n", argv[n]);
          goto help;
          }
        c = argv[n][2]; // layout number
        if ((c < '0') || (c > '9')) {
          printf("'%s' wrong layout number\n\n", argv[n]);
          goto help;
          }
        switcher[n_switcher].layout = c-'0';
        if (n_locker == argc-1){
          printf("keycode missng after '%s'\n\n", argv[n]);
          goto help;
          }
        n++;
        sscanf(argv[n], "%d", &switcher[n_switcher].key);
        if (switcher[n_switcher].key <= 0){
          printf("'%s' wrong keycode\n\n", argv[n]);
          goto help;
          }
        n_switcher++;
      }
    else
      {
      sscanf(argv[n], "%d", &locker[n_locker]);
      if (locker[n_locker] <= 0){
        printf("'%s' wrong keycode\n\n", argv[n]);
        goto help;
        }
      n_locker++;
      }
    }

  if ((n_locker < 2) && (n_switcher == 0)){
help:
    printf("Simple X11 1-key Keyboard layout locker. Version 1.02.\n");
    printf("(c) Alexey Korop, 2019. Free software under GNU GPLv3\n\n");
    printf("  It is recommended to run this program with root privileges for better poll accuracy\n");
    printf("  Command line:   s1kls keycode0 keycode1 ... -s<layout> keycode ...\n\n");
    printf("The key with keycode 'keycode0' sets the layout 0, \n");
    printf("the key with keycode 'keycode1' sets the layout 1 ... \n\n");
    printf("  You may also define several keycodes for switch to desired layout temporary (while pressed)\n");
    printf("Layout is the digit after '-s' (without space), for instance:\n");
    printf("       -s0 108  - this define right Alt as tempoary switch to the layout 0.\n\n");
    printf("Now You may press the desired keys and see their keycodes.\n");
    view_codes = 1;
    }
  }

int main(int argc, char *argv[]) {
  Display *display;
  char keys_cur[32], keys_prev[32];
  unsigned char diff, cur;
  int n_keys_pressed;
  int keycode = 0;
  int i;
  int byte, bit;
  int old_layout = -1; // !=-1 during temporary switching
  int layout = -1;/*
    -2 - Awaiting release all keys;
    -1 - all keys are released;
    0... - the key that selects this layout is pressed; expect its release */

  init(argc, argv);
  set_rt_prio();
  display = XOpenDisplay(NULL);
  if (display == NULL){
    fprintf(stderr, "XOpenDisplay error \n");
    }

  XkbIgnoreExtension(False);
  XQueryKeymap(display, keys_prev);
  while(1) {
    XQueryKeymap(display, keys_cur);
    n_keys_pressed = 0;
    for (byte=0; byte<32; byte++) {
      cur = keys_cur[byte];
      diff = keys_prev[byte] ^ cur;
      if (diff | cur) {
        keys_prev[byte] = cur;
        for (bit = 0; bit<8; bit++){
          if (diff & 0x01){
            if (!view_codes || (cur & 0x01))
              keycode = byte*8 + bit;
#ifdef TRACE
            printf("key %d", keycode);
            if (cur & 0x01)
              printf(" press\n");
            else
              printf(" release\n");
#endif
            }
          if (cur & 0x01)
            n_keys_pressed++;
          diff >>= 1;
          cur >>= 1;
          }
        }
      }

    if (view_codes){
      if (n_keys_pressed == 0){
        if (keycode){
          printf(" %4d \n", keycode);
          keycode = 0;
          }
        }
      }
    else { // normal work
      if (n_keys_pressed == 0){
        if (old_layout >= 0){
#ifdef TRACE
          printf("return to layout %d\n", old_layout);
#endif
          if (!XkbLockGroup(display, XkbUseCoreKbd, old_layout)){
            fprintf(stderr, "XkbLockGroup error \n");
            return(1);
            }
          old_layout = layout = -1;
          }
        else if (layout >= 0){
#ifdef TRACE
          printf("new layout %d\n", layout);
#endif
          if (!XkbLockGroup(display, XkbUseCoreKbd, layout)){
            fprintf(stderr, "XkbLockGroup error \n");
            return(1);
            }
          }
        layout = -1;
        }
      else if (layout == -2)
        ;
      else if ((n_keys_pressed == 1) && (keycode == locker[layout]))
        ; //hold the key
      else if ((n_keys_pressed == 1) && (layout == -1)){
        layout = -2;
        // try key as layout locker
        for (i=0; i<n_locker; ++i)
          if (keycode == locker[i]){
            layout = i;
#ifdef TRACE
            printf("wait for release the key %d\n", locker[layout]);
#endif
            break;
            }
        if (layout == -2){
          // try key as layout switcher
          for (i=0; i<n_switcher; ++i)
            if (keycode == switcher[i].key){
              layout = switcher[i].layout;
              old_layout = get_group(display);
              if (!XkbLockGroup(display, XkbUseCoreKbd, layout)){
                fprintf(stderr, "XkbLockGroup error \n");
                return(1);
                }
  #ifdef TRACE
              printf("key %d switch to layout %d\n", switcher[i].key, switcher[i].layout);
  #endif
              break;
            }
          }
        }
      else{
#ifdef TRACE
        if (layout >= 0)
          printf("reject waiting\n");
#endif
        layout = -2;
        }
      }
    usleep(20000);
    }
//  XCloseDisplay(display);
}
