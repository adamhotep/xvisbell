/*
  xvisbell: visual bell for X11

  Copyright 2015 Rian Hunter <rian@alum.mit.edu>
  Updates by Adam Katz <@adamhotep> <@adamhotep@infosec.exchange>

  Derived from pulseaudio/src/modules/x11/module-x11-bell.c
  Copyright 2004-2006 Lennart Poettering

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 3 of the License,
  or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, sett <http://www.gnu.org/licenses/>.
 */

#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <iostream>
#include <stdexcept>

#include <cstdlib>
#include <climits>

#include <sys/select.h>
#include <sys/time.h>

const char *VERSION = "20251209.0";

const struct timeval window_timeout = {0, 100000};

// -1 means for w or h means screen width or height
struct {
  int x, y;
  int w, h;
} geometry = {0, 0, -1, -1};

const char *color = nullptr;
short flash = -1;
short ms = 100;

int parse_count(const char *num) {
  errno = 0;
  char *end;
  long val = std::strtol(num, &end, 10);
  if (end == num || *end != '\0' ||
      (errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) ||
      val < SHRT_MIN || val > SHRT_MAX) {
    fprintf(stderr, "invalid count: %s\n", num);
    exit(2);
  }
  return (short)val;
}

static bool parse_geometry(const char *s_in) {
  if (!s_in || *s_in == '\0') return true;

  const char *geom = s_in;
  char *end;
  long val;

  // parse width
  val = std::strtol(geom, &end, 10);
  if (end == geom) return false;
  int w = (int)val;
  if (w <= 0) { w = -1; }
  if (*end != 'x' && *end != 'X') return false;
  geom = end + 1;

  // parse height
  val = std::strtol(geom, &end, 10);
  if (end == geom) return false;
  int h = (int)val;
  if (h <= 0) { h = -1; }
  geom = end;

  int x = geometry.x;
  int y = geometry.y;

  // optional X offset
  if (*geom == '+' || *geom == '-') {
    val = std::strtol(geom, &end, 10);
    if (end == geom) return false;
    x = (int)val;
    geom = end;

    // optional Y offset
    if (*geom == '+' || *geom == '-') {
      val = std::strtol(geom, &end, 10);
      if (end == geom) return false;
      y = (int)val;
      geom = end;
    }
  }

  if (*geom != '\0') return false;

  geometry.w = w;
  geometry.h = h;
  geometry.x = x;
  geometry.y = y;
  return true;
}

bool operator<(const struct timeval & a,
               const struct timeval & b) {
  return timercmp(&a, &b, <);
}

struct timeval & operator+=(struct timeval & a, const struct timeval & b) {
  timeradd(&a, &b, &a);
  return a;
}

struct timeval operator-(const struct timeval & a,
                         const struct timeval & b) {
  struct timeval toret;
  timersub(&a, &b, &toret);
  return toret;
}

