/*
Simple X11 1-key Keyboard layout switcher.

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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>

//#define TRACE

static int switcher[16];
/* some keycodes on the pc105 keyboard:
 62   rshift
105   rctrl
108   ralt
134   rwin
 50   lshift
 37   lctrl
 64   lalt
133   lwin
*/

typedef unsigned char bool;

/* set realtime priority if possible */
void set_rt_prio(void){
  struct sched_param param;

  param.__sched_priority = 1;
  pthread_setschedparam(pthread_self(), SCHED_FIFO, &param); // do not check success
  }

int main(int argc, char *argv[]) {
  Display *display;
  char keys_cur[32], keys_prev[32];
  unsigned char diff, cur;
  int n_keys_pressed;
  int keycode = 0;
  int i;
  int byte, bit;
  int n;
  bool view_codes = 0;
  int layout = -1;/*
    -2 - Awaiting release all keys;
    -1 - all keys are released;
    0... - the key that selects this layout is pressed; expect its release */

  for (n=0; n<argc-1; n++){
    sscanf(argv[n+1], "%d", &switcher[n]);
    if (switcher[n] <= 0)
      goto help;
    }
  if (n < 2){
help:
    printf("Simple X11 1-key Keyboard layout switcher. Version 1.01.\n");
    printf("(c) Alexey Korop, 2014. Free software under GNU GPLv3\n\n");
    printf("It is recommended to run this program with root privileges for better poll accuracy\n");
    printf("Command line:   s1kls keycode1 keycode2 ...\n\n");
    printf("The key with keycode 'keycode1' sets the layout 1, \n");
    printf("the key with keycode 'keycode2' sets the layout 2 ... \n\n");
    printf("Now You may press the desired keys and see their keycodes.\n");
    view_codes = 1;
    }

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
        if (layout >= 0){
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
      else if ((n_keys_pressed == 1) && (keycode == switcher[layout]))
        ; //hold the key
      else if ((n_keys_pressed == 1) && (layout == -1)){
        layout = -2;
        for (i=0; i<n; ++i)
          if (keycode == switcher[i]){
            layout = i;
#ifdef TRACE
            printf("wait for release the key %d\n", switcher[layout]);
#endif
            break;
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
