#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  Display* display;
  Window window;
  XEvent event;
  char* window_name = "Button Example";
  char* icon_name = "Button Example";
  int screen_num;
  unsigned long white_pixel;
  unsigned long black_pixel;

  /* Create the main window. */
  Window main_button;
  char* main_button_name = "Press Me";

  /* Create the dialog window and the dialog buttons. */
  Window dialog_window;
  Window dialog_button[3];
  char* dialog_button_name[3] = { "Button 1", "Button 2", "Close" };
  int dialog_button_state[3] = { 0, 0, 0 };  /* 0 = not pressed, 1 = pressed */

  /* Open the display. */
  display = XOpenDisplay(NULL);
  if (display == NULL) {
    fprintf(stderr, "Cannot open display\n");
    exit(1);
  }

  /* Get the screen number and white and black pixels. */
  screen_num = DefaultScreen(display);
  white_pixel = WhitePixel(display, screen_num);
  black_pixel = BlackPixel(display, screen_num);

  /* Create the main window. */
  window = XCreateSimpleWindow(display, RootWindow(display, screen_num),
                               0, 0, 200, 200, 0, black_pixel, black_pixel);

  /* Set the window name and icon name. */
  XStoreName(display, window, window_name);
  XSetIconName(display, window, icon_name);

  /* Set the event mask. */
  XSelectInput(display, window, ExposureMask | ButtonPressMask | KeyPressMask);

  /* Create the main button. */
  main_button = XCreateSimpleWindow(display, window,
                                    50, 50,
                                    100, 40, 2, white_pixel, white_pixel);

  // =====================
    XSelectInput(display, main_button, ButtonPressMask | KeyPressMask);
  XStoreName(display, main_button, main_button_name);

  /* Map the window and the button. */
  XMapWindow(display, window);
  XMapWindow(display, main_button);

// ================

/* Start the event loop. */
  while (1) {
    XNextEvent(display, &event);
    if (event.type == Expose) {
      /* Redraw the window and the button. */
      XSetWindowBackground(display, window, white_pixel);
      XClearWindow(display, window);
      XDrawString(display, main_button, DefaultGC(display, screen_num),
                  10, 30, main_button_name, strlen(main_button_name));
      /* Redraw the dialog window and the dialog buttons, if the dialog is open. */
      if (dialog_window) {
        XSetWindowBackground(display, dialog_window, white_pixel);
        XClearWindow(display, dialog_window);
        XDrawString(display, dialog_window, DefaultGC(display, screen_num),
                    10, 30, "This is a dialog", 16);
        for (int i = 0; i < 3; i++) {
          XSetWindowBackground(display, dialog_button[i],
                                white_pixel);
                               //dialog_button_state[i] ? white_pixel : black_pixel);
          XClearWindow(display, dialog_button[i]);
          XDrawString(display, dialog_button[i], DefaultGC(display, screen_num),
                      10, 30, dialog_button_name[i], strlen(dialog_button_name[i]));
        }
      }
    }
    else if (event.type == ButtonPress) {
      /* Print the name of the button that was pressed. */
      if (event.xbutton.window == main_button) {
        printf("Main button was pressed\n");
        /* Create the dialog window. */
        dialog_window = XCreateSimpleWindow(display, window,
                                            50, 50, 200, 200, 2, white_pixel, white_pixel);
        XSelectInput(display, dialog_window, ExposureMask | ButtonPressMask);
        XMapWindow(display, dialog_window);

        /* Create the dialog buttons. */
        for (int i = 0; i < 3; i++) {
          dialog_button[i] = XCreateSimpleWindow(display, dialog_window,
                                                 50 + 50 * i, 150,
                                                 40, 40, 2, white_pixel, white_pixel);
          XSelectInput(display, dialog_button[i], ButtonPressMask);
          XMapWindow(display, dialog_button[i]);
        }
      }
      /* Close the dialog if the close button was pressed. */
      else if (event.xbutton.window == dialog_button[2]) {
        printf("Close button was pressed\n");
        XDestroyWindow(display, dialog_window);
        dialog_window = 0;
      }
      /* Toggle the state of the other dialog buttons. */
      else {
        for (int i = 0; i < 3; i++) {
          if (event.xbutton.window == dialog_button[i]) {
            dialog_button_state[i] = !dialog_button_state[i];
            printf("Dialog button %d was pressed\n", i + 1);
            break;
          }
        }
      } // end dialog button state
    }

// =====
  }
}