int main(int argc, char **argv) {

  int i = 0;
  auto needs_arg = [&]() {
    fprintf(stderr, "%s: %s requires an argument\n", argv[0], argv[i]);
    exit(2);
  };

  while (++i < argc) {
    std::string arg = argv[i];
    if (arg == "--color") {
      if (i+1 >= argc) { needs_arg(); }
      color = argv[++i];
    } else if (arg == "--flash") {
      if (i+1 >= argc) { needs_arg(); }
      flash = parse_count(argv[++i]);
    } else if (arg == "--geometry" || arg == "--geom") {
      if (i+1 >= argc) { needs_arg(); }
      if (!parse_geometry(argv[++i])) {
        fprintf(stderr, "%s: --geometry could not be parsed from `%s`\n",
          argv[0], argv[i]);
        exit(2);
      }
    } else if (arg == "--once") {
      flash = 1;
    } else if (arg == "--time") {
      if (i+1 >= argc) { needs_arg(); }
      ms = parse_count(argv[++i]);
    } else {
      FILE *out = stderr;
      if (arg == "-h" || arg == "--help") {
        out = stdout;
      } else {
        fprintf(out, "%s: unrecognized option '%s'\n", argv[0], argv[1]);
      }
      fprintf(out, "Usage: %s [OPTIONS]\n", argv[0]);
      if (out == stdout) {
        fprintf(out, "  --color COLOR    flash this color (default=white)\n");
        fprintf(out, "  --flash COUNT    just flash COUNT times and exit\n");
        fprintf(out, "  --geometry GEOM  area to flash (0=all, default=0x0)\n");
        fprintf(out, "  --once           same as --flash 1\n");
        fprintf(out, "  --time TIME      interval (in ms) for --flash\n");
        fprintf(out, "\n%s %s 2015+ by Rian Hunter and Adam Katz, GPLv3+\n",
          argv[0], VERSION);
        exit(0);
      } else {
        fprintf(out, "Try '%s --help' for more information.\n", argv[0]);
        exit(2);
      }
    }
  }

  auto dpy = XOpenDisplay(nullptr);
  if (!dpy) {
    throw std::runtime_error("XOpenDisplay() error");
  }

  auto scr = XDefaultScreen(dpy);
  auto root = XRootWindow(dpy, scr);
  auto vis = XDefaultVisual(dpy, scr);

  auto major = XkbMajorVersion;
  auto minor = XkbMinorVersion;

  if (!XkbLibraryVersion(&major, &minor)) {
    throw std::runtime_error("XkbLibraryVersion() error");
  }

  major = XkbMajorVersion;
  minor = XkbMinorVersion;

  int xkb_event_base;
  if (!XkbQueryExtension(dpy, nullptr, &xkb_event_base,
                         nullptr, &major, &minor)) {
    throw std::runtime_error("XkbQueryExtension() error");
  }

  XkbSelectEvents(dpy, XkbUseCoreKbd, XkbBellNotifyMask, XkbBellNotifyMask);

  unsigned int auto_ctrls, auto_values;
  auto_ctrls = auto_values = XkbAudibleBellMask;

  XkbSetAutoResetControls(dpy, XkbAudibleBellMask, &auto_ctrls, &auto_values);
  XkbChangeEnabledControls(dpy, XkbUseCoreKbd, XkbAudibleBellMask, 0);

  XSetWindowAttributes attrs;

  if (color != nullptr) {
    XColor xcolor;
    Colormap cmap = DefaultColormap(dpy, scr);
    if (XAllocNamedColor(dpy, cmap, color, &xcolor, &xcolor)) {
      attrs.background_pixel = xcolor.pixel;
    } else {
      fprintf(stderr, "Warning: color '%s' not found; using black\n", color);
      attrs.background_pixel = BlackPixel(dpy, scr);
    }
  } else {
    attrs.background_pixel = WhitePixel(dpy, scr);
  }

  attrs.override_redirect = True;
  attrs.save_under = True;

  auto x11_fd = ConnectionNumber(dpy);

  struct timeval future_wakeup;
  bool timeout_is_set = false;

  auto width = geometry.w < 0 ? DisplayWidth(dpy, scr) : geometry.w;
  auto height = geometry.h < 0 ? DisplayHeight(dpy, scr) : geometry.h;

  auto win = XCreateWindow(dpy, root, geometry.x, geometry.y,
                           width, height, 0,
                           XDefaultDepth(dpy, scr), InputOutput,
                           vis,
                           CWBackPixel | CWOverrideRedirect | CWSaveUnder,
                           &attrs);

  if (flash != -1) {
    struct timeval visible;
    struct timeval sleep;

    while (flash--) {
      XMapRaised(dpy, win);
      XFlush(dpy);

      visible = window_timeout;
      select(0, nullptr, nullptr, nullptr, &visible);

      XUnmapWindow(dpy, win);
      XFlush(dpy);

      if (flash > 0) {
        sleep.tv_sec = ms / 1000;
        sleep.tv_usec = (ms % 1000) * 1000;
        select(0, nullptr, nullptr, nullptr, &sleep);
      }
    }
    XCloseDisplay(dpy);
    return 0;
  }

  while (true) {
    struct timeval tv, *wait_tv = nullptr;

    fd_set in_fds;
    FD_ZERO(&in_fds);
    FD_SET(x11_fd, &in_fds);

    if (timeout_is_set) {
      // we really should use a monotonic clock here but
      // there isn't really a portable interface for that yet
      // in the future we can ifdef this for mac/linux
      struct timeval cur_time;
      if (gettimeofday(&cur_time, nullptr) < 0) {
        throw std::runtime_error("getttimeofday() error!");
      }

      // c++ magic, in your face linus!
      tv = (future_wakeup < cur_time
            ? timeval{0, 0}
            : future_wakeup - cur_time);
      wait_tv = &tv;
    }

    while (true) {
      if (select(x11_fd + 1, &in_fds, nullptr, nullptr, wait_tv) < 0) {
        if (errno == EINTR) continue;
        throw std::runtime_error("select() error!");
      }
      break;
    }

    if (timeout_is_set) {
      struct timeval cur_time;
      if (gettimeofday(&cur_time, nullptr) < 0) {
        throw std::runtime_error("getttimeofday() error!");
      }

      if (future_wakeup < cur_time) {
        // timeout fired
        XUnmapWindow(dpy, win);
        timeout_is_set = false;
      }
    }

    while (XPending(dpy)) {
      XEvent ev;
      XNextEvent(dpy, &ev);

      // TODO: handle resize events on root window

      // TODO: this reinterpret cast is not good
      if (reinterpret_cast<XkbEvent *>(&ev)->any.xkb_type != XkbBellNotify) {
        continue;
      }

      XMapRaised(dpy, win);

      // reset timeout
      timeout_is_set = true;
      if (gettimeofday(&future_wakeup, nullptr) < 0) {
        throw std::runtime_error("getttimeofday() error!");
      }
      future_wakeup += window_timeout;

      // ignore for now...
      // auto bne = reinterpret_cast<XkbBellNotifyEvent *>(&ev);
    }
  }
}

