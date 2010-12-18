/* curses */
#undef cbreak
# define cbreak()
#undef clear
# define clear()
#undef clrtoeol
# define clrtoeol()
#undef clrtobot
# define clrtobot()
#undef endwin
# define endwin()
#undef getyx
# define getyx(...)
#undef getmaxyx
# define getmaxyx(...)
#undef initscr
# define initscr()
#undef intrflush
# define intrflush(...)
#undef keypad
# define keypad(...)
#undef noecho
# define noecho()
#undef nonl
# define nonl()
#undef printw
# define printw(...)
#undef refresh
# define refresh()
#undef stdscr
# define stdscr
#undef timeout
# define timeout
#undef wclear
# define wclear()
#undef wclrtoeol
# define wclrtoeol()
#undef move
# define move(...)
#undef wmove
# define wmove(...)
#undef wrefresh
# define wrefresh()

/* readline.h */
#undef ccs_getch2
# define ccs_getch2() EOF
#undef ccs_readline
# define ccs_readline(...) NULL
#undef ccs_add_history
# define ccs_add_history(...) 0
